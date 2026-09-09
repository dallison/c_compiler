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
#include "type_special_member.h"

void SynthesizeDefaultedMemberFunctionBody(TypeParser* parser, Symbol* symbol);
static ASTNode* IdentityCloneNode(ASTNode* node, void* data);
static void SetNeedAddress(ASTNode* node);

static bool ExpressionRequiresASTConstexpr(ASTNode* node, void* data) {
  (void)data;
  return node != NULL &&
         (((node->flags & kASTRequiresASTConstexpr) != 0) ||
          (node->op == AST_OP(identifier) &&
           ((IdentifierASTNode*)node)->symbol != NULL &&
           ((IdentifierASTNode*)node)->symbol->requires_ast_constexpr));
}

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
static int FunctionCallScore(TypeRecord* func, VectorASTNode* node,
                             size_t first_formal_arg);
static bool MemberReceiverIsConst(BinaryASTNode* node);
static bool MemberReceiverIsVolatile(BinaryASTNode* node);
static bool MemberReceiverMatchesRefQualifier(TypeRecord* func,
                                              BinaryASTNode* member_access);
static bool LowerMemberFunctionCall(VectorASTNode* node);
static ASTNode* NewVirtualCalleeFromFunction(ASTNode* receiver,
                                             TypeRecord* function_type,
                                             bool receiver_is_pointer,
                                             SourceLocation location);

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
                                                        bool allow_explicit,
                                                        bool allow_same_class);

static bool ReferenceCanBind(ASTNode* actual, TypeRecord* reference_type);
static bool TypeIsEffectivelyConst(TypeRecord* type);
static ASTNode* TryBindReferenceToBaseSubobject(ASTNode* expr,
                                                TypeRecord* referent);
static Symbol* ResolveFreeFunctionWithADL(String* name, Vector* actuals,
                                          bool diagnose_ambiguous);
static Symbol* FunctionTemplateOverloadCandidate(Symbol* candidate,
                                                 VectorASTNode* node,
                                                 Vector* explicit_args,
                                                 Vector* temporary_candidates);
static Symbol* InstantiateSelectedFunctionTemplateCandidate(Symbol* selected);
static StructMember* InstantiateSelectedMemberTemplateCandidate(
    StructMember* selected);
static int OperatorCoAwaitCallScore(Symbol* candidate, VectorASTNode* node);
static Symbol* ResolveFreeOperatorCoAwaitForActual(ASTNode* actual,
                                                   bool diagnose_ambiguous);
static ASTNode* NewOperatorMemberCall(ASTNode* receiver, const char* op_name,
                                      Vector* actuals,
                                      SourceLocation location);
static ASTNode* NewOperatorFreeCall(Symbol* function, ASTNode* first_actual,
                                    Vector* remaining_actuals,
                                    SourceLocation location);

void SemanticEnsureAutoReturnTypeDeduced(TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func) ||
      !TypeFunctionReturnContainsAuto(func)) {
    return;
  }
  if (func->info.function.body == NULL) {
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
  // Deduction happens once and its result is permanent, so the body analysis is
  // a real instantiation even when the caller is a signature-only probe such as
  // a type trait.  Templates the body needs must therefore be queued for
  // emission; leaving the speculative flag set would drop them for good.
  int saved_speculative_depth =
      compiler->speculative_template_instantiation_depth;
  compiler->speculative_template_instantiation_depth = 0;
  AnalyzeStatement(func->info.function.body);
  compiler->current_function = saved_function;
  compiler->current_class_access_context = saved_class_access_context;
  StatementFinishAutoReturnDeduction(func, /*diagnostic_node=*/NULL);
  compiler->speculative_template_instantiation_depth = saved_speculative_depth;
}

static bool TemplateArgumentVectorContainsTemplateParameter(Vector* args);

static bool CXXInImmediateFunctionContext(void) {
  return compiler->immediate_function_context_depth > 0 ||
         (CompilerCXXAtLeast(kLanguageStandardCXX23) &&
          compiler->constant_evaluation_required_depth > 0) ||
         (compiler->current_function != NULL &&
          TypeIsFunction(compiler->current_function) &&
          compiler->current_function->info.function.is_consteval);
}

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

static bool CXXEscalateCurrentFunction(void) {
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
static void CXXAnalyzeImmediateEscalationCandidate(Symbol* symbol) {
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

static ASTNode* AnalyzeIdentifier(IdentifierASTNode* node) {
  if (CompilerCXXAtLeast(kLanguageStandardCXX26) && node->symbol != NULL &&
      (node->base.flags & kASTNameIndependentLookupAmbiguous) != 0 &&
      (node->base.flags & kASTIsDeclaration) == 0) {
    SemanticError(&node->base,
                  "reference to name-independent declaration '%s' is ambiguous",
                  node->symbol->name.value);
    ASTNodeSetType(&node->base, TypeRecordCopy(node->symbol->type));
    node->base.value_category = kValueCategoryLvalue;
    return &node->base;
  }
  if (CompilerIsCXX() && node->symbol != NULL &&
      (node->base.flags & kASTIsDeclaration) == 0 &&
      StringEqual(&node->symbol->name, "main") &&
      node->symbol->namespace_ == NULL &&
      TypeIsFunction(node->symbol->type) &&
      node->symbol->type->info.function.cxx_member_owner == NULL) {
    SemanticError(&node->base,
                  "the function 'main' cannot be named by an expression");
  }
  // A dependent qualified value name (`T::member`) that still carries its flag
  // here was never resolved during template instantiation, meaning the named
  // member does not exist in the substituted scope type.
  if ((node->base.flags & kASTDependentQualifiedName) != 0 &&
      node->symbol != NULL && node->symbol->type != NULL &&
      node->symbol->type->dependent_member_name != NULL) {
    if (TypeContainsTemplateParameter(node->symbol->type)) {
      return &node->base;
    }
    SemanticError(&node->base, "no member named '%s' in the dependent scope",
                  node->symbol->type->dependent_member_name->value);
    ASTNodeSetType(&node->base, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return &node->base;
  }
  // A variable template-id used as a value (`variant_size_v<T>`): instantiate
  // its initializer with the explicit arguments and fold to a constant.  When
  // the arguments are still dependent (used inside another template), leave the
  // node untouched so it is re-analyzed after substitution.
  if (CompilerIsCXX() && node->symbol != NULL &&
      node->symbol->variable_template != NULL &&
      node->template_arguments != NULL &&
      (node->base.flags & kASTIsDeclaration) == 0 &&
      !TemplateArgumentVectorContainsTemplateParameter(
          node->template_arguments)) {
    TypeRecord* concrete = TypeInstantiateVariableTemplateType(
        &compiler->syntax, node->symbol, node->template_arguments);
    int64_t value = 0;
    if (concrete != NULL && TypeIsIntegral(concrete) &&
        TypeInstantiateVariableTemplateConstant(
            &compiler->syntax, node->symbol, node->template_arguments, &value)) {
      ASTNode* const_node =
          NewIntConstantASTNode(value, concrete, node->base.location);
      ASTNodeReplaceChild(node->base.parent, node->base.child_id, const_node,
                          true);
      const_node->flags |= kASTAnalyzed;
      return const_node;
    }
    double floating_value = 0;
    if (concrete != NULL && TypeIsFloatingPoint(concrete) &&
        TypeInstantiateVariableTemplateFloatingConstant(
            &compiler->syntax, node->symbol, node->template_arguments,
            &floating_value)) {
      ASTNode* const_node = NewRealConstantASTNode(
          floating_value, concrete, node->base.location);
      ASTNodeReplaceChild(node->base.parent, node->base.child_id, const_node,
                          true);
      const_node->flags |= kASTAnalyzed;
      return const_node;
    }
    // A variable template whose instantiation is a class-type tag object (e.g.
    // `std::in_place_index<1>` of type `in_place_index_t<1>`) rather than a
    // folded constant.  Materialize a value-initialized temporary of the
    // concrete type so overload resolution and template argument deduction see
    // the correct `in_place_index_t<1>` type.
    if (concrete != NULL && TypeIsStructOrUnion(concrete)) {
      SourceLocation location = node->base.location;
      Symbol* temp = SyntaxNewTemporary(&compiler->syntax, concrete);
      temp->location = location;
      ASTNode* temp_id = NewIdentifierASTNode(temp, location);
      temp_id->flags |= kASTNeedAddress | kASTIsDeclaration;
      Vector* initializer_values = NewVector();
      Struct* value_class = concrete->info.struct_info;
      bool has_object_state = false;
      for (size_t i = 0;
           value_class != NULL && i < value_class->members.length; ++i) {
        StructMember* member = value_class->members.value.p[i];
        if (member != NULL && member->symbol != NULL &&
            !member->is_static && !member->is_member_function &&
            !StorageIs(member->symbol->storage, STO(typedef))) {
          has_object_state = true;
          break;
        }
      }
      if (has_object_state) {
        ASTNode* concrete_initializer =
            TypeInstantiateVariableTemplateInitializer(
                &compiler->syntax, node->symbol, node->template_arguments);
        if (concrete_initializer != NULL) {
          VectorAppend(initializer_values, concrete_initializer);
        }
      }
      ASTNode* initializer = NewBracedInitializerASTNode(
          initializer_values, NULL, location);
      ASTNode* literal =
          NewCompoundLiteralASTNode(temp_id, location, initializer);
      ASTNodeReplaceChild(node->base.parent, node->base.child_id, literal, true);
      ASTNode* analyzed = AnalyzeExpression(literal);
      // A variable template specialization names a variable, so referring to it
      // yields an lvalue -- a const one, these objects being `constexpr`.
      // Calling it a prvalue made `T&&` deduce `T = X` instead of
      // `T = const X&`, and binding the const object to the resulting `X&&`
      // was then rejected (`std::views::empty<int>` passed to a generic range
      // parameter).
      analyzed->value_category = kValueCategoryLvalue;
      return analyzed;
    }
    TypeRecordDelete(concrete);
  }

  if (CompilerIsCXX() && node->symbol != NULL &&
      node->symbol->flags.is_template && !node->symbol->flags.is_overloaded &&
      !DiagnosticsSuppressed() &&
      TypeIsFunction(node->symbol->type) &&
      (compiler->current_function == NULL ||
       compiler->current_function->info.function.symbol == NULL ||
       !compiler->current_function->info.function.symbol->flags.is_template) &&
      node->base.parent != NULL &&
      node->base.parent->op == AST_OP(address) &&
      node->template_arguments != NULL &&
      !TemplateArgumentVectorContainsTemplateParameter(
          node->template_arguments)) {
    Symbol* instantiated = TypeInstantiateFunctionTemplate(
        &compiler->syntax, node->symbol, node->template_arguments);
    if (instantiated != NULL) {
      node->symbol = instantiated;
      CXXAnalyzeImmediateEscalationCandidate(instantiated);
    }
  }

  if (node->base.parent == NULL || node->base.parent->op != AST_OP(init)) {
    // Symbol has now been used.
    node->symbol->flags.used = true;

    // Track whether the value is ever *read*, to distinguish variables that are
    // only ever written (-Wunused-but-set-variable).  Every reference counts as
    // a read except when the identifier is the direct target of a plain `=`
    // assignment (its old value is discarded).  Compound assignments (`+=`),
    // increments/decrements, address-of and any nested use all read the value
    // and are therefore left to mark it read.
    if (node->base.parent == NULL ||
        node->base.parent->op != AST_OP(assign) || node->base.child_id != 0) {
      node->symbol->is_read = true;
    }

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
    TypeRecord* contract_view =
        SemanticContractIdentifierViewType(node->symbol, node->base.type);
    if (contract_view != NULL) {
      ASTNodeSetType(&node->base, contract_view);
      TypeRecordDelete(contract_view);
    }
  }

  if (TypeIsStructOrUnion(node->base.type) || TypeIsArray(node->base.type) ||
      TypeIsFunction(node->base.type)) {
    if (CompilerIsCXX() && TypeIsFunction(node->base.type)) {
      bool has_implicit_this =
          node->symbol->type->info.function.prototype.length > 0 &&
          node->symbol->type->info.function.prototype.value.p[0] != NULL &&
          StringEqual(&((Symbol*)node->symbol->type->info.function.prototype
                            .value.p[0])
                           ->name,
                      "this");
      // Do not eagerly instantiate a member of an overload set here: the
      // correct overload is only known after overload resolution, which
      // instantiates the selected candidate itself. Instantiating an
      // arbitrary (e.g. first) overload can materialize an unused member of
      // a class template and trigger spurious errors (e.g. default-
      // constructing a non-default-constructible type).
      if (!has_implicit_this && !node->symbol->flags.is_overloaded) {
        TypeEnsureTemplateMemberFunctionDefinition(&compiler->syntax,
                                                   node->symbol);
      }
    }
    node->base.flags |= kASTNeedAddress;
  } else {
    // If this is a declaration, don't try to fold it.
    if ((node->base.flags & kASTIsDeclaration) != 0) {
      return &node->base;
    }
    if ((node->base.flags & kASTNeedAddress) != 0) {
      return &node->base;
    }
    if (node->base.parent != NULL &&
        node->base.parent->op == AST_OP(address)) {
      return &node->base;
    }
    if (!node->symbol->flags.value_set) {
      return &node->base;
    }
    if (!CompilerIsCXX() && CompilerCAtLeast(kLanguageStandardC23) &&
        !node->symbol->flags.is_constexpr) {
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

typedef struct {
  Vector visited_functions;
  bool found;
} ConstevalContextQuery;

static void FindConstevalContextUse(ASTNode* node, void* data, int child_id,
                                    VisitorMode mode);

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

static bool ExpressionMayObserveConstantEvaluation(ASTNode* node) {
  ConstevalContextQuery query;
  VectorInit(&query.visited_functions);
  query.found = false;
  ASTNodeVisit(node, FindConstevalContextUse, 0, &query);
  VectorDestruct(&query.visited_functions);
  return query.found;
}

typedef struct {
  bool found;
} UnboundAutomaticFinder;

static void FindUnboundAutomatic(ASTNode* node, void* data, int child_id,
                                 VisitorMode mode) {
  (void)child_id;
  UnboundAutomaticFinder* finder = data;
  if (mode != kVisitPreChildren || finder->found ||
      node->op != AST_OP(identifier)) {
    return;
  }
  Symbol* symbol = ((IdentifierASTNode*)node)->symbol;
  if (symbol != NULL &&
      (symbol->flags.is_argument ||
       (symbol->flags.is_local &&
        !StorageIs(symbol->storage, STO(static) | STO(thread)) &&
        !symbol->flags.is_constexpr))) {
    finder->found = true;
  }
}

static bool ExpressionHasUnboundAutomatic(ASTNode* node) {
  UnboundAutomaticFinder finder = {false};
  ASTNodeVisit(node, FindUnboundAutomatic, 0, &finder);
  return finder.found;
}

// Attempt to fold a constant expression by evaluating it and if
// successful, replacing it with a constant AST node with the value.
static ASTNode* FoldConstantExpression(ASTNode* node) {
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
  if (CompilerCXXAtLeast(kLanguageStandardCXX20) &&
      compiler->constant_evaluation_required_depth == 0 &&
      ExpressionMayObserveConstantEvaluation(node)) {
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
    BinaryASTNode* subscript = (BinaryASTNode*)expression;
    return StaticAddressTargetFromExpression(subscript->left);
  }
  return NULL;
}

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
  Symbol* target = StaticAddressTargetFromExpression(expression);
  if (target != NULL) {
    if (CompilerSymbolIsMetaPromotedStatic(target)) {
      ConstexprEnsureMetaPromotedStaticObject(target);
    }
    symbol->value.other = target;
    symbol->flags.value_set = true;
    return true;
  }
  if (expression->op == AST_OP(address)) {
    UnaryASTNode* address = (UnaryASTNode*)expression;
    target = StaticAddressTargetFromExpression(address->sub);
    if (target != NULL) {
      if (CompilerSymbolIsMetaPromotedStatic(target)) {
        ConstexprEnsureMetaPromotedStaticObject(target);
      }
      symbol->value.other = target;
      symbol->flags.value_set = true;
      return true;
    }
  }
  return false;
}

static bool MarkCXX26SymbolicConstexprReference(Symbol* symbol,
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
        symbol->constexpr_reference_scope = compiler->current_function;
        ASTNodeDelete(symbol->constexpr_initializer);
        symbol->constexpr_initializer =
            ASTNodeClone(initializer, IdentityCloneNode, NULL, NULL);
        return true;
      }
    }
  }
  return false;
}

static bool EvaluateConstantForSymbol(Symbol* symbol, ASTNode* initializer) {
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

static StructMember* MemberPointerMemberFromExpression(ASTNode* node);

static bool IsNullPointer(ASTNode* node) {
  switch (node->op) {
    case AST_OP(number): {
      ConstantASTNode* c = (ConstantASTNode*)node;
      return c->value.ivalue == 0;
      }
    case AST_OP(question): {      // Conditional expression:
      node = ((BinaryASTNode*)node)->right;     // Colon.
      if (node == NULL || node->op != AST_OP(colon)) {
        return false;
      }
      ASTNode* left = ((BinaryASTNode*)node)->left;
      ASTNode* right = ((BinaryASTNode*)node)->right;
      return left != NULL && right != NULL &&
             IsNullPointer(left) && IsNullPointer(right);
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
  if (!TypeIsVector(node->left->type)) {
    SemanticCheckScalarType(node->left);
  }
  if (!TypeIsVector(node->right->type)) {
    SemanticCheckScalarType(node->right);
  }
}

// Ranks for types.  Larger ranks are closer to the end
// of the array.  These are pointers to functions that return true
// if the type is of the requested value.
static bool TypeHasDoubleConversionRank(TypeRecord* type) {
  return TypeIsDouble(type) || TypeIsFloat64(type);
}

bool (*type_ranks[])(TypeRecord*) = {
    TypeIsBool,       TypeIsCharFamily, TypeIsShort, TypeIsInt,
    TypeIsLong,       TypeIsLongLong, TypeUsesFloat32Representation,
    TypeHasDoubleConversionRank, TypeIsLongDouble, TypeIsVoid, NULL,
};

// Given a type, what is its rank.  According to the standard, types with higher
// precision are higher in rank, with _Bool being the lowest rank.  Floating
// point types have the highest rank.
static int GetRank(TypeRecord* type) {
  if (TypeIsBitInt(type)) {
    return type->bit_width * 16;
  }
  for (int i = 0; type_ranks[i] != NULL; i++) {
    if (type_ranks[i](type)) {
      if (TypeIsIntegral(type)) {
        // A standard integer type outranks _BitInt(N) at the same width, while
        // _BitInt(N+1) outranks every standard type no wider than N bits.
        return type->size * 8 * 16 + i + 1;
      }
      return 4096 + i + 1;
    }
  }
  return -1;
}

// C++23 gives the standard floating-point type the greater conversion subrank
// when an extended type has the same representation and rank.
static int GetFloatingSubrank(TypeRecord* type) {
  if (TypeIsFloat(type) || TypeIsDouble(type)) {
    return 2;
  }
  if (TypeIsFloat32(type) || TypeIsFloat64(type)) {
    return 1;
  }
  return 0;
}

static int IntRank(void) {
  TypeRecord int_type = {
      .type = kTypeInt,
      .declarator = kDeclPrimitive,
      .size = SizeofType(kTypeInt),
  };
  return GetRank(&int_type);
}

static Type ComplexArithmeticElementType(TypeRecord* left,
                                         TypeRecord* right) {
  Type result = kTypeFloat;
  TypeRecord* operands[] = {left, right};
  for (size_t i = 0; i < sizeof(operands) / sizeof(operands[0]); i++) {
    TypeRecord* operand = operands[i];
    Type element =
        TypeIsComplex(operand) ? TypeComplexElementType(operand)
                               : TypeIsLongDouble(operand) ? kTypeLongDouble
                               : TypeIsDouble(operand) || TypeIsFloat64(operand)
                                   ? kTypeDouble
                               : TypeIsFloat(operand) || TypeIsFloat32(operand)
                                   ? kTypeFloat
                                   : kTypeImplicit;
    if (element == kTypeLongDouble) {
      return kTypeLongDouble;
    }
    if (element == kTypeDouble) {
      result = kTypeDouble;
    }
  }
  return result;
}

static TypeRecord* NewLogicalResultType(void) {
  return NewTypeRecordWithSize(CompilerIsCXX() ? kTypeBool : kTypeInt,
                               kQualPlain);
}

static TypeRecord* NewVectorComparisonResultType(TypeRecord* vector_type) {
  TypeRecord* source = TypeVectorElement(vector_type);
  TypeRecord* element = NULL;
  if (TypeIsIntegral(source)) {
    element = TypeRecordCopy(source);
    element->type &= ~kTypeUnsigned;
    element->type |= kTypeSigned;
    element->qualifiers = kQualPlain;
  } else {
    Type type = source->size == 1 ? kTypeChar
                : source->size == 2 ? kTypeShort
                : source->size == 4 ? kTypeInt
                                    : kTypeLongLong;
    element = NewTypeRecordWithSize(type | kTypeSigned, kQualPlain);
  }
  return NewVectorTypeRecord(element, TypeVectorLaneCount(vector_type));
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
  if (!TypeIsVector(node->sub->type)) {
    SemanticCheckScalarType(node->sub);
  } else if (node->base.op == AST_OP(not)) {
    SemanticError((ASTNode*)node, "logical not is not valid for vector types");
  }
  switch (node->base.op) {
    case AST_OP(not):
      // Not operator is boolean.
      break;
    case AST_OP(uminus): {
      if (TypeIsComplex(node->sub->type) || TypeIsVector(node->sub->type)) {
        break;
      }
      int rank = GetRank(node->sub->type);
      if (!TypeIsBitInt(node->sub->type) && rank < IntRank()) {
        NormalConversion(
            node->sub, NewTypeRecordWithSize(kTypeInt, kQualPlain));
      }
      break;
    }
    default:
      break;
      
  }
  ASTNodeSetType((ASTNode*)node, node->base.op == AST_OP(not)
                                      ? NewLogicalResultType()
                                      : node->sub->type);
}

static void AnalyzeNoexceptExpression(UnaryASTNode* node) {
  if (node->sub != NULL) {
    compiler->noexcept_operand_depth++;
    node->sub = AnalyzeExpression(node->sub);
    compiler->noexcept_operand_depth--;
  }
  ASTNodeSetType((ASTNode*)node, NewLogicalResultType());
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

static void DiagnoseCXX26EnumArithmetic(BinaryASTNode* node) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX26)) {
    return;
  }
  TypeRecord* left = node->left->type;
  TypeRecord* right = node->right->type;
  bool left_enum = TypeIsEnum(left);
  bool right_enum = TypeIsEnum(right);
  bool different_enums =
      left_enum && right_enum && left->info.enum_info != right->info.enum_info;
  bool enum_and_floating =
      (left_enum && TypeIsFloatingPoint(right)) ||
      (right_enum && TypeIsFloatingPoint(left));
  if (different_enums || enum_and_floating) {
    SemanticError((ASTNode*)node,
                  "usual arithmetic conversions between different enumeration "
                  "types or between an enumeration and floating-point type are "
                  "not allowed in C++26");
  }
}

// Check that we have a valid operands for a numeric expression
// and insert conversions as necessary.
static void InsertNumericConversions(BinaryASTNode* node, bool promote_to_int) {
  DiagnoseCXX26EnumArithmetic(node);
  if (TypeIsVector(node->left->type) || TypeIsVector(node->right->type)) {
    if (!TypeIsVector(node->left->type) ||
        !TypeIsVector(node->right->type) ||
        !TypeEqual(node->left->type, node->right->type)) {
      SemanticTypeConversionError(node->left, node->right->type,
                                  "Vector operand types '%s' and '%s' differ");
      return;
    }
    TypeRecord* result = TypeRecordCopy(node->left->type);
    result->qualifiers = kQualPlain;
    ASTNodeSetType((ASTNode*)node, result);
    return;
  }
  if (TypeIsMemberPointer(node->left->type) ||
      TypeIsMemberPointer(node->right->type)) {
    return;
  }
  if (TypeIsComplex(node->left->type) || TypeIsComplex(node->right->type)) {
    bool left_arithmetic = TypeIsComplex(node->left->type) ||
                           TypeIsIntegral(node->left->type) ||
                           TypeIsFloatingPoint(node->left->type);
    bool right_arithmetic = TypeIsComplex(node->right->type) ||
                            TypeIsIntegral(node->right->type) ||
                            TypeIsFloatingPoint(node->right->type);
    if (!left_arithmetic || !right_arithmetic) {
      SemanticTypeConversionError(node->left, node->right->type,
                                  "Illegal numeric operand types "
                                  "'%s' and '%s'");
      return;
    }
    TypeRecord* target =
        NewComplexTypeRecord(
            ComplexArithmeticElementType(node->left->type, node->right->type),
            kQualPlain);
    NormalConversion(node->left, target);
    NormalConversion(node->right, target);
    ASTNodeSetType((ASTNode*)node, target);
    return;
  }
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

      } else if (TypeIsNullPointer(nonptr->type)) {
        // A `nullptr` operand has type std::nullptr_t, which has no arithmetic
        // IR opcode of its own.  Convert it to the pointer operand's type so
        // the operation is generated as an ordinary pointer comparison instead
        // of tripping the backend's opcode lookup.  (A plain `0` operand keeps
        // its int type and is handled by the integral path, as before.)
        NormalConversion(nonptr, ptr->type);
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
      if (!TypeIsBitInt(node->left->type) && left_rank > 0 &&
          left_rank < IntRank()) {
        NormalConversion(
            node->left, NewTypeRecordWithSize(kTypeInt, kQualPlain));
        ASTNodeSetType((ASTNode*)node, node->left->type);
        left_rank = GetRank(node->left->type);
      }
      if (!TypeIsBitInt(node->right->type) && right_rank > 0 &&
          right_rank < IntRank()) {
        NormalConversion(
            node->right, NewTypeRecordWithSize(kTypeInt, kQualPlain));
        ASTNodeSetType((ASTNode*)node, node->right->type);
        right_rank = GetRank(node->right->type);
      }
    }
    // After integer promotion, an integer constant no wider than int adapts to
    // the other operand's type (so e.g. `someUnsignedLong + 1` keeps its type)
    // by being treated as the lowest rank.  A constant with an explicit
    // long/long long type keeps its rank so the usual arithmetic conversions
    // widen the result correctly (e.g. `i + 2L` becomes long).
    if (IsIntConstant(node->left) && !TypeIsBitInt(node->left->type) &&
        !TypeIsBitInt(node->right->type) &&
        left_rank <= IntRank() &&
        !(TypeIsUnsigned(node->left->type) &&
          !TypeIsUnsigned(node->right->type) &&
          left_rank == right_rank)) {
      left_rank = 0;
    }
    if (IsIntConstant(node->right) && !TypeIsBitInt(node->right->type) &&
        !TypeIsBitInt(node->left->type) &&
        right_rank <= IntRank() &&
        !(TypeIsUnsigned(node->right->type) &&
          !TypeIsUnsigned(node->left->type) &&
          left_rank == right_rank)) {
      right_rank = 0;
    }
    // Convert smaller rank to larger.
    if (left_rank == -1 || right_rank == -1) {
      SemanticTypeConversionError(node->left, node->right->type,
                                  "Illegal numeric operand types "
                                  "'%s' and '%s'");
      return;
    }
    if (left_rank > right_rank) {
      // Convert right to left.
      NormalConversion(node->right, node->left->type);
      ASTNodeSetType((ASTNode*)node, node->left->type);
    } else if (left_rank < right_rank) {
      // Convert left to right.
      NormalConversion(node->left, node->right->type);
      ASTNodeSetType((ASTNode*)node, node->right->type);
    } else if (GetFloatingSubrank(node->left->type) >
               GetFloatingSubrank(node->right->type)) {
      NormalConversion(node->right, node->left->type);
      ASTNodeSetType((ASTNode*)node, node->left->type);
    } else if (GetFloatingSubrank(node->left->type) <
               GetFloatingSubrank(node->right->type)) {
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
    } else {
      // Equal-rank operands still determine the expression type.  Leaving the
      // node's parser-default `int` type in place breaks expressions such as
      // `false ? declval<long long>() : declval<long long>()`, and therefore
      // `common_type_t<long long, long long>`.
      ASTNodeSetType((ASTNode*)node, node->left->type);
    }
    // The result of the usual arithmetic conversions is a prvalue of a cv-
    // unqualified type.  Operand types can still carry cv-qualifiers when they
    // arrive through the lvalue-to-rvalue conversion (e.g. via decltype /
    // declval, `declval<const int&>() + declval<long>()`), so normalize the
    // result; otherwise decltype would observe a spurious `const`.
    if (node->base.type != NULL &&
        (TypeIsIntegral(node->base.type) ||
         TypeIsFloatingPoint(node->base.type)) &&
        node->base.type->qualifiers != kQualPlain) {
      TypeRecord* unqualified = TypeRecordCopy(node->base.type);
      unqualified->qualifiers = kQualPlain;
      ASTNodeSetType((ASTNode*)node, unqualified);
      TypeRecordDelete(unqualified);
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
    case AST_OP(arrowstar):
      return "operator->*";
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

static void ADLAddAssociatedNamespaceClosure(Vector* namespaces, Namespace* ns) {
  NamespaceCollectADLAssociatedNamespaces(ns, namespaces);
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
    Namespace* ns =
        str->tag_symbol != NULL ? str->tag_symbol->namespace_ : NULL;
    if (ns == NULL && str->tag_symbol != NULL &&
        str->tag_symbol->type != NULL &&
        str->tag_symbol->type->template_origin != NULL) {
      ns = str->tag_symbol->type->template_origin->namespace_;
    }
    if (ns == NULL && type->template_origin != NULL) {
      ns = type->template_origin->namespace_;
    }
    bool has_associated_class = str->tag_symbol != NULL ||
                                type->template_origin != NULL;
    // A nested class (e.g. an instantiated view's __iterator) carries no
    // namespace on its own tag and is not itself a template instantiation, so
    // walk out through enclosing classes to the nearest namespace.  Without
    // this, ADL over such a type finds none of its hidden-friend operators.
    if (ns == NULL) {
      for (Struct* parent = str->lexical_parent; parent != NULL;
           parent = parent->lexical_parent) {
        if (parent->tag_symbol == NULL) {
          continue;
        }
        has_associated_class = true;
        if (parent->tag_symbol->namespace_ != NULL) {
          ns = parent->tag_symbol->namespace_;
          break;
        }
        if (parent->tag_symbol->type != NULL &&
            parent->tag_symbol->type->template_origin != NULL) {
          ns = parent->tag_symbol->type->template_origin->namespace_;
          break;
        }
      }
    }
    if (has_associated_class) {
      ADLAddAssociatedNamespaceClosure(namespaces,
                                       ns != NULL ? ns : compiler->global_namespace);
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
    ADLAddAssociatedNamespaceClosure(namespaces, tag->namespace_ != NULL
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
  Symbol* alias_extensions =
      first != NULL && first->flags.is_using_alias ? first->overload_next : NULL;
  first = FollowUsingAliasForADL(first);
  // Static member functions link their overloads through the StructMember chain
  // rather than the symbol-level overload_next chain that ordinary
  // (namespace-scope) functions use.  When a qualified name resolves to a static
  // member function (e.g. `Class::f(...)` or `traits_type::assign(...)`), only
  // the single member the lookup returned is reachable via overload_next, so
  // gather the rest of the overload set from the owning struct's member list.
  //
  // This must be limited to *static* members.  Non-static member functions are
  // resolved through ResolveMemberFunctionOverload, which applies the
  // receiver-const tie-breaker; gathering their overloads here and rescoring via
  // the ordinary (penalty-free) path would make const/non-const pairs ambiguous.
  // A non-static member carries an implicit `this` as its first parameter, so a
  // member function with an owner but no leading `this` is the static case.
  bool first_is_static_member =
      first != NULL && first->type != NULL && TypeIsFunction(first->type) &&
      first->type->info.function.cxx_member_owner != NULL &&
      (first->type->info.function.prototype.length == 0 ||
       !StringEqual(
           &((Symbol*)first->type->info.function.prototype.value.p[0])->name,
           "this"));
  if (first_is_static_member && first->overload_next == NULL) {
    Struct* owner = first->type->info.function.cxx_member_owner;
    StructMember* head = FindStructMember(owner, &first->name);
    if (head != NULL) {
      for (StructMember* m = head; m != NULL; m = m->overload_next) {
        if (m->symbol != NULL && m->symbol->type != NULL &&
            TypeIsFunction(m->symbol->type) &&
            !VectorContainsPointer(candidates, m->symbol)) {
          VectorAppend(candidates, m->symbol);
        }
      }
      if (candidates->length > 0) {
        return;
      }
    }
  }
  for (Symbol* candidate = first; candidate != NULL;
       candidate = candidate->overload_next) {
    Symbol* effective = FollowUsingAliasForADL(candidate);
    if (effective != NULL && effective->type != NULL &&
        TypeIsFunction(effective->type) &&
        !VectorContainsPointer(candidates, effective)) {
      VectorAppend(candidates, effective);
    }
  }
  for (Symbol* candidate = alias_extensions; candidate != NULL;
       candidate = candidate->overload_next) {
    Symbol* effective = FollowUsingAliasForADL(candidate);
    if (effective != NULL && effective->type != NULL &&
        TypeIsFunction(effective->type) &&
        !VectorContainsPointer(candidates, effective)) {
      VectorAppend(candidates, effective);
    }
  }
}

static void AddCollectedFunctionSymbols(Vector* candidates, Vector* functions) {
  for (size_t i = 0; i < functions->length; i++) {
    Symbol* sym = (Symbol*)functions->value.p[i];
    Symbol* effective = FollowUsingAliasForADL(sym);
    if (effective != NULL && effective->type != NULL &&
        TypeIsFunction(effective->type) &&
        !VectorContainsPointer(candidates, effective)) {
      VectorAppend(candidates, effective);
    }
  }
}

static void AddNamedFunctionCandidates(String* name, Namespace* ns,
                                       Vector* candidates) {
  Vector functions;
  VectorInit(&functions);
  if (ns == NULL || ns == compiler->global_namespace) {
    NamespaceCollectFunctionSymbolsInInlineSet(compiler->global_namespace, name,
                                               &functions);
    AddCollectedFunctionSymbols(candidates, &functions);
    AddFunctionOverloadCandidates(candidates, FindGlobalSymbol(name));
  } else {
    NamespaceCollectFunctionSymbolsInInlineSet(ns, name, &functions);
    AddCollectedFunctionSymbols(candidates, &functions);
  }
  VectorDestruct(&functions);
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
  if (node->sub->type != NULL) {
    TypeRecord* materialized = TypeMaterializeClassTemplateSpecialization(
        &compiler->syntax, node->sub->type);
    if (materialized != node->sub->type) {
      ASTNodeSetType(node->sub, materialized);
    }
  }
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
  if (node->sub->type != NULL) {
    TypeRecord* materialized = TypeMaterializeClassTemplateSpecialization(
        &compiler->syntax, node->sub->type);
    if (materialized != node->sub->type) {
      ASTNodeSetType(node->sub, materialized);
    }
  }
  if (!TypeIsStructOrUnion(node->sub->type)) {
    return NULL;
  }

  bool postfix = IsPostIncDec(node->base.op);
  StructMember* member =
      FindStructMemberByName(node->sub->type->info.struct_info, op_name);
  bool has_matching_member = false;
  for (StructMember* candidate = member; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->is_member_function && candidate->symbol != NULL &&
        candidate->symbol->type != NULL &&
        TypeIsFunction(candidate->symbol->type)) {
      size_t expected_parameters = postfix ? 2 : 1;
      if (candidate->symbol->type->info.function.prototype.length ==
          expected_parameters) {
        has_matching_member = true;
        break;
      }
    }
  }
  if (has_matching_member) {
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

static TypeRecord* CXXOperatorOperandClassType(TypeRecord* type) {
  if (type == NULL) {
    return NULL;
  }
  if (TypeIsReference(type)) {
    type = type->next;
  }
  return TypeIsStructOrUnion(type) ? type : NULL;
}

// Scores the built-in-vs-member competition for a binary operator's member
// candidate set.  Returns true when at least one member operator overload is
// viable for `node`'s operands, setting *out_best_score to the best (lowest)
// candidate score, *out_ambiguous when two or more distinct overloads tie at
// that best score, and *out_has_template when a template member overload is
// present (whose score is not computed here).  A template member overload is
// treated as viable conservatively (deducing it here would be a premature side
// effect).  Used so a genuinely ambiguous member set does not shadow a better
// non-member (free) operator ([over.match.oper]).
static bool BestBinaryMemberOperatorScore(BinaryASTNode* node,
                                          const char* op_name,
                                          int* out_best_score,
                                          bool* out_ambiguous,
                                          bool* out_has_template) {
  if (out_best_score != NULL) {
    *out_best_score = -1;
  }
  if (out_ambiguous != NULL) {
    *out_ambiguous = false;
  }
  if (out_has_template != NULL) {
    *out_has_template = false;
  }
  ASTNode* left = node->left;
  TypeRecord* class_type = CXXOperatorOperandClassType(left->type);
  if (class_type == NULL || class_type->info.struct_info == NULL) {
    return false;
  }
  StructMember* first =
      FindStructMemberByName(class_type->info.struct_info, op_name);
  if (first == NULL || !first->is_member_function) {
    return false;
  }
  Vector children;
  VectorInit(&children);
  VectorAppend(&children, node->right);
  VectorASTNode call = {0};
  call.children = &children;
  bool receiver_const = MemberReceiverIsConst(node);
  bool receiver_volatile = MemberReceiverIsVolatile(node);
  bool viable = false;
  int best_score = -1;
  int best_count = 0;
  for (StructMember* candidate = first; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->symbol == NULL || candidate->symbol->type == NULL ||
        !TypeIsFunction(candidate->symbol->type)) {
      continue;
    }
    if (candidate->symbol->flags.is_template) {
      viable = true;
      if (out_has_template != NULL) {
        *out_has_template = true;
      }
      continue;
    }
    int score = FunctionCallScore(candidate->symbol->type, &call,
                                  /*first_formal_arg=*/1);
    if (score < 0) {
      continue;
    }
    if (receiver_const &&
        !candidate->symbol->type->info.function.is_const_member &&
        !candidate->symbol->type->info.function.is_constructor &&
        !candidate->symbol->type->info.function.is_destructor) {
      continue;
    }
    if (receiver_volatile &&
        !candidate->symbol->type->info.function.is_volatile_member &&
        !candidate->symbol->type->info.function.is_constructor &&
        !candidate->symbol->type->info.function.is_destructor) {
      continue;
    }
    if (!MemberReceiverMatchesRefQualifier(candidate->symbol->type, node)) {
      continue;
    }
    viable = true;
    if (best_score < 0 || score < best_score) {
      best_score = score;
      best_count = 1;
    } else if (score == best_score) {
      best_count++;
    }
  }
  VectorDestruct(&children);
  if (out_best_score != NULL) {
    *out_best_score = best_score;
  }
  if (out_ambiguous != NULL) {
    *out_ambiguous = best_count >= 2;
  }
  return viable;
}

// Conversion rank of a free binary operator's *second* parameter against the
// binary node's right operand.  A member operator is scored over its second
// operand only (its first operand is the implicit object argument), so this is
// the free-operator score on the same scale, letting a member and a non-member
// operator compete on the operand that distinguishes them.  Returns -1 when the
// candidate is not a viable binary free operator.
static int FreeBinaryOperatorSecondOperandScore(Symbol* function,
                                                ASTNode* right) {
  if (function == NULL || function->type == NULL ||
      !TypeIsFunction(function->type)) {
    return -1;
  }
  Vector* prototype = &function->type->info.function.prototype;
  if (prototype->length < 2) {
    return -1;
  }
  Symbol* second = prototype->value.p[1];
  if (second == NULL || second->type == NULL) {
    return -1;
  }
  return OverloadConversionRank(right, second->type);
}

static ASTNode* BuildMemberOperatorCall(BinaryASTNode* node,
                                        const char* op_name) {
  ASTNode* left = ASTNodeMove(node->left);
  ASTNode* right = ASTNodeMove(node->right);
  ASTNode* member_name =
      NewStringConstantASTNode(NewString(op_name), NULL, node->base.location);
  ASTNode* member_access = NewBinaryASTNode(AST_OP(dot), NULL,
                                            node->base.location, left,
                                            member_name);
  Vector* actuals = NewVector();
  VectorAppend(actuals, right);
  ASTNode* call = NewVectorASTNode(AST_OP(call), NULL, node->base.location,
                                   member_access, actuals);
  return ReplaceBinaryWithCall(node, call);
}

static ASTNode* TryAnalyzeOverloadedBinaryOperator(BinaryASTNode* node) {
  const char* op_name = BinaryOperatorFunctionName(node->base.op);
  if (!CompilerIsCXX() || op_name == NULL ||
      node->left == NULL || node->right == NULL ||
      node->left->type == NULL || node->right->type == NULL) {
    return NULL;
  }
  // A constructor member-initializer that targets a reference data member binds
  // the reference; it is not an assignment through the referent.  Do not resolve
  // it as `referent.operator=(...)` -- leave it for AnalyzeAssignmentExpression's
  // reference-binding path.  (Without this, a class-type referent's operator=
  // would be selected here, before the assignment analysis runs.)
  if (node->base.op == AST_OP(assign) &&
      (node->base.flags & kASTCXXMemberInitializer) != 0 &&
      node->left != NULL && node->left->type != NULL &&
      TypeIsReference(node->left->type)) {
    return NULL;
  }
  TypeRecord* left_class = CXXOperatorOperandClassType(node->left->type);
  TypeRecord* right_class = CXXOperatorOperandClassType(node->right->type);
  if (left_class == NULL && right_class == NULL) {
    return NULL;
  }

  bool member_present = false;
  if (left_class != NULL && left_class->info.struct_info != NULL) {
    StructMember* member = FindStructMemberByName(left_class->info.struct_info,
                                                  op_name);
    member_present = member != NULL && member->is_member_function;
  }

  // Member and non-member (free) operators of the same name compete together
  // ([over.match.oper]); a member operator does not hide the free ones.  Score
  // the best member candidate over its second operand and, when a free operator
  // converts that same operand strictly better -- or the member set is ambiguous
  // by itself and the free operator is no worse -- choose the free operator.
  // This is what makes `ostream << "..."` (and `<< 'c'`) select the exact-match
  // free `operator<<(basic_ostream&, const CharT*)` (resp. `CharT`) instead of a
  // member `operator<<(bool)` / `operator<<(const void*)` that only matches via a
  // worse conversion.  A template member overload's conversion cost is not
  // modeled here, so keep the historical member preference when one is viable.
  bool member_has_template = false;
  int member_best_score = -1;
  bool member_ambiguous = false;
  bool member_viable =
      member_present &&
      BestBinaryMemberOperatorScore(node, op_name, &member_best_score,
                                    &member_ambiguous, &member_has_template);

  String name;
  StringInit(&name, op_name);
  Vector lookup_actuals;
  VectorInit(&lookup_actuals);
  VectorAppend(&lookup_actuals, node->left);
  VectorAppend(&lookup_actuals, node->right);
  // Only diagnose an ambiguous free set when there is no member fallback: with a
  // viable member operator, a silently-ambiguous free set must not turn a
  // previously valid member call into a hard error.
  Symbol* function = ResolveFreeFunctionWithADL(
      &name, &lookup_actuals, /*diagnose_ambiguous=*/!member_viable);
  VectorDestruct(&lookup_actuals);

  bool use_free = false;
  if (function != NULL && TypeIsFunction(function->type)) {
    if (!member_viable) {
      use_free = true;
    } else if (!member_has_template) {
      int free_score = FreeBinaryOperatorSecondOperandScore(function,
                                                            node->right);
      if (free_score >= 0 &&
          (free_score < member_best_score ||
           (member_ambiguous && free_score <= member_best_score))) {
        use_free = true;
      }
    }
  }
  if (use_free) {
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

  // The free operator was not preferred.  If a member operator exists, build the
  // member call: the existing member-overload machinery either performs the call
  // or emits a precise diagnostic (including for an ambiguous member set).
  if (member_present) {
    return BuildMemberOperatorCall(node, op_name);
  }
  return NULL;
}

static ASTNode* TryAnalyzeOverloadedBinaryOperatorWithAnalyzedOperands(
    BinaryASTNode* node) {
  if (BinaryOperatorFunctionName(node->base.op) == NULL) {
    return NULL;
  }
  if (node->left == NULL || node->right == NULL) {
    return NULL;
  }
  // A braced-init-list operand (e.g. the right-hand side of `x = {...}`) is not
  // an expression and has no type, so it cannot participate in
  // overloaded-operator resolution here.  Defer to the operator's dedicated
  // analysis, which lowers the braced-init-list to a temporary of the
  // appropriate type first.
  if ((node->left != NULL && node->left->op == AST_OP(braced_init)) ||
      (node->right != NULL && node->right->op == AST_OP(braced_init))) {
    return NULL;
  }
  ASTNode* analyzed_left = AnalyzeExpression(node->left);
  ASTNode* analyzed_right = AnalyzeExpression(node->right);
  if (analyzed_left != node->left) {
    ASTNodeReplaceChild((ASTNode*)node, 0, analyzed_left,
                        /*delete_old_child=*/false);
  }
  if (analyzed_right != node->right) {
    ASTNodeReplaceChild((ASTNode*)node, 1, analyzed_right,
                        /*delete_old_child=*/false);
  }
  ASTNodeSetType((ASTNode*)node, node->left->type);
  return TryAnalyzeOverloadedBinaryOperator(node);
}

// A binary plus operator allows an integer to be added to a pointer (or array).
// The integer is scaled (multiplied) by the size of the thing pointed to.
// The result of pointer arithmetic (p + n, n + p, p - n) is a prvalue whose
// type is the cv-unqualified pointer type: a top-level const/volatile on the
// operand (e.g. `int* const`) does not carry over to the computed value.  Only
// the top-level qualifiers are dropped; the pointee's qualifiers are retained.
static TypeRecord* PointerArithmeticResultType(TypeRecord* pointer_type) {
  if (pointer_type == NULL) {
    return pointer_type;
  }
  // An array operand decays to a (cv-unqualified) prvalue pointer to its
  // element type: `decltype(arr + 0)` is `T*`, not the array type `T[N]`.
  if (TypeIsArray(pointer_type)) {
    TypeRecord* ptr = NewPointerTypeRecord(kQualPlain);
    TypeRecordChain(ptr, pointer_type->next);
    return ptr;
  }
  if (pointer_type->qualifiers == kQualPlain) {
    return pointer_type;
  }
  TypeRecord* plain = TypeRecordCopy(pointer_type);
  plain->qualifiers = kQualPlain;
  return plain;
}

// True if `node` is the multiply-by-element-size node that pointer arithmetic
// inserts, so a re-analyzed `p + n` does not scale `n` twice.  A pointer
// *difference* is also a ptr_scale node, but a dividing one, and it is an
// ordinary integer operand that still needs scaling: without this distinction
// `p + (q - r)` would advance `p` by bytes instead of elements.
static bool ASTNodeIsPointerScaleMultiply(ASTNode* node) {
  return node != NULL && node->op == AST_OP(ptr_scale) &&
         ((PtrScaleASTNode*)node)->scale_op == AST_OP(mult);
}

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
  if (!TypeIsVector(node->left->type)) {
    SemanticCheckScalarType(node->left);
  }
  if (!TypeIsVector(node->right->type)) {
    SemanticCheckScalarType(node->right);
  }
  if (TypeIsPointerOrArray(node->left->type)) {
    ASTNodeSetType((ASTNode*)node,
                   PointerArithmeticResultType(node->left->type));
    if (ASTNodeIsPointerScaleMultiply(node->right)) {
      return &node->base;
    }
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
    ASTNodeSetType((ASTNode*)node,
                   PointerArithmeticResultType(node->right->type));
    if (ASTNodeIsPointerScaleMultiply(node->left)) {
      return &node->base;
    }
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
      ASTNodeSetType((ASTNode*)node,
                     PointerArithmeticResultType(node->left->type));
      // Scale right side by size of left.
      // If the right node is a constant we can do the multiplication now.  The
      // scaled operand is a count of bytes, so it keeps the integer type it had
      // rather than taking the pointer's, as in binary plus above.
      if (ASTNodeIsIntConstant(node->right)) {
        int64_t size = node->left->type->next->size;
        int64_t value = ASTNodeConstantValue(node->right);
        ASTNode* scale = NewIntConstantASTNode(value * size, node->right->type,
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
      // pointed to.  Both operands are pointer-or-array here, so an array
      // operand decays to a pointer to its element type; compare the pointed-to
      // element types rather than requiring identical declarators.
      if (node->left->type->next == NULL || node->right->type->next == NULL ||
          node->left->type->next->type != node->right->type->next->type) {
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
  
        // The result of subtracting two pointers is the signed type
        // ptrdiff_t (`long` on this target), not an unsigned type.
        ASTNodeSetType(scale,
                       NewTypeRecordWithSize(kTypeLong, kQualPlain));
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

// Integer-promote one operand of a shift in place.  A type narrower than int
// becomes int and not merely a wider version of itself, because int can hold
// every one of its values; the exception is an unsigned type int is no wider
// than, which promotes to unsigned int instead (C11 6.3.1.1p2).  Getting this
// wrong leaves `(unsigned short)x << 1` unsigned, and the whole point of
// 6.5.7p3 giving the result the promoted left type is that it is signed.
static void PromoteShiftOperand(ASTNode* operand) {
  int rank = GetRank(operand->type);
  if (TypeIsBitInt(operand->type) || rank <= 0 || rank >= IntRank()) {
    return;
  }
  Type promoted = kTypeInt;
  if (TypeIsUnsigned(operand->type) &&
      operand->type->size >= SizeofType(kTypeInt)) {
    promoted |= kTypeUnsigned;
  }
  NormalConversion(operand, NewTypeRecordWithSize(promoted, kQualPlain));
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
  bool vector_shift =
      TypeIsVector(node->left->type) && TypeIsVector(node->right->type) &&
      TypeEqual(node->left->type, node->right->type);
  if (!vector_shift) {
    SemanticCheckScalarType(node->left);
    SemanticCheckScalarType(node->right);
  }
  if (vector_shift) {
    if (!TypeIsIntegral(TypeVectorElement(node->left->type))) {
      SemanticError((ASTNode*)node, "Shift operator needs integral types");
    }
  } else if (!TypeIsIntegral(node->left->type) ||
             !TypeIsIntegral(node->right->type)) {
    SemanticError((ASTNode*)node, "Shift operator needs integral types");
  }

  // Each operand of a shift is integer-promoted independently and the type of
  // the result is the promoted type of the LEFT operand (C11 6.5.7p3).  The
  // usual arithmetic conversions are NOT applied, so the right operand's type
  // (e.g. a `long long` shift count) must not widen the result.
  if (!vector_shift) {
    PromoteShiftOperand(node->left);
    PromoteShiftOperand(node->right);
  }
  ASTNodeSetType((ASTNode*)node, node->left->type);

  // Convert node opcode to correct shift type.   An unsigned type uses a
  // logical shift an a signed type uses an arithmetic (sign extension) shift.
  if (node->base.op == AST_OP(rshift)) {
    // Right shift only.
    TypeRecord* signedness_type =
        TypeIsVector(node->base.type) ? TypeVectorElement(node->base.type)
                                     : node->base.type;
    if (TypeIsUnsigned(signedness_type)) {
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
  bool vector_bitwise =
      TypeIsVector(node->left->type) && TypeIsVector(node->right->type) &&
      TypeEqual(node->left->type, node->right->type);
  if (!vector_bitwise) {
    SemanticCheckScalarType(node->left);
    SemanticCheckScalarType(node->right);
  }
  if (vector_bitwise) {
    if (!TypeIsIntegral(TypeVectorElement(node->left->type))) {
      SemanticError((ASTNode*)node, "Bitwise operator needs integral types");
    } else {
      InsertNumericConversions(node, true);
    }
  } else if (!TypeIsIntegral(node->left->type) ||
             !TypeIsIntegral(node->right->type)) {
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

// Returns true if `op_name` is a viable binary operator for the ordered operand
// pair `(left, right)`: either a member operator on `left`'s class callable with
// `right`, or a free operator found by ordinary lookup / ADL over `(left,
// right)`.  Used to decide the orientation of a C++20 rewritten comparison
// without committing to (and possibly mis-diagnosing) a non-viable rewrite.
static bool BinaryOperatorViableForOperands(ASTNode* left, ASTNode* right,
                                            const char* op_name) {
  if (left == NULL || right == NULL) {
    return false;
  }
  if (TypeIsStructOrUnion(left->type) && left->type->info.struct_info != NULL) {
    StructMember* first =
        FindStructMemberByName(left->type->info.struct_info, op_name);
    if (first != NULL && first->is_member_function) {
      Vector children;
      VectorInit(&children);
      VectorAppend(&children, right);
      VectorASTNode call = {0};
      call.children = &children;
      bool viable = false;
      for (StructMember* candidate = first; candidate != NULL;
           candidate = candidate->overload_next) {
        if (candidate->symbol == NULL || candidate->symbol->type == NULL ||
            !TypeIsFunction(candidate->symbol->type)) {
          continue;
        }
        if (candidate->symbol->flags.is_template ||
            FunctionCallScore(candidate->symbol->type, &call,
                              /*first_formal_arg=*/1) >= 0) {
          viable = true;
          break;
        }
      }
      VectorDestruct(&children);
      if (viable) {
        return true;
      }
    }
  }
  String name;
  StringInit(&name, op_name);
  Vector actuals;
  VectorInit(&actuals);
  VectorAppend(&actuals, left);
  VectorAppend(&actuals, right);
  Symbol* function =
      ResolveFreeFunctionWithADL(&name, &actuals, /*diagnose_ambiguous=*/false);
  VectorDestruct(&actuals);
  StringDestruct(&name);
  return function != NULL && TypeIsFunction(function->type);
}

// C++20 rewritten comparison candidates ([over.match.oper]): when a relational
// or `!=` operator has a class operand with no directly-usable overload, rewrite
// it in terms of `operator<=>` / `operator==`:
//   a @ b   (@ in < > <= >=)  ->  (a <=> b) @ 0   or   0 @ (b <=> a) reversed
//   a != b                    ->  !(a == b)       (the `==` may reverse below)
// `==` itself is left to operator== (see TryReversedComparisonOperator for the
// reversed `b == a` candidate), matching the standard.
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
    // Prefer the as-written rewrite `(a <=> b) @ 0`.  Only when that spaceship
    // is not viable but the reversed one is, synthesize `0 @ (b <=> a)`.
    bool forward_viable =
        BinaryOperatorViableForOperands(node->left, node->right, "operator<=>");
    bool reverse_viable =
        !forward_viable &&
        BinaryOperatorViableForOperands(node->right, node->left, "operator<=>");
    ASTNode* left = ASTNodeMove(node->left);
    ASTNode* right = ASTNodeMove(node->right);
    ASTNode* zero = NewIntConstantASTNode(
        0, NewTypeRecordWithSize(kTypeInt, kQualPlain), loc);
    ASTNode* rewritten;
    if (reverse_viable) {
      ASTNode* cmp =
          NewBinaryASTNode(AST_OP(spaceship), NULL, loc, right, left);
      rewritten = NewBinaryASTNode(op, NULL, loc, zero, cmp);
    } else {
      ASTNode* cmp =
          NewBinaryASTNode(AST_OP(spaceship), NULL, loc, left, right);
      rewritten = NewBinaryASTNode(op, NULL, loc, cmp, zero);
    }
    return ReplaceBinaryWithCall(node, rewritten);
  }
  return NULL;
}

// C++20 reversed `==` candidate ([over.match.oper]): when `a == b` has no viable
// as-written `operator==`, try the reversed `b == a` (member on `b`'s class or a
// free operator matching the swapped operands).  The reversed node is flagged so
// it is not itself reversed again.  Reversed `!=` is handled by the `==` rewrite
// above whose inner `a == b` reaches this same path.
static ASTNode* TryReversedComparisonOperator(BinaryASTNode* node) {
  if (!CompilerIsCXX() || node->left == NULL || node->right == NULL) {
    return NULL;
  }
  if ((node->base.flags & kASTReversedComparison) != 0) {
    return NULL;
  }
  if (node->base.op != AST_OP(equal)) {
    return NULL;
  }
  if (!TypeIsStructOrUnion(node->left->type) &&
      !TypeIsStructOrUnion(node->right->type)) {
    return NULL;
  }
  if (!BinaryOperatorViableForOperands(node->right, node->left, "operator==")) {
    return NULL;
  }
  SourceLocation loc = node->base.location;
  ASTNode* left = ASTNodeMove(node->left);
  ASTNode* right = ASTNodeMove(node->right);
  ASTNode* reversed = NewBinaryASTNode(AST_OP(equal), NULL, loc, right, left);
  reversed->flags |= kASTReversedComparison;
  return ReplaceBinaryWithCall(node, reversed);
}

// A pointer only compares against another pointer or a null pointer constant.
// C++ has no implicit conversion between pointers and integers, so an operand
// pair like `int* != long` makes the comparison ill-formed -- which is what
// makes `sentinel_for<long, int*>` false and therefore what keeps overload sets
// such as `ranges::advance(i, n)` versus `ranges::advance(i, bound)`
// unambiguous.  C only makes it a constraint violation, so warn there.
static void CheckComparisonOfPointerAndInteger(BinaryASTNode* node) {
  ASTNode* pointer = NULL;
  ASTNode* integer = NULL;
  if (TypeIsPointer(node->left->type) && TypeIsIntegral(node->right->type)) {
    pointer = node->left;
    integer = node->right;
  } else if (TypeIsIntegral(node->left->type) &&
             TypeIsPointer(node->right->type)) {
    pointer = node->right;
    integer = node->left;
  }
  if (pointer == NULL || IsNullPointer(integer)) {
    return;
  }
  if (CompilerIsCXX()) {
    SemanticError((ASTNode*)node,
                  "Comparison between a pointer and an integer");
  } else {
    SemanticWarning((ASTNode*)node, "int-conversion",
                    "comparison between a pointer and an integer");
  }
}

static bool DiagnoseCXX26ArrayComparison(BinaryASTNode* node) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX26) ||
      !TypeIsArray(node->left->type) || !TypeIsArray(node->right->type)) {
    return false;
  }
  SemanticError((ASTNode*)node,
                "comparison between two arrays is not allowed in C++26");
  return true;
}

static ASTNode* AnalyzeComparisonOperator(BinaryASTNode* node) {
  node->left = AnalyzeExpression(node->left);
  node->right = AnalyzeExpression(node->right);
  ASTNodeSetType((ASTNode*)node, node->left->type);
  ASTNode* overloaded = TryAnalyzeOverloadedBinaryOperator(node);
  if (overloaded != NULL) {
    return overloaded;
  }
  if ((TypeIsComplex(node->left->type) ||
       TypeIsComplex(node->right->type)) &&
      node->base.op != AST_OP(equal) &&
      node->base.op != AST_OP(noteq)) {
    SemanticError((ASTNode*)node,
                  "Only == and != are valid for complex operands");
    ASTNodeSetType((ASTNode*)node, NewLogicalResultType());
    return (ASTNode*)node;
  }
  if (DiagnoseCXX26ArrayComparison(node)) {
    ASTNodeSetType((ASTNode*)node, NewLogicalResultType());
    return (ASTNode*)node;
  }
  if (TypeIsMemberPointer(node->left->type) &&
      (TypeIsMemberPointer(node->right->type) ||
       (node->right != NULL &&
        (TypeIsNullPointer(node->right->type) ||
         (IsIntConstant(node->right) &&
          ((ConstantASTNode*)node->right)->value.ivalue == 0))))) {
    if (node->base.op != AST_OP(equal) &&
        node->base.op != AST_OP(noteq)) {
      SemanticError((ASTNode*)node,
                    "Only == and != are valid for pointers to members");
      ASTNodeSetType((ASTNode*)node, NewLogicalResultType());
      return (ASTNode*)node;
    }
    TypeRecord* comparison_type = node->left->type;
    if (TypeIsMemberPointer(node->right->type) &&
        !TypeEqual(node->left->type, node->right->type)) {
      if (MemberPointerCanConvert(node->left->type, node->right->type,
                                  /*is_cast=*/false, NULL)) {
        SemanticConvertType(node->left, node->right->type,
                            kConvertNormal);
        comparison_type = node->right->type;
      } else if (MemberPointerCanConvert(node->right->type, node->left->type,
                                         /*is_cast=*/false, NULL)) {
        SemanticConvertType(node->right, node->left->type,
                            kConvertNormal);
      } else {
        SemanticError((ASTNode*)node,
                      "Pointers to members of unrelated classes cannot be compared");
      }
    }
    SemanticCheckScalarType(node->left);
    if (TypeIsMemberPointer(node->right->type)) {
      SemanticCheckScalarType(node->right);
    } else {
      MemberPointerValue null_value;
      MemberPointerEncodeNull(comparison_type, &null_value);
      ASTNode* converted_null = NewIntConstantASTNode(
          null_value.ptr, comparison_type, node->right->location);
      ASTNodeReplaceChild((ASTNode*)node, 1, converted_null, true);
    }
    MemberPointerValue left_value;
    MemberPointerValue right_value;
    bool left_constant = MemberPointerTryEvaluateConstant(
        node->left, comparison_type, &left_value);
    bool right_constant = false;
    right_constant = MemberPointerTryEvaluateConstant(
        node->right, comparison_type, &right_value);
    if (left_constant && right_constant) {
      bool equal = MemberPointerValuesEqual(comparison_type, &left_value,
                                            &right_value);
      if (node->base.op == AST_OP(noteq)) {
        equal = !equal;
      }
      return NewIntConstantASTNode(
          equal ? 1 : 0, NewLogicalResultType(),
          node->base.location);
    }
    ASTNodeSetType((ASTNode*)node, NewLogicalResultType());
    return (ASTNode*)node;
  }
  if (TypeIsMemberPointer(node->right->type) &&
      node->left != NULL &&
      (TypeIsNullPointer(node->left->type) ||
       (IsIntConstant(node->left) &&
        ((ConstantASTNode*)node->left)->value.ivalue == 0))) {
    if (node->base.op != AST_OP(equal) &&
        node->base.op != AST_OP(noteq)) {
      SemanticError((ASTNode*)node,
                    "Only == and != are valid for pointers to members");
      ASTNodeSetType((ASTNode*)node, NewLogicalResultType());
      return (ASTNode*)node;
    }
    SemanticCheckScalarType(node->right);
    MemberPointerValue null_value;
    MemberPointerEncodeNull(node->right->type, &null_value);
    ASTNode* converted_null = NewIntConstantASTNode(
        null_value.ptr, node->right->type, node->left->location);
    ASTNodeReplaceChild((ASTNode*)node, 0, converted_null, true);
    MemberPointerValue left_value;
    MemberPointerValue right_value;
    if (MemberPointerEncodeNull(node->right->type, &left_value) &&
        MemberPointerTryEvaluateConstant(node->right, node->right->type,
                                         &right_value)) {
      bool equal = MemberPointerValuesEqual(node->right->type, &left_value,
                                            &right_value);
      if (node->base.op == AST_OP(noteq)) {
        equal = !equal;
      }
      return NewIntConstantASTNode(
          equal ? 1 : 0, NewLogicalResultType(),
          node->base.location);
    }
    ASTNodeSetType((ASTNode*)node, NewLogicalResultType());
    return (ASTNode*)node;
  }
  if (TypeIsNullPointer(node->left->type) &&
      TypeIsNullPointer(node->right->type)) {
    if (node->base.op != AST_OP(equal) &&
        node->base.op != AST_OP(noteq)) {
      SemanticError((ASTNode*)node,
                    "Only == and != are valid for %s",
                    CompilerIsCXX() ? "std::nullptr_t" : "nullptr_t");
    }
    ASTNodeSetType((ASTNode*)node, NewLogicalResultType());
    return (ASTNode*)node;
  }
  ASTNode* rewritten = TryRewriteComparisonOperator(node);
  if (rewritten != NULL) {
    return rewritten;
  }
  ASTNode* reversed = TryReversedComparisonOperator(node);
  if (reversed != NULL) {
    return reversed;
  }
  if (TypeIsVector(node->left->type) || TypeIsVector(node->right->type)) {
    InsertNumericConversions(node, true);
    if (TypeIsVector(node->left->type) &&
        TypeIsVector(node->right->type) &&
        TypeEqual(node->left->type, node->right->type)) {
      ASTNodeSetType((ASTNode*)node,
                     NewVectorComparisonResultType(node->left->type));
    }
    return (ASTNode*)node;
  }
  SemanticCheckScalarType(node->left);
  SemanticCheckScalarType(node->right);
  CheckComparisonOfPointerAndInteger(node);
  if (TypeIsIntegral(node->left->type) && TypeIsIntegral(node->right->type) &&
      TypeIsUnsigned(node->left->type) != TypeIsUnsigned(node->right->type) &&
      !IsZeroIntegerConstant(node->left) && !IsZeroIntegerConstant(node->right)) {
    SemanticWarning((ASTNode*)node, "sign-compare",
                    "comparison of integers of different signs");
  }
  InsertNumericConversions(node, true);

  // Comparisons produce bool in C++ and int in C.
  ASTNodeSetType((ASTNode*)node, NewLogicalResultType());
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
  if (TypeIsComplex(node->left->type) ||
      TypeIsComplex(node->right->type)) {
    SemanticError((ASTNode*)node,
                  "Three-way comparison is not valid for complex operands");
    ASTNodeSetType((ASTNode*)node,
                   NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return (ASTNode*)node;
  }
  if (DiagnoseCXX26ArrayComparison(node)) {
    ASTNodeSetType((ASTNode*)node,
                   NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return (ASTNode*)node;
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

static bool TypeEqualIgnoringQualifiers(TypeRecord* left, TypeRecord* right);
static bool TryConvertWithConvertingConstructorImpl(
    ASTNode* from, TypeRecord* to, ConversionContext ctx,
    bool allow_same_class);

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

static bool TryAnalyzeCConditionalObjectPointers(BinaryASTNode* node,
                                                 BinaryASTNode* colon) {
  if (CompilerIsCXX() ||
      !TypeIsPointerOrArray(colon->left->type) ||
      !TypeIsPointerOrArray(colon->right->type) ||
      colon->left->type->next == NULL || colon->right->type->next == NULL ||
      TypeIsFunction(colon->left->type->next) ||
      TypeIsFunction(colon->right->type->next)) {
    return false;
  }
  TypeRecord* left_pointee = colon->left->type->next;
  TypeRecord* right_pointee = colon->right->type->next;
  bool has_void =
      TypeIsVoid(left_pointee) || TypeIsVoid(right_pointee);
  Qualifiers cv_mask =
      kQualConst | kQualVolatile | kQualRestrict | kQualAtomic;
  if (!has_void &&
      !TypeEqualIgnoringTopLevelQualifierMask(left_pointee, right_pointee,
                                               cv_mask)) {
    return false;
  }
  TypeRecord* pointee =
      TypeRecordCopy(TypeIsVoid(left_pointee) ? left_pointee :
                     TypeIsVoid(right_pointee) ? right_pointee :
                     left_pointee);
  pointee->qualifiers =
      (left_pointee->qualifiers | right_pointee->qualifiers) & cv_mask;
  // The pointer owns the pointee from here on, so only the pointer is released.
  TypeRecord* result = NewPointerTo(kQualPlain, pointee);
  ASTNodeSetType((ASTNode*)colon, result);
  ASTNodeSetType((ASTNode*)node, result);
  colon->base.value_category = kValueCategoryPrvalue;
  node->base.value_category = kValueCategoryPrvalue;
  TypeRecordDelete(result);
  return true;
}

static void AnalyzeConditionalExpression(BinaryASTNode* node) {
  node->left = AnalyzeExpression(node->left);
  if (node->left == NULL) {
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return;
  }
  SemanticConvertType(node->left, NewTypeRecordWithSize(kTypeBool, kQualPlain),
                      kConvertContextualBool);
  if (!TypeIsScalar(node->left->type)) {
    SemanticError((ASTNode*)node, "Condition for ? operator must be scalar");
    ASTNodeSetType((ASTNode*)node, node->left->type);
    return;
  }
  BinaryASTNode* colon = (BinaryASTNode*)node->right;
  if (colon == NULL || colon->base.op != AST_OP(colon)) {
    ASTNodeSetType((ASTNode*)node, node->left->type);
    return;
  }
  colon->left = AnalyzeExpression(colon->left);
  colon->right = AnalyzeExpression(colon->right);
  if (colon->left == NULL || colon->right == NULL) {
    ASTNode* arm = colon->left != NULL ? colon->left : colon->right;
    TypeRecord* type = NULL;
    if (arm != NULL && arm->type != NULL) {
      type = arm->type;
    } else if (node->left->type != NULL) {
      type = node->left->type;
    } else {
      type = NewTypeRecordWithSize(kTypeInt, kQualPlain);
      ASTNodeSetType((ASTNode*)colon, type);
      ASTNodeSetType((ASTNode*)node, type);
      TypeRecordDelete(type);
      return;
    }
    ASTNodeSetType((ASTNode*)colon, type);
    ASTNodeSetType((ASTNode*)node, type);
    return;
  }
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
    // In C++ [expr.cond], if exactly one arm has void type -- and neither is a
    // throw-expression (handled above) -- the conditional is ill-formed.  In C,
    // GCC accepts a mixed void/non-void conditional as an extension and gives
    // the whole expression type void (this arises, for instance, when one arm
    // is a statement expression whose last statement is not an expression), so
    // we mirror that and do not diagnose.
    if (CompilerIsCXX() &&
        (!TypeIsVoid(colon->left->type) || !TypeIsVoid(colon->right->type))) {
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
  if (TryAnalyzeCConditionalObjectPointers(node, colon)) {
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
  if (CompilerIsCXX() &&
      TypeIsStructOrUnion(colon->left->type) &&
      TypeIsStructOrUnion(colon->right->type) &&
      TypeEqualIgnoringQualifiers(colon->left->type, colon->right->type)) {
    // [expr.cond]: when same-class operands do not already share a glvalue
    // category, an lvalue arm opposite a prvalue arm is copy-initialized into
    // a prvalue.  Merely assigning the common class type here leaves codegen
    // selecting between object addresses and can return a pointer into the
    // callee's dead stack frame.
    if (colon->left->value_category == kValueCategoryPrvalue &&
        colon->right->value_category != kValueCategoryPrvalue) {
      TryConvertWithConvertingConstructorImpl(
          colon->right, colon->left->type, kConvertNormal,
          /*allow_same_class=*/true);
    } else if (colon->right->value_category == kValueCategoryPrvalue &&
               colon->left->value_category != kValueCategoryPrvalue) {
      TryConvertWithConvertingConstructorImpl(
          colon->left, colon->right->type, kConvertNormal,
          /*allow_same_class=*/true);
    }
    TypeRecord* result_type = TypeRecordCopy(colon->left->type);
    result_type->qualifiers = kQualPlain;
    ASTNodeSetType((ASTNode*)colon, result_type);
    ASTNodeSetType((ASTNode*)node, colon->base.type);
    TypeRecordDelete(result_type);
    return;
  }
  if (CompilerIsCXX() && TypeIsMemberPointer(colon->left->type) &&
      TypeIsMemberPointer(colon->right->type) &&
      TypeEqual(colon->left->type, colon->right->type)) {
    ASTNodeSetType((ASTNode*)colon, colon->left->type);
    ASTNodeSetType((ASTNode*)node, colon->base.type);
    colon->base.value_category = kValueCategoryPrvalue;
    node->base.value_category = kValueCategoryPrvalue;
    return;
  }
  InsertNumericConversions(colon, false);
  if (colon->base.type != NULL && TypeIsComplex(colon->base.type)) {
    ASTNode* operands[] = {colon->left, colon->right};
    for (size_t i = 0; i < sizeof(operands) / sizeof(operands[0]); i++) {
      ASTNode* operand = operands[i];
      int child_id = operand->child_id;
      TypeRecord* target = TypeRecordCopy(colon->base.type);
      ASTNode* converted =
          NewCastASTNode(target, operand->location, operand);
      ASTNodeSetType(converted, target);
      converted->value_category = kValueCategoryPrvalue;
      converted->flags |= kASTAnalyzed;
      ASTNodeReplaceChild((ASTNode*)colon, child_id, converted, false);
    }
    colon->base.value_category = kValueCategoryPrvalue;
  }
  // [expr.cond]/7: when the operands have arithmetic (or unscoped enumeration)
  // type, the usual arithmetic conversions apply and the result is a prvalue of
  // the cv-unqualified common type (InsertNumericConversions strips the cv).
  if (colon->base.type != NULL &&
      (TypeIsIntegral(colon->base.type) ||
       TypeIsFloatingPoint(colon->base.type))) {
    colon->base.value_category = kValueCategoryPrvalue;
  }
  ASTNodeSetType((ASTNode*)node, colon->base.type);
  node->base.value_category = colon->base.value_category;
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
    case AST_OP(comma):
      // In C++, the built-in comma expression has the value category of its
      // right operand.  Converting-constructor lowering represents an already
      // materialized class temporary as `(constructor_call, temporary)`;
      // recognizing the temporary's address here avoids materializing and
      // moving that same object a second time when it binds to a reference.
      return CompilerIsCXX() &&
             HasAddress(((BinaryASTNode*)node)->right);
    default:
      return false;
  }
}

static ASTNode* MaterializeTemporary(ASTNode* expr, TypeRecord* type) {
  SourceLocation location = expr->location;
  // Temporary materialization preserves the prvalue's object type.  The
  // reference target may add top-level cv-qualification (e.g. `const T&`), but
  // that qualification belongs to the reference binding, not to the temporary
  // itself.  Making the temporary `const` also incorrectly sends class
  // prvalues through constexpr-object reconstruction during initialization.
  TypeRecord* temporary_type =
      TypeRecordCopy(expr->type != NULL ? expr->type : type);
  temporary_type->qualifiers = kQualPlain;
  // TypeRecordCopy hands back an unreferenced node, so the symbol below holds the
  // only reference: releasing one here would drop it to zero and tear down the
  // declarator spine, leaving a pointer temporary pointing at nothing.
  Symbol* temp = SyntaxNewTemporary(&compiler->syntax, temporary_type);
  // A materialized temporary exists to be reached through its address -- that is
  // what the reference binding to it holds.  A register-allocated variable has no
  // address to name, so this object has to stay in memory.
  temp->flags.address_taken = true;
  ASTNode* temp_id = NewIdentifierASTNode(temp, location);
  temp_id->flags |= kASTNeedAddress | kASTIsDeclaration;
  ASTNode* initializer = NewExpressionInitializerASTNode(expr, location);
  ASTNode* materialized =
      NewCompoundLiteralASTNode(temp_id, location, initializer);
  ASTNode* analyzed = AnalyzeExpression(materialized);
  analyzed->value_category = kValueCategoryXvalue;
  return analyzed;
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

static bool ClassHasConversionOperatorTo(ASTNode* actual, TypeRecord* target) {
  if (!CompilerIsCXX() || actual == NULL || actual->type == NULL ||
      !TypeIsStructOrUnion(actual->type)) {
    return false;
  }
  // Match against the cv-unqualified target: a parameter of type `const T&`
  // yields target `const T` here, but a conversion function is named
  // "operator T" (top-level cv-qualifiers on the conversion type are dropped)
  // and yields a prvalue `T` that binds to the `const T&`.
  //
  // Compare the conversion function's *result type* rather than looking members
  // up by a reconstructed "operator T" name.  The recorded name of a conversion
  // operator instantiated from a class template can be stale when its result
  // type was a class template that was still incomplete at the point of
  // instantiation (e.g. std::basic_string::operator basic_string_view, whose
  // result names a specialization completed later in the TU): the stored name
  // renders the still-deferred result as the bare primary "operator
  // basic_string_view" while the target here is the fully materialized
  // "basic_string_view<char,...>", so a name-based lookup would miss it.
  // Materializing each candidate's result type at this point -- where the target
  // is necessarily complete -- and comparing types directly sidesteps the name
  // mismatch entirely.
  TypeRecord* unqualified_target = TypeRecordCopy(target);
  unqualified_target->qualifiers = kQualPlain;
  Vector operators;
  VectorInit(&operators);
  CollectConversionOperators(actual->type->info.struct_info, &operators);
  bool found = false;
  for (size_t i = 0; i < operators.length && !found; i++) {
    StructMember* member = operators.value.p[i];
    if (member == NULL || member->symbol == NULL ||
        !TypeIsFunction(member->symbol->type) ||
        member->symbol->type->next == NULL) {
      continue;
    }
    TypeRecord* result = member->symbol->type->next;
    TypeRecord* materialized =
        TypeMaterializeClassTemplateSpecialization(&compiler->syntax, result);
    if (TypeEqualIgnoringQualifiers(materialized, unqualified_target)) {
      found = true;
    }
    if (materialized != result) {
      TypeRecordDelete(materialized);
    }
  }
  VectorDestruct(&operators);
  TypeRecordDelete(unqualified_target);
  return found;
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

static int CXXBracedInitTargetRank(ASTNode* actual, TypeRecord* target);

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
    if (initializer == NULL) {
      return false;
    }
    // Unwrap an `expr_init` wrapper to inspect the element itself, which may be
    // a nested braced-init-list (e.g. `{{1, 1}, {2, 2}}` initializing an
    // `initializer_list<pair<const int, int>>`).
    ASTNode* element = initializer;
    if (initializer->op == AST_OP(expr_init)) {
      element = ((ExpressionInitializerASTNode*)initializer)->expr;
    }
    if (element != NULL && element->op == AST_OP(braced_init)) {
      // A nested braced element is viable if it can list-initialize the
      // element type (via a constructor / aggregate init, or as a further
      // initializer_list).  The array-backed lowering handles the actual
      // construction element-by-element, so only viability matters here.
      if (CXXBracedInitTargetRank(element, element_type) < 0 &&
          !CXXInitializerListBracedInitIsViable(element, element_type)) {
        return false;
      }
      continue;
    }
    if (initializer->op != AST_OP(expr_init)) {
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

static void DiagnoseScalarNarrowing(ASTNode* source, TypeRecord* target);

static StructMember* FindCXXMemberOverloadHead(Struct* owner, String* name);

// True if any constructor in the overload set headed by `ctor` takes a
// std::initializer_list, in which case a braced-init-list is passed to it whole
// rather than element-by-element ([over.match.list]).
static bool CXXConstructorSetTakesInitializerList(StructMember* ctor) {
  for (StructMember* candidate = ctor; candidate != NULL;
       candidate = candidate->overload_next) {
    if (!candidate->is_member_function || candidate->symbol == NULL ||
        candidate->symbol->type == NULL ||
        !TypeIsFunction(candidate->symbol->type) ||
        !candidate->symbol->type->info.function.is_constructor) {
      continue;
    }
    if (CXXConstructorIsInitializerListConstructor(
            &candidate->symbol->type->info.function)) {
      return true;
    }
  }
  return false;
}

// List-initialization of a non-aggregate class type selects a constructor
// ([dcl.init.list]/3.6): the elements of the braced-init-list are its
// arguments.  Build `(temp.T(elements...), temp)`, mirroring what the
// functional-cast form `T(elements...)` lowers to.  Returns NULL when the
// target is not such a class, leaving the caller's aggregate-initialization
// path in charge -- that path only ever initializes members positionally, so
// without this a `return {a, b}` for a class with a constructor would skip the
// constructor entirely (silently leaving members with default values) or, when
// the constructor takes fewer arguments than the class has members, fail with
// "Too many initializers".
static ASTNode* LowerCXXBracedInitToConstructorCall(
    BracedInitializerASTNode* braced, TypeRecord* target,
    SourceLocation location) {
  if (!CompilerIsCXX() || !TypeIsStructOrUnion(target) ||
      target->info.struct_info == NULL ||
      target->info.struct_info->tag_name == NULL ||
      target->info.struct_info->is_aggregate ||
      TypeIsCXXInitializerList(target)) {
    return NULL;
  }
  StructMember* constructor = FindCXXMemberOverloadHead(
      target->info.struct_info, target->info.struct_info->tag_name);
  if (constructor == NULL || !constructor->is_member_function ||
      constructor->symbol == NULL || constructor->symbol->type == NULL ||
      !TypeIsFunction(constructor->symbol->type) ||
      !constructor->symbol->type->info.function.is_constructor) {
    return NULL;
  }
  // `T{}` value-initializes ([dcl.init]/8).  A user-provided default
  // constructor must be run and must *not* be preceded by zero-initialization,
  // so call it here.  Anything else -- a defaulted or implicit default
  // constructor -- is left to the caller's zero-initializing aggregate path,
  // which recurses into the members that still need construction.
  if (braced->initializers == NULL || braced->initializers->length == 0) {
    size_t first_user_formal =
        StructHasVirtualBases(target->info.struct_info) ? 2 : 1;
    bool user_provided_default = false;
    for (StructMember* candidate = constructor;
         candidate != NULL && !user_provided_default;
         candidate = candidate->overload_next) {
      if (candidate->symbol == NULL || candidate->symbol->type == NULL ||
          !TypeIsFunction(candidate->symbol->type)) {
        continue;
      }
      FunctionInfo* info = &candidate->symbol->type->info.function;
      if (!info->is_constructor || info->is_deleted ||
          !info->is_user_provided ||
          info->prototype.length < first_user_formal) {
        continue;
      }
      user_provided_default = true;
      for (size_t i = first_user_formal; i < info->prototype.length; i++) {
        Symbol* formal = info->prototype.value.p[i];
        if (formal == NULL || formal->default_argument == NULL) {
          user_provided_default = false;
          break;
        }
      }
    }
    if (!user_provided_default) {
      return NULL;
    }
  }

  Vector* actuals = NewVector();
  if (CXXConstructorSetTakesInitializerList(constructor)) {
    VectorAppend(actuals, ASTNodeMove((ASTNode*)braced));
  } else {
    for (size_t i = 0; i < braced->initializers->length; i++) {
      ASTNode* element = braced->initializers->value.p[i];
      if (element == NULL) {
        continue;
      }
      if (element->op == AST_OP(expr_init)) {
        element = ((ExpressionInitializerASTNode*)element)->expr;
      }
      VectorAppend(actuals, ASTNodeMove(element));
    }
    braced->initializers->length = 0;
  }
  if (TypeIsStructOrUnion(target) && target->info.struct_info != NULL &&
      StructHasVirtualBases(target->info.struct_info)) {
    VectorInsertBefore(
        actuals, 0,
        NewIntConstantASTNode(1, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                              location));
  }

  TypeRecord* type = TypeRecordCopy(target);
  type->qualifiers = kQualPlain;
  TypeRecordCalculateSize(type);
  Symbol* temp = SyntaxNewTemporary(&compiler->syntax, type);
  temp->location = location;
  ASTNode* member = NewStringConstantASTNode(
      NewString(constructor->symbol->name.value), NULL, location);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location,
                       NewIdentifierASTNode(temp, location), member);
  ASTNode* constructor_call =
      NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals);
  ASTNode* comma =
      NewBinaryASTNode(AST_OP(comma), TypeRecordCopy(type), location,
                       constructor_call, NewIdentifierASTNode(temp, location));
  ASTNode* analyzed = AnalyzeExpression(comma);
  if (analyzed != NULL) {
    analyzed->value_category = kValueCategoryPrvalue;
  }
  return analyzed;
}

ASTNode* LowerCXXBracedClassInitToConstructor(ASTNode* braced,
                                              TypeRecord* target) {
  if (braced == NULL || braced->op != AST_OP(braced_init)) {
    return NULL;
  }
  return LowerCXXBracedInitToConstructorCall(
      (BracedInitializerASTNode*)braced, target, braced->location);
}

// Lower a bare braced-init-list that appears where an expression of a known
// type is required (a function argument, a return value, or the right-hand
// side of an assignment) into a temporary of that `target` type, initialized
// by the braces.  A braced-init-list is not itself an expression in the C++
// grammar, so the backend cannot generate one directly; routing it through the
// compound-literal machinery (which analyzes the initializer via
// AnalyzeInitialization) makes scalar value-initialization, aggregate
// initialization and class construction behave exactly as for `T{...}`.  The
// original braced node is re-parented into the returned compound literal, so
// callers must splice the result in with `delete_old_child = false`.  Returns
// `braced` unchanged when it is not a braced-init-list or no target is known.
ASTNode* LowerCXXBracedInitToTarget(ASTNode* braced, TypeRecord* target) {
  if (braced == NULL || braced->op != AST_OP(braced_init) || target == NULL) {
    return braced;
  }
  SourceLocation location = braced->location;
  BracedInitializerASTNode* b = (BracedInitializerASTNode*)braced;
  // List-initialization of a scalar: `{}` value-initializes (yields 0) and
  // `{v}` initializes from the single element.  Produce the plain scalar
  // expression rather than a temporary object; a scalar compound literal is not
  // an lvalue result the backend loads correctly, and this matches
  // [dcl.init.list] for scalar targets.
  if (TypeIsScalar(target) && b->initializers->length <= 1) {
    if (b->initializers->length == 0) {
      ASTNode* zero =
          TypeIsFloatingPoint(target)
              ? NewRealConstantASTNode(0.0, TypeRecordCopy(target), location)
              : NewIntConstantASTNode(0, TypeRecordCopy(target), location);
      return AnalyzeExpression(zero);
    }
    ASTNode* element = b->initializers->value.p[0];
    if (element != NULL && element->op == AST_OP(expr_init)) {
      element = ((ExpressionInitializerASTNode*)element)->expr;
    }
    element = AnalyzeExpression(element);
    // A braced-init-list forbids narrowing conversions ([dcl.init.list]); e.g.
    // `x = {3.5}` for an `int x` is ill-formed.
    DiagnoseScalarNarrowing(element, target);
    ASTNode* cast = NewCastASTNode(TypeRecordCopy(target), location, element);
    return AnalyzeExpression(cast);
  }
  // A class with constructors is list-initialized by calling one of them, not
  // by initializing its members positionally.
  ASTNode* constructed = LowerCXXBracedInitToConstructorCall(b, target, location);
  if (constructed != NULL) {
    return constructed;
  }
  // Aggregate / class target: build a temporary of `target` initialized by the
  // braces, reusing the compound-literal machinery (which routes through
  // AnalyzeInitialization).
  Symbol* temp = SyntaxNewTemporary(&compiler->syntax, TypeRecordCopy(target));
  temp->location = location;
  ASTNode* temp_id = NewIdentifierASTNode(temp, location);
  temp_id->flags |= kASTNeedAddress | kASTIsDeclaration;
  ASTNode* literal = NewCompoundLiteralASTNode(temp_id, location, braced);
  literal->flags |= kASTCXXBracedTemporary;
  return AnalyzeExpression(literal);
}

// Returns true if the integer constant `value` is representable in the
// integral target type.  Used only for constant narrowing detection, so an
// unknown width is treated as "fits" to avoid false positives.
static bool NarrowingIntegerConstantFits(int64_t value, TypeRecord* target) {
  int bytes = SizeofType(target->type);
  if (bytes <= 0 || bytes > 8) {
    return true;
  }
  if (TypeIsUnsigned(target)) {
    if (value < 0) {
      return false;
    }
    if (bytes >= 8) {
      return true;
    }
    uint64_t max = (1ULL << (bytes * 8)) - 1;
    return (uint64_t)value <= max;
  }
  if (bytes >= 8) {
    return true;
  }
  int64_t max = (1LL << (bytes * 8 - 1)) - 1;
  int64_t min = -(1LL << (bytes * 8 - 1));
  return value >= min && value <= max;
}

// C++ [dcl.init.list]/7: diagnose the unambiguous narrowing conversions that
// occur in scalar list-initialization (`T x{expr}`).  Only cases that never
// produce a false positive are reported: a floating-point source converted to
// an integer target (always narrowing), and a constant integer source whose
// value does not fit in an integer target.  Conversions whose validity depends
// on a runtime value are intentionally left undiagnosed.
static void DiagnoseScalarNarrowing(ASTNode* source, TypeRecord* target) {
  if (!CompilerIsCXX() || source == NULL || target == NULL) {
    return;
  }
  TypeRecord* src = source->type;
  if (src == NULL || !TypeIsIntegral(target)) {
    return;
  }
  bool narrowing = false;
  if (TypeIsFloatingPoint(src)) {
    // Floating-point to integer is always a narrowing conversion.
    narrowing = true;
  } else if (TypeIsIntegral(src) && !TypeIsBool(target)) {
    // A constant integer that does not fit in the target is narrowing.  A
    // successful integer constant evaluation is exactly the standard's
    // "constant expression" gate, so non-constant operands are skipped.
    int64_t value = 0;
    if (EvaluateIntegerExpression(source, &value) &&
        !NarrowingIntegerConstantFits(value, target)) {
      narrowing = true;
    }
  }
  if (!narrowing) {
    return;
  }
  String from_name;
  String to_name;
  StringInit(&from_name, "");
  StringInit(&to_name, "");
  TypeRecordToString(src, &from_name);
  TypeRecordToString(target, &to_name);
  SemanticError(source,
                "narrowing conversion from '%s' to '%s' in list-initialization",
                from_name.value, to_name.value);
  StringDestruct(&from_name);
  StringDestruct(&to_name);
}

static ASTNode* AnalyzeInitialization(ASTNode* node,
                                      ASTNode* target, ASTNode* init) {
  target = AnalyzeExpression(target);
  if (target != NULL && target->type != NULL &&
      TypeIsReference(target->type)) {
    ASTNode* reference_initializer = init;
    while (reference_initializer != NULL &&
           (reference_initializer->op == AST_OP(expr_init) ||
            reference_initializer->op == AST_OP(braced_init) ||
            reference_initializer->op == AST_OP(designated_init))) {
      if (reference_initializer->op == AST_OP(expr_init)) {
        reference_initializer =
            ((ExpressionInitializerASTNode*)reference_initializer)->expr;
      } else if (reference_initializer->op == AST_OP(braced_init)) {
        BracedInitializerASTNode* braced =
            (BracedInitializerASTNode*)reference_initializer;
        reference_initializer =
            braced->initializers != NULL &&
                    braced->initializers->length == 1
                ? braced->initializers->value.p[0]
                : NULL;
      } else {
        reference_initializer =
            ((DesignatedInitializerASTNode*)reference_initializer)->init;
      }
    }
    if (reference_initializer != NULL) {
      reference_initializer->flags |= kASTNeedAddress;
    }
  }
  init = AnalyzeExpression(init);
  if (init == NULL) {
    // Syntax recovery can leave an initialization node without an initializer
    // (for example, when `int []b` is parsed as a malformed structured
    // binding).  Preserve the parser diagnostic and discard the empty
    // initialization instead of dereferencing it below.
    return NULL;
  }
  Symbol* symbol = target != NULL && target->op == AST_OP(identifier)
                       ? ((IdentifierASTNode*)target)->symbol
                       : NULL;
  if (symbol != NULL &&
      ASTNodeAny(init, ExpressionRequiresASTConstexpr, NULL)) {
    symbol->requires_ast_constexpr = true;
  }
  bool deduced_auto = false;
  if (target == NULL || target->type == NULL) {
    SemanticError(node, "Initialization target has no type");
    return init;
  }
  if (symbol != NULL && TypeContainsAuto(symbol->type)) {
    if (!SemanticDeduceAutoType(symbol, init, node)) {
      return init;
    }
    // A dependent initializer can defer deduction until a later template
    // specialization. Do not immediately try to convert its concrete-looking
    // partial initializer to the still-placeholder `auto` target.
    if (TypeContainsAuto(symbol->type)) {
      return init;
    }
    ASTNodeSetType(target, symbol->type);
    ASTNodeSetType(node, symbol->type);
    deduced_auto = true;
  }
  if (deduced_auto && TypeIsStructOrUnion(symbol->type) &&
      init->op == AST_OP(expr_init)) {
    BinaryASTNode* initialization = (BinaryASTNode*)node;
    initialization->right = NULL;
    ASTNode* rewritten = SyntaxRewriteCXXCopyInitConstructorIfNeeded(
        &compiler->syntax, symbol, init);
    if (rewritten != init) {
      return AnalyzeExpression(rewritten);
    }
    initialization->right = init;
  }
  bool is_reference_init = TypeIsReference(target->type);
  switch (init->op) {
    case AST_OP(expr_init):{
      ExpressionInitializerASTNode* e = (ExpressionInitializerASTNode*)init;
      if (is_reference_init) {
        TypeRecord* reference_type = target->type;
        bool rvalue_ref =
            reference_type->declarator == kDeclRValueReference;
        bool discards_qualifiers =
            TypeIsEffectivelyConst(e->expr->type) &&
            !TypeIsEffectivelyConst(reference_type->next);
        if (!TypeEqualIgnoringQualifiers(e->expr->type,
                                         reference_type->next)) {
          ASTNode* base_bound =
              TryBindReferenceToBaseSubobject(e->expr, reference_type->next);
          if (base_bound != NULL) {
            ASTNodeReplaceChild((ASTNode*)e, 0, base_bound, false);
            e->expr = base_bound;
          } else {
            NormalConversion(e->expr,
                             ReferenceConversionTarget(e->expr, reference_type));
          }
        }
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
        } else if (e->expr->op == AST_OP(inline_call) &&
                   TypeIsVoid(e->expr->type) &&
                   TypeIsStructOrUnion(target->type)) {
          // Direct initialization whose constructor call was inlined.  The
          // inlined body already constructs into the object; do not convert
          // the void inline_call to the class type.
          constructor_call = true;
        }
        bool cxx_return_elision_initializer =
            CompilerIsCXX() && e->expr->op == AST_OP(call) &&
            e->expr->value_category != kValueCategoryXvalue &&
            !constructor_call &&
            TypeIsStructOrUnion(target->type) &&
            TypeEqual(e->expr->type, target->type);
        if (!constructor_call && !cxx_return_elision_initializer) {
          NormalConversion(e->expr, target->type);
        }
      }
      break;
    }
    case AST_OP(braced_init): {
      BracedInitializerASTNode* braced = (BracedInitializerASTNode*)init;
      if (!is_reference_init && TypeIsCXXInitializerList(target->type) &&
          !TypeIsCXXInitializerList(init->type)) {
        init = LowerCXXInitializerListBracedInit(target->type, braced,
                                                init->location);
        break;
      }
      if (!is_reference_init && TypeIsScalar(target->type) &&
          braced->initializers->length == 1) {
        ASTNode* initializer = braced->initializers->value.p[0];
        if (initializer->op == AST_OP(designated_init)) {
          DesignatedInitializerASTNode* designated =
              (DesignatedInitializerASTNode*)initializer;
          NormalConversion(designated->init, target->type);
          DiagnoseScalarNarrowing(designated->init, target->type);
        } else {
          ASTNode* value_expr = initializer;
          if (value_expr->op == AST_OP(expr_init)) {
            value_expr = ((ExpressionInitializerASTNode*)value_expr)->expr;
          }
          DiagnoseScalarNarrowing(value_expr, target->type);
        }
      }
      break;
    }
    default:
      break;
  }
  ASTNodeSetType((ASTNode*)init, target->type);
  ASTNodeSetType(node, target->type);

  if (!IsAssignable(target, true)) {
    SemanticError(target,
                  "Cannot initialize a variable of this type");
    return init;
  }

  if (symbol == NULL) {
    ASTNode* simplified_init = AnalyzeInitializer(node->type, init, false);
    ASTNodeReplaceChild(node, 1, simplified_init, true);
    return simplified_init;
  }

  
  bool is_static_storage = StorageIs(symbol->storage, STO(static)) ||
                           StorageIs(symbol->storage, STO(extern));
  bool constants_only = is_static_storage;
  bool is_thread_local = StorageIs(symbol->storage, STO(thread));
  bool is_cxx_local_static =
      CompilerIsCXX() && compiler->current_function != NULL &&
      StorageIs(symbol->storage, STO(static)) && !is_thread_local;
  if (is_thread_local) {
    ASTNode* init_expr = init;
    if (init_expr->op == AST_OP(expr_init)) {
      init_expr = ((ExpressionInitializerASTNode*)init_expr)->expr;
    }
    if (init_expr != NULL && init_expr->op != AST_OP(braced_init)) {
      constants_only = IsConstantExpression(init_expr);
    }
  }
  if (is_cxx_local_static && !symbol->flags.is_constexpr &&
      !symbol->flags.is_constinit) {
    constants_only = false;
  }
  MarkCXX26SymbolicConstexprReference(symbol, init);

  // If we are initializing a constant that is integral or floating point
  // we can evaluate the expression, and if successful, assign the value
  // to the constant so we can use it as a constant in further expressions.
  if (!symbol->flags.is_template &&
      (TypeIsConst(symbol->type) ||
       symbol->flags.is_constexpr ||
       symbol->flags.is_constinit)) {
    EvaluateConstantForSymbol(symbol, init);
  }
  ASTNode* object_init = ConstexprObjectInitializerForSymbol(
      symbol, init->location);
  if (object_init != NULL) {
    init = object_init;
  }
  bool requires_constant_initializer =
      constants_only ||
      ((symbol->flags.is_constexpr || symbol->flags.is_constinit) &&
       !symbol->is_constexpr_representable);
  if (TypeIsMemberPointer(symbol->type)) {
    ASTNode* init_expr = init;
    if (init_expr != NULL && init_expr->op == AST_OP(expr_init)) {
      init_expr = ((ExpressionInitializerASTNode*)init_expr)->expr;
    }
    if (init_expr != NULL && init_expr->op == AST_OP(member_ptr)) {
      UnaryASTNode* unary = (UnaryASTNode*)init_expr;
      if (unary->sub != NULL && unary->sub->op == AST_OP(structmember)) {
        StructMember* member = ((StructMemberASTNode*)unary->sub)->member;
        Struct* class_info = TypeMemberPointerClass(symbol->type);
        MemberPointerValue pm_value;
        if (member != NULL && class_info != NULL &&
            MemberPointerEncodeFromMember(member, class_info, &pm_value)) {
          symbol->value.ivalue = pm_value.ptr;
          symbol->flags.value_set = true;
          symbol->value.other = member;
        }
      }
    } else if (init_expr != NULL) {
      MemberPointerValue pm_value;
      if (MemberPointerTryEvaluateConstant(init_expr, symbol->type,
                                           &pm_value)) {
        StructMember* member = MemberPointerMemberFromExpression(init_expr);
        if (member != NULL) {
          symbol->value.other = member;
        } else if (TypeIsMemberFunctionPointer(symbol->type) &&
            init_expr->op == AST_OP(member_ptr) &&
            ((UnaryASTNode*)init_expr)->sub != NULL &&
            ((UnaryASTNode*)init_expr)->sub->op == AST_OP(structmember)) {
          symbol->value.other =
              ((StructMemberASTNode*)((UnaryASTNode*)init_expr)->sub)->member;
        }
        symbol->value.ivalue = pm_value.ptr;
        symbol->flags.value_set = true;
      }
    }
  }
  ASTNode* simplified_init =
      AnalyzeInitializer(node->type, init, requires_constant_initializer);
  ASTNodeReplaceChild(node, 1, simplified_init, true);
  if ((symbol->flags.is_constexpr || symbol->flags.is_constinit) &&
      !symbol->flags.is_template && !symbol->flags.value_set) {
    // Initializer analysis can finish dependent-template substitutions and
    // expose a scalar constant that was not foldable during the first pass.
    EvaluateConstantForSymbol(symbol, simplified_init);
    if (!symbol->flags.value_set &&
        !symbol->is_constexpr_representable &&
        !ExpressionIsTemplateDependent(simplified_init)) {
      SemanticError(
          simplified_init,
          symbol->flags.is_constinit
              ? "constinit variable initializer is not a constant expression"
              : "constexpr variable initializer is not a constant expression");
    }
  }

  // If the symbol being initialized is static set a flag to tell the
  // code generator not to generate any code for it.
  bool link_time_constant = InitializerIsLinkTimeConstant(simplified_init);
  if (is_static_storage && link_time_constant) {
    node->flags |= kASTStaticInit;
  }
  if (is_cxx_local_static && node->parent != NULL &&
      node->parent->op == AST_OP(vardecl)) {
    VariableDeclarationASTNode* declaration =
        (VariableDeclarationASTNode*)node->parent;
    declaration->local_static_init_kind =
        link_time_constant ? kLocalStaticInitConstant : kLocalStaticInitDynamic;
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
  bool left_float =
      TypeUsesFloat32Representation(node->left->type) ||
      (TypeIsComplex(node->left->type) &&
       TypeComplexElementType(node->left->type) == kTypeFloat);
  bool right_double =
      TypeUsesFloat64Representation(node->right->type) ||
      (TypeIsComplex(node->right->type) &&
       TypeComplexElementType(node->right->type) != kTypeFloat);
  if (left_float && right_double) {
    TypeRecord* operation_type =
        TypeIsComplex(node->left->type) ||
                TypeIsComplex(node->right->type)
            ? NewComplexTypeRecord(kTypeDouble, kQualPlain)
            : NewTypeRecordWithSize(kTypeDouble, kQualPlain);
    NormalConversion(node->right, operation_type);
    return;
  }
  NormalConversion(node->right, node->left->type);
}

static ASTNode* AnalyzeAssignmentExpression(BinaryASTNode* node) {
  node->left = AnalyzeExpression(node->left);
  // `x = {...}` assigns a braced-init-list, which is not itself an expression.
  // Lower it to a temporary of the left-hand side's type so it is analyzed and
  // generated like `x = T{...}`.  (The parser only produces this for simple
  // `=`.)
  if (CompilerIsCXX() && node->base.op == AST_OP(assign) &&
      node->right != NULL && node->right->op == AST_OP(braced_init) &&
      node->left->type != NULL) {
    ASTNode* lowered =
        LowerCXXBracedInitToTarget(node->right, node->left->type);
    if (lowered != node->right) {
      ASTNodeReplaceChild((ASTNode*)node, 1, lowered, false);
      node->right = lowered;
    }
  }
  node->right = AnalyzeExpression(node->right);

  // A constructor member-initializer that targets a reference data member BINDS
  // the reference -- it stores the address of the initializer into the member's
  // pointer slot -- rather than assigning through it.  A reference member access
  // keeps its declared reference type (T&) here, so without this the assignment
  // would resolve `this->ref = x` as `ref.operator=(x)` (for a class referent)
  // or a bogus `T&`-from-`T` conversion (for a scalar referent).  Mirror the
  // reference-initialization / reference-return contract: convert the
  // initializer to the referent type, take its address, and store into the slot
  // (node type T& selects `storea`).
  if (CompilerIsCXX() &&
      (((ASTNode*)node)->flags & kASTCXXMemberInitializer) != 0 &&
      node->base.op == AST_OP(assign) && node->left != NULL &&
      node->left->type != NULL && TypeIsReference(node->left->type)) {
    TypeRecord* reference_type = node->left->type;
    TypeRecord* initializer_type =
        TypeIsReference(node->right->type) ? node->right->type->next
                                           : node->right->type;
    if (!TypeEqualIgnoringQualifiers(initializer_type, reference_type->next)) {
      NormalConversion(node->right,
                       ReferenceConversionTarget(node->right, reference_type));
    }
    if (ReferenceCanBind(node->right, reference_type) &&
        !HasAddress(node->right)) {
      ASTNode* materialized =
          MaterializeTemporary(node->right, reference_type->next);
      ASTNodeReplaceChild((ASTNode*)node, 1, materialized, false);
      node->right = materialized;
    }
    node->left->flags |= kASTNeedAddress;
    node->right->flags |= kASTNeedAddress;
    ASTNodeSetType((ASTNode*)node, reference_type);
    return (ASTNode*)node;
  }

  if (BinaryOperatorFunctionName(node->base.op) != NULL) {
    ASTNode* overloaded = TryAnalyzeOverloadedBinaryOperator(node);
    if (overloaded != NULL) {
      return overloaded;
    }
  }
  bool is_initializer = ((ASTNode*)node)->flags & kASTCXXMemberInitializer;
  if (!IsAssignable(node->left, is_initializer)) {
    SemanticError(node->left, "Cannot assign to this expression");
  }

  // We need the address of this node, not its value.
  node->left->flags |= kASTNeedAddress;

  if (TypeIsAtomic(node->left->type)) {
    bool supported_rmw =
        node->base.op == AST_OP(pluseq) || node->base.op == AST_OP(minuseq);
    if (node->base.op != AST_OP(assign) && !supported_rmw) {
      SemanticError(
          (ASTNode*)node,
          "atomic compound assignment is not supported for this operator");
    }
    if (supported_rmw && !TypeIsIntegral(node->left->type) &&
        !TypeIsPointer(node->left->type)) {
      SemanticError((ASTNode*)node,
                    "atomic arithmetic requires integral or pointer type");
    }
  }

  switch (node->base.op) {
    case AST_OP(assign):
      // During the first (class-level) instantiation of a member function
      // template, the enclosing template parameters are concrete but the
      // member template's own parameters are not, so an assignment such as a
      // constructor mem-initializer `__rep_ = value` may have a still-dependent
      // right-hand side (`value` has type `const Rep2&`).  Choosing an
      // arithmetic conversion now would bake in the wrong one (a dependent type
      // is treated as `int`, yielding a bogus `i2d`/`cvtsi2sd` once the real
      // argument is a floating type).  Leave the operand unconverted; the
      // member template's second-stage instantiation re-analyzes the
      // mem-initializer with the concrete argument type and inserts the correct
      // conversion.
      if (TypeContainsTemplateParameter(node->right->type) ||
          TypeContainsTemplateParameter(node->left->type)) {
        ((ASTNode*)node)->flags |= kASTDeferredDependentAssign;
      } else {
        NormalConversion(node->right, node->left->type);
      }
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
  if (TypeIsComplex(node->sub->type)) {
    SemanticError(node->sub,
                  "Cannot increment or decrement a complex value");
  }
  if (TypeIsAtomic(node->sub->type) &&
      !TypeIsIntegral(node->sub->type) &&
      !TypeIsPointer(node->sub->type)) {
    SemanticError(node->sub,
                  "atomic increment and decrement require integral or pointer type");
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
  if (CompilerIsCXX() && node->left != NULL &&
      (TypeIsUnknown(node->left->type) ||
       TypeContainsTemplateParameter(node->left->type))) {
    ASTNodeSetType((ASTNode*)node,
                   NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain));
    node->base.value_category = kValueCategoryLvalue;
    return (ASTNode*)node;
  }
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
  if (node->right != NULL && !TypeIsIntegral(node->right->type)) {
    SemanticError(node->right, "Subscripts must be integral types");
  }
  if (node->left != NULL && !TypeIsPointerOrArray(node->left->type) &&
      !TypeIsVector(node->left->type)) {
    SemanticError(node->left, "Can only subscript arrays, pointers, and vectors");
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return (ASTNode*)node;
  }

  // Dereference the array type.
  TypeRecord* subtype = node->left->type->next;
  ASTNodeSetType((ASTNode*)node, subtype);
  node->base.value_category = kValueCategoryLvalue;
  if (TypeIsVector(node->left->type)) {
    node->left->flags |= kASTNeedAddress;
  }
  return (ASTNode*)node;
}

static ASTNode* AnalyzeMultidimensionalSubscript(VectorASTNode* node) {
  // Analyze the receiver while detached.  VectorASTNode's legacy child
  // replacer indexes only the argument vector, so an in-place receiver rewrite
  // would otherwise replace argument zero.
  ASTNode* receiver = node->left;
  node->left = NULL;
  if (receiver != NULL) {
    receiver->parent = NULL;
  }
  ASTNode* analyzed_receiver = AnalyzeExpression(receiver);
  if (analyzed_receiver != receiver) {
    ASTNodeDelete(receiver);
  }
  node->left = analyzed_receiver;
  if (analyzed_receiver != NULL) {
    analyzed_receiver->parent = &node->base;
    analyzed_receiver->child_id = 0;
  }

  TypeRecord* receiver_type =
      analyzed_receiver != NULL ? analyzed_receiver->type : NULL;
  bool dependent_receiver =
      receiver_type == NULL || TypeIsUnknown(receiver_type) ||
      TypeContainsTemplateParameter(receiver_type);
  if (dependent_receiver || TypeIsStructOrUnion(receiver_type)) {
    Vector* actuals = NewVector();
    for (size_t i = 0; i < node->children->length; i++) {
      ASTNode* actual = (ASTNode*)VectorGet(node->children, i);
      VectorSet(node->children, i, NULL);
      if (actual != NULL) {
        actual->parent = NULL;
      }
      VectorAppend(actuals, actual);
    }
    node->left = NULL;
    if (analyzed_receiver != NULL) {
      analyzed_receiver->parent = NULL;
    }
    ASTNode* call =
        NewOperatorMemberCall(analyzed_receiver, "operator[]", actuals,
                              node->base.location);
    return ReplaceVectorWithCall(node, call);
  }

  for (size_t i = 0; i < node->children->length; i++) {
    ASTNode* old_index = (ASTNode*)VectorGet(node->children, i);
    ASTNode* index = AnalyzeExpression(old_index);
    if (index != old_index) {
      VectorSet(node->children, i, index);
      if (index != NULL) {
        index->parent = &node->base;
        index->child_id = (int)i;
      }
      ASTNodeDelete(old_index);
    }
  }
  SemanticError((ASTNode*)node,
                "Built-in subscripting requires exactly one index");
  ASTNodeSetType((ASTNode*)node,
                 NewTypeRecordWithSize(kTypeInt, kQualPlain));
  return (ASTNode*)node;
}

// Inliner data.
typedef struct {
  Map argument_map;     // Map of callee symbols to caller-local clones.
  ASTNode* end_label;   // End label for return conversion.
  Symbol* return_value; // Return value symbol.
  ASTNode* top_stmt;    // Top level compound statement.
  bool return_is_reference;
} Inliner;

static void AddInlineLocalSymbol(Inliner* inliner, Symbol* symbol) {
  if (symbol == NULL || !symbol->flags.is_local ||
      StorageIs(symbol->storage, STO(static))) {
    return;
  }
  MapKeyType key = {.p = symbol};
  if (MapFind(&inliner->argument_map, key) != NULL) {
    return;
  }
  Symbol* clone = SymbolClone(symbol);
  clone->is_nrvo = false;
  VectorAppend(&compiler->syntax.all_local_symbols, clone);
  MapKeyValue kv = {.key.p = symbol, .value.p = clone};
  MapInsert(&inliner->argument_map, kv);
}

static void CollectInlineLocalSymbols(ASTNode* node, void* data, int child_id,
                                      VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren) {
    return;
  }
  Inliner* inliner = data;
  if (node->op == AST_OP(vardecl)) {
    AddInlineLocalSymbol(
        inliner, ((VariableDeclarationASTNode*)node)->symbol);
  } else if (node->op == AST_OP(identifier)) {
    // Lowered expressions can contain compiler-generated locals without a
    // VariableDeclarationASTNode (for example the temporary that carries a
    // placement-new result). They still need a caller-local clone when the
    // containing function is inlined.
    AddInlineLocalSymbol(inliner, ((IdentifierASTNode*)node)->symbol);
  } else if (node->op == AST_OP(catch)) {
    AddInlineLocalSymbol(inliner, ((CatchASTNode*)node)->symbol);
  }
}

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
      id_node->base.flags &= ~kASTNrvoMarker;
    }
    return node;
  }
  if (node->op == AST_OP(vardecl)) {
    VariableDeclarationASTNode* declaration =
        (VariableDeclarationASTNode*)node;
    MapKeyType key = {.p = declaration->symbol};
    Symbol* new_sym = MapFind(&inliner->argument_map, key);
    if (new_sym != NULL) {
      declaration->symbol = new_sym;
    }
    return node;
  }
  if (node->op == AST_OP(catch)) {
    CatchASTNode* catch_node = (CatchASTNode*)node;
    MapKeyType key = {.p = catch_node->symbol};
    Symbol* new_sym = MapFind(&inliner->argument_map, key);
    if (new_sym != NULL) {
      catch_node->symbol = new_sym;
    }
    return node;
  }
  if (node->op == AST_OP(return)) {
    CombinedStatementASTNode* ret_node = (CombinedStatementASTNode*)node;
    SourceLocation return_location = ret_node->base.location;
    Vector* new_ret = NewVector();
    if (ret_node->cond != NULL) {
      ASTNode* value = ASTNodeMove(ret_node->cond);
      // The cloned expression is no longer a return from the original
      // function: it initializes the inliner's local result temporary. An RVO
      // marker copied from the original return would incorrectly route a
      // nested aggregate-returning call to the caller's hidden struct-return
      // slot, which may not exist (for example when inlining into main).
      value->flags &= ~kASTRvoCall;
      ASTNode* statement;
      if (inliner->return_value != NULL) {
        if (inliner->return_is_reference) {
          value = NewUnaryASTNode(AST_OP(address), inliner->return_value->type,
                                  value->location, value);
        }
        ASTNode* ret_value =
            NewIdentifierASTNode(inliner->return_value, return_location);
        ASTNode* ret_assign = NewBinaryASTNode(AST_OP(assign),
                                               value->type,
                                               value->location,
                                               ret_value,
                                               value);
        statement =
            NewExpressionStatementASTNode(ret_assign, ret_assign->location);
      } else {
        // `return expression;` in a function returning void -- which a wrapper
        // forwarding to another void call is written as -- has no result to
        // store, but the expression is still evaluated for its side effects.
        statement = NewExpressionStatementASTNode(value, value->location);
      }
      AnalyzeStatement(statement);
      VectorAppend(new_ret, statement);
    }
    // Now make a goto node to the end_label.
    LabelASTNode* end_label = (LabelASTNode*)inliner->end_label;
    GotoStatementASTNode* goto_node = (GotoStatementASTNode*)
                    NewGotoStatementASTNode(
                                  NewString(end_label->name.value),
                                  return_location);
    goto_node->label = inliner->end_label;
    goto_node->lca = inliner->top_stmt;
    VectorAppend(new_ret, goto_node);
    
    // Don't need the return now.
    ASTNodeDelete(node);
    
    // Build a new Compound statement containing the assignment to the
    // return value (if necessary) and the goto.
    return NewCompoundStatementASTNode(new_ret, return_location);
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
static ASTNode* CopyArguments(FunctionInfo* info, VectorASTNode* call,
                              Inliner* inliner) {
  SourceLocation location = call->base.location;
  Vector* decls = NewVector();
  for (size_t i = 0; i < info->prototype.length; i++) {
    Symbol* original_formal = info->prototype.value.p[i];
    ASTNode* actual = ASTNodeMove(call->children->value.p[i]);
    Symbol* formal = SymbolClone(original_formal);
    formal->flags.is_argument = false;
    formal->flags.is_local = true;
    VectorAppend(&compiler->syntax.all_local_symbols, formal);
    
    // Insert old and new into inliner's argument map so we can translate
    // the argument references to the local variables.
    MapKeyValue kv;
    kv.key.p = original_formal;
    kv.value.p = formal;
    MapInsert(&inliner->argument_map, kv);
    
    // Initialize the local parameter object directly. Assignment would be
    // ill-formed for const parameters, would assign through references instead
    // of binding them, and would give class parameters the wrong copy semantics.
    ASTNode* formal_id = NewIdentifierASTNode(formal, location);
    formal_id->flags |= kASTNeedAddress | kASTIsDeclaration;
    ASTNode* initializer = NewBinaryASTNode(
        AST_OP(init), formal->type, actual->location, formal_id,
        NewExpressionInitializerASTNode(actual, actual->location));
    initializer = AnalyzeExpression(initializer);
    VectorAppend(
        decls,
        NewVariableDeclarationASTNode(formal, initializer, location));
  }
  
  // Allocate a temporary for the return value if it's not void.
  if (!TypeIsVoid(call->base.type)) {
    TypeRecord* temp_type =
        inliner->return_is_reference
            ? NewPointerTo(kQualPlain, call->base.type)
            : call->base.type;
    Symbol* temp = SyntaxNewTemporary(&compiler->syntax, temp_type);
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
  inliner.return_is_reference =
      info->symbol != NULL && info->symbol->type != NULL &&
      TypeIsReference(info->symbol->type->next);
  
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
  // Replacement statements analyzed by the clone callback are semantically
  // part of the inlined function's definition. Without this context, member
  // and friend access checks run as the caller and wrongly reject private or
  // protected members used by the inlined body.
  TypeRecord* saved_function = compiler->current_function;
  Struct* saved_access_context = compiler->current_class_access_context;
  compiler->current_function = info->symbol->type;
  compiler->current_class_access_context =
      info->symbol->type->info.function.cxx_member_owner;
  ASTNodeVisit(info->body, CollectInlineLocalSymbols, 0, &inliner);
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
  compiler->current_function = saved_function;
  compiler->current_class_access_context = saved_access_context;
  ASTNode* ret_node = NULL;
  if (inliner.return_value != NULL) {
    // Void function, no return value;
    ret_node = NewIdentifierASTNode(inliner.return_value, location);
  }
  MapDestruct(&inliner.argument_map);
  ASTNode* result =
      NewInlineCallASTNode(call->base.type, location, inlined, ret_node);
  result->value_category = call->base.value_category;
  return result;
}

static Symbol* InlineMappedSymbol(Inliner* inliner, Symbol* original) {
  if (inliner == NULL || original == NULL) {
    return NULL;
  }
  MapKeyType key = {.p = original};
  return MapFind(&inliner->argument_map, key);
}

static ASTNode* NewCallerContractStatement(ContractAssertion* assertion,
                                           Inliner* inliner,
                                           VectorASTNode* call,
                                           TypeRecord* caller_func) {
  if (assertion == NULL || assertion->predicate == NULL) {
    return NULL;
  }
  ASTNode* predicate =
      ASTNodeClone(assertion->predicate, InlineFunctionBodyStatement,
                   inliner, NULL);
  FunctionInfo* info = &caller_func->info.function;
  bool has_runtime_dedup_guard = false;
  if (!info->is_pure_virtual && info->symbol != NULL &&
      call->left != NULL) {
    ASTNode* selected =
        ASTNodeClone(call->left, IdentityCloneNode, NULL, NULL);
    ASTNode* statically_chosen =
        AnalyzeExpression(NewIdentifierASTNode(info->symbol,
                                               assertion->location));
    ASTNode* same_function = NewBinaryASTNode(
        AST_OP(equal), NULL, assertion->location, selected, statically_chosen);
    same_function = AnalyzeExpression(same_function);
    predicate = AnalyzeExpression(NewBinaryASTNode(
        AST_OP(logor), NULL, assertion->location, same_function, predicate));
    has_runtime_dedup_guard = true;
  }
  Vector attributes = {0};
  AttributeListClone(&attributes, &assertion->attributes);
  ASTNode* statement = NewContractAssertASTNode(
      predicate, &attributes, assertion->location);
  ((ContractAssertASTNode*)statement)->kind = assertion->kind;
  statement->flags |= kASTAnalyzed;
  if (has_runtime_dedup_guard) {
    statement->flags |= kASTVirtualCallerContract;
  }
  return statement;
}

static void AppendCallerContractStatements(Vector* statements,
                                           TypeRecord* func,
                                           ContractAssertionKind kind,
                                           Inliner* inliner,
                                           VectorASTNode* call) {
  Vector* assertions = &func->info.function.contract_assertions;
  for (size_t i = 0; i < assertions->length; i++) {
    ContractAssertion* assertion = assertions->value.p[i];
    if (assertion == NULL || assertion->kind != kind) {
      continue;
    }
    ASTNode* statement =
        NewCallerContractStatement(assertion, inliner, call, func);
    if (statement != NULL) {
      VectorAppend(statements, statement);
    }
  }
}

static bool VirtualCallerContractsRequireNontrivialCopy(TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func)) {
    return false;
  }
  TypeRecord* return_type = func->next;
  if (return_type != NULL && !TypeIsReference(return_type) &&
      TypeIsStructOrUnion(return_type) &&
      !CXXTypeIsTriviallyCopyable(return_type)) {
    return true;
  }
  Vector* prototype = &func->info.function.prototype;
  for (size_t i = 0; i < prototype->length; i++) {
    Symbol* formal = prototype->value.p[i];
    if (formal != NULL && !TypeIsReference(formal->type) &&
        TypeIsStructOrUnion(formal->type) &&
        !CXXTypeIsTriviallyCopyable(formal->type)) {
      return true;
    }
  }
  return false;
}

static ASTNode* LowerVirtualCallerContracts(VectorASTNode* call) {
  TypeRecord* caller_func = call->caller_contract_function;
  if (caller_func == NULL || !TypeIsFunction(caller_func) ||
      caller_func->info.function.contract_assertions.length == 0 ||
      call->children == NULL ||
      call->children->length !=
          caller_func->info.function.prototype.length) {
    return (ASTNode*)call;
  }
  if (VirtualCallerContractsRequireNontrivialCopy(caller_func)) {
    SemanticError(
        (ASTNode*)call,
        "caller-facing virtual contracts with non-trivially-copyable "
        "by-value parameters or results are not supported");
    TypeRecordDelete(call->caller_contract_function);
    call->caller_contract_function = NULL;
    return (ASTNode*)call;
  }

  ASTNode* outer_parent = call->base.parent;
  int outer_child_id = call->base.child_id;
  if (outer_parent != NULL) {
    ASTNodeReplaceChild(outer_parent, outer_child_id, NULL, false);
    call->base.parent = NULL;
  }

  call->caller_contract_function = NULL;
  SourceLocation location = call->base.location;
  Inliner inliner = {0};
  MapInitForPointerKeys(&inliner.argument_map);
  inliner.return_is_reference = TypeIsReference(caller_func->next);

  Vector* statements = NewVector();
  VectorAppend(statements,
               CopyArguments(&caller_func->info.function, call, &inliner));

  Symbol* this_formal =
      caller_func->info.function.prototype.length != 0
          ? caller_func->info.function.prototype.value.p[0]
          : NULL;
  Symbol* local_this = InlineMappedSymbol(&inliner, this_formal);
  if (local_this != NULL) {
    ASTNode* receiver = NewIdentifierASTNode(local_this, location);
    receiver = AnalyzeExpression(receiver);
    ASTNode* virtual_callee = NewVirtualCalleeFromFunction(
        receiver, caller_func, true, location);
    if (virtual_callee != NULL) {
      ASTNodeDelete(call->left);
      call->left = virtual_callee;
      call->left->parent = &call->base;
      call->left->child_id = 0;
    }
  }

  for (size_t i = 0;
       i < caller_func->info.function.prototype.length; i++) {
    Symbol* formal =
        caller_func->info.function.prototype.value.p[i];
    Symbol* local = InlineMappedSymbol(&inliner, formal);
    ASTNode* actual = NewIdentifierASTNode(local, location);
    actual = AnalyzeExpression(actual);
    VectorSet(call->children, i, actual);
    actual->parent = &call->base;
    actual->child_id = (int)i;
  }

  AppendCallerContractStatements(statements, caller_func,
                                 kContractPrecondition, &inliner, call);

  ASTNode* call_statement = NULL;
  if (inliner.return_value == NULL) {
    call_statement = NewExpressionStatementASTNode((ASTNode*)call, location);
  } else {
    ASTNode* value = (ASTNode*)call;
    if (inliner.return_is_reference) {
      value = NewUnaryASTNode(AST_OP(address), inliner.return_value->type,
                              location, value);
    }
    ASTNode* assignment = NewBinaryASTNode(
        AST_OP(assign), value->type, location,
        NewIdentifierASTNode(inliner.return_value, location), value);
    assignment = AnalyzeExpression(assignment);
    call_statement = NewExpressionStatementASTNode(assignment, location);
  }
  VectorAppend(statements, call_statement);

  Vector* assertions = &caller_func->info.function.contract_assertions;
  for (size_t i = 0; i < assertions->length; i++) {
    ContractAssertion* assertion = assertions->value.p[i];
    if (assertion != NULL &&
        assertion->kind == kContractPostcondition &&
        assertion->result_binding != NULL &&
        inliner.return_value != NULL) {
      Symbol* mapped_result = inliner.return_value;
      if (inliner.return_is_reference) {
        mapped_result = SymbolClone(assertion->result_binding);
        mapped_result->flags.is_argument = false;
        mapped_result->flags.is_local = true;
        VectorAppend(&compiler->syntax.all_local_symbols, mapped_result);

        ASTNode* returned_pointer =
            NewIdentifierASTNode(inliner.return_value, location);
        ASTNode* returned_reference = NewUnaryASTNode(
            AST_OP(contents), call->base.type, location, returned_pointer);
        returned_reference = AnalyzeExpression(returned_reference);
        ASTNode* result_id =
            NewIdentifierASTNode(mapped_result, assertion->location);
        result_id->flags |= kASTNeedAddress | kASTIsDeclaration;
        ASTNode* initializer = NewBinaryASTNode(
            AST_OP(init), mapped_result->type, assertion->location, result_id,
            NewExpressionInitializerASTNode(returned_reference,
                                            assertion->location));
        initializer = AnalyzeExpression(initializer);
        VectorAppend(
            statements,
            NewVariableDeclarationASTNode(
                mapped_result, initializer, assertion->location));
      }
      MapKeyValue mapping = {
          .key.p = assertion->result_binding,
          .value.p = mapped_result,
      };
      MapInsert(&inliner.argument_map, mapping);
    }
  }
  AppendCallerContractStatements(statements, caller_func,
                                 kContractPostcondition, &inliner, call);

  ASTNode* compound = NewCompoundStatementASTNode(statements, location);
  for (size_t i = 0; i < statements->length; i++) {
    ASTNode* statement = statements->value.p[i];
    statement->parent = compound;
    statement->child_id = (int)i;
  }
  ASTNode* result = NULL;
  if (inliner.return_value != NULL) {
    result = NewIdentifierASTNode(inliner.return_value, location);
  }
  ASTNode* lowered =
      NewInlineCallASTNode(call->base.type, location, compound, result);
  lowered->value_category = call->base.value_category;
  MapDestruct(&inliner.argument_map);
  TypeRecordDelete(caller_func);

  if (outer_parent != NULL) {
    ASTNodeReplaceChild(outer_parent, outer_child_id, lowered, false);
  }
  return lowered;
}

typedef struct {
  int node_count;
  bool found_goto;
} GotoFinder;

static void ExamineBody(ASTNode* node, void* data, int child_id, VisitorMode mode) {
  GotoFinder* finder = data;
  finder->node_count++;
  if (node->op == AST_OP(goto) || node->op == AST_OP(inline_call) ||
      node->op == AST_OP(stmt_expr)) {
    finder->found_goto = true;
  }
}

typedef struct {
  bool found_unresolved;
} UnresolvedMemberFinder;

typedef struct {
  bool found_nrvo;
} NRVOFinder;

static void FindNRVOLocal(ASTNode* node, void* data, int child_id,
                          VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren) {
    return;
  }
  NRVOFinder* finder = data;
  if ((node->flags & kASTNrvoMarker) != 0 ||
      (node->op == AST_OP(vardecl) &&
       ((VariableDeclarationASTNode*)node)->symbol != NULL &&
       ((VariableDeclarationASTNode*)node)->symbol->is_nrvo)) {
    finder->found_nrvo = true;
  }
}

static void FindUnresolvedMemberAccess(ASTNode* node, void* data, int child_id,
                                       VisitorMode mode) {
  if (mode != kVisitPreChildren) {
    return;
  }
  UnresolvedMemberFinder* finder = data;
  if ((node->op == AST_OP(dot) || node->op == AST_OP(arrow)) &&
      ((BinaryASTNode*)node)->right != NULL &&
      ((BinaryASTNode*)node)->right->op != AST_OP(structmember)) {
    finder->found_unresolved = true;
  }
  if (node->op == AST_OP(init) &&
      ((BinaryASTNode*)node)->right != NULL &&
      ((BinaryASTNode*)node)->right->op != AST_OP(braced_init)) {
    finder->found_unresolved = true;
  }
  // An analyzed braced initializer carries the type it initializes and holds a
  // designator per entry.  One with no type at all has not been analyzed, which
  // happens to a body reached from a caller that is analyzed first, and its
  // entries are still the plain expressions the parser left: code generation
  // has nowhere to place them.  The analysis that follows lowers the body the
  // function keeps and never sees a copy taken now.
  if (node->op == AST_OP(braced_init) && node->type == NULL) {
    finder->found_unresolved = true;
  }
  if (node->op == AST_OP(call)) {
    ASTNode* callee = ((VectorASTNode*)node)->left;
    if (callee != NULL && callee->op == AST_OP(identifier)) {
      Symbol* symbol = ((IdentifierASTNode*)callee)->symbol;
      // A callee still naming the template itself has not been resolved to the
      // specialization the call reaches.  Copying it emits a call to the
      // template's own name, which nothing defines.
      if (symbol != NULL && (symbol->flags.is_template_type_parameter ||
                             symbol->flags.is_template)) {
        finder->found_unresolved = true;
      }
    }
  }
}

// Does the callee expression name the function it is typed as, rather than hold
// a value that merely has that function type?  A function type record carries
// the body of the function it was declared for, and the lowering of a
// pointer-to-member-function call dereferences the loaded pointer using the
// member function's own type record.  What that call reaches is whatever the
// pointer holds, so inlining the body found on the type would call the wrong
// member.  Calls through a plain function pointer or a vtable slot never get
// here: their callee expression has pointer type.
static bool CalleeNamesItsFunction(ASTNode* callee) {
  if (callee == NULL || callee->op != AST_OP(identifier)) {
    return false;
  }
  Symbol* symbol = ((IdentifierASTNode*)callee)->symbol;
  return symbol != NULL && symbol->type != NULL && TypeIsFunction(symbol->type);
}

// We can only inline a function if:
// 1. It is defined and has a body
// 2. It is not the current function.
// 3. It's not a varargs function or has unknown args.
// 4. It has no goto statements.
// 5. The number of AST nodes is reasonably small.
// 6. It has no unresolved member accesses or unlowered initializers. Template
//    bodies can retain either until their concrete instantiation is analyzed;
//    cloning such a body would leave nodes that code generation cannot handle.
// 7. It has no named-return-value object. NRVO binds that local directly to the
//    callee's hidden aggregate-result slot, which does not exist after inlining.
//
// Constructors may be inlined when they initialize a named variable
// (`T x(args)`).  Other constructor expressions construct a temporary and
// rely on codegen to retarget `this` via current_struct_address, which an
// inline_call cannot do.  Destructors are never inlined: exception cleanup
// recognizes `receiver.~T()` expression statements.
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
  // Required constant expressions are interpreted from their semantic AST.
  // Replacing a constexpr call with the normal runtime inline_call lowering
  // before that interpretation loses the call boundary and makes the constant
  // evaluator reject an otherwise valid expression.
  if (compiler->constant_evaluation_required_depth > 0) {
    return false;
  }
  // __attribute__((noinline)) blocks inlining outright.
  if (func->symbol != NULL && func->symbol->flags.noinline) {
    return false;
  }
  // The body being analyzed is itself still a template if any of its own
  // parameters are unbound -- a member function template of a class template
  // reaches this state, its class arguments substituted and its own left
  // symbolic.  That body will be cloned again for each instantiation, and the
  // clone substitutes the member's arguments into everything it copies.  An
  // inlined body brought in here comes from a function of the *enclosing*
  // class, so its parameter placeholders are numbered for the class's list;
  // substituting the member's list into them binds them to the wrong
  // arguments (`hive<int>::sort<less<int>>` turns `static_cast<T*>` inlined
  // from the iterator's `operator*` into `static_cast<less<int>*>`, and the
  // subscript that follows scales by the wrong element size).  Leave the call
  // alone; the per-instantiation clone is analyzed too, and inlining there
  // sees fully concrete types.
  if (compiler->current_function != NULL &&
      TypeIsFunction(compiler->current_function) &&
      TypeContainsTemplateParameter(compiler->current_function)) {
    return false;
  }
  // Inlining copies the body into the caller and generates code from the
  // copy, so the body has to be finished: every expression typed and every
  // overload resolved.  A template instantiation is not finished when it is
  // created.  Its body is cloned from the primary at that point and analyzed
  // later off the pending-instantiation queue, so in between it still holds
  // expressions written in terms of the template parameters -- `pred(*first)`
  // where `pred` is a closure object, say, whose `operator()` nothing has
  // picked yet.  Copying one of those into the caller leaves it there
  // untyped, with no later pass that would come back and resolve it.
  if (func->body != NULL && (func->body->flags & kASTAnalyzed) == 0) {
    return false;
  }
  // A function's preconditions and postconditions are generated around its
  // body when the function itself is compiled, from the assertion list on its
  // type -- they are not statements in the body.  A body copied into the caller
  // therefore arrives without them, and inlining a checked function would stop
  // checking it.  Putting the predicates back is not just a matter of cloning
  // them either: a predicate may name the parameters through the const views
  // contract analysis gave them, and may contain a lambda or a splice, none of
  // which survives being re-analyzed in the caller.  Leave the call alone.
  if (func->contract_assertions.length != 0) {
    return false;
  }
  // Inside a contract predicate the names in scope are seen through the const
  // views contract analysis gives them, and the inlined body is analyzed here,
  // in that scope, rather than in the callee's.  A body that assigns to one of
  // its own parameters -- which an immediately-invoked lambda in a predicate
  // does as soon as it captures anything -- is then rejected for writing
  // through a const view it never had.  Call it instead; the definition is
  // compiled in its own scope where the types are the written ones.
  if (compiler->contract_assertion_depth > 0) {
    return false;
  }
  // A lazily instantiated function can also temporarily *share* the primary
  // template's body, before its own concrete body is cloned.  That body may
  // well have been analyzed -- as the template's own definition, where the
  // expressions in it are dependent by design -- so the check above says
  // nothing about it.
  if (func->template_origin != NULL &&
      func->template_origin->type != NULL &&
      TypeIsFunction(func->template_origin->type) &&
      func->body == func->template_origin->type->info.function.body) {
    return false;
  }
  // The inline-call argument copier binds `this` as the static member-owner
  // pointer.  It does not reproduce the runtime adjustment needed to reach a
  // virtual base, so inlining such a member can turn uses of that base into
  // invalid object accesses.
  if (func->cxx_member_owner != NULL &&
      func->cxx_member_owner->virtual_bases.length != 0) {
    return false;
  }
  // Exception cleanup ranges are built by recognizing the compiler-inserted
  // `receiver.~T()` statements at the end of a block and pairing them with the
  // declarations above (see statement_codegen.c).  Turning such a call into an
  // inline_call hides that shape, so the block gets no cleanup range at all and
  // an exception unwinding through it destroys nothing.  Some destructor
  // statements are also analyzed before they are wrapped in an expression
  // statement, so a call-site parent check is not enough.
  if (func->is_destructor) {
    return false;
  }
  // __attribute__((always_inline)) forces inlining even without the 'inline'
  // keyword (and bypasses the size heuristic below).
  bool force_inline = func->symbol != NULL && func->symbol->flags.always_inline;
  bool unmarked_at_o3 = OptLevel3() && !func->is_inline && !force_inline;
  if ((!func->is_inline && !force_inline && !unmarked_at_o3) ||
      !func->symbol->flags.is_defined ||
      func->body == NULL || compiler->current_function == NULL ||
      compiler->current_function->info.function.is_constexpr ||
      func->symbol == compiler->current_function->info.function.symbol ||
      func->unknown_args || func->varargs) {
    return false;
  }
  const int kMaxInlineNodeCount = OptLevel3() ? 250 : 100;
  const int kMaxUnmarkedInlineNodeCount = 40;
  GotoFinder finder = {0, false};
  ASTNodeVisit(func->body, ExamineBody, 0, &finder);
  if (finder.found_goto) {
    // Can't inline a function containing a goto regardless of always_inline.
    return false;
  }
  UnresolvedMemberFinder unresolved = {false};
  ASTNodeVisit(func->body, FindUnresolvedMemberAccess, 0, &unresolved);
  if (unresolved.found_unresolved) {
    return false;
  }
  NRVOFinder nrvo = {false};
  ASTNodeVisit(func->body, FindNRVOLocal, 0, &nrvo);
  if (nrvo.found_nrvo) {
    return false;
  }
  if (force_inline) {
    return true;
  }
  int max_nodes =
      unmarked_at_o3 ? kMaxUnmarkedInlineNodeCount : kMaxInlineNodeCount;
  return finder.node_count < max_nodes;
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
    return strchr("dioubxXaAeEfFgGsScCpn[%", conv) != NULL;
  }
  return strchr("dioubBxXcCaAeEfFgGsSpn%", conv) != NULL;
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
    case 'd': case 'i': case 'u': case 'o': case 'b': case 'B':
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
    if (*p == 'w') {
      p++;
      if (*p == 'f') {
        p++;
      }
      bool valid_width = *p >= '1' && *p <= '9';
      int width = 0;
      while (isdigit((unsigned char)*p)) {
        if (width <= 64) {
          width = width * 10 + *p - '0';
          if (width > 64) {
            width = 65;
          }
        }
        p++;
      }
      if (!valid_width ||
          (width != 8 && width != 16 && width != 32 && width != 64)) {
        SemanticWarning((ASTNode*)node, "format-invalid-specifier",
                        "unsupported width-specific format modifier");
      }
    } else {
      while (*p == 'h' || *p == 'l' || *p == 'L' || *p == 'j' ||
             *p == 'z' || *p == 't') {
        p++;
      }
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

typedef enum {
  kPrintfProfileFull,
  kPrintfProfileLiteral,
  kPrintfProfileInt,
  kPrintfProfileLong,
  kPrintfProfileFP,
} PrintfProfile;

static int PrintfFormatArgument(const char* name) {
  if (strcmp(name, "printf") == 0) {
    return 0;
  }
  if (strcmp(name, "fprintf") == 0 || strcmp(name, "sprintf") == 0) {
    return 1;
  }
  if (strcmp(name, "snprintf") == 0) {
    return 2;
  }
  return -1;
}

static PrintfProfile ClassifyPrintfFormat(const char* format) {
  PrintfProfile profile = kPrintfProfileLiteral;
  const char* p = format;
  while (*p != '\0') {
    if (*p++ != '%') {
      continue;
    }
    if (*p == '%') {
      p++;
      profile = kPrintfProfileInt;
      continue;
    }
    profile = kPrintfProfileInt;
    while (*p == '-' || *p == '+' || *p == ' ' || *p == '#' || *p == '0') {
      p++;
    }
    if (*p == '*') {
      p++;
    } else {
      while (isdigit((unsigned char)*p)) {
        p++;
      }
    }
    if (*p == '.') {
      p++;
      if (*p == '*') {
        p++;
      } else {
        while (isdigit((unsigned char)*p)) {
          p++;
        }
      }
    }
    bool long_value = false;
    if (*p == 'w') {
      return kPrintfProfileFull;
    } else if (*p == 'l') {
      long_value = true;
      p++;
      if (*p == 'l') {
        p++;
      }
    } else if (*p == 'L' || *p == 'j' || *p == 'z' || *p == 't') {
      long_value = true;
      p++;
    } else if (*p == 'h') {
      p++;
      if (*p == 'h') {
        p++;
      }
    }
    if (*p == '\0') {
      return kPrintfProfileFull;
    }
    char conversion = *p++;
    if (strchr("fFeEgGaA", conversion) != NULL) {
      profile = kPrintfProfileFP;
    } else if (long_value && profile != kPrintfProfileFP) {
      profile = kPrintfProfileLong;
    } else if (strchr("diuoxXpcsn", conversion) == NULL) {
      return kPrintfProfileFull;
    }
  }
  return profile;
}

static Symbol* GetPrintfSpecializationSymbol(Symbol* original,
                                             const char* name) {
  String symbol_name;
  StringInit(&symbol_name, name);
  Symbol* symbol = FindGlobalSymbol(&symbol_name);
  StringDestruct(&symbol_name);
  if (symbol != NULL) {
    return symbol;
  }
  symbol = NewSymbol(name, original->type, original->storage);
  symbol->flags.is_forward_declared = true;
  symbol->flags.is_c_linkage = true;
  symbol->flags.used = true;
  bool inserted = InsertGlobalSymbol(symbol);
  assert(inserted);
  (void)inserted;
  return symbol;
}

static void SpecializePrintfCall(VectorASTNode* node, Symbol* callee) {
  if (!compiler->printf_specialize || callee == NULL ||
      callee->name.value == NULL) {
    return;
  }
  int format_arg = PrintfFormatArgument(callee->name.value);
  if (format_arg < 0 || (size_t)format_arg >= node->children->length) {
    return;
  }
  ASTNode* arg = node->children->value.p[format_arg];
  if (arg == NULL || arg->op != AST_OP(string)) {
    return;
  }
  String* value = ((ConstantASTNode*)arg)->value.string;
  PrintfProfile profile =
      ClassifyPrintfFormat(value->value == NULL ? "" : value->value);
  const char* suffix = NULL;
  switch (profile) {
    case kPrintfProfileLiteral:
      suffix = "literal";
      break;
    case kPrintfProfileInt:
      suffix = "int";
      break;
    case kPrintfProfileLong:
      suffix = "long";
      break;
    case kPrintfProfileFP:
      suffix = "fp";
      break;
    case kPrintfProfileFull:
      return;
  }
  char specialized_name[64];
  snprintf(specialized_name, sizeof(specialized_name), "__%s_%s",
           callee->name.value, suffix);
  IdentifierASTNode* identifier = (IdentifierASTNode*)node->left;
  identifier->symbol =
      GetPrintfSpecializationSymbol(callee, specialized_name);
}

static void RenumberVectorChildren(VectorASTNode* node) {
  for (size_t i = 0; i < node->children->length; i++) {
    ASTNode* child = node->children->value.p[i];
    if (child == NULL) {
      continue;
    }
    child->parent = &node->base;
    child->child_id = (int)i;
  }
}

static bool MemberReceiverIsConst(BinaryASTNode* node);
static bool MemberReceiverIsVolatile(BinaryASTNode* node);
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
      use == NULL ||
      !TypeIsFunction(function->type) ||
      (use->flags & kASTDeletedFunctionDiagnosed) != 0) {
    return;
  }
  TypeEnsureTemplateMemberFunctionDefinition(&compiler->syntax, function);
  TypeRecord* deleted_type = function->type;
  if (!deleted_type->info.function.is_deleted &&
      function->value.func_defn != NULL &&
      function->value.func_defn->type != NULL &&
      TypeIsFunction(function->value.func_defn->type) &&
      function->value.func_defn->type->info.function.is_deleted) {
    deleted_type = function->value.func_defn->type;
  }
  if (!deleted_type->info.function.is_deleted &&
      function->type->info.function.template_origin != NULL) {
    Symbol* origin = function->type->info.function.template_origin;
    if (origin->type != NULL && TypeIsFunction(origin->type) &&
        origin->type->info.function.is_deleted) {
      deleted_type = origin->type;
    } else if (origin->value.func_defn != NULL &&
               origin->value.func_defn->type != NULL &&
               TypeIsFunction(origin->value.func_defn->type) &&
               origin->value.func_defn->type->info.function.is_deleted) {
      deleted_type = origin->value.func_defn->type;
    }
  }
  if (!deleted_type->info.function.is_deleted) {
    return;
  }
  use->flags |= kASTDeletedFunctionDiagnosed;
  String function_name;
  StringInit(&function_name, NULL);
  SymbolFunctionDiagnosticName(function, &function_name);
  if (deleted_type->info.function.is_implicitly_deleted) {
    SemanticError(use, "Use of implicitly deleted function %s",
                  function_name.value);
  } else if (deleted_type->info.function.deleted_reason != NULL) {
    SemanticError(
        use, "Use of deleted function %s: %s", function_name.value,
        deleted_type->info.function.deleted_reason->value);
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
         StringEqual(owner->tag_name, "source_location") &&
         SymbolIsInStdNamespace(owner->tag_symbol);
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
    // A stored child_id on a call argument is its index in `children`, not the
    // number visitors pass down (those reserve 0 for the callee in `left`).
    default_arg->child_id = (int)i;
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
  if (sub != NULL && sub->op == AST_OP(contents)) {
    ASTNode* pointer = ASTNodeMove(((UnaryASTNode*)sub)->sub);
    ASTNodeDelete(sub);
    return pointer;
  }
  TypeRecord* pointer_type = NewPointerTo(kQualPlain, sub->type);
  ASTNode* address =
      NewUnaryASTNode(AST_OP(address), pointer_type, location, sub);
  ASTNodeSetType(address, pointer_type);
  sub->flags |= kASTNeedAddress;
  address->flags |= kASTAnalyzed;
  return address;
}

static ASTNode* NewVirtualCalleeFromFunction(ASTNode* receiver,
                                            TypeRecord* function_type,
                                            bool receiver_is_pointer,
                                            SourceLocation location) {
  if (receiver == NULL || function_type == NULL ||
      !TypeIsFunction(function_type) ||
      function_type->info.function.virtual_index < 0) {
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
      NewIntConstantASTNode(function_type->info.function.virtual_index,
                            NewTypeRecordWithSize(kTypeInt, kQualPlain),
                            location);
  ASTNode* slot =
      NewBinaryASTNode(AST_OP(subscript), NULL, location, vptr, index);
  TypeRecord* callable_type =
      CopyFunctionTypeForVirtualCall(function_type);
  TypeRecord* function_pointer = NewPointerTo(kQualPlain, callable_type);
  slot = AnalyzeExpression(slot);
  ASTNodeSetType(slot, function_pointer);
  return slot;
}

static ASTNode* NewVirtualCalleeFromReceiver(ASTNode* receiver,
                                            StructMember* member,
                                            bool receiver_is_pointer,
                                            SourceLocation location) {
  return member != NULL && member->symbol != NULL
             ? NewVirtualCalleeFromFunction(receiver, member->symbol->type,
                                            receiver_is_pointer, location)
             : NULL;
}

static StructMember* FindCXXMemberOverloadHead(Struct* owner, String* name);

static StructMember* FindCXXMemberOverloadHead(Struct* owner, String* name);

static Symbol* CXXStructTemplateFamilyOrigin(Struct* owner) {
  if (owner == NULL || owner->tag_symbol == NULL) {
    return NULL;
  }
  TypeRecord* tag_type = owner->tag_symbol->type;
  return tag_type != NULL && tag_type->template_origin != NULL
             ? tag_type->template_origin
             : owner->tag_symbol;
}

static bool CXXStructsShareTemplateFamily(Struct* left, Struct* right) {
  if (left == right) {
    return left != NULL;
  }
  Symbol* left_origin = CXXStructTemplateFamilyOrigin(left);
  Symbol* right_origin = CXXStructTemplateFamilyOrigin(right);
  return left_origin != NULL && left_origin == right_origin;
}

static bool LowerMemberPointerFunctionCall(VectorASTNode* node);

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

  TypeRecord* concrete_receiver_type = member_access->left->type;
  if (member_access->base.op == AST_OP(arrow) &&
      TypeIsStructOrUnionPointer(concrete_receiver_type)) {
    concrete_receiver_type = concrete_receiver_type->next;
  }
  Struct* concrete_receiver =
      concrete_receiver_type != NULL &&
              TypeIsStructOrUnion(concrete_receiver_type)
          ? concrete_receiver_type->info.struct_info
          : NULL;
  Struct* member_owner =
      member->symbol != NULL && member->symbol->type != NULL &&
              TypeIsFunction(member->symbol->type)
          ? member->symbol->type->info.function.cxx_member_owner
          : NULL;
  TypeRecord* qualified_owner_type = member_node->owner_type;
  bool qualified_base_member =
      member_access->right != NULL &&
      (member_access->right->flags & kASTQualifiedName) != 0;
  if (qualified_base_member && qualified_owner_type != NULL &&
      TypeIsStructOrUnion(qualified_owner_type) &&
      qualified_owner_type->info.struct_info != NULL &&
      member->symbol != NULL) {
    StructMember* base_member = FindStructMemberWithAccessAndOffsetByName(
        qualified_owner_type->info.struct_info, member->symbol->name.value,
        NULL, NULL, NULL);
    if (base_member != NULL && base_member->symbol != NULL) {
      member = base_member;
      StructMemberASTNodeSetMember(member_node, base_member);
      member_owner =
          base_member->symbol->type != NULL &&
                  TypeIsFunction(base_member->symbol->type)
              ? base_member->symbol->type->info.function.cxx_member_owner
              : member_owner;
    }
  }
  if (!qualified_base_member && concrete_receiver != NULL &&
      member->symbol != NULL) {
    StructMember* concrete_member =
        FindStructMember(concrete_receiver, &member->symbol->name);
    if (concrete_member != NULL && concrete_member->symbol != NULL &&
        concrete_member->symbol->type != NULL &&
        TypeIsFunction(concrete_member->symbol->type) &&
        (concrete_receiver != member_owner ||
         concrete_member->symbol != member->symbol)) {
      member = concrete_member;
      StructMemberASTNodeSetMember(member_node, concrete_member);
    }
  }

  member = ResolveMemberFunctionOverload(member, node, member_access);
  if (member != NULL) {
    StructMemberASTNodeSetMember(member_node, member);
  }
  if (member != NULL) {
    if (member->symbol != NULL && member->symbol->type != NULL &&
        TypeIsFunction(member->symbol->type) &&
        member->symbol->type->info.function.is_constructor &&
        member_access->left != NULL) {
      Struct* concrete_owner = NULL;
      if (TypeIsStructOrUnion(member_access->left->type)) {
        concrete_owner = member_access->left->type->info.struct_info;
      } else if (TypeIsStructOrUnionPointer(member_access->left->type)) {
        concrete_owner = member_access->left->type->next->info.struct_info;
      }
      Struct* selected_owner =
          member->symbol->type->info.function.cxx_member_owner;
      bool same_constructor_owner =
          CXXStructsShareTemplateFamily(concrete_owner, selected_owner);
      if (same_constructor_owner &&
          (compiler->current_class_access_context != NULL ||
           (compiler->current_function != NULL &&
            TypeIsFunction(compiler->current_function) &&
            compiler->current_function->info.function.cxx_member_owner != NULL))) {
        Struct* current_owner = compiler->current_class_access_context != NULL
                                    ? compiler->current_class_access_context
                                    : compiler->current_function->info.function
                                          .cxx_member_owner;
        // Rebind a nested constructor through the current specialization only
        // when that specialization belongs to the receiver's lexical parent.
        // Looking up by the injected class name alone can otherwise capture an
        // unrelated nested class with the same name (for example two different
        // distributions' `param_type` classes).
        if (concrete_owner->lexical_parent != NULL &&
            CXXStructsShareTemplateFamily(concrete_owner->lexical_parent,
                                          current_owner)) {
          StructMember* canonical =
              FindStructMember(current_owner, concrete_owner->tag_name);
          if (canonical != NULL && canonical->symbol != NULL &&
              StorageIs(canonical->symbol->storage, STO(typedef)) &&
              TypeIsStructOrUnion(canonical->symbol->type)) {
            concrete_owner = canonical->symbol->type->info.struct_info;
          }
        }
      }
      bool selected_member_template_specialization =
          member->symbol->type->info.function.template_origin != NULL;
      if (same_constructor_owner && concrete_owner != NULL &&
          selected_owner != concrete_owner &&
          !selected_member_template_specialization &&
          concrete_owner->tag_name != NULL) {
        StructMember* concrete_head =
            FindCXXMemberOverloadHead(concrete_owner, concrete_owner->tag_name);
        size_t selected_arity =
            member->symbol->type->info.function.prototype.length;
        StructMember* arity_fallback = NULL;
        for (StructMember* candidate = concrete_head; candidate != NULL;
             candidate = candidate->overload_next) {
          if (candidate->is_member_function && candidate->symbol != NULL &&
              candidate->symbol->type != NULL &&
              TypeIsFunction(candidate->symbol->type) &&
              candidate->symbol->type->info.function.is_constructor &&
              candidate->symbol->type->info.function.prototype.length ==
                  selected_arity) {
            if (arity_fallback == NULL) {
              arity_fallback = candidate;
            }
            bool same_formals = true;
            for (size_t i = 1; i < selected_arity; i++) {
              Symbol* selected_formal =
                  member->symbol->type->info.function.prototype.value.p[i];
              Symbol* candidate_formal =
                  candidate->symbol->type->info.function.prototype.value.p[i];
              if (selected_formal == NULL || candidate_formal == NULL ||
                  !TypeEqualIgnoringQualifiers(selected_formal->type,
                                               candidate_formal->type)) {
                same_formals = false;
                break;
              }
            }
            if (same_formals) {
              member = candidate;
              StructMemberASTNodeSetMember(member_node, candidate);
              break;
            }
          }
        }
        if (member_node->member != arity_fallback && arity_fallback != NULL &&
            member->symbol->type->info.function.cxx_member_owner !=
                concrete_owner &&
            member->symbol->type->info.function.cxx_special_member_kind ==
                kCXXSpecialMemberNone) {
          member = arity_fallback;
          StructMemberASTNodeSetMember(member_node, arity_fallback);
        }
      }
      if (same_constructor_owner && concrete_owner != NULL) {
        member->symbol->type->info.function.cxx_member_owner = concrete_owner;
        StringClear(&member->symbol->asm_name);
        SymbolSetCXXMangledAsmName(member->symbol);
      }
    }
    CheckDeletedFunctionUse(member->symbol, (ASTNode*)node);
  }

  Struct* owner = member->symbol->type->info.function.cxx_member_owner;
  CXXAccess access = member->access;
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
  ASTNode* discarded_static_receiver = NULL;
  bool explicit_object =
      FunctionHasExplicitObjectParameter(member->symbol->type);
  bool use_virtual_dispatch =
      member->symbol->type->info.function.is_virtual &&
      !member->is_static && !explicit_object &&
      !qualified_base_member &&
      !CurrentFunctionIsCXXCtorOrDtor();
  if (use_virtual_dispatch &&
      CompilerCXXAtLeast(kLanguageStandardCXX29) &&
      compiler->contract_semantic != kContractSemanticIgnore &&
      member->symbol->type->info.function.contract_assertions.length != 0) {
    TypeRecordDelete(node->caller_contract_function);
    node->caller_contract_function =
        TypeRecordCopy(member->symbol->type);
  }
  bool polymorphic_special_member =
      (member->symbol->type->info.function.is_constructor ||
       member->symbol->type->info.function.is_destructor) &&
      owner != NULL && owner->virtual_members.length > 0;
  if (!member->is_static) {
    if (!explicit_object &&
        !member->symbol->type->info.function.is_const_member &&
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
    if (!explicit_object &&
        !member->symbol->type->info.function.is_volatile_member &&
        !member->symbol->type->info.function.is_constructor &&
        !member->symbol->type->info.function.is_destructor &&
        MemberReceiverIsVolatile(member_access)) {
      String function_name;
      StringInit(&function_name, NULL);
      SymbolFunctionDiagnosticSuffix(member->symbol, &function_name);
      SemanticError(
          (ASTNode*)member_access,
          "Cannot call non-volatile member function %s on volatile object%s",
          member->symbol->name.value, function_name.value);
      StringDestruct(&function_name);
    }
    if (!explicit_object &&
        !MemberReceiverMatchesRefQualifier(member->symbol->type, member_access)) {
      SemanticError((ASTNode*)member_access,
                    "Cannot call ref-qualified member function %s on this "
                    "object value category",
                    member->symbol->name.value);
    }
    receiver = ASTNodeMove(member_access->left);
    if (explicit_object && member_access->base.op == AST_OP(arrow)) {
      TypeRecord* object_type =
          receiver->type != NULL ? receiver->type->next : NULL;
      receiver = NewUnaryASTNode(AST_OP(contents), object_type,
                                 receiver->location, receiver);
      receiver = AnalyzeExpression(receiver);
    } else if (!explicit_object && member_access->base.op == AST_OP(dot)) {
      receiver = NewAnalyzedBuiltinAddressOf(receiver, receiver->location);
    }
    int this_adjustment = explicit_object ? 0 : member_node->byte_offset;
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
  } else {
    // A static member selected through an object expression still evaluates
    // that object expression for side effects.  Preserve it as the left side
    // of a comma expression while omitting it from the function's arguments.
    discarded_static_receiver = ASTNodeMove(member_access->left);
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
  } else if (discarded_static_receiver != NULL) {
    if (node->children->length > 0) {
      ASTNode* first_actual = ASTNodeMove(node->children->value.p[0]);
      ASTNode* comma = NewBinaryASTNode(
          AST_OP(comma), NULL, discarded_static_receiver->location,
          discarded_static_receiver, first_actual);
      comma = AnalyzeExpression(comma);
      VectorSet(node->children, 0, comma);
      comma->parent = &node->base;
      comma->child_id = 0;
    } else {
      ASTNode* callee = node->left;
      node->left = NULL;
      callee->parent = NULL;
      ASTNode* comma = NewBinaryASTNode(
          AST_OP(comma), NULL, discarded_static_receiver->location,
          discarded_static_receiver, callee);
      node->left = AnalyzeExpression(comma);
      node->left->parent = &node->base;
      node->left->child_id = 0;
    }
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

static Struct* CurrentFunctionMemberOwner(void) {
  TypeRecord* current = compiler->current_function;
  if (current == NULL || !TypeIsFunction(current)) {
    return NULL;
  }
  if (current->info.function.cxx_member_owner != NULL) {
    return current->info.function.cxx_member_owner;
  }
  if (current->info.function.prototype.length == 0) {
    return NULL;
  }
  Symbol* this_sym = current->info.function.prototype.value.p[0];
  if (this_sym == NULL || this_sym->type == NULL ||
      !StringEqual(&this_sym->name, "this") || this_sym->type->next == NULL ||
      !TypeIsStructOrUnion(this_sym->type->next)) {
    return NULL;
  }
  return this_sym->type->next->info.struct_info;
}

static Symbol* CXXStructTemplateOrigin(Struct* str) {
  if (str == NULL || str->tag_symbol == NULL) {
    return NULL;
  }
  if (str->tag_symbol->type != NULL &&
      str->tag_symbol->type->template_origin != NULL) {
    return str->tag_symbol->type->template_origin;
  }
  if (str->is_template || str->tag_symbol->flags.is_template) {
    return str->tag_symbol;
  }
  return NULL;
}

static bool CXXSameAccessClass(Struct* a, Struct* b) {
  if (a == b) {
    return true;
  }
  Symbol* a_origin = CXXStructTemplateOrigin(a);
  Symbol* b_origin = CXXStructTemplateOrigin(b);
  if (a_origin != NULL && b_origin != NULL) {
    if (a_origin == b_origin ||
        StringEqualString(&a_origin->name, &b_origin->name)) {
      return true;
    }
  }
  if (a == NULL || b == NULL || a->tag_name == NULL || b->tag_name == NULL) {
    return false;
  }
  const char* a_name = a->tag_name->value;
  const char* b_name = b->tag_name->value;
  size_t a_len = strcspn(a_name, "<");
  size_t b_len = strcspn(b_name, "<");
  return a_len == b_len && strncmp(a_name, b_name, a_len) == 0;
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
  Struct* current_owner = CurrentFunctionMemberOwner();
  if (current_owner != NULL) {
    for (size_t i = 0; i < owner->friend_classes.length; i++) {
      Struct* friend_class = owner->friend_classes.value.p[i];
      if (friend_class == current_owner ||
          CXXSameAccessClass(friend_class, current_owner)) {
        return true;
      }
    }
  }
  // 'friend <function>;': match the befriended declaration either by symbol
  // identity or, since a friend declaration and the later definition may be
  // separate symbols, by name and signature.
  Symbol* current_symbol = current->info.function.symbol;
  Symbol* current_template = current->info.function.template_origin;
  for (size_t i = 0; i < owner->friend_functions.length; i++) {
    Symbol* friend_symbol = owner->friend_functions.value.p[i];
    if (friend_symbol == NULL) {
      continue;
    }
    if (friend_symbol == current_symbol) {
      return true;
    }
    if (current_template != NULL &&
        (friend_symbol == current_template ||
         (friend_symbol->type != NULL &&
          TypeIsFunction(friend_symbol->type) &&
          friend_symbol->type->info.function.template_origin ==
              current_template))) {
      return true;
    }
    if (current_template != NULL && friend_symbol->type != NULL &&
        current_template->type != NULL &&
        StringEqualString(&friend_symbol->name, &current_template->name) &&
        TypeEqual(friend_symbol->type, current_template->type)) {
      return true;
    }
    if (current_symbol != NULL &&
        friend_symbol->asm_name.length != 0 &&
        current_symbol->asm_name.length != 0 &&
        StringEqualString(&friend_symbol->asm_name,
                          &current_symbol->asm_name)) {
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
  if (compiler->current_class_access_context != NULL) {
    // A static data member initializer is in the scope of its class and may
    // name the class's private and protected members. Template body cloning also
    // re-analyzes member expressions before current_function has a recoverable
    // owner, but still within the instantiated class context.
    current_owner = compiler->current_class_access_context;
  } else if (compiler->current_function != NULL &&
             TypeIsFunction(compiler->current_function)) {
    current_owner = CurrentFunctionMemberOwner();
  }
  if (CurrentFunctionIsFriendOf(owner) ||
      (lookup_context != owner && CurrentFunctionIsFriendOf(lookup_context))) {
    return true;
  }
  if (current_owner == NULL) {
    return false;
  }
  if (owner != NULL) {
    for (size_t i = 0; i < owner->friend_classes.length; i++) {
      Struct* friend_class = owner->friend_classes.value.p[i];
      if (friend_class == current_owner ||
          CXXSameAccessClass(friend_class, current_owner)) {
        return true;
      }
    }
  }
  if (lookup_context != owner && lookup_context != NULL) {
    for (size_t i = 0; i < lookup_context->friend_classes.length; i++) {
      Struct* friend_class = lookup_context->friend_classes.value.p[i];
      if (friend_class == current_owner ||
          CXXSameAccessClass(friend_class, current_owner)) {
        return true;
      }
    }
  }
  if (CXXSameAccessClass(current_owner, owner)) {
    return true;
  }
  // A nested class is a member of each enclosing class and has the same access
  // rights as any other member, including access to private members of those
  // enclosing classes ([class.access.nest]).
  for (Struct* enclosing = current_owner->lexical_parent; enclosing != NULL;
       enclosing = enclosing->lexical_parent) {
    if (CXXSameAccessClass(enclosing, owner)) {
      return true;
    }
  }
  // Nested lambda closures: while instantiating an outer generic lambda, an
  // inner lambda's body may still be re-analyzed with the outer operator() as
  // current_function.  Capture fields are private to the inner closure; allow
  // access when both the accessor and the member owner are invented lambda
  // closures (the rewritten `this->capture` form is only produced for the
  // owning operator()'s body).
  if (current_owner->tag_symbol != NULL &&
      current_owner->tag_symbol->flags.invented && owner->tag_symbol != NULL &&
      owner->tag_symbol->flags.invented) {
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

static bool MemberReceiverIsVolatile(BinaryASTNode* node) {
  if (node->base.op == AST_OP(arrow)) {
    return TypeIsPointerOrArray(node->left->type) &&
           TypeIsVolatile(node->left->type->next);
  }
  return TypeIsVolatile(node->left->type);
}

static bool MemberReceiverIsLValue(BinaryASTNode* node) {
  if (node->base.op == AST_OP(arrow)) {
    return true;
  }
  return node->left != NULL && node->left->value_category == kValueCategoryLvalue;
}

static bool MemberReceiverMatchesRefQualifier(TypeRecord* func,
                                              BinaryASTNode* member_access) {
  if (func == NULL || !TypeIsFunction(func)) {
    return true;
  }
  switch (func->info.function.ref_qualifier) {
    case kCXXRefQualifierNone:
      return true;
    case kCXXRefQualifierLValue:
      return MemberReceiverIsLValue(member_access);
    case kCXXRefQualifierRValue:
      return !MemberReceiverIsLValue(member_access);
  }
  return true;
}

static bool TypeEqualIgnoringQualifiers(TypeRecord* left, TypeRecord* right);

static TypeRecord* NewStructTypeForAdjustment(Struct* str) {
  TypeRecord* type = NewTypeRecord(kTypeStruct, kQualPlain);
  TypeRecordSetStructInfo(type, str);
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
  // The receiver expression is referenced twice below: once to load the
  // virtual-base offset (`receiver->__vbptr[index]`) and once as the base of
  // the pointer adjustment (`receiver + offset`).  If it is anything other than
  // a plain identifier it may have side effects (e.g. a function call whose
  // result yields the object), so evaluate it exactly once into a temporary and
  // reference the temporary in both positions.  Otherwise the receiver would be
  // cloned and evaluated twice.
  ASTNode* seed_assign = NULL;
  if (receiver->op != AST_OP(identifier)) {
    SourceLocation rloc = receiver->location;
    Symbol* temp =
        SyntaxNewTemporary(&compiler->syntax, TypeRecordCopy(receiver->type));
    temp->location = rloc;
    ASTNode* temp_lhs = NewIdentifierASTNode(temp, rloc);
    temp_lhs->flags |= kASTNeedAddress;
    seed_assign = NewBinaryASTNode(AST_OP(assign), receiver->type, rloc,
                                   temp_lhs, receiver);
    seed_assign->flags |= kASTAnalyzed;
    receiver = NewIdentifierASTNode(temp, rloc);
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
  if (seed_assign != NULL) {
    adjusted = NewBinaryASTNode(AST_OP(comma), adjusted_type,
                                node->base.location, seed_assign, adjusted);
    adjusted->flags |= kASTAnalyzed;
  }
  node->base.op = AST_OP(arrow);
  node->left = adjusted;
  node->left->parent = (ASTNode*)node;
  node->left->child_id = 0;
  if (member_offset != NULL) {
    *member_offset = -1;
  }
}

static bool TypeIsEffectivelyConst(TypeRecord* type) {
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (TypeIsConst(t)) {
      return true;
    }
    if (t->declarator != kDeclArray) {
      break;
    }
  }
  return false;
}

static bool ReferenceCanBind(ASTNode* actual, TypeRecord* reference_type) {
  if (!TypeIsReference(reference_type)) {
    return false;
  }
  if (TypeIsEffectivelyConst(actual->type) &&
      !TypeIsEffectivelyConst(reference_type->next)) {
    return false;
  }
  if (reference_type->declarator == kDeclRValueReference) {
    return !ASTNodeIsLValue(actual);
  }
  if (ASTNodeIsLValue(actual)) {
    return true;
  }
  return TypeIsEffectivelyConst(reference_type->next);
}

static int ReferenceBindingRank(ASTNode* actual, TypeRecord* reference_type) {
  if (!ReferenceCanBind(actual, reference_type)) {
    return -1;
  }
  bool target_const = TypeIsEffectivelyConst(reference_type->next);
  if (reference_type->declarator == kDeclRValueReference) {
    return target_const && !TypeIsConst(actual->type) ? 1 : 0;
  }
  if (ASTNodeIsLValue(actual)) {
    return target_const ? 1 : 0;
  }
  return 2;
}

static int OverloadBaseConversionRank(TypeRecord* actual, TypeRecord* target) {
  if (TypeIsPointer(actual) && TypeIsPointer(target) &&
      actual->next != NULL && target->next != NULL) {
    Qualifiers discarded =
        (actual->next->qualifiers & (kQualConst | kQualVolatile)) &
        ~(target->next->qualifiers & (kQualConst | kQualVolatile));
    if (discarded != 0) {
      return -1;
    }
  }
  if (TypeEqual(actual, target) || TypeEqualIgnoringQualifiers(actual, target)) {
    return 0;
  }
  if (TypeIsStructOrUnion(actual) || TypeIsStructOrUnion(target)) {
    // Two distinct class types convert only via a derived-to-base relationship.
    // TypeEqual above handles separately materialized records for the same
    // template specialization by comparing their origins and arguments.
    if (TypeIsStructOrUnion(actual) && TypeIsStructOrUnion(target)) {
      if (TypeIsDerivedFrom(actual, target)) {
        return 2;
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
  if (TypeIsInt(target) && !TypeIsUnsigned(target) &&
      (TypeIsCharFamily(actual) || TypeIsShort(actual) ||
       TypeIsBool(actual))) {
    return 1;
  }
  if (TypeIsIntegral(actual) && TypeIsIntegral(target)) {
    return 2;
  }
  // Standard arithmetic conversion sequences.  In particular, a converted
  // prvalue can bind to a const reference parameter after this conversion
  // (for example, float -> const double&).
  if (TypeIsFloat(actual) && TypeIsDouble(target)) {
    return 1;
  }
  if ((TypeIsFloatingPoint(actual) && TypeIsFloatingPoint(target)) ||
      (TypeIsIntegral(actual) && TypeIsFloatingPoint(target)) ||
      (TypeIsFloatingPoint(actual) && TypeIsIntegral(target))) {
    return 2;
  }
  if (TypeIsPointerOrArray(actual) && TypeIsPointerOrArray(target)) {
    if (TypeChar8IdentityDiffers(actual, target)) {
      return -1;
    }
    if (CompilerIsCXX() && TypeIsVoidPointer(actual) &&
        TypeIsPointer(target) && !TypeIsVoidPointer(target)) {
      return -1;
    }
    if (CompilerIsCXX() && actual->next != NULL && target->next != NULL &&
        !TypeIsVoid(actual->next) && !TypeIsVoid(target->next) &&
        !TypeEqualIgnoringQualifiers(actual->next, target->next) &&
        !(TypeIsStructOrUnion(actual->next) &&
          TypeIsStructOrUnion(target->next) &&
          TypeIsDerivedFrom(actual->next, target->next))) {
      return -1;
    }
    if (TypeIsVoidPointer(actual) || TypeIsVoidPointer(target)) {
      return 2;
    }
    if (TypeAssignmentCompatible(actual, target)) {
      return 1;
    }
  }
  // Function-to-pointer conversion ([conv.func]): a function lvalue argument
  // binds to a matching function-pointer parameter.  This is an lvalue
  // transformation (exact-match rank), so a function argument such as a free
  // function passed to a `Pred`-deduced `R(*)(Args...)` parameter is viable.
  if (TypeIsFunction(actual) && TypeIsPointer(target) && target->next != NULL &&
      TypeIsFunction(target->next) &&
      (TypeEqual(actual, target->next) ||
       TypeEqualIgnoringQualifiers(actual, target->next))) {
    return 0;
  }
  return -1;
}

// Overload-resolution rank for list-initializing a *non-`initializer_list`*
// target (a class or scalar) from the braced-init-list `actual`.  Returns a
// non-negative base rank if the braces can initialize `target`, or -1.  This
// mirrors the list-initialization that LowerCXXBracedInitToTarget performs at
// call time, so that an overloaded function taking a class/scalar (by value or
// reference) is viable for a braced-init argument -- e.g. `insert({k, v})`
// where the parameter is `pair<const K,V>` (or a reference to it).  Without this
// the scoring path treats the braced list as `<unknown>` and rejects every
// candidate whenever more than one overload exists.
static int CXXBracedInitTargetRank(ASTNode* actual, TypeRecord* target) {
  if (actual == NULL || actual->op != AST_OP(braced_init) || target == NULL) {
    return -1;
  }
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)actual;
  if (TypeIsScalar(target)) {
    if (braced->initializers->length == 0) {
      return 0;
    }
    if (braced->initializers->length != 1) {
      return -1;
    }
    ASTNode* init = braced->initializers->value.p[0];
    if (init == NULL || init->op != AST_OP(expr_init)) {
      return -1;
    }
    ExpressionInitializerASTNode* ei = (ExpressionInitializerASTNode*)init;
    ei->expr = AnalyzeExpression(ei->expr);
    int r = OverloadBaseConversionRank(ei->expr->type, target);
    return r < 0 ? -1 : r;
  }
  if (!TypeIsStructOrUnion(target) || target->info.struct_info == NULL) {
    return -1;
  }
  Struct* str = target->info.struct_info;
  // Analyze the element expressions once; any non-expression element (nested
  // braces, designated initializers) is left to call-time lowering.
  Vector elements;
  VectorInit(&elements);
  bool simple_elements = true;
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* init = braced->initializers->value.p[i];
    if (init == NULL || init->op != AST_OP(expr_init)) {
      simple_elements = false;
      break;
    }
    ExpressionInitializerASTNode* ei = (ExpressionInitializerASTNode*)init;
    ei->expr = AnalyzeExpression(ei->expr);
    VectorAppend(&elements, ei->expr);
  }
  int best = -1;
  if (simple_elements) {
    VectorASTNode call = {0};
    call.children = &elements;
    StructMember* ctor =
        str->tag_name != NULL ? FindStructMember(str, str->tag_name) : NULL;
    for (StructMember* c = ctor; c != NULL; c = c->overload_next) {
      if (!c->is_member_function || c->symbol == NULL ||
          c->symbol->type == NULL || !TypeIsFunction(c->symbol->type) ||
          !c->symbol->type->info.function.is_constructor ||
          c->symbol->type->info.function.is_deleted ||
          c->symbol->flags.is_template) {
        continue;
      }
      int s = FunctionCallScore(c->symbol->type, &call, /*first_formal_arg=*/1);
      if (s >= 0 && (best < 0 || s < best)) {
        best = s;
      }
    }
    // Aggregate initialization: element-wise conversion to each data member.
    if (best < 0 && str->is_aggregate) {
      size_t member_index = 0;
      bool ok = true;
      for (size_t i = 0; i < elements.length; i++) {
        while (member_index < str->members.length) {
          StructMember* m = str->members.value.p[member_index];
          if (m != NULL && !m->is_member_function && m->symbol != NULL) {
            break;
          }
          member_index++;
        }
        if (member_index >= str->members.length) {
          ok = false;
          break;
        }
        StructMember* m = str->members.value.p[member_index++];
        ASTNode* element = elements.value.p[i];
        if (element == NULL || m->symbol == NULL ||
            OverloadBaseConversionRank(element->type, m->symbol->type) < 0) {
          ok = false;
          break;
        }
      }
      if (ok) {
        best = 0;
      }
    }
  }
  VectorDestruct(&elements);
  return best;
}

// [over.over] viability during overload resolution: `actual` names a function
// template or overload set and `target` is a pointer-to-function.  Returns a
// base rank (0, an exact function-to-pointer conversion) when a unique matching
// function can be formed, or -1.  Deduction/instantiation diagnostics are
// suppressed since this is a speculative probe over every candidate parameter.
static int FuncAddrBaseRank(ASTNode* actual, TypeRecord* target) {
  if (!CompilerIsCXX() || actual == NULL || actual->op != AST_OP(identifier)) {
    return -1;
  }
  if (!TypeIsPointer(target) || target->next == NULL ||
      !TypeIsFunction(target->next)) {
    return -1;
  }
  if (actual->type == NULL || !TypeIsFunction(actual->type)) {
    return -1;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)actual;
  if (id->symbol == NULL) {
    return -1;
  }
  DiagnosticSuppressBegin();
  Symbol* resolved = CXXResolveFunctionAddressForTargetType(
      id->symbol, id->template_arguments, target->next);
  DiagnosticSuppressEnd();
  return resolved != NULL ? 0 : -1;
}

static int OverloadConversionRank(ASTNode* actual, TypeRecord* formal_type) {
  TypeRecord* target = formal_type;
  bool reference = TypeIsReference(formal_type);
  if (reference) {
    target = formal_type->next;
    if (actual != NULL && actual->op == AST_OP(braced_init)) {
      if (CXXInitializerListBracedInitIsViable(actual, target)) {
        return 0;
      }
      int base = CXXBracedInitTargetRank(actual, target);
      if (base < 0) {
        return -1;
      }
      // A braced-init-list yields a prvalue temporary: it binds to an rvalue
      // reference or a const lvalue reference, but never a non-const lvalue
      // reference.  Prefer the rvalue reference so, e.g., `insert(value_type&&)`
      // wins over `insert(const value_type&)` for `insert({k, v})`.
      if (formal_type->declarator == kDeclRValueReference) {
        return base * 10;
      }
      if (TypeIsConst(target)) {
        return base * 10 + 1;
      }
      return -1;
    }
    int binding_rank = ReferenceBindingRank(actual, formal_type);
    if (binding_rank < 0) {
      return -1;
    }
    int base_rank = OverloadBaseConversionRank(actual->type, target);
    if (base_rank >= 0) {
      return base_rank * 10 + binding_rank;
    }
    if (FuncAddrBaseRank(actual, target) >= 0) {
      return binding_rank;
    }
    // A user-defined conversion produces a temporary. It can satisfy an rvalue
    // reference or a const lvalue reference, but never a non-const lvalue
    // reference.  Both directions are eligible: a converting constructor of the
    // target class, or a conversion operator on the source class yielding the
    // target (e.g. binding a `std::string` to `const std::string_view&` via
    // `std::string::operator string_view`).
    if (CompilerIsCXX() && !g_suppress_user_defined_conversion_rank &&
        (formal_type->declarator == kDeclRValueReference ||
         TypeIsConst(target)) &&
        TypeIsStructOrUnion(target) &&
        (FindConvertingConstructorCandidate(target, actual,
                                            /*allow_explicit=*/false,
                                            /*allow_same_class=*/false) != NULL ||
         ClassHasConversionOperatorTo(actual, target))) {
      return 100;
    }
    return -1;
  }

  if (actual != NULL && actual->op == AST_OP(braced_init)) {
    if (CXXInitializerListBracedInitIsViable(actual, target)) {
      return 0;
    }
    int base = CXXBracedInitTargetRank(actual, target);
    return base < 0 ? -1 : base * 10;
  }
  int base_rank = OverloadBaseConversionRank(actual->type, target);
  if (base_rank >= 0) {
    // [over.ics.rank]: when two standard conversion sequences are otherwise
    // indistinguishable, the one that is not a qualification conversion is
    // better.  Binding a `T*` argument to a `const T*` parameter differs from an
    // exact `T*` parameter only by adding pointee cv-qualifiers; give it a small
    // penalty so the exact `T*` overload wins instead of being ambiguous (e.g.
    // `get_if<0>(variant<...>*)` between the `variant<Types...>*` and
    // `const variant<Types...>*` overloads).  This applies to either const or
    // volatile qualification; otherwise the standard volatile overloads of
    // the atomic free functions are ambiguous for non-volatile objects.  Each
    // added qualifier contributes one sub-tier so adding const is better
    // than adding both const and volatile, without outweighing a genuine
    // conversion in another argument.
    Qualifiers added_pointer_qualifiers =
        (target->next != NULL && actual->type->next != NULL)
            ? ((target->next->qualifiers & (kQualConst | kQualVolatile)) &
               ~(actual->type->next->qualifiers &
                 (kQualConst | kQualVolatile)))
            : 0;
    bool pointer_qualification_conversion =
        TypeIsPointer(actual->type) && TypeIsPointer(target) &&
        actual->type->next != NULL && target->next != NULL &&
        added_pointer_qualifiers != 0 &&
        TypeEqualIgnoringQualifiers(actual->type, target);
    int qualification_penalty =
        pointer_qualification_conversion
            ? ((added_pointer_qualifiers & kQualConst) != 0) +
                  ((added_pointer_qualifiers & kQualVolatile) != 0)
            : 0;
    return base_rank * 10 + 5 + qualification_penalty;
  }
  if (FuncAddrBaseRank(actual, target) >= 0) {
    // Exact function-to-pointer conversion (an lvalue transformation).
    return 5;
  }
  if (IsZeroIntegerConstant(actual) && TypeIsPointer(target)) {
    return 25;
  }
  // As a last resort consider a user-defined conversion through a converting
  // constructor of a class target or a conversion operator on the source.
  // This ranks worse than any standard conversion sequence above, matching
  // the standard's ordering.
  if (CompilerIsCXX() && !g_suppress_user_defined_conversion_rank &&
      ((TypeIsStructOrUnion(target) &&
        FindConvertingConstructorCandidate(target, actual,
                                           /*allow_explicit=*/false,
                                           /*allow_same_class=*/false) != NULL) ||
       ClassHasConversionOperatorTo(actual, target))) {
    return 100;
  }
  return -1;
}

static int ConvertingConstructorRank(StructMember* member, ASTNode* from,
                                     bool allow_explicit,
                                     bool allow_invented) {
  Symbol* ctor_symbol = member->symbol;
  Symbol* temporary = NULL;
  if (ctor_symbol->flags.is_template) {
    Vector actuals;
    VectorInit(&actuals);
    VectorAppend(&actuals, from);
    DiagnosticSuppressBegin();
    temporary = TypeCreateFunctionTemplateCandidate(
        &compiler->syntax, ctor_symbol, NULL, &actuals,
        /*first_formal_arg=*/1);
    DiagnosticSuppressEnd();
    VectorDestruct(&actuals);
    if (temporary == NULL || temporary->type == NULL ||
        !TypeIsFunction(temporary->type)) {
      if (temporary != NULL) {
        SymbolDelete(temporary);
      }
      return -1;
    }
    ctor_symbol = temporary;
  }

  int rank = -1;
  FunctionInfo* fi = &ctor_symbol->type->info.function;
  if (!fi->is_constructor || fi->is_deleted ||
      (fi->is_explicit && !allow_explicit) ||
      (member->symbol->flags.invented && !allow_invented) ||
      fi->prototype.length < 2) {
    goto done;
  }
  for (size_t i = 2; i < fi->prototype.length; i++) {
    Symbol* param = fi->prototype.value.p[i];
    if (param == NULL || param->default_argument == NULL) {
      goto done;
    }
  }
  Symbol* param = fi->prototype.value.p[1];
  if (param == NULL || param->type == NULL) {
    goto done;
  }
  bool saved = g_suppress_user_defined_conversion_rank;
  g_suppress_user_defined_conversion_rank = true;
  rank = OverloadConversionRank(from, param->type);
  g_suppress_user_defined_conversion_rank = saved;

done:
  if (temporary != NULL) {
    SymbolDelete(temporary);
  }
  return rank;
}

static StructMember* FindConvertingConstructorCandidate(TypeRecord* to,
                                                        ASTNode* from,
                                                        bool allow_explicit,
                                                        bool allow_same_class) {
  if (!CompilerIsCXX() || to == NULL || from == NULL || from->type == NULL ||
      !TypeIsStructOrUnion(to) || to->info.struct_info == NULL ||
      to->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  bool same_class_source =
      TypeIsStructOrUnion(from->type) &&
      TypeEqualIgnoringQualifiers(from->type, to);
  // Same-class sources are only considered when an explicit class cast needs
  // to materialize a copy or move. Other conversion contexts already have
  // their own copy-initialization path. Derived sources use the standard
  // derived-to-base conversion.
  if (same_class_source && !allow_same_class) {
    return NULL;
  }
  if (TypeIsStructOrUnion(from->type) && !same_class_source &&
      TypeIsDerivedFrom(from->type, to)) {
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
    int rank = ConvertingConstructorRank(
        c, from, allow_explicit, allow_same_class && same_class_source);
    if (rank < 0) {
      continue;
    }
    if (best == NULL || rank < best_rank) {
      best = c;
      best_rank = rank;
      ambiguous = false;
    } else if (rank == best_rank) {
      bool best_is_template =
          best->symbol != NULL && best->symbol->flags.is_template;
      bool candidate_is_template = c->symbol->flags.is_template;
      if (best_is_template != candidate_is_template) {
        // [over.match.best.general]: for otherwise indistinguishable
        // candidates, a non-template function is better than a function
        // template specialization.  This is common for a class that has both
        // `T(const T&)` and `template<class U> T(const T<U>&)`.
        if (best_is_template) {
          best = c;
        }
        ambiguous = false;
      } else {
        ambiguous = true;
      }
    }
  }
  return ambiguous ? NULL : best;
}

StructMember* CXXFindConvertingConstructorCandidate(TypeRecord* to, ASTNode* from,
                                                    bool allow_explicit) {
  return FindConvertingConstructorCandidate(
      to, from, allow_explicit, /*allow_same_class=*/false);
}

void CXXValidateReturnInitialization(TypeRecord* to, ASTNode* from) {
  StructMember* constructor = FindConvertingConstructorCandidate(
      to, from, /*allow_explicit=*/false, /*allow_same_class=*/true);
  if (constructor == NULL || constructor->symbol == NULL) {
    SemanticError(from, "No viable constructor for return value");
    return;
  }
  CheckDeletedFunctionUse(constructor->symbol, from);
}

// Converts `from` to the class type `to` by constructing a temporary through a
// viable converting constructor (`to(from)`), splicing the resulting
// prvalue temporary in place of `from`.  Returns true if such a conversion was
// performed.  `ctx == kConvertCast` additionally allows explicit constructors
// (matching the explicit-conversion semantics of a cast).
static bool TryConvertWithConvertingConstructorImpl(
    ASTNode* from, TypeRecord* to, ConversionContext ctx,
    bool allow_same_class) {
  if (!CompilerIsCXX() || from == NULL || to == NULL ||
      !TypeIsStructOrUnion(to)) {
    return false;
  }
  // [over.best.ics]/4: an implicit conversion sequence contains at most one
  // user-defined conversion.  While we are already materializing one converting
  // constructor, a nested argument conversion must not invoke another, otherwise
  // resolving the constructor call can pick a constructor whose own parameter
  // requires the very same user-defined conversion, recursing without bound
  // (e.g. a copy constructor `T(const T&)` selected for a non-`T` argument).
  if (g_suppress_user_defined_conversion_rank) {
    return false;
  }
  StructMember* ctor = FindConvertingConstructorCandidate(
      to, from, /*allow_explicit=*/ctx == kConvertCast, allow_same_class);
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
  // Analyze the synthesized constructor call with user-defined conversions
  // suppressed: the argument must reach the chosen constructor's parameter by a
  // standard conversion only (the one user-defined conversion in this sequence
  // is the constructor itself), which prevents unbounded re-entry through
  // another converting constructor.
  bool saved_suppress = g_suppress_user_defined_conversion_rank;
  g_suppress_user_defined_conversion_rank = true;
  int errors_before_analysis = compiler->num_errors;
  ASTNode* analyzed = AnalyzeExpression(comma);
  g_suppress_user_defined_conversion_rank = saved_suppress;
  if (ctor->symbol != NULL && ctor->symbol->type != NULL &&
      TypeIsFunction(ctor->symbol->type) &&
      ctor->symbol->type->info.function.is_consteval &&
      compiler->num_errors == errors_before_analysis &&
      !CXXInImmediateFunctionContext() &&
      analyzed != NULL && analyzed->op == AST_OP(comma)) {
    ASTNode* converted_call = ((BinaryASTNode*)analyzed)->left;
    if (converted_call != NULL && converted_call->op == AST_OP(call)) {
      VectorASTNode* call = (VectorASTNode*)converted_call;
      if (call->left != NULL && call->left->op == AST_OP(identifier)) {
        IdentifierASTNode* id = (IdentifierASTNode*)call->left;
        if (id->symbol != NULL && id->symbol->flags.is_template) {
          Symbol* instantiated =
              TypeDeduceFunctionTemplateFromCallWithExplicitArgs(
                  &compiler->syntax, id->symbol, id->template_arguments,
                  call->children);
          if (instantiated != NULL && instantiated != id->symbol) {
            id->symbol = instantiated;
            ASTNodeSetInstantiatedCalleeType(call->left, instantiated->type);
          }
        }
      }
    }
    ConstEvalContext context;
    ConstEvalContextInit(&context);
    bool constant =
        converted_call != NULL && converted_call->op == AST_OP(call) &&
        ConstexprEvaluateConstructorCallForSymbol(&context, converted_call,
                                                  temp);
    ASTNode* initializer =
        constant ? ConstexprObjectInitializerForSymbol(temp, location) : NULL;
    temp->flags.value_set = false;
    temp->value.other = NULL;
    ConstEvalContextDestruct(&context);
    if (!constant) {
      if (!CXXEscalateCurrentFunction()) {
        SemanticError(converted_call != NULL ? converted_call : analyzed,
                      "consteval function call is not a constant expression");
      }
    } else if (initializer != NULL) {
      ASTNode* simplified =
          AnalyzeInitializer(temp->type, initializer, true);
      ASTNode* destination = NewIdentifierASTNode(temp, location);
      destination->flags |= kASTNeedAddress | kASTIsDeclaration;
      ASTNode* materialized =
          NewBinaryASTNode(AST_OP(init), TypeRecordCopy(temp->type), location,
                           destination, simplified);
      ASTNodeReplaceChild(analyzed, 0, materialized, true);
    }
  }
  analyzed->value_category = kValueCategoryPrvalue;
  if (parent != NULL) {
    ASTNodeReplaceChild(parent, child_id, analyzed, false);
  }
  return true;
}

bool TryConvertWithConvertingConstructor(ASTNode* from, TypeRecord* to,
                                         ConversionContext ctx) {
  return TryConvertWithConvertingConstructorImpl(
      from, to, ctx, /*allow_same_class=*/false);
}

static bool CallActualIsPackExpansion(ASTNode* actual) {
  if (actual == NULL) {
    return false;
  }
  if ((actual->flags & kASTPackExpansion) != 0) {
    return true;
  }
  if (actual->op == AST_OP(contents) || actual->op == AST_OP(address) ||
      actual->op == AST_OP(cast)) {
    ASTNode* sub = actual->op == AST_OP(cast)
                       ? ((CastASTNode*)actual)->expr
                       : ((UnaryASTNode*)actual)->sub;
    return CallActualIsPackExpansion(sub);
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
    bool constructor_this =
        func->info.function.is_constructor &&
        i + first_formal_arg == 0 &&
        formal != NULL && strcmp(formal->name.value, "this") == 0;
    int rank =
        constructor_this &&
                TypeEqualIgnoringQualifiers(actual->type, formal->type)
            ? 0
            : OverloadConversionRank(actual, formal->type);
    if (rank < 0) {
      return -1;
    }
    score += rank;
  }
  if (func->info.function.varargs && num_actual_args > num_user_formal_args) {
    // [over.ics.rank]: an ellipsis conversion sequence is worse than any
    // standard or user-defined conversion sequence.  Keep it viable for true
    // varargs calls, but do not let ignored tail arguments make `f(T, ...)`
    // look better than an overload with declared parameters for those arguments.
    score += 1000 * (int)(num_actual_args - num_user_formal_args);
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

static Symbol* TemplateOriginForConstraintOrdering(Symbol* candidate) {
  if (candidate == NULL) {
    return NULL;
  }
  if (candidate->flags.is_template) {
    return candidate;
  }
  if (candidate->type != NULL && TypeIsFunction(candidate->type)) {
    return candidate->type->info.function.template_origin;
  }
  return NULL;
}

static int ConstraintTieBreak(Symbol* best, Symbol* candidate) {
  Symbol* best_origin = TemplateOriginForConstraintOrdering(best);
  Symbol* candidate_origin = TemplateOriginForConstraintOrdering(candidate);
  return ConceptsCompareFunctionTemplateConstraints(candidate_origin,
                                                   best_origin);
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

static void ReportUnsatisfiedFunctionTemplateConstraints(VectorASTNode* node,
                                                         Symbol* templ,
                                                         Vector* explicit_args,
                                                         size_t first_formal_arg);
static void EmitFunctionTemplateCandidateRejectionNote(
    Symbol* candidate, VectorASTNode* call, Vector* explicit_args,
    size_t first_formal_arg);

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
      EmitFunctionTemplateCandidateRejectionNote(candidate, call, NULL, 0);
      continue;
    }
    String reason;
    StringInit(&reason, NULL);
    DescribeFunctionNonViability(candidate->type, call, 0, &reason);
    EmitCandidateNote(candidate, reason.value);
    StringDestruct(&reason);
  }
}

int CompareFunctionTemplateSpecificity(Symbol* left, Symbol* right);

static void DeleteTemporaryFunctionTemplateCandidates(Vector* candidates,
                                                      Symbol* keep) {
  if (candidates == NULL) {
    return;
  }
  for (size_t i = 0; i < candidates->length; i++) {
    Symbol* candidate = candidates->value.p[i];
    if (candidate != NULL && candidate != keep) {
      SymbolDelete(candidate);
    }
  }
  VectorDestruct(candidates);
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
  Vector temporary_candidates;
  VectorInit(&temporary_candidates);
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
          FunctionTemplateOverloadCandidate(candidate, &call, explicit_args,
                                            &temporary_candidates);
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
        } else if (best_is_template && effective_is_template) {
          int constraint_tie_break = ConstraintTieBreak(best, effective);
          if (constraint_tie_break > 0) {
            best = effective;
            ambiguous = false;
          } else if (constraint_tie_break < 0) {
            ambiguous = false;
          } else if (best != effective) {
            int template_order =
                CompareFunctionTemplateSpecificity(effective, best);
            if (template_order > 0) {
              best = effective;
              ambiguous = false;
            } else if (template_order < 0) {
              ambiguous = false;
            } else {
              ambiguous = true;
            }
          }
        } else if (best != effective &&
                   best_is_template == effective_is_template) {
          int template_order =
              CompareFunctionTemplateSpecificity(effective, best);
          if (template_order > 0) {
            best = effective;
            ambiguous = false;
          } else if (template_order < 0) {
            ambiguous = false;
          } else {
            ambiguous = true;
          }
        }
      }
    }
  }

  bool already_diagnosed =
      diagnostic_node != NULL &&
      (diagnostic_node->flags & kASTOverloadDiagnosed) != 0;
  if (best == NULL) {
    if (diagnose_no_match && !already_diagnosed) {
      Symbol* constraint_rejected = NULL;
      for (size_t i = 0; i < candidates->length; i++) {
        Symbol* candidate = candidates->value.p[i];
        if (candidate == NULL || !SymbolIsTemplateFunction(candidate)) {
          continue;
        }
        if (TypeClassifyFunctionTemplateCandidate(&compiler->syntax, candidate,
                                                  explicit_args, actuals,
                                                  0) ==
            kFunctionTemplateCandidateConstraintsNotSatisfied) {
          constraint_rejected = candidate;
          break;
        }
      }
      if (constraint_rejected != NULL &&
          ConceptsFunctionTemplateHasAssociatedConstraint(constraint_rejected)) {
        VectorASTNode call_node = {0};
        call_node.children = actuals;
        ReportUnsatisfiedFunctionTemplateConstraints(
            &call_node, constraint_rejected, explicit_args, 0);
        diagnostic_node->flags |= kASTOverloadDiagnosed;
        DeleteTemporaryFunctionTemplateCandidates(&temporary_candidates, NULL);
        return NULL;
      }
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
    DeleteTemporaryFunctionTemplateCandidates(&temporary_candidates, NULL);
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
    DeleteTemporaryFunctionTemplateCandidates(&temporary_candidates, best);
    return best;
  }
  Symbol* resolved = InstantiateSelectedFunctionTemplateCandidate(best);
  DeleteTemporaryFunctionTemplateCandidates(
      &temporary_candidates, resolved == best ? best : NULL);
  return resolved;
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
  // ResolveFunctionCandidateVector already turns the selected temporary
  // template candidate into its concrete specialization.  Instantiating that
  // specialization a second time reuses its argument vector against the
  // primary template and can permute dependent operands in operator templates.
  return best;
}

/* Public entry used when instantiating a cloned template body: a call whose
 * callee names an *overloaded* function template must undergo full overload
 * resolution once the arguments are concrete, not be bound to whichever single
 * overload the identifier happened to reference during the first (dependent)
 * pass.  Gathers the whole overload set reachable from `callee` (plus ADL over
 * the now-concrete actuals), resolves it, and returns the best concrete
 * instantiation, or NULL when none is viable. */
Symbol* CXXResolveOverloadedFunctionTemplateCall(Symbol* callee,
                                                 Vector* explicit_args,
                                                 Vector* actuals) {
  if (!CompilerIsCXX() || callee == NULL || actuals == NULL) {
    return NULL;
  }
  Vector candidates;
  VectorInit(&candidates);
  AddFunctionOverloadCandidates(&candidates, callee);
  AddADLFunctionCandidates(&callee->name, actuals, &candidates);
  ASTNode* diagnostic_node =
      actuals->length > 0 ? actuals->value.p[0] : NULL;
  Symbol* best = ResolveFunctionCandidateVector(
      &callee->name, &candidates, actuals, explicit_args,
      /*diagnose_no_match=*/false, /*diagnose_ambiguous=*/false,
      diagnostic_node);
  VectorDestruct(&candidates);
  return best;
}

static Symbol* FunctionTemplateOverloadCandidate(Symbol* candidate,
                                                 VectorASTNode* node,
                                                 Vector* explicit_args,
                                                 Vector* temporary_candidates) {
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
  Vector* prototype = &candidate->type->info.function.prototype;
  for (size_t i = 0; node->children != NULL && i < node->children->length &&
                     i < prototype->length; i++) {
    Symbol* formal = prototype->value.p[i];
    ASTNode* actual = node->children->value.p[i];
    if (formal != NULL && actual != NULL &&
        TypeIsCXXInitializerList(formal->type) &&
        actual->op != AST_OP(braced_init)) {
      return NULL;
    }
    if (formal != NULL && actual != NULL &&
        !TypeContainsTemplateParameter(formal->type) &&
        OverloadConversionRank(actual, formal->type) < 0) {
      return NULL;
    }
  }
  bool saved_trap = DiagnosticErrorTrapBegin();
  DiagnosticSuppressBegin();
  Symbol* instantiated = TypeCreateFunctionTemplateCandidate(
      &compiler->syntax, candidate, explicit_args, node->children, 0);
  bool substitution_failed = DiagnosticErrorTrapped();
  DiagnosticSuppressEnd();
  DiagnosticErrorTrapEnd(saved_trap);
  if (substitution_failed) {
    instantiated = NULL;
  }
  if (instantiated != NULL && temporary_candidates != NULL) {
    VectorAppend(temporary_candidates, instantiated);
  }
  return instantiated;
}

static Symbol* InstantiateSelectedFunctionTemplateCandidate(Symbol* selected) {
  if (selected == NULL || selected->type == NULL ||
      !TypeIsFunction(selected->type) ||
      selected->type->info.function.template_origin == NULL ||
      selected->type->template_arguments == NULL) {
    return selected;
  }
  Symbol* instantiated =
      TypeInstantiateFunctionTemplateWithCompletedArguments(
          &compiler->syntax, selected->type->info.function.template_origin,
          selected->type->template_arguments);
  CXXAnalyzeImmediateEscalationCandidate(instantiated);
  return instantiated != NULL ? instantiated : selected;
}

// [over.over] / [temp.deduct.funcaddr]: given a (possibly overloaded) function
// name -- the head |head| of an overload chain, optionally with explicitly
// written template arguments |explicit_args| -- used where a specific function
// type |target_fn| is required (the pointee of a destination function pointer),
// return the unique matching concrete function: a non-template overload whose
// type matches |target_fn| exactly, or a function-template specialization
// deduced/instantiated from |target_fn|.  Non-template matches are preferred
// over template specializations.  Returns NULL when there is no unique match.
Symbol* CXXResolveFunctionAddressForTargetType(Symbol* head,
                                               Vector* explicit_args,
                                               TypeRecord* target_fn) {
  if (!CompilerIsCXX() || head == NULL || target_fn == NULL ||
      !TypeIsFunction(target_fn)) {
    return NULL;
  }
  Symbol* non_template_match = NULL;
  bool non_template_ambiguous = false;
  Symbol* template_match = NULL;
  bool template_ambiguous = false;
  for (Symbol* cand = head; cand != NULL; cand = cand->overload_next) {
    if (cand->type == NULL || !TypeIsFunction(cand->type)) {
      continue;
    }
    if (SymbolIsTemplateFunction(cand)) {
      Symbol* inst = NULL;
      if (!cand->flags.is_template &&
          cand->type->info.function.template_origin != NULL &&
          cand->type->template_arguments != NULL) {
        // Overload resolution may already have replaced the identifier's
        // primary template with a non-emitting specialization candidate.  Its
        // signature and arguments are concrete, but the body has not been
        // instantiated yet.  Re-deducing from this candidate cannot work (it
        // is no longer marked as a primary template), so materialize the
        // selected specialization directly.
        if (!(TypeEqual(cand->type, target_fn) ||
              TypeEqualIgnoringQualifiers(cand->type, target_fn))) {
          continue;
        }
        inst = InstantiateSelectedFunctionTemplateCandidate(cand);
      } else {
        Vector* args = TypeDeduceFunctionTemplateArgumentsFromFunctionType(
            &compiler->syntax, cand, explicit_args, target_fn);
        if (args == NULL) {
          continue;
        }
        inst = TypeInstantiateFunctionTemplateWithCompletedArguments(
            &compiler->syntax, cand, args);
        VectorDeleteWithContents(
            args, (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
      }
      if (inst == NULL || inst->type == NULL || !TypeIsFunction(inst->type) ||
          !(TypeEqual(inst->type, target_fn) ||
            TypeEqualIgnoringQualifiers(inst->type, target_fn))) {
        continue;
      }
      if (template_match != NULL && template_match != inst) {
        template_ambiguous = true;
      } else {
        template_match = inst;
      }
    } else {
      // Explicit template arguments cannot apply to a non-template function.
      if (explicit_args != NULL && explicit_args->length > 0) {
        continue;
      }
      if (TypeEqual(cand->type, target_fn) ||
          TypeEqualIgnoringQualifiers(cand->type, target_fn)) {
        if (non_template_match != NULL && non_template_match != cand) {
          non_template_ambiguous = true;
        } else {
          non_template_match = cand;
        }
      }
    }
  }
  if (non_template_match != NULL) {
    return non_template_ambiguous ? NULL : non_template_match;
  }
  if (template_match != NULL) {
    return template_ambiguous ? NULL : template_match;
  }
  return NULL;
}

// [over.over]: when `from` names a (possibly overloaded / templated) function
// and the required type `to` is a pointer-to-function (or a function type in a
// reference-binding context), resolve the unique matching function and rewrite
// `from` in place to refer to the concrete specialization.  Returns true on a
// successful rewrite.  A no-op (returns false) for non-C++, non-identifier
// operands, or when no unique function matches.
bool CXXTryResolveFunctionAddressNode(ASTNode* from, TypeRecord* to) {
  if (!CompilerIsCXX() || from == NULL || to == NULL) {
    return false;
  }
  TypeRecord* target_fn = NULL;
  if (TypeIsPointer(to) && to->next != NULL && TypeIsFunction(to->next)) {
    target_fn = to->next;
  } else if (TypeIsReference(to) && to->next != NULL && TypeIsPointer(to->next) &&
             to->next->next != NULL && TypeIsFunction(to->next->next)) {
    target_fn = to->next->next;
  } else if (TypeIsFunction(to)) {
    target_fn = to;
  } else {
    return false;
  }
  if (from->type == NULL || !TypeIsFunction(from->type) ||
      from->op != AST_OP(identifier)) {
    return false;
  }
  // Only the un-substituted template pattern (or an unresolved overload set)
  // needs help here; a concrete function whose type already matches is handled
  // by the ordinary function-to-pointer conversion.
  IdentifierASTNode* id = (IdentifierASTNode*)from;
  if (id->symbol == NULL) {
    return false;
  }
  Symbol* resolved = CXXResolveFunctionAddressForTargetType(
      id->symbol, id->template_arguments, target_fn);
  if (resolved == NULL || resolved->type == NULL) {
    return false;
  }
  id->symbol = resolved;
  resolved->flags.used = true;
  ASTNodeSetType(from, resolved->type);
  from->flags |= kASTNeedAddress;
  return true;
}

static void EmitFunctionTemplateCandidateRejectionNote(
    Symbol* candidate, VectorASTNode* call, Vector* explicit_args,
    size_t first_formal_arg) {
  if (candidate == NULL || !SymbolIsTemplateFunction(candidate)) {
    return;
  }
  FunctionTemplateCandidateStatus status =
      TypeClassifyFunctionTemplateCandidate(&compiler->syntax, candidate,
                                            explicit_args, call->children,
                                            first_formal_arg);
  if (status == kFunctionTemplateCandidateConstraintsNotSatisfied) {
    EmitCandidateNote(candidate, NULL);
    Vector* args = TypeDeduceFunctionTemplateArgumentsFromCall(
        candidate, call->children, first_formal_arg);
    if (args == NULL && explicit_args != NULL) {
      args = TemplateArgumentVectorCopy(explicit_args);
    }
    if (args != NULL) {
      ConceptsReportFunctionTemplateConstraintFailure(candidate, args);
      VectorDeleteWithContents(
          args, (VectorElementDestructor)TemplateArgumentDelete,
          /*free_element=*/false);
    }
    return;
  }
  EmitCandidateNote(candidate,
                    "could not deduce template arguments for this call");
}

static void ReportUnsatisfiedFunctionTemplateConstraints(VectorASTNode* node,
                                                         Symbol* templ,
                                                         Vector* explicit_args,
                                                         size_t first_formal_arg) {
  String function_name;
  StringInit(&function_name, NULL);
  SymbolFunctionDiagnosticName(templ, &function_name);
  SemanticError((ASTNode*)node, "constraints not satisfied for function template %s",
                function_name.value);
  StringDestruct(&function_name);
  if (templ != NULL && TypeIsFunction(templ->type)) {
    Vector* args =
        TypeDeduceFunctionTemplateArgumentsFromCall(templ, node->children,
                                                    first_formal_arg);
    if (args == NULL && explicit_args != NULL) {
      args = TemplateArgumentVectorCopy(explicit_args);
    }
    ConceptsReportFunctionTemplateConstraintFailure(templ, args);
    if (args != NULL) {
      VectorDeleteWithContents(args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
  }
}

static int FunctionTemplatePatternTypeSpecificity(TypeRecord* type) {
  if (type == NULL) {
    return 0;
  }
  int score = 0;
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (t->template_parameter_index >= 0) {
      continue;
    }
    if (TypeIsReference(t)) {
      // A forwarding-reference pattern `T&&` accepts essentially any type and
      // is less specialized than structural patterns such as `T*`.
      score += 0;
    } else if (TypeIsPointer(t) || TypeIsArray(t)) {
      score += 1;
    } else {
      score += 2;
    }
    if (TypeIsStructOrUnion(t) && t->template_origin != NULL &&
        t->template_arguments != NULL) {
      score += 4;
      for (size_t i = 0; i < t->template_arguments->length; i++) {
        TemplateArgument* arg = t->template_arguments->value.p[i];
        if (arg != NULL && arg->kind == kTemplateParameterType) {
          score += FunctionTemplatePatternTypeSpecificity(arg->type);
        }
      }
    }
  }
  return score;
}

static int FunctionTemplatePatternSpecificity(Symbol* instantiated) {
  if (instantiated == NULL || instantiated->type == NULL ||
      !TypeIsFunction(instantiated->type)) {
    return 0;
  }
  Symbol* origin = instantiated->type->info.function.template_origin;
  if (origin == NULL || origin->type == NULL || !TypeIsFunction(origin->type)) {
    return 0;
  }
  int score = 0;
  Vector* prototype = &origin->type->info.function.prototype;
  for (size_t i = 0; i < prototype->length; i++) {
    Symbol* formal = prototype->value.p[i];
    if (formal != NULL) {
      score += FunctionTemplatePatternTypeSpecificity(formal->type);
    }
  }
  return score;
}

int CompareFunctionTemplateSpecificity(Symbol* left, Symbol* right) {
  bool left_is_template = left != NULL && left->type != NULL &&
                          TypeIsFunction(left->type) &&
                          left->type->info.function.template_origin != NULL;
  bool right_is_template = right != NULL && right->type != NULL &&
                           TypeIsFunction(right->type) &&
                           right->type->info.function.template_origin != NULL;
  if (!left_is_template || !right_is_template) {
    return 0;
  }
  int left_score = FunctionTemplatePatternSpecificity(left);
  int right_score = FunctionTemplatePatternSpecificity(right);
  if (left_score > right_score) {
    return 1;
  }
  if (right_score > left_score) {
    return -1;
  }
  return 0;
}

static int MemberOverloadCallScore(StructMember* candidate,
                                   VectorASTNode* node,
                                   BinaryASTNode* member_access,
                                   bool check_receiver_const);

static Vector* ClassTemplateArgumentsFromMemberAccess(
    BinaryASTNode* member_access) {
  if (member_access == NULL || member_access->left == NULL) {
    return NULL;
  }
  TypeRecord* receiver_type = member_access->left->type;
  if (receiver_type == NULL) {
    return NULL;
  }
  if (TypeIsPointer(receiver_type)) {
    receiver_type = receiver_type->next;
  }
  if (receiver_type == NULL) {
    return NULL;
  }
  if (receiver_type->template_arguments != NULL) {
    return receiver_type->template_arguments;
  }
  if (TypeIsStructOrUnion(receiver_type) &&
      receiver_type->info.struct_info != NULL) {
    if (receiver_type->template_arguments != NULL) {
      return receiver_type->template_arguments;
    }
    if (receiver_type->info.struct_info->tag_symbol != NULL &&
        receiver_type->info.struct_info->tag_symbol->type != NULL &&
        receiver_type->info.struct_info->tag_symbol->type->template_arguments !=
            NULL) {
      return receiver_type->info.struct_info->tag_symbol->type
          ->template_arguments;
    }
  }
  return NULL;
}

static bool MemberFunctionConstraintsSatisfied(StructMember* candidate,
                                               BinaryASTNode* member_access) {
  if (candidate == NULL || candidate->symbol == NULL ||
      candidate->symbol->type == NULL ||
      !TypeIsFunction(candidate->symbol->type) ||
      candidate->symbol->type->info.function.associated_constraint == NULL) {
    return true;
  }
  ConstraintExpr* constraint =
      candidate->symbol->type->info.function.associated_constraint;
  Vector* class_args = ClassTemplateArgumentsFromMemberAccess(member_access);
  if (class_args == NULL) {
    Struct* owner =
        candidate->symbol->type->info.function.cxx_member_owner;
    if (owner != NULL && owner->tag_symbol != NULL &&
        owner->tag_symbol->type != NULL) {
      class_args = owner->tag_symbol->type->template_arguments;
    }
  }
  if (class_args == NULL &&
      ConceptsConstraintContainsTemplateParameter(constraint)) {
    return true;
  }
  return ConceptsConstraintSatisfied(constraint, class_args);
}

static ASTNode* ExplicitObjectReceiverForCall(BinaryASTNode* member_access,
                                              ASTNode* scratch) {
  if (member_access == NULL || member_access->left == NULL) {
    return NULL;
  }
  if (member_access->base.op == AST_OP(dot)) {
    return member_access->left;
  }
  if (!TypeIsPointer(member_access->left->type) ||
      member_access->left->type->next == NULL) {
    return NULL;
  }
  *scratch = *member_access->left;
  scratch->op = AST_OP(contents);
  scratch->type = member_access->left->type->next;
  scratch->value_category = kValueCategoryLvalue;
  return scratch;
}

static void BuildExplicitObjectCallActuals(VectorASTNode* call,
                                           BinaryASTNode* member_access,
                                           ASTNode* receiver_scratch,
                                           Vector* actuals) {
  VectorInit(actuals);
  ASTNode* receiver =
      ExplicitObjectReceiverForCall(member_access, receiver_scratch);
  if (receiver != NULL) {
    VectorAppend(actuals, receiver);
  }
  for (size_t i = 0; call->children != NULL &&
                     i < call->children->length; i++) {
    VectorAppend(actuals, call->children->value.p[i]);
  }
}

static int MemberOverloadCallScore(StructMember* candidate,
                                   VectorASTNode* node,
                                   BinaryASTNode* member_access,
                                   bool check_receiver_const) {
  TypeRecord* function_type = candidate->symbol->type;
  if (FunctionHasExplicitObjectParameter(function_type)) {
    ASTNode receiver_scratch;
    Vector actuals;
    BuildExplicitObjectCallActuals(node, member_access, &receiver_scratch,
                                   &actuals);
    VectorASTNode explicit_call = *node;
    explicit_call.children = &actuals;
    int score = FunctionCallScore(function_type, &explicit_call, 0);
    VectorDestruct(&actuals);
    if (score >= 0 &&
        !MemberFunctionConstraintsSatisfied(candidate, member_access)) {
      return -1;
    }
    return score;
  }
  size_t first_formal_arg = candidate->is_static ? 0 : 1;
  if (check_receiver_const && !candidate->is_static &&
      !candidate->symbol->type->info.function.is_const_member &&
      !candidate->symbol->type->info.function.is_constructor &&
      !candidate->symbol->type->info.function.is_destructor &&
      MemberReceiverIsConst(member_access)) {
    return -1;
  }
  if (check_receiver_const && !candidate->is_static &&
      !candidate->symbol->type->info.function.is_volatile_member &&
      !candidate->symbol->type->info.function.is_constructor &&
      !candidate->symbol->type->info.function.is_destructor &&
      MemberReceiverIsVolatile(member_access)) {
    return -1;
  }
  if (!candidate->is_static &&
      !MemberReceiverMatchesRefQualifier(candidate->symbol->type,
                                         member_access)) {
    return -1;
  }
  int score = FunctionCallScore(candidate->symbol->type, node, first_formal_arg);
  if (score >= 0 &&
      !MemberFunctionConstraintsSatisfied(candidate, member_access)) {
    return -1;
  }
  if (score >= 0 && !candidate->is_static &&
      candidate->symbol->type->info.function.is_const_member &&
      !MemberReceiverIsConst(member_access)) {
    score++;
  }
  if (score >= 0 && !candidate->is_static &&
      candidate->symbol->type->info.function.is_volatile_member &&
      !MemberReceiverIsVolatile(member_access)) {
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
                                     Vector* explicit_args, bool ambiguous,
                                     int best_score) {
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
      size_t first_formal_arg = candidate->is_static ? 0 : 1;
      EmitFunctionTemplateCandidateRejectionNote(candidate->symbol, node,
                                                 explicit_args,
                                                 first_formal_arg);
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
                                                     BinaryASTNode* member_access,
                                                     Vector* explicit_args,
                                                     Vector* temporary_members) {
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
  bool explicit_object =
      FunctionHasExplicitObjectParameter(candidate->symbol->type);
  size_t first_formal_arg = candidate->is_static || explicit_object ? 0 : 1;
  ASTNode receiver_scratch;
  Vector explicit_object_actuals;
  Vector* deduction_actuals = node->children;
  if (explicit_object) {
    BuildExplicitObjectCallActuals(node, member_access, &receiver_scratch,
                                   &explicit_object_actuals);
    deduction_actuals = &explicit_object_actuals;
  }
  Vector* prototype = &candidate->symbol->type->info.function.prototype;
  for (size_t i = 0; deduction_actuals != NULL && i < deduction_actuals->length &&
                     i + first_formal_arg < prototype->length; i++) {
    Symbol* formal = prototype->value.p[i + first_formal_arg];
    ASTNode* actual = deduction_actuals->value.p[i];
    TypeRecord* target =
        formal != NULL && TypeIsReference(formal->type) ? formal->type->next
                                                        : formal->type;
    // An `initializer_list<U>` parameter is satisfied either by a braced-init
    // argument (`{1, 2, 3}`) or by an argument that is already an
    // `initializer_list<V>` (e.g. forwarding a bound `init` parameter through a
    // second member template); reject only arguments that are neither.
    if (formal != NULL && actual != NULL && actual->type != NULL &&
        TypeIsCXXInitializerList(target) &&
        actual->op != AST_OP(braced_init) &&
        !TypeIsCXXInitializerList(TypeIsReference(actual->type)
                                      ? actual->type->next
                                      : actual->type)) {
      if (explicit_object) {
        VectorDestruct(&explicit_object_actuals);
      }
      return NULL;
    }
  }
  DiagnosticSuppressBegin();
  Symbol* instantiated = TypeCreateFunctionTemplateCandidate(
      &compiler->syntax, candidate->symbol, explicit_args, deduction_actuals,
      first_formal_arg);
  DiagnosticSuppressEnd();
  if (explicit_object) {
    VectorDestruct(&explicit_object_actuals);
  }
  if (instantiated == NULL) {
    return NULL;
  }
  StructMember* member = NewStructMember(instantiated);
  member->is_member_function = true;
  member->is_static = candidate->is_static;
  member->access = candidate->access;
  if (temporary_members != NULL) {
    VectorAppend(temporary_members, member);
  }
  return member;
}

static void DeleteTemporaryMemberTemplateCandidates(Vector* members,
                                                    StructMember* keep) {
  if (members == NULL) {
    return;
  }
  // Candidate members can be installed into member-access ASTs while an
  // enclosing expression is still being re-analysed (notably auto-return and
  // requires-expression instantiation).  A later pass may retain one that was
  // not the final `keep` value observed by this invocation.  Reclaiming the
  // rejected wrappers here leaves that AST with a dangling StructMember and
  // eventually corrupts code generation.  Their symbols/types are already
  // compilation-lifetime template candidates, so keep these tiny wrappers for
  // the same lifetime.
  (void)keep;
  VectorDestruct(members);
}

static StructMember* InstantiateSelectedMemberTemplateCandidate(
    StructMember* selected) {
  if (selected == NULL || selected->symbol == NULL ||
      selected->symbol->type == NULL ||
      !TypeIsFunction(selected->symbol->type) ||
      selected->symbol->type->info.function.template_origin == NULL ||
      selected->symbol->type->template_arguments == NULL) {
    return selected;
  }
  Symbol* instantiated =
      TypeInstantiateFunctionTemplateWithCompletedArguments(
          &compiler->syntax,
          selected->symbol->type->info.function.template_origin,
          selected->symbol->type->template_arguments);
  if (instantiated == NULL) {
    return selected;
  }
  CXXAnalyzeImmediateEscalationCandidate(instantiated);
  StructMember* member = NewStructMember(instantiated);
  member->is_member_function = true;
  member->is_static = selected->is_static;
  member->access = selected->access;
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
  Vector temporary_members;
  VectorInit(&temporary_members);
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
          MemberTemplateOverloadCandidate(candidate, node, member_access,
                                          explicit_args,
                                          &temporary_members);
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
      DeleteTemporaryMemberTemplateCandidates(&temporary_members, NULL);
      return first;
    }
    StructMember* constraint_rejected = NULL;
    for (StructMember* candidate = first; candidate != NULL;
         candidate = candidate->overload_next) {
      if (candidate->symbol == NULL ||
          !ConceptsFunctionTemplateHasAssociatedConstraint(candidate->symbol)) {
        if (candidate->symbol != NULL &&
            candidate->symbol->type != NULL &&
            TypeIsFunction(candidate->symbol->type) &&
            candidate->symbol->type->info.function.associated_constraint !=
                NULL &&
            !MemberFunctionConstraintsSatisfied(candidate, member_access)) {
          constraint_rejected = candidate;
          break;
        }
        continue;
      }
      size_t first_formal_arg = candidate->is_static ? 0 : 1;
      Vector* args = TypeDeduceFunctionTemplateArgumentsFromCall(
          candidate->symbol, node->children, first_formal_arg);
      bool rejected = args != NULL &&
                      !ConceptsFunctionTemplateConstraintsSatisfied(
                          candidate->symbol, args);
      if (args != NULL) {
        VectorDeleteWithContents(
            args, (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
      }
      if (rejected) {
        constraint_rejected = candidate;
        break;
      }
    }
    if (constraint_rejected != NULL) {
      if (ConceptsFunctionTemplateHasAssociatedConstraint(
              constraint_rejected->symbol)) {
        size_t first_formal_arg = constraint_rejected->is_static ? 0 : 1;
        ReportUnsatisfiedFunctionTemplateConstraints(
            node, constraint_rejected->symbol, explicit_args, first_formal_arg);
      } else {
        SemanticError((ASTNode*)node, "constraints not satisfied");
        if (constraint_rejected->symbol != NULL &&
            TypeIsFunction(constraint_rejected->symbol->type)) {
          ConceptsReportAssociatedConstraintFailure(
              constraint_rejected->symbol->type->info.function
                  .associated_constraint,
              ClassTemplateArgumentsFromMemberAccess(member_access),
              constraint_rejected->symbol->location, NULL);
        }
      }
      ((ASTNode*)node)->flags |= kASTOverloadDiagnosed;
    } else if (receiver_const_rejected) {
      String function_name;
      StringInit(&function_name, NULL);
      SymbolFunctionDiagnosticSuffix(first->symbol, &function_name);
      SemanticError((ASTNode*)member_access,
                    "Cannot call non-const member function %s on const object%s",
                    first->symbol->name.value, function_name.value);
      StringDestruct(&function_name);
      EmitMemberCandidateNotes(first, node, member_access, explicit_args,
                               /*ambiguous=*/false, -1);
      ((ASTNode*)node)->flags |= kASTOverloadDiagnosed;
    } else if (first->overload_next != NULL ||
               first->symbol->type->info.function.is_constructor) {
      String function_name;
      StringInit(&function_name, NULL);
      SymbolFunctionDiagnosticName(first->symbol, &function_name);
      SemanticError((ASTNode*)node, "No matching overload for %s",
                    function_name.value);
      StringDestruct(&function_name);
      EmitMemberCandidateNotes(first, node, member_access, explicit_args,
                               /*ambiguous=*/false, -1);
      ((ASTNode*)node)->flags |= kASTOverloadDiagnosed;
    } else if (first != NULL &&
               MemberOverloadCallScore(first, node, member_access, true) < 0 &&
               first->symbol != NULL && first->symbol->type != NULL &&
               TypeIsFunction(first->symbol->type) &&
               first->symbol->type->info.function.associated_constraint != NULL) {
      SemanticError((ASTNode*)node, "constraints not satisfied");
      ConceptsReportAssociatedConstraintFailure(
          first->symbol->type->info.function.associated_constraint,
          ClassTemplateArgumentsFromMemberAccess(member_access),
          first->symbol->location, NULL);
      ((ASTNode*)node)->flags |= kASTOverloadDiagnosed;
    }
    DeleteTemporaryMemberTemplateCandidates(&temporary_members, NULL);
    return first;
  }
  if (ambiguous) {
    if (already_diagnosed) {
      DeleteTemporaryMemberTemplateCandidates(&temporary_members, best);
      return best;
    }
    String function_name;
    StringInit(&function_name, NULL);
    SymbolFunctionDiagnosticName(first->symbol, &function_name);
    SemanticError((ASTNode*)node, "Ambiguous overload for %s",
                  function_name.value);
    StringDestruct(&function_name);
    EmitMemberCandidateNotes(first, node, member_access, explicit_args,
                             /*ambiguous=*/true, best_score);
    ((ASTNode*)node)->flags |= kASTOverloadDiagnosed;
    DeleteTemporaryMemberTemplateCandidates(&temporary_members, best);
    return best;
  }
  if (best != NULL && !MemberFunctionConstraintsSatisfied(best, member_access)) {
    if (!already_diagnosed) {
      SemanticError((ASTNode*)node, "constraints not satisfied");
      if (best->symbol != NULL && TypeIsFunction(best->symbol->type)) {
        ConceptsReportAssociatedConstraintFailure(
            best->symbol->type->info.function.associated_constraint,
            ClassTemplateArgumentsFromMemberAccess(member_access),
            best->symbol->location, NULL);
      }
      ((ASTNode*)node)->flags |= kASTOverloadDiagnosed;
    }
    DeleteTemporaryMemberTemplateCandidates(
        &temporary_members, best);
    return best;
  }
  StructMember* resolved = InstantiateSelectedMemberTemplateCandidate(best);
  if (resolved != NULL) {
    CXXAnalyzeImmediateEscalationCandidate(resolved->symbol);
  }
  DeleteTemporaryMemberTemplateCandidates(
      &temporary_members, resolved == best ? best : NULL);
  return resolved;
}

static void ResolveOverloadedFunctionCall(VectorASTNode* node) {
  if (node->left == NULL || node->left->op != AST_OP(identifier)) {
    return;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node->left;
  if (id->symbol != NULL && id->symbol->type != NULL &&
      TypeIsFunction(id->symbol->type) &&
      id->symbol->type->info.function.template_origin != NULL) {
    Symbol* instantiated = InstantiateSelectedFunctionTemplateCandidate(id->symbol);
    if (instantiated != NULL) {
      id->symbol = instantiated;
      ASTNodeSetInstantiatedCalleeType(node->left, instantiated->type);
      CheckDeletedFunctionUse(instantiated, (ASTNode*)node);
    }
    return;
  }
  if (!CompilerIsCXX() && !id->symbol->flags.is_overloaded) {
    return;
  }
  Vector candidates;
  VectorInit(&candidates);
  bool gathered_inline_overloads = false;
  bool is_namespace_scope_function =
      id->symbol != NULL && id->symbol->type != NULL &&
      TypeIsFunction(id->symbol->type) &&
      id->symbol->type->info.function.cxx_member_owner == NULL &&
      !id->symbol->flags.is_block_scope;
  if (is_namespace_scope_function) {
    Namespace* lookup_ns =
        NamespaceParentForInlineTransparentLookup(id->symbol->namespace_);
    AddNamedFunctionCandidates(&id->symbol->name, lookup_ns, &candidates);
    gathered_inline_overloads = candidates.length > 1;
  } else {
    AddFunctionOverloadCandidates(&candidates, id->symbol);
  }
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
  if (!gathered_inline_overloads && !id->symbol->flags.is_overloaded &&
      !has_adl_candidates && !ordinary_unknown) {
    VectorDestruct(&candidates);
    return;
  }
  Symbol* best = ResolveFunctionCandidateVector(
      &id->symbol->name, &candidates, node->children, id->template_arguments,
      /*diagnose_no_match=*/id->symbol->flags.is_overloaded || has_adl_candidates ||
          ordinary_unknown || gathered_inline_overloads,
      /*diagnose_ambiguous=*/id->symbol->flags.is_overloaded ||
          has_adl_candidates || ordinary_unknown || gathered_inline_overloads,
      (ASTNode*)node);
  VectorDestruct(&candidates);
  if (best == NULL) {
    return;
  }

  best = InstantiateSelectedFunctionTemplateCandidate(best);
  id->symbol = best;
  ASTNodeSetInstantiatedCalleeType(node->left, best->type);
  CheckDeletedFunctionUse(best, (ASTNode*)node);
}

static void InstantiateResolvedFunctionTemplateCall(VectorASTNode* node) {
  if (node == NULL || node->left == NULL || node->left->op != AST_OP(identifier)) {
    return;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node->left;
  if (id->symbol == NULL || id->symbol->type == NULL ||
      !TypeIsFunction(id->symbol->type) ||
      id->symbol->type->info.function.template_origin == NULL) {
    return;
  }
  Symbol* instantiated =
      id->symbol->type->template_arguments != NULL
          ? InstantiateSelectedFunctionTemplateCandidate(id->symbol)
          : TypeDeduceFunctionTemplateFromCallWithExplicitArgs(
                &compiler->syntax,
                id->symbol->type->info.function.template_origin,
                /*explicit_args=*/NULL, node->children);
  if (instantiated != NULL) {
    id->symbol = instantiated;
    ASTNodeSetInstantiatedCalleeType(node->left, instantiated->type);
    CheckDeletedFunctionUse(instantiated, (ASTNode*)node);
  }
}

static bool TemplateArgumentVectorContainsTemplateParameter(Vector* args);

static StructMember* FindCXXMemberOverloadHead(Struct* owner, String* name) {
  StructMember* fallback = FindStructMember(owner, name);
  if (owner == NULL || name == NULL) {
    return fallback;
  }
  for (size_t i = 0; i < owner->members.length; i++) {
    StructMember* member = owner->members.value.p[i];
    if (member == NULL || member->symbol == NULL ||
        !StringEqualString(&member->symbol->name, name)) {
      continue;
    }
    if (member->overload_next != NULL) {
      return member;
    }
    if (fallback == NULL) {
      fallback = member;
    }
  }
  return fallback;
}

static ASTNode* AnalyzeCXXFunctionalClassConstruction(VectorASTNode* node) {
  if (!CompilerIsCXX() || node->left == NULL) {
    return NULL;
  }
  if (node->left->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node->left;
    if (id->symbol != NULL && id->symbol->flags.is_template &&
        StorageIs(id->symbol->storage, STO(typedef)) &&
        id->template_arguments != NULL &&
        !TemplateArgumentVectorContainsTemplateParameter(
            id->template_arguments)) {
      TypeRecord* alias_type = TypeInstantiateClassTemplate(
          &compiler->syntax, id->symbol, id->template_arguments);
      if (alias_type != NULL) {
        ASTNodeSetType(node->left, alias_type);
      }
    }
  }
  if (!TypeIsStructOrUnion(node->left->type)) {
    if (node->left->op != AST_OP(identifier)) {
      return NULL;
    }
    IdentifierASTNode* id = (IdentifierASTNode*)node->left;
    if (id->symbol == NULL ||
        !StorageIs(id->symbol->storage, STO(typedef)) ||
        node->children->length > 1) {
      return NULL;
    }
    SourceLocation location = node->base.location;
    ASTNode* initializer =
        node->children->length == 0
            ? NewIntConstantASTNode(0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                                    location)
            : ASTNodeMove(node->children->value.p[0]);
    ASTNode* cast =
        NewCastASTNode(TypeRecordCopy(node->left->type), location, initializer);
    ASTNode* parent = node->base.parent;
    if (parent != NULL) {
      ASTNodeReplaceChild(parent, node->base.child_id, cast, true);
    }
    return AnalyzeExpression(cast);
  }
  if (node->left->type->info.struct_info == NULL ||
      node->left->type->info.struct_info->tag_name == NULL) {
    return NULL;
  }

  // An explicit template-id used as a functional cast (`A<int>(...)`) names the
  // concrete specialization directly; instantiate it from those arguments
  // rather than attempting class template argument deduction from the
  // constructor call.  When the construction appears inside another template,
  // the body cloner substitutes the arguments to concrete types before this
  // runs, so this handles the dependent case (e.g. `allocator<CharT>()`) too.
  TypeRecord* explicit_type = NULL;
  if (node->left->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node->left;
    if (id->symbol != NULL && id->symbol->flags.is_template &&
        id->symbol->type != NULL &&
        id->template_arguments != NULL &&
        !TemplateArgumentVectorContainsTemplateParameter(
            id->template_arguments)) {
      bool class_template =
          TypeIsStructOrUnion(id->symbol->type) &&
          id->symbol->type->template_arguments == NULL &&
          id->symbol->type->info.struct_info != NULL &&
          id->symbol->type->info.struct_info->is_template;
      bool alias_template = StorageIs(id->symbol->storage, STO(typedef));
      if (class_template || alias_template) {
        explicit_type = TypeInstantiateClassTemplate(
            &compiler->syntax, id->symbol, id->template_arguments);
      }
    }
  }

  TypeRecord* placeholder_type = NULL;
  TypeRecord* construction_type =
      explicit_type != NULL ? explicit_type : node->left->type;
  if (explicit_type == NULL &&
      !TypeIsClassTemplatePlaceholder(construction_type) &&
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
  TypeRecord* deduced_type = explicit_type;
  if (explicit_type == NULL && TypeIsClassTemplatePlaceholder(construction_type)) {
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
  StructMember* constructor =
      FindCXXMemberOverloadHead(type->info.struct_info, constructor_name);
  bool has_user_declared_constructor = false;
  for (StructMember* candidate = constructor; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->is_member_function && candidate->symbol != NULL &&
        !candidate->symbol->flags.invented) {
      has_user_declared_constructor = true;
      break;
    }
  }
  if (type->info.struct_info->lexical_parent == NULL &&
      compiler->current_function != NULL &&
      TypeIsFunction(compiler->current_function) &&
      compiler->current_function->info.function.cxx_member_owner != NULL &&
      constructor_name != NULL) {
    Struct* current_owner =
        compiler->current_function->info.function.cxx_member_owner;
    StructMember* nested = FindStructMember(current_owner, constructor_name);
    if (nested != NULL && nested->symbol != NULL &&
        StorageIs(nested->symbol->storage, STO(typedef)) &&
        TypeIsStructOrUnion(nested->symbol->type)) {
      type->info.struct_info->lexical_parent = current_owner;
    }
  }
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
  // A class template's primary struct can retain a stale aggregate flag even
  // after user-declared constructors have been parsed.  Do not route a
  // concrete functional construction through aggregate initialization solely
  // on that flag; verify the constructor set just as the declaration parser
  // does for parenthesized initialization.
  if (type->info.struct_info->is_aggregate &&
      !type->info.struct_info->is_template &&
      !has_user_declared_constructor) {
    Symbol* temp = SyntaxNewTemporary(&compiler->syntax, type);
    temp->location = location;
    ASTNode* temp_id = NewIdentifierASTNode(temp, location);
    temp_id->flags |= kASTNeedAddress | kASTIsDeclaration;
    Vector* elements = NewVector();
    for (size_t i = 0; i < node->children->length; i++) {
      ASTNode* child = node->children->value.p[i];
      if (child != NULL) {
        VectorAppend(elements, ASTNodeMove(child));
      }
    }
    ASTNode* initializer =
        NewBracedInitializerASTNode(elements, NULL, location);
    ASTNode* literal =
        NewCompoundLiteralASTNode(temp_id, location, initializer);
    ASTNode* parent = node->base.parent;
    if (parent != NULL) {
      ASTNodeReplaceChild(parent, node->base.child_id, literal, true);
    }
    ASTNode* analyzed = AnalyzeExpression(literal);
    analyzed->value_category = kValueCategoryPrvalue;
    return analyzed;
  }
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
    if (node->children->length == 0) {
      // `T()` value-initialization of an aggregate class with no constructor
      // (e.g. an empty struct like std::monostate).  Zero-initialize a
      // temporary via an empty compound literal `(T){}`, matching `T{}`.
      Symbol* temp = SyntaxNewTemporary(&compiler->syntax, type);
      temp->location = location;
      ASTNode* temp_id = NewIdentifierASTNode(temp, location);
      temp_id->flags |= kASTNeedAddress | kASTIsDeclaration;
      ASTNode* initializer =
          NewBracedInitializerASTNode(NewVector(), NULL, location);
      ASTNode* literal =
          NewCompoundLiteralASTNode(temp_id, location, initializer);
      ASTNode* parent = node->base.parent;
      if (parent != NULL) {
        ASTNodeReplaceChild(parent, node->base.child_id, literal, true);
      }
      ASTNode* analyzed = AnalyzeExpression(literal);
      analyzed->value_category = kValueCategoryPrvalue;
      return analyzed;
    }
    return NULL;
  }
  bool has_non_invented_constructor = false;
  for (StructMember* candidate = constructor; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->is_member_function && candidate->symbol != NULL &&
        candidate->symbol->type != NULL &&
        TypeIsFunction(candidate->symbol->type)) {
      candidate->symbol->type->info.function.cxx_member_owner =
          type->info.struct_info;
      StringClear(&candidate->symbol->asm_name);
      SymbolSetCXXMangledAsmName(candidate->symbol);
    }
    if (candidate->is_member_function && candidate->symbol != NULL &&
        !candidate->symbol->flags.invented) {
      has_non_invented_constructor = true;
      break;
    }
  }
  if (!has_non_invented_constructor && node->children->length == 0) {
    Symbol* temp = SyntaxNewTemporary(&compiler->syntax, type);
    temp->location = location;
    ASTNode* temp_id = NewIdentifierASTNode(temp, location);
    temp_id->flags |= kASTNeedAddress | kASTIsDeclaration;
    ASTNode* initializer =
        NewBracedInitializerASTNode(NewVector(), NULL, location);
    ASTNode* literal =
        NewCompoundLiteralASTNode(temp_id, location, initializer);
    ASTNode* parent = node->base.parent;
    if (parent != NULL) {
      ASTNodeReplaceChild(parent, node->base.child_id, literal, true);
    }
    ASTNode* analyzed = AnalyzeExpression(literal);
    analyzed->value_category = kValueCategoryPrvalue;
    return analyzed;
  }
  if (!has_non_invented_constructor && node->children->length > 1) {
    return NULL;
  }

  Symbol* temp = SyntaxNewTemporary(&compiler->syntax, type);
  temp->location = location;
  ASTNode* receiver = NewIdentifierASTNode(temp, location);
  ASTNode* member = NewStructMemberASTNode(constructor, location);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, member);

  Vector* actuals = NewVector();
  for (size_t i = 0; i < node->children->length; i++) {
    VectorAppend(actuals, ASTNodeMove(node->children->value.p[i]));
  }
  ASTNode* constructor_call =
      NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals);
  LowerMemberFunctionCall((VectorASTNode*)constructor_call);
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
      formal_type->info.struct_info->tag_name == NULL) {
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
  analyzed->flags |= kASTFunctionParameterTemporary;
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
  if (node->left->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node->left;
    if (id->symbol != NULL && StorageIs(id->symbol->storage, STO(typedef))) {
      return NULL;
    }
  }

  StructMember* member =
      FindStructMemberByName(node->left->type->info.struct_info, "operator()");
  if (member == NULL || !member->is_member_function) {
    return NULL;
  }

  // A generic lambda captured inside a template whose closure does not depend
  // on the enclosing template keeps its call operator numbered relative to that
  // template; rebase it to a standalone 0-based template now so deduction and
  // instantiation of `operator()` can proceed (no-op in the common case).
  Struct* closure = node->left->type->info.struct_info;
  for (Symbol* overload = member->symbol; overload != NULL;
       overload = overload->overload_next) {
    TypeRebaseNonDependentLambdaCallOperator(closure, overload);
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

// Visitor that flags when an expression references a template parameter, either
// directly (a bare parameter identifier) or through a parameter-dependent type
// or template argument.  Used to detect value-dependent non-type template
// arguments carried as unevaluated expressions.
static void ExprContainsTemplateParameterVisitor(ASTNode* n, void* data,
                                                  int child_id,
                                                  VisitorMode mode) {
  (void)child_id;
  (void)mode;
  if (n == NULL || n->op != AST_OP(identifier)) {
    return;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)n;
  if (id->symbol == NULL) {
    return;
  }
  if ((id->symbol->flags.is_template_parameter &&
       id->symbol->template_parameter_index >= 0) ||
      id->symbol->dependent_value_template_parameter_index >= 0 ||
      TypeContainsTemplateParameter(id->symbol->type) ||
      TemplateArgumentVectorContainsTemplateParameter(id->template_arguments)) {
    *(bool*)data = true;
  }
}

static bool ExpressionContainsTemplateParameter(ASTNode* expr) {
  bool found = false;
  ASTNodeVisit(expr, ExprContainsTemplateParameterVisitor, 0, &found);
  return found;
}

static bool TemplateArgumentContainsTemplateParameter(TemplateArgument* arg) {
  if (arg == NULL) {
    return false;
  }
  if (arg->template_parameter_index >= 0 ||
      TypeContainsTemplateParameter(arg->type)) {
    return true;
  }
  if (arg->dependent_expr != NULL &&
      ExpressionContainsTemplateParameter(arg->dependent_expr)) {
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

static bool CallActualsContainDependentFunctorCall(VectorASTNode* call) {
  for (size_t i = 0; call != NULL && i < call->children->length; i++) {
    ASTNode* actual = call->children->value.p[i];
    if (actual != NULL && (actual->flags & kASTDependentFunctorCall) != 0) {
      return true;
    }
  }
  return false;
}

// True when `node` is a call through a member-function access whose overload
// cannot yet be resolved because it is still value/type-dependent: either the
// explicit template arguments or an actual argument mention a template
// parameter.  This arises when partially instantiating an enclosing member
// template (its own parameters remain generic while the class arguments are
// baked in), e.g. `this->emplace<index_of<remove_cvref_t<T>, Types...>::value>(
// forward<T>(value))` inside a variant's `operator=`.  Such a call must be
// deferred (left unresolved) until the enclosing member template is
// instantiated with concrete arguments, rather than run through overload
// resolution now (which would fail to deduce and diagnose a spurious error).
static bool IsDependentMemberTemplateCall(VectorASTNode* node) {
  if (!CompilerIsCXX() || node->left == NULL ||
      (node->left->op != AST_OP(dot) && node->left->op != AST_OP(arrow))) {
    return false;
  }
  BinaryASTNode* access = (BinaryASTNode*)node->left;
  if (access->right == NULL || access->right->op != AST_OP(structmember)) {
    return false;
  }
  StructMemberASTNode* member_node = (StructMemberASTNode*)access->right;
  if (member_node->member == NULL || !member_node->member->is_member_function) {
    return false;
  }
  // Dependence in an argument only requires deferral when the member overload
  // set itself contains a function template whose deduction must wait.  An
  // ordinary member of an already-concrete class can accept a function
  // template name contextually through a concrete function-pointer parameter
  // ([temp.deduct.funcaddr]); treating that template name as an unresolved
  // call dependency skips overload resolution and leaves the primary template
  // address in the generated code (e.g. ostream::operator<<(std::endl)).
  bool has_member_template = false;
  for (Symbol* candidate = member_node->member->symbol; candidate != NULL;
       candidate = candidate->overload_next) {
    if (SymbolIsTemplateFunction(candidate)) {
      has_member_template = true;
      break;
    }
  }
  if (TemplateArgumentVectorContainsTemplateParameter(
          member_node->template_arguments)) {
    return true;
  }
  if (!CallActualsContainTemplateParameter(node)) {
    return false;
  }
  if (has_member_template) {
    return true;
  }
  // An ordinary member call still has to wait for genuinely dependent values
  // (for example, a member-template constructor initializing a concrete member
  // from its U&& argument).  The one exception is a bare function-template
  // name: its pattern type contains template parameters, but a concrete
  // function-pointer formal supplies the target type needed by [over.over].
  for (size_t i = 0; i < node->children->length; i++) {
    ASTNode* actual = node->children->value.p[i];
    if (actual == NULL || !TypeContainsTemplateParameter(actual->type)) {
      continue;
    }
    if (actual->op == AST_OP(identifier)) {
      IdentifierASTNode* id = (IdentifierASTNode*)actual;
      if (id->symbol != NULL && TypeIsFunction(actual->type) &&
          SymbolIsTemplateFunction(id->symbol)) {
        continue;
      }
    }
    return true;
  }
  return false;
}

/* True when a member-function-template call names its member template with
 * explicit template arguments that are still template-dependent (e.g.
 * `rest.template ctor<Target>(...)` where `Target` is an unresolved parameter
 * of the enclosing member template). Such a call cannot be resolved to a
 * concrete member overload no matter what its actuals look like -- even when an
 * actual is a pack expansion -- so it must be deferred until the member
 * template is instantiated with concrete arguments. */
static bool IsMemberTemplateCallWithDependentExplicitArgs(VectorASTNode* node) {
  if (!CompilerIsCXX() || node->left == NULL ||
      (node->left->op != AST_OP(dot) && node->left->op != AST_OP(arrow))) {
    return false;
  }
  BinaryASTNode* access = (BinaryASTNode*)node->left;
  if (access->right == NULL || access->right->op != AST_OP(structmember)) {
    return false;
  }
  StructMemberASTNode* member_node = (StructMemberASTNode*)access->right;
  if (member_node->member == NULL || !member_node->member->is_member_function) {
    return false;
  }
  return TemplateArgumentVectorContainsTemplateParameter(
      member_node->template_arguments);
}

static void SetDependentMemberTemplateCallType(VectorASTNode* node) {
  BinaryASTNode* access = (BinaryASTNode*)node->left;
  StructMemberASTNode* member_node = (StructMemberASTNode*)access->right;
  TypeRecord* return_type = TypeSubstituteFunctionTemplateReturnType(
      &compiler->syntax, member_node->member->symbol,
      member_node->template_arguments);
  if (return_type == NULL) {
    ASTNodeSetType((ASTNode*)node,
                   NewTypeRecordWithSize(kTypeInt | kTypeUnknown,
                                         kQualPlain));
    return;
  }
  if (TypeIsReference(return_type)) {
    ASTNodeSetType((ASTNode*)node, return_type->next);
    node->base.value_category =
        return_type->declarator == kDeclRValueReference
            ? kValueCategoryXvalue
            : kValueCategoryLvalue;
  } else {
    ASTNodeSetType((ASTNode*)node, return_type);
  }
  TypeRecordDelete(return_type);
}

// A compiler-generated base-subobject destructor cleanup call spells the base
// by the name recorded when the enclosing template was *defined* -- the base's
// primary template name (e.g. "A" for a base `A<T>").  After instantiation the
// base subobject `A<int>` registers its destructor under the *specialized* tag
// ("~A<int>"), so the spelled "~A" no longer resolves by exact name.  Search the
// (direct or indirect) base subobjects of `str` for one whose tag name or
// primary-template name equals `base_name`, returning that base's actual tag
// name so the spelling can be rewritten to a resolvable destructor name.  Only
// bases are searched: a spelling that names the object's own type is handled by
// the own-destructor rewrite instead.
static String* CXXFindBaseTagNameForDestructorSpelling(Struct* str,
                                                       const char* base_name) {
  if (str == NULL || base_name == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base == NULL || base->type == NULL ||
        !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    Struct* base_struct = base->type->info.struct_info;
    if (base_struct->tag_name != NULL &&
        strcmp(base_struct->tag_name->value, base_name) == 0) {
      return base_struct->tag_name;
    }
    Symbol* origin = base->type->template_origin;
    if (origin == NULL && base_struct->tag_symbol != NULL &&
        base_struct->tag_symbol->type != NULL) {
      origin = base_struct->tag_symbol->type->template_origin;
    }
    if (origin != NULL && strcmp(origin->name.value, base_name) == 0) {
      return base_struct->tag_name;
    }
    // A template-id base ("O<int>") whose destructor is spelled by its primary
    // name ("~O").  template_origin is not always propagated onto the base type
    // (notably for instantiations that carry virtual bases), so also match the
    // primary-template prefix textually: the tag name begins with `base_name`
    // immediately followed by the template-argument list opener '<'.
    if (base_struct->tag_name != NULL) {
      size_t n = strlen(base_name);
      const char* tag = base_struct->tag_name->value;
      if (strncmp(tag, base_name, n) == 0 && tag[n] == '<') {
        return base_struct->tag_name;
      }
    }
    String* nested =
        CXXFindBaseTagNameForDestructorSpelling(base_struct, base_name);
    if (nested != NULL) {
      return nested;
    }
  }
  return NULL;
}

// Handles an explicit destructor or pseudo-destructor call written through a
// member-access callee: `obj.~T()`, `p->~T()`, or `p->~int()`.  This is needed
// for generic code (e.g. containers destroying their elements) where the named
// type may resolve to a class or to a scalar, and where the spelled name (such
// as a template parameter `~T`) does not textually match the class's own
// destructor name.
//
//   * If the object is a class type with a materialized (user-declared or
//     synthesized non-trivial) destructor, the spelled name is rewritten to the
//     object type's real destructor name and NULL is returned so the ordinary
//     member-call path resolves and invokes it.
//   * Otherwise -- a scalar type, or a class whose destructor is trivial and
//     therefore not materialized as a member -- the call is a well-formed no-op
//     of type void that still evaluates the object expression for its side
//     effects, and that replacement expression is returned.
//
// Returns NULL when this is not an explicit destructor call, or when the object
// type is still dependent (so analysis is deferred to instantiation).
static ASTNode* TryAnalyzeCXXExplicitDestructorCall(VectorASTNode* node) {
  if (!CompilerIsCXX() || node->left == NULL ||
      (node->left->op != AST_OP(dot) && node->left->op != AST_OP(arrow))) {
    return NULL;
  }
  // A user-written destructor call takes no explicit arguments, but a
  // compiler-generated base-subobject cleanup call carries the implicit
  // `__complete_object` flag argument when the base has virtual bases.  We must
  // still rewrite a dependent/primary-name spelling ("~O" -> "~O<int>") in that
  // case, so do not bail here; instead only the trivial/pseudo-destructor no-op
  // replacement below is suppressed when arguments are present (that path would
  // otherwise drop the argument).
  bool has_arguments = node->children != NULL && node->children->length != 0;
  BinaryASTNode* access = (BinaryASTNode*)node->left;
  if (access->right == NULL || access->right->op != AST_OP(string)) {
    return NULL;
  }
  String* spelled = ((ConstantASTNode*)access->right)->value.string;
  if (spelled == NULL || spelled->length == 0 || spelled->value[0] != '~') {
    return NULL;
  }

  // Learn the (possibly concrete) type of the object being destroyed.
  access->left = AnalyzeExpression(access->left);
  if (access->left != NULL) {
    access->left->parent = (ASTNode*)access;
    access->left->child_id = 0;
  }
  TypeRecord* object_type = access->left != NULL ? access->left->type : NULL;
  if (access->base.op == AST_OP(arrow)) {
    if (!TypeIsPointer(object_type)) {
      // Not a raw pointer (e.g. dependent, or an overloaded operator->); let
      // the ordinary member-reference path handle or diagnose it.
      return NULL;
    }
    object_type = object_type->next;
  }
  if (object_type == NULL || TypeContainsTemplateParameter(object_type)) {
    // Dependent: defer to instantiation, where the type becomes concrete.
    return NULL;
  }

  if (TypeIsStructOrUnion(object_type) &&
      object_type->info.struct_info != NULL &&
      object_type->info.struct_info->tag_name != NULL) {
    Struct* struct_info = object_type->info.struct_info;
    // If the spelled name already names a destructor reachable from the object
    // -- its own destructor (`p->~Der()`) or an inherited base-subobject
    // destructor (`derived_ptr->~Base()`, as generated for base cleanup) --
    // leave it entirely to the ordinary member-call path.  Only names that do
    // NOT resolve (e.g. an unsubstituted template parameter `~T`) are rewritten.
    CXXAccess spelled_access = kAccessPublic;
    Struct* spelled_owner = NULL;
    int spelled_offset = 0;
    StructMember* spelled_member = FindStructMemberWithAccessAndOffset(
        struct_info, spelled, &spelled_access, &spelled_owner, &spelled_offset);
    if (spelled_member != NULL && spelled_member->is_member_function &&
        spelled_member->symbol != NULL &&
        TypeIsFunction(spelled_member->symbol->type) &&
        spelled_member->symbol->type->info.function.is_destructor) {
      return NULL;
    }
    // A base-subobject cleanup call `this->~A()` generated inside a class
    // template spells the base by its primary name ("~A"); after instantiation
    // the base subobject is `A<int>` with destructor "~A<int>".  If the spelled
    // name names a base by tag or primary-template name, rewrite it to that
    // base's specialized destructor name so the ordinary member-call path binds
    // to the base's destructor -- rather than falling through to the own-type
    // rewrite below, which would bind to the derived class's own destructor and
    // recurse forever.
    String* base_tag = CXXFindBaseTagNameForDestructorSpelling(
        struct_info, spelled->value + 1);
    if (base_tag != NULL) {
      String base_destructor_name;
      StringInit(&base_destructor_name, "~");
      StringAppendString(&base_destructor_name, base_tag);
      if (!StringEqualString(spelled, &base_destructor_name)) {
        ConstantASTNode* name_node = (ConstantASTNode*)access->right;
        StringSet(name_node->value.string, base_destructor_name.value);
      }
      StringDestruct(&base_destructor_name);
      return NULL;
    }
    String destructor_name;
    StringInit(&destructor_name, "~");
    StringAppendString(&destructor_name, struct_info->tag_name);
    StructMember* destructor = FindStructMember(struct_info, &destructor_name);
    if (destructor != NULL && destructor->is_member_function &&
        destructor->symbol != NULL &&
        TypeIsFunction(destructor->symbol->type) &&
        destructor->symbol->type->info.function.is_destructor) {
      // Rewrite the unresolved spelled name (e.g. "~T") to the object type's
      // real destructor name so the ordinary member-call path resolves it.
      if (!StringEqualString(spelled, &destructor_name)) {
        ConstantASTNode* name_node = (ConstantASTNode*)access->right;
        StringSet(name_node->value.string, destructor_name.value);
      }
      StringDestruct(&destructor_name);
      return NULL;
    }
    StringDestruct(&destructor_name);
    // Class with a trivial (unmaterialized) destructor: fall through to no-op.
  }

  // If the call carries arguments (the implicit `__complete_object` flag of a
  // generated base cleanup, or ill-formed user arguments) we must not collapse
  // it into an argument-dropping no-op; hand it to the ordinary member-call path
  // which either binds a materialized destructor or diagnoses the arguments.
  if (has_arguments) {
    return NULL;
  }

  // Scalar pseudo-destructor, or trivial-destructor class: evaluate the object
  // expression for its side effects and yield void.
  ASTNode* object_expr = access->left;
  access->left = NULL;
  if (object_expr != NULL) {
    object_expr->parent = NULL;
    object_expr->child_id = 0;
  }
  ASTNode* discard =
      NewCastASTNode(NewTypeRecordWithSize(kTypeVoid, kQualPlain),
                     node->base.location, object_expr);
  ((CastASTNode*)discard)->kind = kCastStatic;
  return ReplaceVectorWithCall(node, discard);
}

static ASTNode* AnalyzeFunctionCall(VectorASTNode* node) {
  ASTNode* destructor_call = TryAnalyzeCXXExplicitDestructorCall(node);
  if (destructor_call != NULL) {
    return destructor_call;
  }
  // A call node stores its callee separately from its argument vector, but its
  // legacy child replacer indexes only that argument vector.  Detach the callee
  // while analyzing it so a transformation such as `T{}` -> a materialized
  // class temporary cannot accidentally replace argument zero.  This is
  // especially important for immediate functor calls (`T{}(arg)`).
  ASTNode* old_callee = node->left;
  node->left = NULL;
  if (old_callee != NULL) {
    old_callee->parent = NULL;
  }
  bool syntactic_class_construction =
      old_callee != NULL &&
      (old_callee->flags & kASTCXXFunctionalConstruction) != 0;
  ASTNode* analyzed_callee =
      syntactic_class_construction ? old_callee : AnalyzeExpression(old_callee);
  if (analyzed_callee != old_callee) {
    if (old_callee != NULL && analyzed_callee != NULL) {
      analyzed_callee->flags |=
          old_callee->flags & kASTReferencesParameterPack;
    }
    ASTNodeDelete(old_callee);
  }
  node->left = analyzed_callee;
  if (analyzed_callee != NULL) {
    analyzed_callee->parent = &node->base;
    analyzed_callee->child_id = 0;
  }
  // A dependent pseudo-destructor can become scalar only while the member
  // access above is being instantiated.  Re-run the explicit-destructor path
  // after that access has acquired its concrete receiver type.
  destructor_call = TryAnalyzeCXXExplicitDestructorCall(node);
  if (destructor_call != NULL) {
    return destructor_call;
  }
  // A member-function-template call named with dependent explicit template
  // arguments (e.g. `rest.template ctor<Target>(std::forward<Args>(args)...)`
  // inside a member template whose parameter `Target` is not yet concrete)
  // cannot be resolved to a member overload no matter what its actuals are.
  // Defer it *before* analyzing the actuals: the actuals may be pack-expansion
  // patterns over the member template's own packs, and analyzing them here
  // would prematurely concretize those patterns against the enclosing class's
  // arguments.
  if (IsMemberTemplateCallWithDependentExplicitArgs(node)) {
    node->base.flags |= kASTDependentFunctorCall;
    SetDependentMemberTemplateCallType(node);
    return (ASTNode*)node;
  }
  ASTNode* early_meta_synthesis =
      SemanticTryAnalyzeMetaSynthesisCallEarly(node);
  if (early_meta_synthesis != NULL) {
    early_meta_synthesis->flags |= kASTRequiresASTConstexpr;
    return early_meta_synthesis;
  }
  size_t num_actual_args = node->children->length;
  for (size_t i = 0; i < num_actual_args; i++) {
    node->children->value.p[i] = AnalyzeExpression((ASTNode*)node->children->value.p[i]);
  }
  ASTNode* meta_call = SemanticTryAnalyzeMetaCall(node);
  if (meta_call != NULL) {
    meta_call->flags |= kASTRequiresASTConstexpr;
    return meta_call;
  }
  ASTNode* meta_trait_call = SemanticTryAnalyzeMetaTraitCall(node);
  if (meta_trait_call != NULL) {
    meta_trait_call->flags |= kASTRequiresASTConstexpr;
    return meta_trait_call;
  }
  ASTNode* meta_synthesis_call = SemanticTryAnalyzeMetaSynthesisCall(node);
  if (meta_synthesis_call != NULL) {
    meta_synthesis_call->flags |= kASTRequiresASTConstexpr;
    return meta_synthesis_call;
  }
  bool has_pack_expansion_actual = CallHasPackExpansionActual(node);
  if (CallActualsContainDependentFunctorCall(node)) {
    node->base.flags |= kASTDependentFunctorCall;
    ASTNodeSetType((ASTNode*)node,
                   NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain));
    return (ASTNode*)node;
  }
  if (syntactic_class_construction) {
    ASTNode* construction = AnalyzeCXXFunctionalClassConstruction(node);
    if (construction != NULL) {
      return construction;
    }
  }
  // A member-function-template call whose explicit template arguments or actuals
  // are still template-dependent cannot be resolved yet; defer it so it is
  // re-analyzed once the enclosing template is instantiated with concrete
  // arguments.
  if (!has_pack_expansion_actual && IsDependentMemberTemplateCall(node)) {
    node->base.flags |= kASTDependentFunctorCall;
    SetDependentMemberTemplateCallType(node);
    return (ASTNode*)node;
  }
  if (node->left != NULL && node->left->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node->left;
    if (id->symbol != NULL && id->symbol->flags.is_template &&
        !id->symbol->flags.is_overloaded &&
        id->symbol->overload_next == NULL &&
        TypeIsFunction(id->symbol->type) &&
        !has_pack_expansion_actual &&
        !TemplateArgumentVectorContainsTemplateParameter(id->template_arguments) &&
        !CallActualsContainTemplateParameter(node)) {
      if (FunctionTemplateHasDefinition(id->symbol)) {
        Symbol* instantiated =
            TypeDeduceFunctionTemplateFromCallWithExplicitArgs(
                &compiler->syntax, id->symbol, id->template_arguments,
                node->children);
        if (instantiated != id->symbol) {
          id->symbol = instantiated;
          ASTNodeSetInstantiatedCalleeType(node->left, instantiated->type);
        }
      }
      if (id->symbol->flags.is_template &&
          !TypeCanDeduceFunctionTemplateFromCallWithExplicitArgsAndOffset(
              id->symbol, id->template_arguments, node->children, 0)) {
        if (ConceptsFunctionTemplateHasAssociatedConstraint(id->symbol)) {
          ReportUnsatisfiedFunctionTemplateConstraints(node, id->symbol,
                                                       id->template_arguments,
                                                       0);
        } else {
          SemanticError((ASTNode*)node, "Template argument deduction failed");
        }
        ASTNodeSetType((ASTNode*)node,
                       NewTypeRecordWithSize(kTypeInt, kQualPlain));
        return (ASTNode*)node;
      }
    }
  }
  if (node->left != NULL && node->left->op == AST_OP(identifier) &&
      CallActualsContainTemplateParameter(node)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node->left;
    if (id->symbol != NULL &&
        (id->symbol->flags.is_overloaded ||
         id->symbol->flags.is_template ||
         id->symbol->overload_next != NULL)) {
      node->base.flags |= kASTDependentFunctorCall;
      ASTNodeSetType((ASTNode*)node,
                     NewTypeRecordWithSize(kTypeInt | kTypeUnknown,
                                           kQualPlain));
      return (ASTNode*)node;
    }
  }
  if (node->left != NULL && node->left->op == AST_OP(identifier)) {
    ResolveOverloadedFunctionCall(node);
  }
  bool lowered_member_call = LowerMemberFunctionCall(node);
  if (!lowered_member_call) {
    lowered_member_call = LowerMemberPointerFunctionCall(node);
  }
  if (!lowered_member_call && node->left != NULL &&
      (node->left->op == AST_OP(dot) || node->left->op == AST_OP(arrow))) {
    BinaryASTNode* member_access = (BinaryASTNode*)node->left;
    TypeRecord* receiver_type =
        member_access->left != NULL ? member_access->left->type : NULL;
    if (receiver_type != NULL && node->left->op == AST_OP(arrow) &&
        TypeIsPointerOrArray(receiver_type)) {
      receiver_type = receiver_type->next;
    }
    if (receiver_type != NULL &&
        TypeContainsTemplateParameter(receiver_type)) {
      node->base.flags |= kASTDependentFunctorCall;
      ASTNodeSetType((ASTNode*)node,
                     NewTypeRecordWithSize(kTypeInt | kTypeUnknown,
                                           kQualPlain));
      return (ASTNode*)node;
    }
  }
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
        (TypeIsStructOrUnion(callee_id->symbol->type) ||
         StorageIs(callee_id->symbol->storage, STO(typedef))) &&
        !has_pack_expansion_actual &&
        !CallActualsContainTemplateParameter(node) &&
        !TemplateArgumentVectorContainsTemplateParameter(
            callee_id->template_arguments)) {
      is_concrete_ctad_construction = true;
    }
  }
  // A call whose callee type still depends on template parameters (e.g. a
  // functor or lambda received as a template parameter) cannot be resolved
  // until instantiation.  Flag it and give it a placeholder type so it is
  // re-analyzed once the template arguments are known.
  if (CompilerIsCXX() && node->left != NULL && node->left->type != NULL &&
      !is_concrete_ctad_construction &&
      (TypeContainsTemplateParameter(node->left->type) ||
       TypeIsUnknown(node->left->type))) {
    if (node->left->op == AST_OP(identifier)) {
      IdentifierASTNode* id = (IdentifierASTNode*)node->left;
      TypeRecord* return_type = TypeSubstituteFunctionTemplateReturnType(
          &compiler->syntax, id->symbol, id->template_arguments);
      if (return_type != NULL) {
        // Even though the *generic* callee type still mentions a template
        // parameter, a call with fully concrete explicit template arguments and
        // actuals (e.g. a declaration-only helper such as
        // `std::declval<int*&>()`) resolves to a concrete return type now.  In
        // that case the call is NOT dependent: leaving it flagged would make
        // any enclosing call that receives it as an argument needlessly defer
        // (and later lose reference qualifiers on the deferred result).
        bool result_is_concrete =
            !TypeContainsTemplateParameter(return_type) &&
            !TypeIsUnknown(return_type) &&
            !TemplateArgumentVectorContainsTemplateParameter(
                id->template_arguments) &&
            !CallActualsContainTemplateParameter(node) &&
            !CallActualsContainDependentFunctorCall(node);
        if (!result_is_concrete) {
          node->base.flags |= kASTDependentFunctorCall;
        }
        if (TypeIsReference(return_type)) {
          ASTNodeSetType((ASTNode*)node, return_type->next);
          node->base.value_category =
              return_type->declarator == kDeclRValueReference
                  ? kValueCategoryXvalue
                  : kValueCategoryLvalue;
        } else {
          ASTNodeSetType((ASTNode*)node, return_type);
        }
        if (result_is_concrete) {
          CheckDeletedFunctionUse(id->symbol, (ASTNode*)node);
        }
        TypeRecordDelete(return_type);
        return (ASTNode*)node;
      }
      if (id->symbol != NULL && !id->symbol->flags.is_overloaded &&
          id->symbol->overload_next == NULL) {
        CheckDeletedFunctionUse(id->symbol, (ASTNode*)node);
      }
    }
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
  InstantiateResolvedFunctionTemplateCall(node);
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
  // Reaching here means every deferral path above was skipped: this call is
  // resolved to a concrete function (or function pointer).  A cloned template
  // body may have left a stale `kASTDependentFunctorCall` marker on the node
  // (e.g. a single-element pack expansion of `std::forward<T>(arg)`, whose
  // callee substitution flags the enclosing call as dependent).  Clear it now
  // so any enclosing call does not needlessly defer -- and therefore fail to
  // lower -- on a fully-resolved actual.
  node->base.flags &= ~kASTDependentFunctorCall;
  if (CompilerIsCXX() && node->left->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node->left;
    TypeEnsureTemplateMemberFunctionDefinition(&compiler->syntax, id->symbol);
    CXXAnalyzeImmediateEscalationCandidate(id->symbol);
  } else if (CompilerIsCXX() &&
             (node->left->op == AST_OP(dot) ||
              node->left->op == AST_OP(arrow))) {
    BinaryASTNode* access = (BinaryASTNode*)node->left;
    if (access->right != NULL &&
        access->right->op == AST_OP(structmember)) {
      StructMemberASTNode* member =
          (StructMemberASTNode*)access->right;
      if (member->member != NULL && member->member->symbol != NULL) {
        TypeEnsureTemplateMemberFunctionDefinition(
            &compiler->syntax, member->member->symbol);
        CXXAnalyzeImmediateEscalationCandidate(member->member->symbol);
      }
    }
  }

  // Set node type by dereferencing the function.  We've already checked that
  // the left type is a function or a pointer to a function.  The 'next' field
  // of the function is the type of this node (the return type of the function).
  TypeRecord* subtype = node->left->type;
  if (TypeIsPointer(node->left->type)) {
    subtype = subtype->next;
  }
  SemanticEnsureAutoReturnTypeDeduced(subtype);

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
      // A bare braced-init-list argument (`f({})`, `f({1, 2})`) targeting a
      // non-`initializer_list` parameter is not itself an expression; lower it
      // to a temporary of the parameter type so the backend receives a real
      // value rather than a raw braced-init node (which it cannot generate).
      if (actual != NULL && actual->op == AST_OP(braced_init)) {
        TypeRecord* braced_target = TypeIsReference(formal->type)
                                        ? formal->type->next
                                        : formal->type;
        ASTNode* lowered = LowerCXXBracedInitToTarget(actual, braced_target);
        if (lowered != actual) {
          ASTNodeReplaceChild((ASTNode*)node, (int)i, lowered, false);
          actual = lowered;
          // A braced-init-list argument denotes a temporary (a prvalue) rather
          // than a named object; LowerCXXBracedInitToTarget materializes it as a
          // compound literal, which is categorized as an lvalue.  Re-categorize
          // it as an xvalue so it can bind to an rvalue-reference parameter
          // (e.g. `insert(value_type&&)` for `insert({k, v})`) while still
          // binding to a const lvalue reference.
          if (TypeIsReference(formal->type) &&
              actual->op == AST_OP(compound_literal)) {
            actual->value_category = kValueCategoryXvalue;
          }
        }
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
            TypeIsEffectivelyConst(actual->type) &&
            !TypeIsEffectivelyConst(reference_type->next);
        if (!polymorphic_special_this &&
            !TypeEqualIgnoringQualifiers(actual->type,
                                         reference_type->next)) {
          ASTNode* base_bound =
              TryBindReferenceToBaseSubobject(actual, reference_type->next);
          if (base_bound != NULL) {
            ASTNodeReplaceChild((ASTNode*)node, (int)i, base_bound, false);
            actual = base_bound;
          } else {
            NormalConversion(actual,
                             ReferenceConversionTarget(actual, reference_type));
            actual = node->children->value.p[i];
          }
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
        bool materialize_scalar_lvalue_reference =
            reference_type->declarator == kDeclReference &&
            actual->value_category == kValueCategoryPrvalue &&
            !TypeIsStructOrUnion(actual->type);
        if (ReferenceCanBind(actual, reference_type) && !HasAddress(actual) &&
            (TypeIsStructOrUnion(actual->type) ||
             materialize_scalar_lvalue_reference)) {
          ASTNode* materialized =
              MaterializeTemporary(actual, reference_type->next);
          ASTNodeReplaceChild((ASTNode*)node, (int)i, materialized, false);
          actual = materialized;
        }
        SetNeedAddress(actual);
      } else {
        if (actual->value_category != kValueCategoryPrvalue) {
          ASTNode* materialized =
              MaterializeCXXByValueClassArgument(actual, formal->type);
          if (materialized != actual) {
            ASTNodeReplaceChild((ASTNode*)node, (int)i, materialized, false);
            actual = materialized;
          }
        }
        if (TypeIsStructOrUnion(formal->type) &&
            TypeIsStructOrUnion(actual->type)) {
          actual->flags |= kASTFunctionParameterTemporary;
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
  
  TypeRecord* immediate_function_type = subtype;
  if (node->left->op == AST_OP(identifier)) {
    Symbol* resolved = ((IdentifierASTNode*)node->left)->symbol;
    if (resolved != NULL && resolved->type != NULL &&
        TypeIsFunction(resolved->type)) {
      immediate_function_type = resolved->type;
    }
  }
  if (call_ok && immediate_function_type->info.function.is_consteval &&
      !CXXInImmediateFunctionContext()) {
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
    } else if (TypeIsVoid(return_type)) {
      ConstEvalContext ctx;
      ConstEvalContextInit(&ctx);
      ok = ConstexprEvaluateCall(&ctx, (ASTNode*)node);
      ConstEvalContextDestruct(&ctx);
    }
    if (!ok) {
      if (!CXXEscalateCurrentFunction()) {
        SemanticError((ASTNode*)node,
                      "consteval function call is not a constant expression");
      }
    }
  }

  // Validate printf/scanf-style format strings on functions annotated with
  // __attribute__((format(...))).
  if (call_ok && node->left->op == AST_OP(identifier)) {
    Symbol* callee = ((IdentifierASTNode*)node->left)->symbol;
    if (callee != NULL) {
      CheckFormatCall(node, callee);
      SpecializePrintfCall(node, callee);
    }
  }

  if (call_ok && node->caller_contract_function != NULL &&
      compiler->constant_evaluation_required_depth == 0) {
    return LowerVirtualCallerContracts(node);
  }

  // Inline function call if possible.  Only possible if we are calling
  // a function (not a function pointer) and it was tagged as inline.
  if (call_ok && TypeIsFunction(node->left->type) &&
      CalleeNamesItsFunction(node->left)) {
    FunctionInfo* func = &node->left->type->info.function;
    ASTNode* parent = node->base.parent;
    // Direct `T x(args)` binds `this` to x in the call arguments.  Functional
    // casts and designated initializers construct a temporary and retarget
    // `this` during codegen; inlining would keep the temporary.
    bool constructor_needs_call =
        func->is_constructor && (parent == NULL || parent->op != AST_OP(vardecl));
    if (!TypeIsStructOrUnion(return_type) && !constructor_needs_call &&
        FunctionCanBeInlined(func)) {
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

static void AnalyzeMemberReference(BinaryASTNode* node);

static StructMember* MemberPointerMemberFromExpression(ASTNode* node) {
  if (node == NULL) {
    return NULL;
  }
  if (node->op == AST_OP(member_ptr)) {
    UnaryASTNode* unary = (UnaryASTNode*)node;
    if (unary->sub != NULL && unary->sub->op == AST_OP(structmember)) {
      return ((StructMemberASTNode*)unary->sub)->member;
    }
    return NULL;
  }
  if (node->op == AST_OP(cast)) {
    return MemberPointerMemberFromExpression(((CastASTNode*)node)->expr);
  }
  return NULL;
}

static StructMember* MemberPointerExpressionMember(ASTNode* node) {
  if (node == NULL) {
    return NULL;
  }
  if (node->op == AST_OP(member_ptr)) {
    UnaryASTNode* unary = (UnaryASTNode*)node;
    if (unary->sub != NULL && unary->sub->op == AST_OP(structmember)) {
      return ((StructMemberASTNode*)unary->sub)->member;
    }
    return NULL;
  }
  if (node->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node;
    if (id->symbol != NULL && TypeIsMemberPointer(id->symbol->type) &&
        id->symbol->flags.value_set && id->symbol->value.other != NULL) {
      return (StructMember*)id->symbol->value.other;
    }
  }
  return NULL;
}

static ASTNode* AnalyzeMemberPointerReference(BinaryASTNode* node) {
  node->left = AnalyzeExpression(node->left);
  node->right = AnalyzeExpression(node->right);
  bool right_is_nttp =
      node->right != NULL && node->right->op == AST_OP(identifier) &&
      ((IdentifierASTNode*)node->right)->symbol != NULL &&
      ((IdentifierASTNode*)node->right)->symbol->flags.is_template_parameter &&
      !((IdentifierASTNode*)node->right)->symbol->flags.is_template_type_parameter;
  if (right_is_nttp || ExpressionIsTemplateDependent(node->right) ||
      (node->right != NULL && node->right->type != NULL &&
       (TypeContainsTemplateParameter(node->right->type) ||
        TypeIsUnknown(node->right->type))) ||
      (node->left != NULL && node->left->type != NULL &&
       TypeContainsTemplateParameter(node->left->type))) {
    node->base.flags |= kASTDependentFunctorCall;
    ASTNodeSetType((ASTNode*)node,
                   NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain));
    return (ASTNode*)node;
  }
  if (node->right == NULL || !TypeIsMemberPointer(node->right->type)) {
    SemanticError((ASTNode*)node, "Right operand of .* / ->* is not a pointer to member");
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return (ASTNode*)node;
  }

  bool receiver_is_pointer = node->base.op == AST_OP(arrowstar);
  TypeRecord* receiver_type = node->left != NULL ? node->left->type : NULL;
  while (receiver_type != NULL && TypeIsReference(receiver_type)) {
    receiver_type = receiver_type->next;
  }
  if (receiver_is_pointer) {
    if (receiver_type == NULL || !TypeIsPointer(receiver_type)) {
      SemanticError((ASTNode*)node, "Left operand of ->* is not a pointer");
      ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
      return (ASTNode*)node;
    }
    receiver_type = receiver_type->next;
  }

  Struct* pm_class = TypeMemberPointerClass(node->right->type);
  if (receiver_type == NULL || pm_class == NULL ||
      !TypeIsStructOrUnion(receiver_type)) {
    SemanticError((ASTNode*)node,
                  "Pointer-to-member object expression has incompatible class type");
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return (ASTNode*)node;
  }
  TypeRecord* receiver_class_type =
      NewTypeRecord(receiver_type->info.struct_info->is_union ? kTypeUnion
                                                                : kTypeStruct,
                    kQualPlain);
  TypeRecordSetStructInfo(receiver_class_type, receiver_type->info.struct_info);
  TypeRecord* pm_class_type =
      NewTypeRecord(pm_class->is_union ? kTypeUnion : kTypeStruct, kQualPlain);
  TypeRecordSetStructInfo(pm_class_type, pm_class);
  CXXBaseAdjustment member_base_adjustment;
  bool compatible_class =
      TypeEqual(receiver_class_type, pm_class_type) ||
      TypeBaseAdjustment(receiver_class_type, pm_class_type,
                         /*public_only=*/true, &member_base_adjustment);
  if (!compatible_class) {
    String receiver_name;
    String member_class_name;
    StringInit(&receiver_name, NULL);
    StringInit(&member_class_name, NULL);
    TypeRecordToString(receiver_type, &receiver_name);
    TypeRecordToString(pm_class_type, &member_class_name);
    SemanticError((ASTNode*)node,
                  "Pointer-to-member object type '%s' is incompatible with "
                  "member class '%s'",
                  receiver_name.value, member_class_name.value);
    StringDestruct(&receiver_name);
    StringDestruct(&member_class_name);
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return (ASTNode*)node;
  }

  TypeRecord* pointee = TypeMemberPointerPointeeType(node->right->type);
  if (pointee == NULL) {
    SemanticError((ASTNode*)node, "Invalid pointer-to-member type");
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return (ASTNode*)node;
  }

  StructMember* known_member = MemberPointerExpressionMember(node->right);
  if (known_member != NULL && node->right->op != AST_OP(identifier) &&
      known_member->symbol != NULL &&
      known_member->symbol->type != NULL && !known_member->is_member_function) {
    ASTOpcode access_op =
        node->base.op == AST_OP(arrowstar) ? AST_OP(arrow) : AST_OP(dot);
    ASTNode* old_right = node->right;
    ASTNode* old_left = node->left;
    node->base.op = access_op;
    node->right = NewStructMemberASTNode(known_member, node->base.location);
    ((StructMemberASTNode*)node->right)->byte_offset = known_member->byte_offset;
    ASTNodeDelete(old_right);
    AnalyzeMemberReference(node);
    node->left = old_left;
    return (ASTNode*)node;
  }

  if (TypeIsFunction(pointee)) {
    ASTNodeSetType((ASTNode*)node, TypeRecordCopy(pointee));
    node->base.value_category = kValueCategoryLvalue;
    return (ASTNode*)node;
  }

  ASTNode* member_ptr = node->right;
  MemberPointerValue pm_value;
  if (MemberPointerTryEvaluateConstant(member_ptr, member_ptr->type, &pm_value)) {
    member_ptr = NewIntConstantASTNode(pm_value.ptr, member_ptr->type,
                                        node->base.location);
    member_ptr = AnalyzeExpression(member_ptr);
  }

  ASTNode* result = MemberPointerApplyDataAccess(&compiler->syntax, node->base.location,
                                        node->left, receiver_is_pointer,
                                        member_ptr, TypeRecordCopy(pointee));
  result->value_category = kValueCategoryLvalue;
  return result;
}

static void AnalyzePointerToMember(UnaryASTNode* node) {
  if (node->sub == NULL || node->sub->op != AST_OP(structmember)) {
    SemanticError((ASTNode*)node, "Invalid pointer-to-member expression");
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return;
  }
  StructMemberASTNode* member_node = (StructMemberASTNode*)node->sub;
  if (member_node->member == NULL || member_node->member->symbol == NULL) {
    SemanticError((ASTNode*)node, "Invalid pointer-to-member expression");
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return;
  }
  if ((member_node->base.flags & kASTNameIndependentLookupAmbiguous) != 0) {
    SemanticError((ASTNode*)node,
                  "reference to name-independent declaration '%s' is ambiguous",
                  member_node->member->symbol->name.value);
    ASTNodeSetType((ASTNode*)node,
                   NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return;
  }
  member_node->byte_offset = member_node->member->byte_offset;
  node->base.value_category = kValueCategoryPrvalue;
}

static bool LowerMemberPointerFunctionCall(VectorASTNode* node) {
  if (node->left == NULL ||
      (node->left->op != AST_OP(dotstar) &&
       node->left->op != AST_OP(arrowstar))) {
    return false;
  }
  BinaryASTNode* access = (BinaryASTNode*)node->left;
  StructMember* member = MemberPointerExpressionMember(access->right);
  if (member != NULL && member->is_member_function &&
      member->symbol != NULL && member->symbol->type != NULL) {
    ASTOpcode access_op =
        access->base.op == AST_OP(arrowstar) ? AST_OP(arrow) : AST_OP(dot);
    ASTNode* member_node = NewStructMemberASTNode(member, access->base.location);
    ((StructMemberASTNode*)member_node)->byte_offset = member->byte_offset;
    node->left = NewBinaryASTNode(access_op, NULL, access->base.location,
                                  access->left, member_node);
    return LowerMemberFunctionCall(node);
  }

  return MemberPointerLowerRuntimeFunctionCall(
      node, access->left, access->base.op == AST_OP(arrowstar), access->right,
      access->right->type);
}

// A data member named while analyzing a compiler-synthesized special member
// (implicit or `= default` copy/move constructor, assignment operator, or
// destructor) is not a real use for -Wunused-private-field: those functions
// touch every member mechanically.  Only references in user-provided code
// count, matching clang's behavior.
static bool CurrentFunctionCountsMemberUses(void) {
  TypeRecord* function = compiler->current_function;
  if (function == NULL || !TypeIsFunction(function)) {
    // A reference outside any function (e.g. a namespace-scope initializer) is
    // genuine user code.
    return true;
  }
  FunctionInfo* info = &function->info.function;
  if (info->cxx_special_member_kind != kCXXSpecialMemberNone &&
      !info->is_user_provided) {
    return false;
  }
  return true;
}

// A member reference that is the target of a constructor member-initializer
// (the synthetic `this->member = ...` assignment) does not count as a use for
// -Wunused-private-field: clang reports fields that are initialized but never
// otherwise referenced.
static bool MemberReferenceIsInitializerTarget(BinaryASTNode* node) {
  ASTNode* parent = node->base.parent;
  return parent != NULL &&
         (parent->flags & kASTCXXMemberInitializer) != 0 &&
         node->base.child_id == 0;
}

// Whether a data-member reference at `node` should mark the member as used.
static bool ShouldCountMemberReferenceUse(BinaryASTNode* node) {
  return CurrentFunctionCountsMemberUses() &&
         !MemberReferenceIsInitializerTarget(node);
}

static ASTValueCategory DataMemberAccessValueCategory(BinaryASTNode* node,
                                                       StructMember* member) {
  if (node == NULL || member == NULL || member->is_static ||
      node->base.op == AST_OP(arrow) || node->left == NULL ||
      node->left->value_category == kValueCategoryLvalue) {
    return kValueCategoryLvalue;
  }
  return kValueCategoryXvalue;
}

static void AnalyzeMemberReference(BinaryASTNode* node) {
  if (node->base.type != NULL) {
    // A cloned dependent expression can retain its resolved member and type
    // while its receiver changes from an lvalue to an xvalue. Recompute the
    // category because data-member access propagates that distinction.
    if (CompilerIsCXX() && node->right != NULL &&
        node->right->op == AST_OP(structmember)) {
      StructMember* member =
          ((StructMemberASTNode*)node->right)->member;
      if (member != NULL && !member->is_member_function &&
          member->symbol != NULL) {
        node->base.value_category =
            TypeIsReference(member->symbol->type)
                ? kValueCategoryLvalue
                : DataMemberAccessValueCategory(node, member);
      }
    }
    return;
  }
  Struct* struct_info = NULL;

  node->left = AnalyzeExpression(node->left);
  if (CompilerIsCXX() && node->base.op == AST_OP(dot) &&
      node->left != NULL && node->left->type != NULL &&
      TypeIsStructOrUnion(node->left->type) &&
      node->left->value_category == kValueCategoryPrvalue &&
      !HasAddress(node->left)) {
    node->left = MaterializeTemporary(node->left, node->left->type);
    node->left->parent = (ASTNode*)node;
    node->left->child_id = 0;
  }
  int overloaded_arrow_depth = 0;
  while (node->base.op == AST_OP(arrow) && node->left != NULL &&
         !TypeIsStructOrUnionPointer(node->left->type) &&
         overloaded_arrow_depth++ < 64) {
    ASTNode* overloaded_arrow =
        TryAnalyzeOverloadedArrowOperator(node->left, node->base.location);
    if (overloaded_arrow == NULL) {
      break;
    }
    node->left = overloaded_arrow;
    node->left->parent = (ASTNode*)node;
    node->left->child_id = 0;
  }
  // A class-template primary still carrying concrete template arguments (e.g.
  // `variant` + `<int,long>` on a lambda capture field) must be materialized
  // before member lookup, or we resolve against the primary's unrebased
  // member templates.
  if (node->left != NULL && node->left->type != NULL) {
    TypeRecord* materialized = TypeMaterializeClassTemplateSpecialization(
        &compiler->syntax, node->left->type);
    if (materialized != node->left->type) {
      ASTNodeSetType(node->left, materialized);
    }
  }
  node->right = AnalyzeExpression(node->right);
  if (node->right != NULL && node->right->op == AST_OP(structmember)) {
    StructMemberASTNode* member_node = (StructMemberASTNode*)node->right;
    StructMember* member = member_node->member;
    TypeRecord* receiver_type = node->left != NULL ? node->left->type : NULL;
    if (TypeIsReference(receiver_type)) {
      receiver_type = receiver_type->next;
    }
    if (node->base.op == AST_OP(arrow) && TypeIsPointer(receiver_type)) {
      receiver_type = receiver_type->next;
    }
    Struct* receiver_struct =
        receiver_type != NULL && TypeIsStructOrUnion(receiver_type)
            ? receiver_type->info.struct_info
            : NULL;
    bool qualified_base_lookup =
        (node->right->flags & kASTQualifiedName) != 0;
    if (qualified_base_lookup && member != NULL && member->symbol != NULL &&
        (node->right->flags & kASTNameIndependentLookupAmbiguous) != 0) {
      SemanticError((ASTNode*)node,
                    "reference to name-independent declaration '%s' is ambiguous",
                    member->symbol->name.value);
      ASTNodeSetType((ASTNode*)node,
                     NewTypeRecordWithSize(kTypeInt, kQualPlain));
      return;
    }
    if (qualified_base_lookup) {
      Struct* qualified_owner =
          member_node->owner_type != NULL &&
                  member_node->owner_type->info.struct_info != NULL
              ? member_node->owner_type->info.struct_info
              : NULL;
      if (qualified_owner == NULL && member != NULL &&
          member->symbol != NULL && member->symbol->type != NULL &&
          TypeIsFunction(member->symbol->type) &&
          member->symbol->type->info.function.cxx_member_owner != NULL) {
        qualified_owner = member->symbol->type->info.function.cxx_member_owner;
      }
      if (member != NULL && member->symbol != NULL && qualified_owner != NULL) {
        if ((node->right->flags & kASTQualifiedName) == 0) {
          node->right->flags |= kASTQualifiedName;
        }
        CXXAccess access = kAccessPublic;
        Struct* member_owner = NULL;
        int member_offset = 0;
        StructMember* resolved = FindStructMemberWithAccessAndOffsetByName(
            qualified_owner, member->symbol->name.value, &access, &member_owner,
            &member_offset);
        if (resolved != NULL) {
          member = resolved;
          StructMemberASTNodeSetMember(member_node, resolved);
        }
        if (receiver_struct != NULL) {
          ApplyVirtualBaseAdjustmentToMemberReference(node, receiver_struct,
                                                      member_owner,
                                                      &member_offset);
        }
        if (member_offset < 0) {
          member_offset =
              CXXBaseOffsetForMember(receiver_struct, member) + member->byte_offset;
        }
        member_node->access = access;
        member_node->byte_offset = member_offset;
        if (!member->is_member_function &&
            !CurrentFunctionCanAccessMember(receiver_struct, member_owner,
                                            member->access, access)) {
          const char* owner_name =
              member_owner != NULL && member_owner->tag_name != NULL
                  ? member_owner->tag_name->value
                  : "<anonymous>";
          SemanticError((ASTNode*)node, "%s is a %s member of %s",
                        member->symbol->name.value, CXXAccessName(access),
                        owner_name);
        }
        if (!member->is_member_function && member->symbol != NULL &&
            ShouldCountMemberReferenceUse(node)) {
          member->symbol->flags.used = true;
        }
        TypeRecord* member_type = member->symbol->type;
        if (!member->is_static && !member->is_member_function &&
            TypeIsReference(member_type) &&
            !MemberReferenceIsInitializerTarget(node)) {
          ASTNodeSetType((ASTNode*)node, member_type->next);
          node->base.value_category = kValueCategoryLvalue;
          return;
        }
        if (!member->is_static && !member->is_member_function &&
            !member->is_mutable && MemberReceiverIsConst(node)) {
          member_type = TypeRecordCopy(member_type);
          member_type->qualifiers |= kQualConst;
        }
        ASTNodeSetType((ASTNode*)node, member_type);
        if (!member->is_member_function) {
          node->base.value_category =
              DataMemberAccessValueCategory(node, member);
        }
        return;
      }
    }
  }
  if (node->right != NULL && node->right->op == AST_OP(structmember)) {
    StructMemberASTNode* member_node = (StructMemberASTNode*)node->right;
    StructMember* member = member_node->member;
    if (member != NULL && member->symbol != NULL) {
      // A data member named in a member-access expression counts as used for
      // -Wunused-private-field (constructor member-initializers do not reach
      // this path, matching clang's "initialized but never used" reporting).
      if (!member->is_member_function && ShouldCountMemberReferenceUse(node)) {
        member->symbol->flags.used = true;
      }
      TypeRecord* member_type = member->symbol->type;
      // A reference data member behaves as the object it is bound to: reads,
      // writes, and address-of all act on the referent.  Strip the access to the
      // referent type (mirroring the referent-type rewrite a reference
      // *variable* read receives) so downstream analysis and codegen treat it as
      // an lvalue of the referent.  The one exception is a constructor
      // member-initializer target, which binds the reference and must retain the
      // reference type so codegen writes the pointer slot, not the referent.
      // Receiver const-ness is not propagated: a reference member of a const
      // object still designates a non-const referent.
      if (!member->is_static && !member->is_member_function &&
          TypeIsReference(member_type) &&
          !MemberReferenceIsInitializerTarget(node)) {
        ASTNodeSetType((ASTNode*)node, member_type->next);
        node->base.value_category = kValueCategoryLvalue;
        return;
      }
      if (!member->is_static && !member->is_member_function &&
          !member->is_mutable && MemberReceiverIsConst(node)) {
        member_type = TypeRecordCopy(member_type);
        member_type->qualifiers |= kQualConst;
      }
      ASTNodeSetType((ASTNode*)node, member_type);
      if (!member->is_member_function) {
        node->base.value_category =
            DataMemberAccessValueCategory(node, member);
      }
      return;
    }
  }
  TypeRecord* receiver_type = node->left != NULL ? node->left->type : NULL;
  if (TypeIsReference(receiver_type)) {
    receiver_type = receiver_type->next;
  }
  if (CompilerIsCXX() && receiver_type != NULL &&
      TypeContainsTemplateParameter(receiver_type)) {
    TypeRecord* placeholder = TypeRecordCopy(receiver_type);
    placeholder->type |= kTypeUnknown;
    ASTNodeSetType((ASTNode*)node, placeholder);
    return;
  }
  if (CompilerIsCXX() && receiver_type != NULL && TypeIsUnknown(receiver_type)) {
    TypeRecord* placeholder = TypeRecordCopy(receiver_type);
    ASTNodeSetType((ASTNode*)node, placeholder);
    return;
  }
  if (CompilerIsCXX() && node->base.op == AST_OP(arrow) &&
      TypeIsPointer(receiver_type) &&
      !TypeIsStructOrUnion(receiver_type->next) && node->right != NULL &&
      node->right->op == AST_OP(string)) {
    String* member_name = ((ConstantASTNode*)node->right)->value.string;
    if (member_name != NULL && member_name->length != 0 &&
        member_name->value[0] == '~') {
      // Leave a scalar pseudo-destructor member access deferred to its
      // enclosing call.  AnalyzeFunctionCall will replace the complete call
      // with the required object-evaluating void expression.
      TypeRecord* placeholder =
          NewTypeRecordWithSize(kTypeUnknown, kQualPlain);
      ASTNodeSetType((ASTNode*)node, placeholder);
      return;
    }
  }
  if (node->base.op == AST_OP(arrow)) {
    // Op is ->, needs to be a pointer to a struct/union.
    if (!TypeIsStructOrUnionPointer(receiver_type)) {
      SemanticError((ASTNode*)node,
                    "Left of -> is not a pointer to a "
                    "struct/union; did you mean to use '.'");
    } else {
      // Dereference the pointer to get the struct info.
      struct_info = receiver_type->next->info.struct_info;
    }
  } else if (!TypeIsStructOrUnion(receiver_type)) {
    if (TypeIsStructOrUnionPointer(receiver_type)) {
      SemanticError((ASTNode*)node,
                    "Left of '.' is a pointer; did you mean to use ->?");
    } else {
      SemanticError((ASTNode*)node, "Left of '.' is not a struct/union");
    }
  } else {
    // Node is AST_OP(dot) and left is a struct/union.
    struct_info = receiver_type->info.struct_info;
  }

  if (struct_info == NULL) {
    // Error case, assign type as integer.
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return;
  }

  if (TypeIsStructOrUnion(receiver_type)) {
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
      FindStructMemberWithAccessAndOffsetByName(
          struct_info, member_name->value, &access, &member_owner,
          &member_offset);
  if (member == NULL && CompilerIsCXX() && struct_info->tag_name != NULL) {
    TypeRecord* class_type = receiver_type;
    if (node->base.op == AST_OP(arrow) && TypeIsPointer(class_type)) {
      class_type = class_type->next;
    }
    const char* origin_name =
        class_type != NULL && class_type->template_origin != NULL
            ? class_type->template_origin->name.value
            : NULL;
    size_t origin_length = origin_name != NULL ? strlen(origin_name) : 0;
    const char* source_name = member_name->value;
    bool names_injected_constructor =
        origin_name != NULL &&
        strncmp(source_name, origin_name, origin_length) == 0 &&
        (source_name[origin_length] == '\0' ||
         source_name[origin_length] == '<' ||
         source_name[origin_length] == '#');
    if (names_injected_constructor &&
        !StringEqual(member_name, struct_info->tag_name->value)) {
      member = FindStructMemberWithAccessAndOffsetByName(
          struct_info, struct_info->tag_name->value, &access, &member_owner,
          &member_offset);
      if (member != NULL && member->symbol != NULL &&
          member->symbol->type != NULL &&
          TypeIsFunction(member->symbol->type) &&
          member->symbol->type->info.function.is_constructor) {
        StringSet(member_name, struct_info->tag_name->value);
      } else {
        member = NULL;
      }
    }
  }
  if (member == NULL && CompilerIsCXX() && member_name->length > 1 &&
      member_name->value[0] == '~') {
    String* base_tag = CXXFindBaseTagNameForDestructorSpelling(
        struct_info, member_name->value + 1);
    if (base_tag != NULL) {
      String concrete_name;
      StringInit(&concrete_name, "~");
      StringAppendString(&concrete_name, base_tag);
      StringSet(member_name, concrete_name.value);
      StringDestruct(&concrete_name);
      member = FindStructMemberWithAccessAndOffsetByName(
          struct_info, member_name->value, &access, &member_owner,
          &member_offset);
    }
  }
  if (member == NULL) {
    const char* suggestion =
        TypoCorrectionFindMemberName(struct_info, member_name->value);
    const char* tag =
        struct_info->tag_name != NULL ? struct_info->tag_name->value : "";
    if (suggestion != NULL) {
      SemanticError((ASTNode*)node,
                    "%s is not a member of struct/union %s; did you mean \"%s\"?",
                    member_name->value, tag, suggestion);
    } else {
      SemanticError((ASTNode*)node, "%s is not a member of struct/union %s",
                    member_name->value, tag);
    }
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return;
  }
  if (member->symbol != NULL &&
      (node->right->flags & kASTNameIndependentLookupAmbiguous) != 0) {
    SemanticError((ASTNode*)node,
                  "reference to name-independent declaration '%s' is ambiguous",
                  member->symbol->name.value);
    ASTNodeSetType((ASTNode*)node,
                   NewTypeRecordWithSize(kTypeInt, kQualPlain));
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
  StructMemberASTNode* member_node = (StructMemberASTNode*)node->right;
  member_node->access = access;
  member_node->byte_offset = member_offset;
  member_node->template_arguments =
      TemplateArgumentVectorCopy(explicit_template_arguments);
  member_node->owner_type =
      member->symbol != NULL && member->symbol->type != NULL &&
              TypeIsFunction(member->symbol->type) &&
              member->symbol->type->info.function.cxx_member_owner != NULL &&
              member->symbol->type->info.function.cxx_member_owner->tag_symbol !=
                  NULL
          ? member->symbol->type->info.function.cxx_member_owner->tag_symbol->type
          : receiver_type;
  TypeRecordIncRef(member_node->owner_type);
  ASTNodeDelete(old_right);
  // A data member named in a member-access expression counts as used for
  // -Wunused-private-field.
  if (!member->is_member_function && member->symbol != NULL &&
      ShouldCountMemberReferenceUse(node)) {
    member->symbol->flags.used = true;
  }
  TypeRecord* member_type = member->symbol->type;
  // A reference data member behaves as its referent (see the pre-resolved
  // member path above): strip the access to the referent type for every use
  // except a constructor member-initializer target, which binds the reference.
  if (!member->is_static && !member->is_member_function &&
      TypeIsReference(member_type) &&
      !MemberReferenceIsInitializerTarget(node)) {
    ASTNodeSetType((ASTNode*)node, member_type->next);
    node->base.value_category = kValueCategoryLvalue;
    return;
  }
  if (!member->is_static && !member->is_member_function && !member->is_mutable &&
      MemberReceiverIsConst(node)) {
    member_type = TypeRecordCopy(member_type);
    member_type->qualifiers |= kQualConst;
  }
  ASTNodeSetType((ASTNode*)node, member_type);
  if (!member->is_member_function) {
    node->base.value_category = DataMemberAccessValueCategory(node, member);
  }
}

// Address-of operator.  If the operand has an address the type is
// a pointer to the type of the operand.
static void AnalyzeAddressOperator(UnaryASTNode* node) {
  node->sub = AnalyzeExpression(node->sub);
  node->base.value_category = kValueCategoryPrvalue;
  // Forming a pointer to a function bakes its signature into the result type,
  // so a placeholder return type has to be resolved first.  This is how a
  // closure's `&C::operator()` reaches deduction guides such as std::function's.
  if (node->sub != NULL) {
    SemanticEnsureAutoReturnTypeDeduced(node->sub->type);
  }
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
  if (CompilerIsCXX() && node->sub->op == AST_OP(identifier) &&
      TypeIsFunction(node->sub->type)) {
    TypeEnsureTemplateMemberFunctionDefinition(
        &compiler->syntax, ((IdentifierASTNode*)node->sub)->symbol);
    IdentifierASTNode* identifier = (IdentifierASTNode*)node->sub;
    if (CompilerCXXAtLeast(kLanguageStandardCXX20) &&
        identifier->symbol != NULL && identifier->symbol->type != NULL &&
        TypeIsFunction(identifier->symbol->type) &&
        identifier->symbol->type->info.function.is_consteval &&
        !CXXInImmediateFunctionContext() &&
        (!CompilerCXXAtLeast(kLanguageStandardCXX23) ||
         !CXXEscalateCurrentFunction())) {
      SemanticError(node->sub,
                    "immediate function may only be named in an immediate "
                    "function context");
    }
  }
}

// Contents-of operator.  If the operand is a pointer the result type
// is the type pointed to.
static void AnalyzeContentsOperator(UnaryASTNode* node) {
  node->sub = AnalyzeExpression(node->sub);
  // A dependent operand (a template parameter or otherwise unknown type, such
  // as `*declval<T&>()` inside a decltype) has no known pointee yet.  Its
  // built-in or overloaded meaning is resolved after substitution, so leave the
  // result dependent rather than diagnosing it here.
  if (CompilerIsCXX() && node->sub->type != NULL &&
      (TypeIsUnknown(node->sub->type) ||
       ((((ASTNode*)node)->flags & kASTDeferredRangeContents) != 0 &&
        TypeContainsAuto(node->sub->type)) ||
       TypeContainsTemplateParameter(node->sub->type))) {
    ASTNodeSetType((ASTNode*)node,
                   NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain));
    node->base.value_category = kValueCategoryLvalue;
    return;
  }
  if (!TypeIsPointerOrArray(node->sub->type)) {
    SemanticError(node->sub, "Cannot take contents of this expression");
    // Fake an integer type for the result.
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return;
  }

  TypeRecord* pointee = node->sub->type->next;
  TypeRecord* materialized =
      TypeMaterializeClassTemplateSpecialization(&compiler->syntax, pointee);
  if (materialized != pointee) {
    // Keep the pointer spine consistent with the materialized pointee so
    // subsequent member access sees the concrete specialization.
    TypeRecord* ptr = TypeRecordCopy(node->sub->type);
    TypeRecordIncRef(materialized);
    TypeRecordDelete(ptr->next);
    ptr->next = materialized;
    ptr->type = materialized->type;
    ASTNodeSetType(node->sub, TypeRecordCalculateSize(ptr));
    pointee = materialized;
  }
  ASTNodeSetType((ASTNode*)node, pointee);
  node->base.value_category = kValueCategoryLvalue;
}

static TypeRecord* SizeofOperandType(TypeRecord* type) {
  // Per [expr.sizeof] and [expr.alignof], applying either operator to a
  // reference measures the referenced type, not the compiler's pointer-sized
  // representation of the reference.
  return CompilerIsCXX() && TypeIsReference(type) ? type->next : type;
}

static void AnalyzePackIndexExpression(BinaryASTNode* node) {
  bool indexes_template_name =
      node->left != NULL && node->left->op == AST_OP(identifier) &&
      ((IdentifierASTNode*)node->left)->symbol != NULL &&
      ((IdentifierASTNode*)node->left)
          ->symbol->flags.is_template_template_parameter;
  if (!indexes_template_name) {
    node->left = AnalyzeExpression(node->left);
  }
  node->right = AnalyzeExpression(node->right);

  if (node->left == NULL || node->left->op != AST_OP(identifier) ||
      ((IdentifierASTNode*)node->left)->symbol == NULL ||
      !((IdentifierASTNode*)node->left)->symbol->flags.is_parameter_pack) {
    SemanticError((ASTNode*)node,
                  "pack indexing requires an unexpanded parameter pack name");
  }

  int64_t index = 0;
  if (!ExpressionIsTemplateDependent(node->right)) {
    if (!EvaluateIntegerExpression(node->right, &index)) {
      SemanticError(node->right,
                    "pack index must be a constant expression");
    } else if (index < 0) {
      SemanticError(node->right, "pack index cannot be negative");
    }
  }

  if (node->left != NULL && node->left->type != NULL) {
    ASTNodeSetType((ASTNode*)node, node->left->type);
    node->base.value_category = node->left->value_category;
  }
}

static void AnalyzeSizeofExpression(SizeofASTNode* node) {
  bool is_alignof = node->base.base.op == AST_OP(alignof);
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
    if (node->expr != NULL && node->expr->op == AST_OP(identifier)) {
      Symbol* pack = ((IdentifierASTNode*)node->expr)->symbol;
      if (pack != NULL && pack->structured_binding_pack_size >= 0) {
        node->base.value.ivalue = pack->structured_binding_pack_size;
      }
    }
    ASTNodeSetType((ASTNode*)node, NewSizeTypeRecord());
    return;
  }
  if (node->expr != NULL) {
    node->expr = AnalyzeExpression(node->expr);
    TypeRecord* operand_type = SizeofOperandType(node->expr->type);
    if (TypeIsVLA(operand_type)) {
      // sizeof(vla) is calculated at runtime.
    } else {
      node->base.value.ivalue =
          is_alignof ? TypeRecordAlignment(operand_type) : operand_type->size;
    }
  } else if (node->type_operand != NULL &&
             !TypeContainsTemplateParameter(node->type_operand)) {
    // A `sizeof(type-id)` / `alignof(type-id)` whose operand has become
    // concrete (e.g. after template instantiation): re-measure now.
    TypeRecord* operand_type = SizeofOperandType(node->type_operand);
    TypeRecordCalculateSize(operand_type);
    node->base.value.ivalue =
        is_alignof ? TypeRecordAlignment(operand_type) : operand_type->size;
  }
  ASTNodeSetType((ASTNode*)node, NewSizeTypeRecord());
}

// Finds the std::<name> class type, or NULL if it is not declared (e.g. the
// relevant standard header has not been included).
static TypeRecord* FindStdClassType(const char* name) {
  Namespace* std_ns = NamespaceFindStdNamespace();
  if (std_ns == NULL) {
    return NULL;
  }
  String class_name;
  StringInit(&class_name, name);
  NamespaceInlineSymbolLookup result =
      NamespaceResolveSymbolInInlineSet(std_ns, &class_name);
  StringDestruct(&class_name);
  if (result.status != kInlineLookupUnique || result.symbol == NULL ||
      result.symbol->type == NULL || !TypeIsStructOrUnion(result.symbol->type)) {
    return NULL;
  }
  return result.symbol->type;
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

  TypeRecord* type_info_ptr =
      NewPointerTo(kQualPlain, TypeRecordCopy(type_info_type));
  type_info_ptr->next->qualifiers |= kQualConst;

  ASTNode* result;
  if (polymorphic) {
    // The Itanium address point is the first virtual function entry, with the
    // type_info pointer exactly one pointer-width before it. Do the byte
    // adjustment explicitly: applying the ordinary subscript lowering to the
    // compiler-invented void** member can inherit the vtable aggregate stride.
    ASTNode* address =
        NewUnaryASTNode(AST_OP(address), NULL, location, operand);
    ASTNode* vptr = NewBinaryASTNode(
        AST_OP(arrow), NULL, location, address,
        NewStringConstantASTNode(NewString("__vptr"), NULL, location));
    TypeRecord* byte_ptr = NewPointerTo(
        kQualPlain, NewTypeRecordWithSize(kTypeChar, kQualPlain));
    ASTNode* vptr_bytes = NewCastASTNode(byte_ptr, location, vptr);
    ASTNode* slot_address = NewBinaryASTNode(
        AST_OP(minus), NULL, location, vptr_bytes,
        NewIntConstantASTNode(SizeofPointer(),
                              NewTypeRecordWithSize(kTypeInt, kQualPlain),
                              location));
    TypeRecord* type_info_ptr_ptr =
        NewPointerTo(kQualPlain, TypeRecordCopy(type_info_ptr));
    ASTNode* slot = NewUnaryASTNode(
        AST_OP(contents), NULL, location,
        NewCastASTNode(type_info_ptr_ptr, location, slot_address));
    result = NewUnaryASTNode(AST_OP(contents), NULL, location, slot);
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
    // The RTTI symbol is emitted as raw storage, but expressions observe it as
    // a const type_info lvalue. Keep the storage symbol's byte-array type
    // private to the emitter instead of round-tripping through a synthetic
    // cast whose pointee type can be discarded with the replaced typeid node.
    result = NewIdentifierASTNode(type_info_symbol, location);
    ASTNodeSetType(result, type_info_ptr->next);
    result->value_category = kValueCategoryLvalue;
    result->flags |= kASTAnalyzed;
    TypeRecordDelete(type_info_ptr);
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
    case kDeclVector:
      return left->info.array.size.fixed == right->info.array.size.fixed &&
             TypeEqualIgnoringQualifiers(left->next, right->next);
    case kDeclMemberPointer:
      return TypeMemberPointerClass(left) == TypeMemberPointerClass(right) &&
             TypeEqualIgnoringQualifiers(
                 TypeMemberPointerPointeeType(left),
                 TypeMemberPointerPointeeType(right));
    case kDeclFunction:
      return TypeEqual(left, right);
    case kDeclPrimitive:
      if (TypeIsStructOrUnion(left) || TypeIsStructOrUnion(right)) {
        if (!TypeIsStructOrUnion(left) || !TypeIsStructOrUnion(right)) {
          return false;
        }
        TypeRecord unqualified_left = *left;
        TypeRecord unqualified_right = *right;
        unqualified_left.qualifiers = kQualPlain;
        unqualified_right.qualifiers = kQualPlain;
        return TypeEqual(&unqualified_left, &unqualified_right);
      }
      if (TypeIsEnum(left) || TypeIsEnum(right)) {
        return TypeIsEnum(left) && TypeIsEnum(right) &&
               left->info.enum_info == right->info.enum_info;
      }
      return left->type == right->type;
  }
  return false;
}

static void ValidateCXXConstCast(CastASTNode* node) {
  TypeRecord* to = node->cast_type;
  TypeRecord* from = node->expr->type;
  bool valid = false;
  if (TypeIsPointer(to) && TypeIsPointer(from) &&
      to->next != NULL && from->next != NULL &&
      !TypeIsFunction(to->next) && !TypeIsFunction(from->next)) {
    valid = TypeEqualIgnoringQualifiers(to->next, from->next);
  } else if (TypeIsReference(to) && to->next != NULL &&
             !TypeIsFunction(to->next)) {
    TypeRecord* from_object = TypeIsReference(from) ? from->next : from;
    bool compatible_value_category =
        to->declarator == kDeclRValueReference
            ? node->expr->value_category == kValueCategoryXvalue
            : node->expr->value_category == kValueCategoryLvalue;
    valid = compatible_value_category && from_object != NULL &&
            TypeEqualIgnoringQualifiers(to->next, from_object);
  }
  if (!valid) {
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

// A cast to a reference of a base or derived class (e.g. the CRTP downcast
// `static_cast<Derived&>(*this)`) is a reference conversion, not a value
// conversion.  Rewrite it as `*static_cast<Target*>(&expr)` so the established
// pointer up/down-cast path (which applies any base-class offset adjustment)
// handles it, leaving an lvalue of the target class type.  Returns true when the
// rewrite was applied (i.e. the classes are base/derived related).
static bool TryCastReferenceRelatedClass(CastASTNode* node) {
  if (!CompilerIsCXX() || node->expr == NULL || node->expr->type == NULL) {
    return false;
  }
  TypeRecord* target = node->cast_type->next;
  if (target == NULL || !TypeIsStructOrUnion(node->expr->type) ||
      !TypeIsStructOrUnion(target)) {
    return false;
  }
  CXXBaseAdjustment adjustment;
  bool related =
      TypeBaseAdjustment(node->expr->type, target, /*public_only=*/false,
                         &adjustment) ||
      TypeBaseAdjustment(target, node->expr->type, /*public_only=*/false,
                         &adjustment);
  if (!related) {
    return false;
  }
  SourceLocation loc = node->expr->location;
  ASTNode* addr = NewAnalyzedBuiltinAddressOf(node->expr, loc);
  TypeRecord* target_ptr = NewPointerTo(kQualPlain, TypeRecordCopy(target));
  ASTNode* ptr_cast = NewCastASTNode(target_ptr, loc, addr);
  ((CastASTNode*)ptr_cast)->kind = node->kind;
  ptr_cast = AnalyzeExpression(ptr_cast);
  ASTNode* deref = NewUnaryASTNode(AST_OP(contents), NULL, loc, ptr_cast);
  deref = AnalyzeExpression(deref);
  node->expr = deref;
  deref->parent = (ASTNode*)node;
  return true;
}

// Bind a class lvalue/xvalue `expr` to a reference whose referent type is a
// (possibly virtual, possibly offset) base class of `expr`'s class.  The base
// subobject may live at a non-zero offset -- and for a virtual base, only
// reachable through the run-time __vbptr -- so the reference must point at the
// adjusted subobject address, not the raw derived-object address.  Rewrites
// `expr` to `*static_cast<Base*>(&expr)`, reusing the pointer up-cast path that
// already emits the correct static/virtual adjustment.  Returns the new lvalue
// node (detached from any parent) when the rewrite applies, or NULL when `expr`
// is not a base-class subobject reference (leaving `expr` untouched).
static ASTNode* TryBindReferenceToBaseSubobject(ASTNode* expr,
                                                TypeRecord* referent) {
  if (!CompilerIsCXX() || expr == NULL || expr->type == NULL ||
      referent == NULL || !TypeIsStructOrUnion(expr->type) ||
      !TypeIsStructOrUnion(referent) ||
      TypeEqualIgnoringQualifiers(expr->type, referent)) {
    return NULL;
  }
  // Only meaningful for an object that already has an address; a materialized
  // temporary (no address) is handled separately by the caller.
  if (!HasAddress(expr)) {
    return NULL;
  }
  CXXBaseAdjustment adjustment;
  if (!TypeBaseAdjustment(expr->type, referent, /*public_only=*/true,
                          &adjustment)) {
    return NULL;
  }
  SourceLocation loc = expr->location;
  ASTNode* moved = ASTNodeMove(expr);
  ASTNode* addr = NewAnalyzedBuiltinAddressOf(moved, loc);
  TypeRecord* base_ptr = NewPointerTo(kQualPlain, TypeRecordCopy(referent));
  ASTNode* ptr_cast = NewCastASTNode(base_ptr, loc, addr);
  ((CastASTNode*)ptr_cast)->kind = kCastStatic;
  ptr_cast = AnalyzeExpression(ptr_cast);
  ASTNode* deref = NewUnaryASTNode(AST_OP(contents), NULL, loc, ptr_cast);
  return AnalyzeExpression(deref);
}

static ASTNode* AnalyzeCastExpression(CastASTNode* node) {
  node->expr = AnalyzeExpression(node->expr);
  bool auto_paren = node->kind == kCastAutoParen;
  bool auto_brace = node->kind == kCastAutoBrace;
  if (auto_paren || auto_brace) {
    TypeRecord* deduced =
        TypeDecayForByValueDeduction(node->expr != NULL ? node->expr->type
                                                        : NULL);
    if (deduced == NULL) {
      SemanticError((ASTNode*)node,
                    "Cannot deduce the target type of an auto cast");
      deduced = NewTypeRecordWithSize(kTypeInt, kQualPlain);
    }
    TypeRecordDelete(node->cast_type);
    node->cast_type = deduced;
    TypeRecordIncRef(node->cast_type);
    // Direct-list-initialization from a prvalue of the deduced type uses that
    // prvalue as the result object; it must not synthesize an extra move.
    if (auto_brace && !TypeIsVoid(node->cast_type) && node->expr != NULL &&
        node->expr->value_category == kValueCategoryPrvalue &&
        TypeEqualIgnoringQualifiers(node->expr->type, node->cast_type)) {
      node->kind = kCastAutoParen;
      auto_paren = true;
      auto_brace = false;
    }
  }
  if (node->kind == kCastDynamic) {
    AnalyzeDynamicCast(node);
    return (ASTNode*)node;
  }
  if (node->kind == kCastConst) {
    ValidateCXXConstCast(node);
  }
  // A cast whose target or operand type still mentions an unbound template
  // parameter is dependent: during the first (class-level) instantiation of a
  // member function template, the enclosing class parameters are concrete but
  // the member template's own parameters are not, so a cast such as a
  // constructor mem-initializer's `static_cast<T&&>(t)` cannot be checked yet.
  // Forcing a conversion now would either bake in the wrong one or, when a
  // dependent parameter is treated as an incomplete class, spuriously reject a
  // struct-to-struct cast.  Leave the operand unconverted and take the target
  // type; the member template's second-stage (per-call) instantiation re-clones
  // and re-analyzes the cast against the concrete argument type.
  if (CompilerIsCXX() &&
      (TypeContainsTemplateParameter(node->cast_type) ||
       (node->expr != NULL && node->expr->type != NULL &&
        TypeContainsTemplateParameter(node->expr->type)))) {
    if (TypeIsReference(node->cast_type)) {
      ASTNodeSetType((ASTNode*)node, node->cast_type->next);
      node->base.value_category =
          node->cast_type->declarator == kDeclRValueReference
              ? kValueCategoryXvalue
              : kValueCategoryLvalue;
    } else {
      ASTNodeSetType((ASTNode*)node, node->cast_type);
      node->base.value_category = kValueCategoryPrvalue;
    }
    return (ASTNode*)node;
  }
  if (auto_brace && TypeIsVoid(node->cast_type)) {
    SemanticError((ASTNode*)node,
                  "a braced auto cast cannot initialize void");
    ASTNodeSetType((ASTNode*)node, node->cast_type);
    node->base.value_category = kValueCategoryPrvalue;
    return (ASTNode*)node;
  }
  if (auto_brace) {
    Vector* elements = NewVector();
    VectorAppend(elements, ASTNodeMove(node->expr));
    ASTNode* braced =
        NewBracedInitializerASTNode(elements, NULL, node->base.location);
    ASTNode* lowered =
        LowerCXXBracedInitToTarget(braced, node->cast_type);
    ASTNode* parent = node->base.parent;
    if (parent != NULL) {
      ASTNodeReplaceChild(parent, node->base.child_id, lowered, true);
    } else {
      ASTNodeDelete((ASTNode*)node);
    }
    return lowered;
  }
  if (TypeIsReference(node->cast_type)) {
    if (!TypeEqualIgnoringQualifiers(node->expr->type, node->cast_type->next)) {
      if (!TryCastReferenceRelatedClass(node)) {
        SemanticConvertType(node->expr, node->cast_type->next, kConvertCast);
      }
    }
    ASTNodeSetType((ASTNode*)node, node->cast_type->next);
    node->base.value_category =
        node->cast_type->declarator == kDeclRValueReference
            ? kValueCategoryXvalue
            : kValueCategoryLvalue;
  } else {
    bool same_class =
        CompilerIsCXX() && TypeIsStructOrUnion(node->cast_type) &&
        TypeEqualIgnoringQualifiers(node->expr->type, node->cast_type);
    bool identity_same_class_prvalue =
        auto_paren && same_class &&
        node->expr->value_category == kValueCategoryPrvalue;
    bool materialized_same_class =
        identity_same_class_prvalue ||
        (same_class &&
         TryConvertWithConvertingConstructorImpl(
             node->expr, node->cast_type, kConvertCast,
             /*allow_same_class=*/true));
    if (auto_paren && same_class && !materialized_same_class) {
      SemanticError((ASTNode*)node,
                    "auto cast cannot construct its result from the operand");
    }
    if (!materialized_same_class) {
      SemanticConvertType(node->expr, node->cast_type, kConvertCast);
    }
    // Result is the requested type.
    ASTNodeSetType((ASTNode*)node, node->cast_type);
    node->base.value_category = kValueCategoryPrvalue;
  }
  return (ASTNode*)node;
}

static void AnalyzeCompoundLiteral(CompoundLiteralASTNode* node) {
  node->initializer =
      AnalyzeInitialization(&node->base, node->sym,
                            node->initializer);
  // A C++ lambda-expression is a prvalue that materializes a temporary closure.
  // Keep that category so forwarding-reference deduction of `F&&` / `T&&` works.
  if ((node->base.flags &
       (kASTLambdaExpression | kASTCXXBracedTemporary)) != 0) {
    node->base.value_category = kValueCategoryPrvalue;
  } else {
    node->base.value_category = kValueCategoryLvalue;
  }
}

static void AnalyzeLogicalOperator(BinaryASTNode* node) {
  TypeRecord* bool_type = NewTypeRecordWithSize(kTypeBool, kQualPlain);
  
  node->left = AnalyzeExpression(node->left);
  node->right = AnalyzeExpression(node->right);
  if (ExpressionIsTemplateDependent(node->left) ||
      ExpressionIsTemplateDependent(node->right)) {
    ASTNodeSetType((ASTNode*)node, bool_type);
    return;
  }
  SemanticConvertType(node->left, bool_type, kConvertContextualBool);
  SemanticConvertType(node->right, bool_type, kConvertContextualBool);
  ASTNodeSetType((ASTNode*)node, node->left->type);
  SemanticCheckScalarType(node->left);
  SemanticCheckScalarType(node->right);

  ASTNodeSetType((ASTNode*)node, NewLogicalResultType());
}

static void EnsureThrowCopyConstructor(ThrowASTNode* node) {
  if (node == NULL || node->expr == NULL ||
      !TypeIsStructOrUnion(node->expr->type) ||
      node->expr->value_category == kValueCategoryPrvalue) {
    return;
  }
  StructMember* constructor = FindConvertingConstructorCandidate(
      node->expr->type, node->expr, false, true);
  Symbol* symbol = constructor != NULL ? constructor->symbol : NULL;
  if (symbol == NULL || symbol->type == NULL ||
      !symbol->type->info.function.is_defaulted ||
      symbol->type->info.function.body != NULL) {
    return;
  }
  TypeParser parser;
  TypeParserInit(&parser, compiler->syntax.lex, &compiler->syntax,
                 STO(implicit), compiler->syntax.context);
  SynthesizeDefaultedMemberFunctionBody(&parser, symbol);
  TypeParserDestruct(&parser);
}

static bool IsCXX23MoveEligibleThrowOperand(ASTNode* expression) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX23) || expression == NULL ||
      expression->op != AST_OP(identifier)) {
    return false;
  }
  Symbol* sym = ((IdentifierASTNode*)expression)->symbol;
  if (sym == NULL || (!sym->flags.is_local && !sym->flags.is_argument) ||
      sym->flags.is_temp || StorageIs(sym->storage, STO(static))) {
    return false;
  }
  TypeRecord* object_type = sym->type;
  if (object_type == NULL) {
    return false;
  }
  if (TypeIsReference(object_type)) {
    if (object_type->declarator != kDeclRValueReference) {
      return false;
    }
    object_type = object_type->next;
  }
  return object_type != NULL && !TypeIsConst(object_type) &&
         !TypeIsVolatile(object_type);
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
    if (IsCXX23MoveEligibleThrowOperand(node->expr)) {
      node->expr->value_category = kValueCategoryXvalue;
    }
    if (TypeIsVoid(node->expr->type) || TypeIsFunction(node->expr->type)) {
      SemanticError(node->expr, "Cannot throw expression of this type");
    }
    EnsureThrowCopyConstructor(node);
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
    if (CompilerIsCXX() && TypeIsFunction(node->type)) {
      TypeEnsureTemplateMemberFunctionDefinition(&compiler->syntax,
                                                 idnode->symbol);
    }
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
  if (CompilerTargetSupportsAtomics() &&
      !CompilerTargetSupportsAtomicSize(ptr->type->next->size)) {
    SemanticError((ASTNode*)node,
                  "atomic object size is not supported on this target");
  }
  if (!TypeIsIntegral(ptr->type->next) &&
      !TypeIsFloatingPoint(ptr->type->next) &&
      !TypeIsPointer(ptr->type->next)) {
    SemanticError((ASTNode*)node,
                  "atomic builtin requires scalar or pointer object type");
  }
  return ptr->type->next;
}

static TypeRecord* AtomicOperationValueType(TypeRecord* object_type) {
  TypeRecord* value_type = TypeRecordCopy(object_type);
  if (value_type != NULL) {
    value_type->qualifiers &=
        ~(kQualAtomic | kQualConst | kQualVolatile | kQualRestrict);
  }
  return value_type;
}

static void ValidateAtomicTarget(VectorASTNode* node) {
  if (!CompilerTargetSupportsAtomics()) {
    if (compiler != NULL && compiler->target_name != NULL &&
        (StringEqual(compiler->target_name, "6502") ||
         StringEqual(compiler->target_name, "65c02"))) {
      SemanticError((ASTNode*)node,
                    "atomic operations are unavailable in the single-threaded "
                    "65(C)02 profile");
    } else {
      SemanticError((ASTNode*)node,
                    "atomic operations are unavailable on this target");
    }
  }
}

static void AnalyzeAtomicBuiltinChildren(VectorASTNode* node) {
  ValidateAtomicTarget(node);
  for (size_t i = 0; i < node->children->length; i++) {
    node->children->value.p[i] =
        AnalyzeExpression((ASTNode*)node->children->value.p[i]);
  }
}

typedef enum {
  kAtomicOrderLoad,
  kAtomicOrderStore,
  kAtomicOrderReadModifyWrite,
  kAtomicOrderFailure,
  kAtomicOrderFence,
} AtomicOrderUse;

static int ValidateAtomicMemoryOrder(ASTNode* order, AtomicOrderUse use) {
  int64_t value = -1;
  if (order == NULL || !TypeIsIntegral(order->type)) {
    SemanticError(order, "atomic memory order must have integral type");
    return 5;
  }
  // GCC permits a runtime memory-model expression and conservatively maps it
  // to sequential consistency.  Validate all constants precisely; dynamic
  // values retain that documented seq_cst fallback in target lowering.
  if (!EvaluateIntegerExpression(order, &value)) {
    return 5;
  }
  if (value < 0 || value > 5) {
    SemanticError(order, "atomic memory order must be a constant from 0 to 5");
    return 5;
  }
  if (use == kAtomicOrderLoad && (value == 3 || value == 4)) {
    SemanticError(order, "atomic load cannot use release or acq_rel order");
  } else if (use == kAtomicOrderStore &&
             (value == 1 || value == 2 || value == 4)) {
    SemanticError(order,
                  "atomic store cannot use consume, acquire, or acq_rel order");
  } else if (use == kAtomicOrderFailure && (value == 3 || value == 4)) {
    SemanticError(order,
                  "atomic compare-exchange failure order cannot be release or acq_rel");
  }
  return (int)value;
}

static bool AtomicFailureOrderAllowed(int success, int failure) {
  if (failure == 0) {
    return true;
  }
  if (failure == 1) {
    return success == 1 || success == 2 || success == 4 || success == 5;
  }
  if (failure == 2) {
    return success == 2 || success == 4 || success == 5;
  }
  return failure == 5 && success == 5;
}

static void AnalyzeAtomicLoadBuiltin(VectorASTNode* node) {
  AnalyzeAtomicBuiltinChildren(node);
  TypeRecord* value_type =
      AtomicOperationValueType(AtomicPointerPointee(node));
  if (node->children->length > 1) {
    ValidateAtomicMemoryOrder(node->children->value.p[1], kAtomicOrderLoad);
  }
  ASTNodeSetType(&node->base, value_type);
}

static void AnalyzeAtomicStoreBuiltin(VectorASTNode* node) {
  AnalyzeAtomicBuiltinChildren(node);
  TypeRecord* value_type =
      AtomicOperationValueType(AtomicPointerPointee(node));
  if (node->children->length > 1) {
    NormalConversion(node->children->value.p[1], value_type);
  }
  if (node->children->length > 2) {
    ValidateAtomicMemoryOrder(node->children->value.p[2], kAtomicOrderStore);
  }
  ASTNodeSetType(&node->base, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
  TypeRecordDelete(value_type);
}

static void AnalyzeAtomicFetchBuiltin(VectorASTNode* node, bool returns_new) {
  (void)returns_new;
  AnalyzeAtomicBuiltinChildren(node);
  TypeRecord* value_type =
      AtomicOperationValueType(AtomicPointerPointee(node));
  if (!TypeIsIntegral(value_type) && !TypeIsPointer(value_type)) {
    SemanticError((ASTNode*)node,
                  "atomic arithmetic builtin requires integral or pointer type");
  }
  if (node->children->length > 1) {
    ASTNode* operand = node->children->value.p[1];
    if (TypeIsPointer(value_type)) {
      if (!TypeIsIntegral(operand->type)) {
        SemanticError(operand,
                      "atomic pointer arithmetic requires an integer operand");
      } else {
        ASTNode* scale =
            NewPtrScaleASTNode(value_type->next, AST_OP(mult), operand,
                               operand->location);
        scale->parent = (ASTNode*)node;
        node->children->value.p[1] = scale;
        ASTNodeSetType(scale, value_type);
      }
    } else {
      NormalConversion(operand, value_type);
    }
  }
  if (node->children->length > 2) {
    ValidateAtomicMemoryOrder(node->children->value.p[2],
                              kAtomicOrderReadModifyWrite);
  }
  ASTNodeSetType(&node->base, value_type);
}

static void AnalyzeAtomicCompareExchangeBuiltin(VectorASTNode* node,
                                                bool expected_is_pointer,
                                                bool returns_bool) {
  AnalyzeAtomicBuiltinChildren(node);
  TypeRecord* value_type =
      AtomicOperationValueType(AtomicPointerPointee(node));
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
  if (expected_is_pointer && node->children->length > 5) {
    int64_t weak = -1;
    ASTNode* weak_node = node->children->value.p[3];
    if (!EvaluateIntegerExpression(weak_node, &weak) ||
        (weak != 0 && weak != 1)) {
      SemanticError(weak_node,
                    "atomic compare-exchange weak argument must be constant 0 or 1");
    }
    int success = ValidateAtomicMemoryOrder(
        node->children->value.p[4], kAtomicOrderReadModifyWrite);
    int failure = ValidateAtomicMemoryOrder(
        node->children->value.p[5], kAtomicOrderFailure);
    if (!AtomicFailureOrderAllowed(success, failure)) {
      SemanticError(node->children->value.p[5],
                    "atomic compare-exchange failure order is stronger than success order");
    }
  }
  if (returns_bool) {
    ASTNodeSetType(&node->base,
                   NewTypeRecordWithSize(kTypeBool, kQualPlain));
    TypeRecordDelete(value_type);
  } else {
    ASTNodeSetType(&node->base, value_type);
  }
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

static void AnalyzeBuiltinExpect(VectorASTNode* node) {
  TypeRecord* result_type = NewTypeRecordWithSize(kTypeLong, kQualPlain);
  if (node->children->length != 2) {
    ASTNodeSetType(&node->base, result_type);
    return;
  }
  for (size_t i = 0; i < node->children->length; i++) {
    ASTNode* child = node->children->value.p[i];
    child = AnalyzeExpression(child);
    node->children->value.p[i] = child;
    NormalConversion(child, NewTypeRecordWithSize(kTypeLong, kQualPlain));
  }
  ASTNodeSetType(&node->base, result_type);
}

static void AnalyzeBuiltinBitOperation(VectorASTNode* node) {
  for (size_t i = 0; i < node->children->length; i++) {
    node->children->value.p[i] =
        AnalyzeExpression(node->children->value.p[i]);
  }
  if (node->children->length == 0) {
    ASTNodeSetType(&node->base,
                   NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return;
  }
  ASTNode* value = node->children->value.p[0];
  if (!TypeIsIntegral(value->type) || !TypeIsUnsigned(value->type)) {
    SemanticError(value, "bit builtin requires an unsigned integer operand");
  }
  bool rotate = node->base.op == AST_OP(builtin_rotl) ||
                node->base.op == AST_OP(builtin_rotr);
  if (node->children->length == 2 &&
      !TypeIsIntegral(((ASTNode*)node->children->value.p[1])->type)) {
    SemanticError(node->children->value.p[1],
                  "bit builtin count or width must be an integer");
  }
  ASTNodeSetType(&node->base,
                 rotate ? TypeRecordCopy(value->type)
                        : NewTypeRecordWithSize(kTypeInt, kQualPlain));
}

static void AnalyzeBuiltinPrefetch(VectorASTNode* node) {
  for (size_t i = 0; i < node->children->length; i++) {
    node->children->value.p[i] =
        AnalyzeExpression(node->children->value.p[i]);
  }
  if (node->children->length >= 1) {
    ASTNode* address = node->children->value.p[0];
    if (!TypeIsPointerOrArray(address->type)) {
      SemanticError(address, "__builtin_prefetch address must be a pointer");
    }
  }
  for (size_t i = 1; i < node->children->length; i++) {
    ASTNode* option = node->children->value.p[i];
    int64_t value = -1;
    if (!TypeIsIntegral(option->type) ||
        !EvaluateIntegerExpression(option, &value)) {
      SemanticError(option,
                    "__builtin_prefetch optional arguments must be constant integers");
      continue;
    }
    if ((i == 1 && (value < 0 || value > 1)) ||
        (i == 2 && (value < 0 || value > 3))) {
      SemanticError(option,
                    "__builtin_prefetch argument is outside its valid range");
    }
  }
  ASTNodeSetType(&node->base,
                 NewTypeRecordWithSize(kTypeVoid, kQualPlain));
}

static void AnalyzeBuiltinStartLifetime(VectorASTNode* node) {
  for (size_t i = 0; i < node->children->length; i++) {
    node->children->value.p[i] =
        AnalyzeExpression(node->children->value.p[i]);
  }
  ASTNode* operand =
      node->children->length == 1 ? node->children->value.p[0] : NULL;
  TypeRecord* target =
      operand != NULL && operand->type != NULL &&
              TypeIsPointer(operand->type)
          ? operand->type->next
          : NULL;
  if (target == NULL) {
    SemanticError(&node->base,
                  "__builtin_start_lifetime requires a pointer operand");
  } else if (target->size <= 0) {
    SemanticError(&node->base,
                  "__builtin_start_lifetime requires a complete type");
  } else if (!CXXTypeIsImplicitLifetimeAggregate(target)) {
    SemanticError(
        &node->base,
        "__builtin_start_lifetime requires an implicit-lifetime aggregate");
  }
  ASTNodeSetType(&node->base,
                 NewTypeRecordWithSize(kTypeVoid, kQualPlain));
}

static void AnalyzeBuiltinObservableCheckpoint(VectorASTNode* node) {
  ASTNodeSetType(&node->base,
                 NewTypeRecordWithSize(kTypeVoid, kQualPlain));
}

static void AnalyzeBuiltinTerminator(VectorASTNode* node) {
  ASTNodeSetType(&node->base,
                 NewTypeRecordWithSize(kTypeVoid, kQualPlain));
}

static void AnalyzeBuiltinIsConstantEvaluated(VectorASTNode* node) {
  ASTNodeSetType(&node->base,
                 NewTypeRecordWithSize(kTypeBool, kQualPlain));
  node->base.value_category = kValueCategoryPrvalue;
}

static ASTNode* AnalyzeTypeTraitBuiltin(VectorASTNode* node) {
  SourceLocation location = node->base.location;
  if (node->children == NULL || node->children->length == 0) {
    return (ASTNode*)NewIntConstantASTNode(
        0, NewTypeRecordWithSize(kTypeInt, kQualPlain), location);
  }
  ASTNode* kind_node = node->children->value.p[0];
  int64_t kind_value = 0;
  if (!EvaluateIntegerExpression(kind_node, &kind_value)) {
    SemanticError(&node->base, "Invalid type trait builtin");
    return (ASTNode*)NewIntConstantASTNode(
        0, NewTypeRecordWithSize(kTypeInt, kQualPlain), location);
  }
  Vector type_args;
  VectorInit(&type_args);
  bool dependent = false;
  for (size_t i = 1; i < node->children->length; i++) {
    ASTNode* arg = node->children->value.p[i];
    if (arg == NULL || arg->type == NULL) {
      continue;
    }
    TypeRecord* type = TypeRecordCopy(arg->type);
    if (type != NULL) {
      dependent = dependent || TypeIsUnknown(type) ||
                  TypeContainsTemplateParameter(type);
      VectorAppend(&type_args, type);
    }
  }
  if (dependent) {
    VectorDestructWithContents(
        &type_args, (VectorElementDestructor)TypeRecordDelete,
        /*free_element=*/false);
    ASTNodeSetType((ASTNode*)node,
                   NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return (ASTNode*)node;
  }
  bool value = CXXTypeTraitEvaluateBool(&compiler->syntax,
                                        (CXXTypeTraitKind)kind_value,
                                        &type_args);
  VectorDestructWithContents(&type_args, (VectorElementDestructor)TypeRecordDelete,
                             /*free_element=*/false);
  ASTNode* constant = (ASTNode*)NewIntConstantASTNode(
      value ? 1 : 0, NewTypeRecordWithSize(kTypeInt, kQualPlain), location);
  if (node->base.parent != NULL) {
    ASTNodeReplaceChild(node->base.parent, node->base.child_id, constant, false);
  }
  return constant;
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
    case AST_OP(requires_expr):
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
      TypeRecord* modulus_type =
          TypeIsVector(binary_node->left->type)
              ? TypeVectorElement(binary_node->left->type)
              : binary_node->left->type;
      if (!TypeIsIntegral(modulus_type)) {
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
      if ((binary_node->left != NULL &&
           (binary_node->left->op == AST_OP(reflect) ||
            binary_node->left->op == AST_OP(reflection_constant) ||
            binary_node->left->op == AST_OP(token_sequence_literal) ||
            TypeIsReflection(binary_node->left->type))) ||
          (binary_node->right != NULL &&
           (binary_node->right->op == AST_OP(reflect) ||
            binary_node->right->op == AST_OP(reflection_constant) ||
            binary_node->right->op == AST_OP(token_sequence_literal) ||
            TypeIsReflection(binary_node->right->type)))) {
        node = SemanticAnalyzeReflectionComparison(binary_node);
      } else {
        node = AnalyzeComparisonOperator(binary_node);
      }
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
    case AST_OP(alignof):
      AnalyzeSizeofExpression((SizeofASTNode*)node);
      break;

    case AST_OP(noexcept_expr):
      AnalyzeNoexceptExpression(unary_node);
      break;

    case AST_OP(typeid):
      return AnalyzeTypeidExpression((TypeidASTNode*)node);

    case AST_OP(reflect):
    case AST_OP(reflection_constant):
    case AST_OP(token_sequence_literal):
      return SemanticAnalyzeReflection((ReflectionASTNode*)node);

    case AST_OP(splice):
      return SemanticAnalyzeSplice((SpliceASTNode*)node);

    case AST_OP(splice_qualified):
      return SemanticAnalyzeSpliceQualified((SpliceQualifiedASTNode*)node);

    case AST_OP(cast):
      node = AnalyzeCastExpression((CastASTNode*)node);
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
      if (unary_node->sub != NULL &&
          unary_node->sub->op == AST_OP(splice)) {
        ASTNode* addressed = SemanticAnalyzeAddressedSplice(unary_node);
        if (addressed != node) {
          return addressed;
        }
      }
      AnalyzeAddressOperator(unary_node);
      break;

    case AST_OP(contents):
      AnalyzeContentsOperator(unary_node);
      break;

    case AST_OP(subscript):  // Array subscript.
      node = ASTNodeGetShape(node) == kASTShapeVector
                 ? AnalyzeMultidimensionalSubscript(vector_node)
                 : AnalyzeArraySubscript(binary_node);
      break;

    case AST_OP(pack_index):
      AnalyzePackIndexExpression(binary_node);
      break;

    case AST_OP(call):  // Function call.
      node = AnalyzeFunctionCall(vector_node);
      break;

    case AST_OP(range_begin):
    case AST_OP(range_end):
      // A range-for over a dependent range: now that the range's type is known,
      // pick the array, member or ADL form and analyze that instead.
      node = AnalyzeExpression(SyntaxResolveRangeForIterator(node));
      break;

    case AST_OP(dot):
    case AST_OP(arrow):
      if (!SemanticLowerMemberSplice(binary_node)) {
        AnalyzeMemberReference(binary_node);
      }
      break;

    case AST_OP(dotstar):
      node = AnalyzeMemberPointerReference(binary_node);
      break;
    case AST_OP(arrowstar): {
      ASTNode* overloaded =
          TryAnalyzeOverloadedBinaryOperatorWithAnalyzedOperands(binary_node);
      if (overloaded != NULL) {
        node = overloaded;
        break;
      }
      node = AnalyzeMemberPointerReference(binary_node);
      break;
    }

    case AST_OP(member_ptr):
      AnalyzePointerToMember(unary_node);
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
      {
        ASTNode* analyzed = AnalyzeInitialization(
            node, binary_node->left, binary_node->right);
        if (binary_node->right == NULL) {
          ASTNode* parent = node->parent;
          int child_id = node->child_id;
          if (parent != NULL) {
            ASTNodeReplaceChild(parent, child_id, analyzed, true);
          } else {
            ASTNodeDelete(node);
          }
          node = analyzed;
        } else {
          binary_node->right = analyzed;
        }
      }
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
      if (vector_node->children->length > 0) {
        ValidateAtomicMemoryOrder(vector_node->children->value.p[0],
                                  kAtomicOrderFence);
      }
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

    case AST_OP(builtin_expect):
      AnalyzeBuiltinExpect(vector_node);
      break;

    case AST_OP(builtin_clz):
    case AST_OP(builtin_ctz):
    case AST_OP(builtin_popcount):
    case AST_OP(builtin_rotl):
    case AST_OP(builtin_rotr):
      AnalyzeBuiltinBitOperation(vector_node);
      break;

    case AST_OP(builtin_prefetch):
      AnalyzeBuiltinPrefetch(vector_node);
      break;

    case AST_OP(builtin_start_lifetime):
      AnalyzeBuiltinStartLifetime(vector_node);
      break;

    case AST_OP(builtin_observable_checkpoint):
      AnalyzeBuiltinObservableCheckpoint(vector_node);
      break;

    case AST_OP(builtin_trap):
    case AST_OP(builtin_unreachable):
      AnalyzeBuiltinTerminator(vector_node);
      break;

    case AST_OP(builtin_is_constant_evaluated):
      AnalyzeBuiltinIsConstantEvaluated(vector_node);
      break;

    case AST_OP(builtin_type_trait):
      return AnalyzeTypeTraitBuiltin(vector_node);

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

  if (node == NULL) {
    return NULL;
  }

  // Attempt to fold a constant expression.
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
