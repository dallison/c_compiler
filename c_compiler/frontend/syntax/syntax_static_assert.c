//
//  syntax_static_assert.c
//  c_compiler
//
//  static_assert declarations and the template-dependence checks that decide
//  when their conditions can be evaluated.
//

#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "concepts.h"
#include "constexpr.h"
#include "expr_evaluator.h"
#include "expr_parser.h"
#include "expr_semantics.h"
#include "semantics.h"
#include "statement_parser.h"
#include "symbol_table.h"
#include "syntax.h"
#include "macro_expansion.h"
#include "typo_correction.h"
#include "type.h"
#include "type_class_internal.h"
#include "type_inheritance.h"
#include "type_internal.h"
#include "type_member.h"
#include "type_special_member.h"
#include "errors.h"
#include "compiler.h"
#include "module_identity.h"
#include "module_syntax.h"
#include "reflection_semantics.h"
#include "syntax_internal.h"

static ASTNode* ConvertStaticAssertMessageResult(ASTNode* expression,
                                                 TypeRecord* target);

// True when `arg` or any nested pack argument names a template parameter or
// carries a dependent type.
static bool StaticAssertTemplateArgumentContainsTemplateParameter(
    TemplateArgument* arg) {
  if (arg == NULL) {
    return false;
  }
  if (arg->template_parameter_index >= 0 ||
      TypeContainsTemplateParameter(arg->type)) {
    return true;
  }
  for (size_t i = 0; arg->pack_arguments != NULL &&
                     i < arg->pack_arguments->length; i++) {
    if (StaticAssertTemplateArgumentContainsTemplateParameter(
            arg->pack_arguments->value.p[i])) {
      return true;
    }
  }
  return false;
}

// True when any element of `args` is template-dependent, skipping plain
// materialized scalar template arguments that cannot mention parameters.
static bool StaticAssertTemplateArgumentVectorContainsTemplateParameter(
    Vector* args) {
  for (size_t i = 0; args != NULL && i < args->length; i++) {
    TemplateArgument* arg = args->value.p[i];
    // A materialized scalar argument such as `value<42>` has no parameter,
    // pack, dependent metadata, or nontrivial type spine. Its stored shape is
    // already a compact non-dependence summary, so avoid recursive checking.
    TypeRecord* type = arg != NULL ? arg->type : NULL;
    bool plain_type =
        type == NULL ||
        (type->next == NULL && type->declarator == kDeclPrimitive &&
         type->template_parameter_index < 0 && type->template_origin == NULL &&
         type->template_arguments == NULL &&
         TypeDependentOf(type)->dependent_member_name == NULL &&
         TypeDependentOf(type)->dependent_member_template_arguments == NULL &&
         TypeDependentOf(type)->dependent_decltype_expr == NULL &&
         TypeDependentOf(type)->dependent_splice_expr == NULL &&
         !type->is_pack_index &&
         TypeDependentOf(type)->pack_index_expr == NULL &&
         TypeDependentOf(type)->pack_index_pack == NULL &&
         !TypeIsStructOrUnion(type) && !TypeIsEnum(type));
    if (arg != NULL && arg->template_parameter_index < 0 &&
        arg->pack_arguments == NULL && plain_type) {
      continue;
    }
    if (StaticAssertTemplateArgumentContainsTemplateParameter(arg)) {
      return true;
    }
  }
  return false;
}

// A call to a constexpr member of a class template is value-dependent when
// the callee's body still mentions a template parameter, even though the
// function's declared return type does not.  `DefaultCapacity()` is `size_t`,
// but its body calls `SooEnabled()`, which calls `Traits<Policy>::soo_enabled()`.
// The call must not be folded (or rejected) until the class is instantiated.
// Free functions and members of ordinary classes are left alone: their bodies
// are not patterns, and treating every constexpr call as dependent would skip
// real constant folding.
#define kConstexprBodyDependenceLimit 64

static Symbol* g_constexpr_body_dependence_stack[kConstexprBodyDependenceLimit];

static int g_constexpr_body_dependence_depth;

// True when `symbol` is a constexpr member of a class template whose body
// still mentions a template parameter; uses a small recursion stack to avoid
// cycles.
static bool ConstexprMemberBodyIsTemplateDependent(Symbol* symbol) {
  if (symbol == NULL || symbol->type == NULL || !TypeIsFunction(symbol->type)) {
    return false;
  }
  FunctionInfo* func = &symbol->type->info.function;
  if (!func->is_constexpr && !func->is_consteval) {
    return false;
  }
  Struct* owner = func->cxx_member_owner;
  if (owner == NULL ||
      (!owner->is_template && owner->defining_template_scope_count <= 0)) {
    return false;
  }
  // An instantiation's body is cloned before it is evaluated.  A null body
  // still names the pattern via func_defn; walking that would keep every
  // specialization dependent and block constant folding.
  ASTNode* body = func->body;
  if (body == NULL) {
    return false;
  }
  for (int i = 0; i < g_constexpr_body_dependence_depth; i++) {
    if (g_constexpr_body_dependence_stack[i] == symbol) {
      return false;
    }
  }
  if (g_constexpr_body_dependence_depth >= kConstexprBodyDependenceLimit) {
    return false;
  }
  g_constexpr_body_dependence_stack[g_constexpr_body_dependence_depth++] =
      symbol;
  bool dependent = ExpressionIsTemplateDependent(body);
  g_constexpr_body_dependence_depth--;
  return dependent;
}

// Visitor predicate: true when `node` is value-dependent for template
// instantiation, including identifiers, sizeof operands, and constexpr calls.
static bool ExpressionNodeIsTemplateDependent(ASTNode* node, void* data) {
  (void)data;
  if (node == NULL) {
    return false;
  }
  if (TypeContainsTemplateParameter(node->type) ||
      TypeContainsAuto(node->type)) {
    return true;
  }
  if ((node->flags & kASTDependentQualifiedName) != 0 ||
      (node->flags & kASTDependentFunctorCall) != 0 ||
      node->op == AST_OP(requires_expr)) {
    return true;
  }
  if (node->op == AST_OP(sizeof) || node->op == AST_OP(alignof)) {
    // `sizeof(T)` / `alignof(T)` on a dependent type is value-dependent even
    // though the node's own type is size_t; the operand type is retained only
    // when dependent (see SizeofASTNode::type_operand).
    SizeofASTNode* s = (SizeofASTNode*)node;
    if (TypeContainsTemplateParameter(s->type_operand)) {
      return true;
    }
  }
  if (node->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node;
    bool structured_binding_dependent =
        id->symbol != NULL &&
        id->symbol->structured_binding_pack_size >= -1;
    if (structured_binding_dependent &&
        id->symbol->flags.is_parameter_pack &&
        id->symbol->structured_binding_pack_size >= 0 &&
        node->parent != NULL && node->parent->op == AST_OP(sizeof) &&
        ((SizeofASTNode*)node->parent)->is_pack_size) {
      structured_binding_dependent = false;
    }
    // A function template's own signature always mentions its parameters
    // (`declval`, `GetData`).  Naming that template is not what makes an
    // expression dependent; the explicit arguments and the call's actuals are.
    // Treating the signature as dependent left `decltype(GetData(declval<T>()))`
    // unresolved after `T` was concrete, so `Span`'s converting constructor
    // never became viable.
    bool function_template =
        id->symbol != NULL && id->symbol->type != NULL &&
        TypeIsFunction(id->symbol->type) &&
        (id->symbol->flags.is_template ||
         id->symbol->type->info.function.template_origin != NULL);
    // Inline member-template bodies are parsed only after the surrounding
    // class is complete.  Until then a call in the same class-template pattern
    // is dependent: its apparently concrete return type does not reveal
    // whether the deferred body uses an enclosing template parameter.
    bool deferred_member_template_body =
        function_template && id->symbol->type->info.function.body == NULL &&
        id->symbol->type->info.function.cxx_member_owner != NULL &&
        (id->symbol->type->info.function.cxx_member_owner->is_template ||
         id->symbol->type->info.function.cxx_member_owner
                 ->defining_template_scope_count > 0);
    if (id->symbol != NULL &&
        (structured_binding_dependent ||
         deferred_member_template_body ||
         id->symbol->template_parameter_index >= 0 ||
         (!function_template &&
          TypeContainsTemplateParameter(id->symbol->type)) ||
         StaticAssertTemplateArgumentVectorContainsTemplateParameter(
             id->template_arguments))) {
      return true;
    }
    // `constexpr bool b = Trait<T>::value; static_assert(b);` inside a
    // function template: `b` is not a template parameter, but its value is
    // not known until the function is instantiated.
    if (id->symbol != NULL && id->symbol->flags.is_constexpr &&
        !id->symbol->flags.value_set) {
      bool in_template =
          compiler->syntax.current_template_parameter_count > 0 ||
          (compiler->current_function != NULL &&
           TypeIsFunction(compiler->current_function) &&
           compiler->current_function->info.function.template_parameter_count >
               0);
      if (in_template) {
        ASTNode* init = ConstexprInitializerExpression(
            SymbolExtraOf(id->symbol)->constexpr_initializer);
        if (init != NULL && !ExpressionIsTemplateDependent(init)) {
          // Only a scalar constant can be cached in `value.ivalue`.  An
          // aggregate such as `static constexpr std::array<size_t, 1>
          // kStaticSizes = {1}` also evaluates as the integer 1, and storing
          // that in the symbol makes later constexpr code treat `1` as a
          // ConstexprObject pointer.
          if (id->symbol->type != NULL &&
              (TypeIsIntegral(id->symbol->type) ||
               TypeIsEnum(id->symbol->type))) {
            int64_t ivalue = 0;
            if (EvaluateIntegerExpression(init, &ivalue)) {
              id->symbol->flags.value_set = true;
              id->symbol->value.ivalue = ivalue;
              return false;
            }
          }
          return false;
        }
        return true;
      }
    }
    // `enum { N = sizeof...(Ts) }; static_assert(N > 0)` — the enumerator is
    // not itself constexpr, but its value is still the dependent initializer.
    if (id->symbol != NULL &&
        SymbolExtraOf(id->symbol)->constexpr_initializer != NULL &&
        !id->symbol->flags.value_set &&
        (compiler->syntax.current_template_parameter_count > 0 ||
         ExpressionIsTemplateDependent(
             SymbolExtraOf(id->symbol)->constexpr_initializer))) {
      return true;
    }
    // `static constexpr auto value = T{}()` is value-dependent even though
    // `value`'s type is not.  Uses inside the class (a later static_assert)
    // must wait until the enclosing class template is instantiated.
    if (id->symbol != NULL && compiler->syntax.cxx_class_head != NULL) {
      Struct* cls = compiler->syntax.cxx_class_head;
      StructMember* member = FindStructMember(cls, &id->symbol->name);
      for (StructMember* candidate = member; candidate != NULL;
           candidate = candidate->overload_next) {
        if (candidate->symbol == NULL) {
          continue;
        }
        bool same_symbol = candidate->symbol == id->symbol;
        bool same_entity =
            candidate->symbol->type == id->symbol->type &&
            StringEqualString(&candidate->symbol->name, &id->symbol->name);
        if (!same_symbol && !same_entity) {
          continue;
        }
        if (DependentExpressionContainsTemplateParameter(
                candidate->default_initializer)) {
          return true;
        }
      }
    }
    if (ConstexprMemberBodyIsTemplateDependent(id->symbol)) {
      return true;
    }
  }
  if (node->op == AST_OP(structmember)) {
    StructMemberASTNode* member_node = (StructMemberASTNode*)node;
    if (member_node->member != NULL &&
        ConstexprMemberBodyIsTemplateDependent(member_node->member->symbol)) {
      return true;
    }
  }
  return false;
}

// True when any subexpression of `expr` is template-dependent per
// `ExpressionNodeIsTemplateDependent`.
bool ExpressionIsTemplateDependent(ASTNode* expr) {
  return ASTNodeAny(expr, ExpressionNodeIsTemplateDependent, NULL);
}

// Visitor predicate: true when `node` is an identifier for a structured
// binding whose pack size is not yet known.
static bool ExpressionNodeNamesPendingStructuredBinding(ASTNode* node,
                                                        void* data) {
  (void)data;
  return node != NULL && node->op == AST_OP(identifier) &&
         ((IdentifierASTNode*)node)->symbol != NULL &&
         ((IdentifierASTNode*)node)->symbol->structured_binding_pack_size >= -1;
}

// Visitor predicate: true when `node` names a constexpr aggregate or array
// object whose value is not yet materialized, or a C++26 type with virtual
// bases used in static_assert folding.
static bool ExpressionNodeNamesPendingConstexprObject(ASTNode* node,
                                                      void* data) {
  (void)data;
  if (node == NULL || node->op != AST_OP(identifier)) {
    return false;
  }
  Symbol* symbol = ((IdentifierASTNode*)node)->symbol;
  if (symbol == NULL || symbol->type == NULL) {
    return false;
  }
  if (CompilerCXXAtLeast(kLanguageStandardCXX26) &&
      TypeIsStructOrUnion(symbol->type) &&
      StructHasVirtualBases(symbol->type->info.struct_info)) {
    return true;
  }
  return symbol->flags.is_constexpr && !symbol->flags.value_set &&
         (TypeIsStructOrUnion(symbol->type) ||
          TypeIsFixedArray(symbol->type)) &&
         SymbolExtraOf(symbol)->constexpr_initializer != NULL;
}

// Clears `kASTAnalyzed` on each visited node so a cloned static_assert
// operand can be re-analyzed for constant evaluation.
static void ClearStaticAssertExprAnalysis(ASTNode* node, void* data, int child_id,
                                          VisitorMode mode) {
  (void)data;
  (void)child_id;
  if (mode == kVisitPreChildren && node != NULL) {
    node->flags &= ~kASTAnalyzed;
  }
}

// Returns the function `Symbol` for a call-shaped `node`, including simple
// identifier and member calls; NULL when the callee is not a direct symbol.
static Symbol* StaticAssertCallCallee(ASTNode* node) {
  if (node == NULL || node->op != AST_OP(call) ||
      ASTNodeGetShape(node) != kASTShapeVector) {
    return NULL;
  }
  ASTNode* callee = ((VectorASTNode*)node)->left;
  if (callee == NULL) {
    return NULL;
  }
  if (callee->op == AST_OP(identifier) &&
      ASTNodeGetShape(callee) == kASTShapeIdentifier) {
    return ((IdentifierASTNode*)callee)->symbol;
  }
  if ((callee->op == AST_OP(dot) || callee->op == AST_OP(arrow)) &&
      ASTNodeGetShape(callee) == kASTShapeBinary) {
    ASTNode* member = ((BinaryASTNode*)callee)->right;
    if (member != NULL && member->op == AST_OP(identifier) &&
        ASTNodeGetShape(member) == kASTShapeIdentifier) {
      return ((IdentifierASTNode*)member)->symbol;
    }
  }
  return NULL;
}

// Inline member bodies are captured and re-parsed only after the class is
// complete.  A static_assert earlier in the class can already name those
// functions; constant-evaluation has to wait until the body exists.  A
// constexpr member of a class template may be defined out of line after the
// class, so a static_assert in another member is checked at instantiation.
bool ExpressionReferencesDeferredConstexprFunction(ASTNode* node,
                                                  void* data) {
  (void)data;
  Symbol* symbol = StaticAssertCallCallee(node);
  if (symbol == NULL || symbol->type == NULL || !TypeIsFunction(symbol->type)) {
    return false;
  }
  Symbol* canonical = symbol->type->info.function.symbol;
  if (canonical != NULL && canonical != symbol && canonical->type == symbol->type) {
    symbol = canonical;
  }
  if (symbol->type->info.function.body == NULL &&
      symbol->value.func_defn != NULL &&
      symbol->value.func_defn->type != NULL &&
      TypeIsFunction(symbol->value.func_defn->type) &&
      symbol->value.func_defn->type->info.function.body != NULL) {
    symbol = symbol->value.func_defn;
  }
  if (!symbol->type->info.function.is_constexpr && !symbol->flags.is_constexpr) {
    return false;
  }
  if (symbol->type->info.function.body != NULL) {
    return false;
  }
  if (symbol->flags.is_defined && symbol->flags.is_inline_defn) {
    return true;
  }
  return compiler->syntax.current_template_parameter_count > 0;
}

// `static_assert(is_trivially_destructible<T>{})` contextually converts the
// trait object to bool through `operator bool`, and a member pointer to
// whether it is null.  Returns NULL (deleting `expression`) when that fails.
static ASTNode* StaticAssertConvertToBool(ASTNode* expression) {
  if (expression == NULL || expression->type == NULL ||
      !(TypeIsStructOrUnion(expression->type) ||
        TypeIsMemberPointer(expression->type))) {
    return expression;
  }
  TypeRecord* boolean = NewTypeRecordWithSize(kTypeBool, kQualPlain);
  TypeRecordCalculateSize(boolean);
  return ConvertStaticAssertMessageResult(expression, boolean);
}

// Clones and constant-evaluates `expr` for a static_assert condition,
// contextualizing class types to bool; returns NULL when evaluation fails.
static ASTNode* EvaluateStaticAssertExpression(ASTNode* expr) {
  ASTNode* cloned = ASTNodeClone(expr, IdentityCloneNode, NULL, NULL);
  if (cloned == NULL) {
    return NULL;
  }
  ASTNodeVisit(cloned, ClearStaticAssertExprAnalysis, 0, NULL);
  // The operand is interpreted by the constant evaluator, which rejects the
  // body a call is replaced with once it is inlined.
  compiler->constant_evaluation_required_depth++;
  cloned = AnalyzeExpression(cloned);
  compiler->constant_evaluation_required_depth--;
  if (cloned == NULL) {
    ASTNodeDelete(cloned);
    return NULL;
  }
  return StaticAssertConvertToBool(cloned);
}

// Builds and analyzes a member call `message_expr`.`member_name`() for
// extracting a C++26 static_assert message; caller owns the returned node.
static ASTNode* AnalyzeStaticAssertMessageMemberCall(ASTNode* message_expr,
                                                     const char* member_name) {
  ASTNode* receiver =
      ASTNodeClone(message_expr, IdentityCloneNode, NULL, NULL);
  if (receiver == NULL) {
    return NULL;
  }
  ASTNodeVisit(receiver, ClearStaticAssertExprAnalysis, 0, NULL);
  ASTNode* member = NewStringConstantASTNode(
      NewString(member_name), NULL, message_expr->location);
  ASTNode* access =
      NewBinaryASTNode(AST_OP(dot), NULL, message_expr->location, receiver,
                       member);
  ASTNode* call = NewVectorASTNode(AST_OP(call), NULL, message_expr->location,
                                   access, NewVector());
  compiler->constant_evaluation_required_depth++;
  call = AnalyzeExpression(call);
  compiler->constant_evaluation_required_depth--;
  return call;
}

// Applies a normal conversion of `expression` to `target`; deletes `expression`
// and `target` and returns NULL when conversion fails. Caller owns the result.
static ASTNode* ConvertStaticAssertMessageResult(ASTNode* expression,
                                                 TypeRecord* target) {
  if (expression == NULL || target == NULL) {
    ASTNodeDelete(expression);
    TypeRecordDelete(target);
    return NULL;
  }
  target = TypeRecordAdopt(target);
  ASTNode* wrapper =
      NewExpressionStatementASTNode(expression, expression->location);
  NormalConversion(expression, target);
  expression = ((ExpressionStatementASTNode*)wrapper)->expr;
  bool converted =
      expression != NULL && expression->type != NULL &&
      TypeEqual(expression->type, target);
  expression = expression != NULL ? ASTNodeMove(expression) : NULL;
  ASTNodeDelete(wrapper);
  TypeRecordDelete(target);
  if (!converted) {
    ASTNodeDelete(expression);
    return NULL;
  }
  return expression;
}

// Evaluates a C++26 static_assert `message_expr` as a string-like object via
// `size()` and `data()`; appends the character sequence to `message`. Returns
// false and emits semantic errors when the message is not a constant string.
bool SyntaxEvaluateStaticAssertMessage(ASTNode* message_expr, String* message) {
  if (message_expr == NULL || message == NULL) {
    return false;
  }
  ASTNode* size_call =
      AnalyzeStaticAssertMessageMemberCall(message_expr, "size");
  size_call =
      ConvertStaticAssertMessageResult(size_call, NewSizeTypeRecord());
  int64_t size = -1;
  bool size_ok = size_call != NULL &&
                 EvaluateIntegerExpression(size_call, &size) && size >= 0;
  ASTNodeDelete(size_call);
  if (!size_ok) {
    SemanticError(
        message_expr,
        "static_assert message size() must be an integral constant expression");
    return false;
  }

  ASTNode* data_call =
      AnalyzeStaticAssertMessageMemberCall(message_expr, "data");
  TypeRecord* char_type =
      NewTypeRecordWithSize(kTypeChar, kQualConst);
  data_call = ConvertStaticAssertMessageResult(
      data_call, NewPointerTo(kQualPlain, char_type));
  bool data_type_ok = data_call != NULL;
  if (!data_type_ok) {
    ASTNodeDelete(data_call);
    SemanticError(message_expr,
                  "static_assert message data() must be convertible to "
                  "'const char*'");
    return false;
  }
  bool data_ok =
      ConstexprEvaluateCharacterSequence(data_call, (size_t)size, message);
  ASTNodeDelete(data_call);
  if (!data_ok) {
    SemanticError(
        message_expr,
        "static_assert message data() must be a constant expression");
    return false;
  }
  return true;
}

// Parses `static_assert`, evaluates the condition when it is not dependent,
// emits failure diagnostics, and returns a deferred node or NULL. Consumes
// tokens through the closing semicolon.
ASTNode* SyntaxParseStaticAssert(Syntax* syntax) {
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // static_assert
  SyntaxNeedBracket(syntax, TOK(lparen), TC(openbra));

  ASTNode* expr = SyntaxParseSingleExpression(syntax, TC(exprsep));

  String message = {0};
  StringInit(&message, "static assertion failed");
  ASTNode* message_expr = NULL;
  if (LexMatch(syntax->lex, TOK(comma))) {
    if (LexLookingAtStringLiteral(syntax->lex)) {
      LexValidateUnevaluatedString(syntax->lex, "static_assert",
                                   /*allow_user_defined_suffix=*/false);
      StringSetString(&message, &syntax->lex->spelling);
      LexNextToken(syntax->lex);
      while (LexLookingAtStringLiteral(syntax->lex)) {
        LexValidateUnevaluatedString(syntax->lex, "static_assert",
                                     /*allow_user_defined_suffix=*/false);
        StringAppend(&message, syntax->lex->spelling.value);
        LexNextToken(syntax->lex);
      }
    } else if (CompilerCXXAtLeast(kLanguageStandardCXX26)) {
      message_expr =
          SyntaxParseSingleExpression(syntax, TC(closebra));
    } else {
      SyntaxError(syntax, "static_assert message must be a string literal");
    }
  } else if (!CompilerIsCXX() &&
             !CompilerCAtLeast(kLanguageStandardC23)) {
    SyntaxError(syntax, "_Static_assert requires a message before C23");
  }
  SyntaxNeedBracket(syntax, TOK(rparen), TC(closebra));
  SyntaxNeedBracket(syntax, TOK(semicolon), TC(semicolon));

  bool dependent =
      syntax->parsing_lambda_body_depth > 0 ||
      (syntax->current_template_parameter_count > 0 &&
       (ExpressionIsTemplateDependent(expr) ||
        (message_expr != NULL &&
         ExpressionIsTemplateDependent(message_expr)))) ||
      ASTNodeAny(expr, ExpressionNodeNamesPendingStructuredBinding, NULL);
  dependent =
      dependent ||
      ASTNodeAny(expr, ExpressionNodeNamesPendingConstexprObject, NULL);
  if (dependent) {
    ASTNode* node =
        NewStaticAssertASTNode(expr, &message, message_expr, location);
    StringDestruct(&message);
    return node;
  }

  ASTNode* evaluated = EvaluateStaticAssertExpression(expr);
  ASTNodeDelete(expr);
  if (evaluated == NULL) {
    ASTNodeDelete(message_expr);
    StringDestruct(&message);
    SyntaxErrorAtLocation(
        syntax, location,
        "static_assert expression is not an integer constant expression");
    return NULL;
  }

  int64_t value = 0;
  if (!EvaluateTruthExpression(evaluated, &value)) {
    bool defer =
        ExpressionIsTemplateDependent(evaluated) ||
        ASTNodeAny(evaluated, ExpressionReferencesDeferredConstexprFunction,
                   NULL);
    if (defer) {
      ASTNode* node =
          NewStaticAssertASTNode(evaluated, &message, message_expr, location);
      StringDestruct(&message);
      return node;
    }
    ASTNodeDelete(evaluated);
    ASTNodeDelete(message_expr);
    StringDestruct(&message);
    SyntaxErrorAtLocation(
        syntax, location,
        "static_assert expression is not an integer constant expression");
    return NULL;
  }

  if (value == 0) {
    if (message_expr == NULL ||
        SyntaxEvaluateStaticAssertMessage(message_expr, &message)) {
      SyntaxErrorAtLocation(syntax, location, "%s", message.value);
    }
  }
  ASTNodeDelete(message_expr);
  StringDestruct(&message);
  ASTNodeDelete(evaluated);
  return NULL;
}

// Re-evaluates a deferred static_assert `node` at instantiation time. Returns
// 1 when the condition is true, 0 when it fails (emitting the message), -1
// when still not an ICE; sets `*dependent` when evaluation remains dependent.
int SyntaxEvaluateDeferredStaticAssert(ASTNode* node, bool* dependent) {
  if (dependent != NULL) {
    *dependent = false;
  }
  if (node == NULL || ASTNodeGetShape(node) != kASTShapeStaticAssert) {
    return -1;
  }
  StaticAssertASTNode* assert_node = (StaticAssertASTNode*)node;
  ASTNode* cloned =
      ASTNodeClone(assert_node->expr, IdentityCloneNode, NULL, NULL);
  if (cloned == NULL) {
    return -1;
  }
  ASTNodeVisit(cloned, ClearStaticAssertExprAnalysis, 0, NULL);
  compiler->constant_evaluation_required_depth++;
  cloned = AnalyzeExpression(cloned);
  compiler->constant_evaluation_required_depth--;
  if (cloned != NULL && !ExpressionIsTemplateDependent(cloned)) {
    cloned = StaticAssertConvertToBool(cloned);
  }
  if (cloned == NULL) {
    return -1;
  }
  int64_t value = 0;
  if (!EvaluateTruthExpression(cloned, &value)) {
    if (dependent != NULL) {
      *dependent = ExpressionIsTemplateDependent(cloned);
    }
    ASTNodeDelete(cloned);
    return -1;
  }
  ASTNodeDelete(cloned);
  if (value == 0) {
    String message;
    StringInit(&message, assert_node->message.value);
    if (assert_node->message_expr == NULL ||
        SyntaxEvaluateStaticAssertMessage(assert_node->message_expr,
                                          &message)) {
      SyntaxErrorAtLocation(&compiler->syntax, node->location, "%s",
                            message.value);
    }
    StringDestruct(&message);
    return 0;
  }
  return 1;
}
