//
//  expr_semantics_lambda.c
//  c_compiler
//
//  Lambda conversion and capture-object semantic analysis.
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

static bool ClosureHasCaptureFields(Struct* closure);
static StructMember* ClosureCallOperatorMember(Struct* closure);
static ASTNode* AddressOfFunctionSymbol(Symbol* function, SourceLocation location);
static bool ClosureCallOperatorIsGeneric(Symbol* call_op);
static Symbol* ResolveClosureCallOperatorForFunctionPointer( Struct* closure, TypeRecord* target_fn);
static ASTNode* LambdaCaptureInitializerExpr(ASTNode* init);
static void ClearCaptureInitializerAnalyzed(ASTNode* node, void* data, int child_id, VisitorMode mode);
static void FinishAutoLambdaCaptureFields(CompoundLiteralASTNode* node);
static void ExpandLambdaArrayCaptureCopies(CompoundLiteralASTNode* node);

// Contract: Read-only; true when closure has non-sentinel data members.
static bool ClosureHasCaptureFields(Struct* closure) {
  if (closure == NULL) {
    return true;
  }
  for (size_t i = 0; i < closure->members.length; i++) {
    StructMember* member = closure->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function || member->is_using_declaration ||
        StringEqual(&member->symbol->name, "__lambda_empty")) {
      continue;
    }
    return true;
  }
  return false;
}

// Contract: Read-only lookup of `operator()` member on closure class.
static StructMember* ClosureCallOperatorMember(Struct* closure) {
  if (closure == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < closure->members.length; i++) {
    StructMember* member = closure->members.value.p[i];
    if (member == NULL || !member->is_member_function ||
        member->symbol == NULL || member->symbol->type == NULL ||
        !TypeIsFunction(member->symbol->type)) {
      continue;
    }
    if (StringEqual(&member->symbol->name, "operator()")) {
      return member;
    }
  }
  return NULL;
}

// Contract: Returns a new prvalue address-of subtree; caller owns it. Does not
// mutate `function` or existing AST.
static ASTNode* AddressOfFunctionSymbol(Symbol* function, SourceLocation location) {
  if (function == NULL || function->type == NULL ||
      !TypeIsFunction(function->type)) {
    return NULL;
  }
  ASTNode* id = NewIdentifierASTNode(function, location);
  ASTNodeSetType(id, function->type);
  id->value_category = kValueCategoryLvalue;
  TypeRecord* pointer = NewPointerTo(kQualPlain, TypeRecordCopy(function->type));
  TypeRecordCalculateSize(pointer);
  ASTNode* address =
      NewUnaryASTNode(AST_OP(address), pointer, location, id);
  address->value_category = kValueCategoryPrvalue;
  return address;
}

// Contract: Read-only; true when closure `operator()` is still a template.
static bool ClosureCallOperatorIsGeneric(Symbol* call_op) {
  return call_op != NULL &&
         (call_op->flags.is_template ||
          (call_op->type != NULL &&
           call_op->type->info.function.template_parameters.length > 0));
}

/* [temp.deduct.funcaddr]: instantiate a generic lambda's `operator()` against
 * the destination function-pointer signature. */
// Contract: May instantiate generic `operator()`; queues member definition materialization.
static Symbol* ResolveClosureCallOperatorForFunctionPointer(
    Struct* closure, TypeRecord* target_fn) {
  if (closure == NULL || target_fn == NULL || !TypeIsFunction(target_fn)) {
    return NULL;
  }
  StructMember* call_member = ClosureCallOperatorMember(closure);
  Symbol* call_op = call_member != NULL ? call_member->symbol : NULL;
  if (call_op == NULL || call_op->type == NULL || !TypeIsFunction(call_op->type)) {
    return NULL;
  }
  if (!ClosureCallOperatorIsGeneric(call_op)) {
    return call_op;
  }
  if (target_fn == NULL) {
    return NULL;
  }
  Vector* actuals = NewVector();
  for (size_t i = 0; i < target_fn->info.function.prototype.length; i++) {
    Symbol* formal = target_fn->info.function.prototype.value.p[i];
    if (formal == NULL || formal->type == NULL) {
      VectorDelete(actuals);
      return NULL;
    }
    Symbol* standin = NewSymbol("__deduce", TypeRecordCopy(formal->type),
                                STO(auto));
    standin->flags.is_defined = true;
    standin->location = call_op->location;
    ASTNode* arg = NewIdentifierASTNode(standin, call_op->location);
    ASTNodeSetType(arg, standin->type);
    arg->flags |= kASTAnalyzed;
    VectorAppend(actuals, arg);
  }
  Symbol* resolved = TypeDeduceFunctionTemplateFromCallWithOffset(
      &compiler->syntax, call_op, actuals, 1);
  VectorDelete(actuals);
  if (resolved == NULL || resolved == call_op || resolved->type == NULL ||
      !TypeIsFunction(resolved->type)) {
    return NULL;
  }
  TypeEnsureTemplateMemberFunctionDefinition(&compiler->syntax, resolved);
  return resolved;
}

/* Non-capturing lambda `+[](Args) -> R { ... }` converts to `R (*)(Args)`.
 * The pointer addresses a thunk that default-constructs the closure and
 * calls its `operator()`. */
// Contract: Synthesizes a global thunk (`InsertGlobalSymbol`, pending defn queue)
// and returns a new address-of node; caller owns the subtree. May deduce
// `operator()` return type via `StatementFinishAutoReturnDeduction`.
ASTNode* LambdaToFunctionPointer(ASTNode* lambda,
                                        TypeRecord* target_fn) {
  if (lambda == NULL || (lambda->flags & kASTLambdaExpression) == 0 ||
      lambda->type == NULL || !TypeIsStructOrUnion(lambda->type) ||
      lambda->type->info.struct_info == NULL) {
    return NULL;
  }
  Struct* closure = lambda->type->info.struct_info;
  if (ClosureHasCaptureFields(closure)) {
    return NULL;
  }
  StructMember* call_member = ClosureCallOperatorMember(closure);
  Symbol* call_op =
      ResolveClosureCallOperatorForFunctionPointer(closure, target_fn);
  if (call_op == NULL || call_member == NULL) {
    return NULL;
  }
  TypeRecord* method = call_op->type;
  if (TypeFunctionReturnContainsAuto(method) &&
      method->info.function.body == NULL) {
    TypeEnsureTemplateMemberFunctionDefinition(&compiler->syntax, call_op);
  }
  if (TypeFunctionReturnContainsAuto(method)) {
    StatementFinishAutoReturnDeduction(method, lambda);
  }
  if (TypeFunctionReturnContainsAuto(method) || method->next == NULL) {
    return NULL;
  }
  if (call_member->is_static) {
    return AddressOfFunctionSymbol(call_op, lambda->location);
  }
  TypeRecord* func = NewFunctionTypeRecord();
  size_t start = method->info.function.cxx_member_owner != NULL ? 1 : 0;
  int arg_number = 0;
  for (size_t i = start; i < method->info.function.prototype.length; i++) {
    Symbol* src = method->info.function.prototype.value.p[i];
    if (src == NULL || src->type == NULL) {
      continue;
    }
    const char* pname =
        src->name.value != NULL && src->name.length > 0 ? src->name.value
                                                        : "arg";
    Symbol* formal = NewSymbol(pname, TypeRecordCopy(src->type), STO(auto));
    formal->flags.is_argument = true;
    formal->flags.is_defined = true;
    formal->location = lambda->location;
    formal->value.arg_number = arg_number++;
    VectorAppend(&func->info.function.prototype, formal);
  }
  TypeRecord* return_type =
      method->next != NULL ? TypeRecordCopy(method->next)
                           : NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecordChain(func, return_type);
  func->info.function.definition = true;

  Symbol* object =
      NewSymbol("__closure", TypeRecordCopy(lambda->type), STO(auto));
  object->flags.is_defined = true;
  object->flags.is_local = true;
  object->location = lambda->location;
  ASTNode* object_decl =
      NewVariableDeclarationASTNode(object, NULL, lambda->location);

  Vector* actuals = NewVector();
  for (size_t i = 0; i < func->info.function.prototype.length; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    VectorAppend(actuals, NewIdentifierASTNode(formal, lambda->location));
  }
  ASTNode* call = NewOperatorMemberCall(NewIdentifierASTNode(object, lambda->location),
                                        "operator()", actuals, lambda->location);
  ASTNode* invocation =
      TypeIsVoid(return_type)
          ? NewExpressionStatementASTNode(call, lambda->location)
          : NewCombinedStatementASTNode(AST_OP(return), call, NULL,
                                        lambda->location);
  Vector* statements = NewVector();
  VectorAppend(statements, object_decl);
  VectorAppend(statements, invocation);
  func->info.function.body =
      NewCompoundStatementASTNode(statements, lambda->location);

  static int lambda_fn_id = 0;
  char name[64];
  snprintf(name, sizeof(name), "__lambda_fn_%d", lambda_fn_id++);
  Symbol* thunk = NewSymbol(name, func, STO(static));
  thunk->flags.invented = true;
  thunk->flags.is_defined = true;
  thunk->location = lambda->location;
  thunk->value.func_defn = thunk;
  func->info.function.symbol = thunk;
  InsertGlobalSymbol(thunk);
  CompilerQueuePendingFunctionDefinition(thunk);
  return AddressOfFunctionSymbol(thunk, lambda->location);
}

// Contract: Read-only signature comparison after optional template deduction;
// may run auto-return deduction on the call operator type record.
bool LambdaCallMatchesFunctionPointer(ASTNode* lambda,
                                             TypeRecord* pointer) {
  if (lambda == NULL || pointer == NULL || !TypeIsPointer(pointer) ||
      pointer->next == NULL || !TypeIsFunction(pointer->next) ||
      (lambda->flags & kASTLambdaExpression) == 0 || lambda->type == NULL ||
      !TypeIsStructOrUnion(lambda->type)) {
    return false;
  }
  Struct* closure = lambda->type->info.struct_info;
  if (ClosureHasCaptureFields(closure)) {
    return false;
  }
  TypeRecord* target = pointer->next;
  StructMember* call_member = ClosureCallOperatorMember(closure);
  Symbol* primary = call_member != NULL ? call_member->symbol : NULL;
  Symbol* call_op =
      ResolveClosureCallOperatorForFunctionPointer(closure, target);
  if (call_op == NULL || call_op->type == NULL ||
      !TypeIsFunction(call_op->type)) {
    return false;
  }
  if (primary != NULL && ClosureCallOperatorIsGeneric(primary)) {
    return true;
  }
  TypeRecord* method = call_op->type;
  if (TypeFunctionReturnContainsAuto(method) &&
      method->info.function.body == NULL) {
    TypeEnsureTemplateMemberFunctionDefinition(&compiler->syntax, call_op);
  }
  if (TypeFunctionReturnContainsAuto(method) || method->next == NULL) {
    StatementFinishAutoReturnDeduction(method, lambda);
  }
  if (method->next == NULL) {
    return false;
  }
  if (method->info.function.varargs != target->info.function.varargs) {
    return false;
  }
  size_t start = method->info.function.cxx_member_owner != NULL ? 1 : 0;
  if (method->info.function.prototype.length < start ||
      method->info.function.prototype.length - start !=
          target->info.function.prototype.length) {
    return false;
  }
  for (size_t i = 0; i < target->info.function.prototype.length; i++) {
    Symbol* src = method->info.function.prototype.value.p[start + i];
    Symbol* dst = target->info.function.prototype.value.p[i];
    if (src == NULL || dst == NULL || src->type == NULL || dst->type == NULL ||
        !TypeEqual(src->type, dst->type)) {
      return false;
    }
  }
  if (TypeIsVoid(method->next) && TypeIsVoid(target->next)) {
    return true;
  }
  return TypeEqual(method->next, target->next) ||
         TypeEqualIgnoringQualifiers(method->next, target->next);
}

// Contract: Peels init/expr_init wrappers; does not mutate `from`.
ASTNode* CXXLambdaConversionExpression(ASTNode* from) {
  ASTNode* expr = from;
  if (expr != NULL && expr->op == AST_OP(init)) {
    expr = ((BinaryASTNode*)expr)->right;
  }
  if (expr != NULL && expr->op == AST_OP(expr_init)) {
    expr = ((ExpressionInitializerASTNode*)expr)->expr;
  }
  return expr;
}

// Contract: Replaces the lambda subtree in `expr->parent` with
// `LambdaToFunctionPointer` result (deletes lambda); requires parent link.
bool CXXConvertNonCapturingLambdaToFunctionPointer(ASTNode* from,
                                                   TypeRecord* to) {
  ASTNode* expr = CXXLambdaConversionExpression(from);
  if (expr == NULL || (expr->flags & kASTLambdaExpression) == 0) {
    return false;
  }
  if (!LambdaCallMatchesFunctionPointer(expr, to)) {
    return false;
  }
  TypeRecord* target_fn =
      TypeIsPointer(to) && to->next != NULL && TypeIsFunction(to->next)
          ? to->next
          : NULL;
  ASTNode* converted = LambdaToFunctionPointer(expr, target_fn);
  if (converted == NULL) {
    return false;
  }
  ASTNode* parent = expr->parent;
  int child_id = expr->child_id;
  if (parent == NULL) {
    ASTNodeDelete(converted);
    return false;
  }
  ASTNodeReplaceChild(parent, child_id, converted, true);
  return true;
}

// A lambda parsed inside a function template captures `auto` locals before
// those locals are deduced, so the closure field is still `auto`.  By the
// time this prvalue is analyzed the local has a concrete type; copy it onto
// the field (this closure was rebuilt per instantiation) and lay the closure
// out again.
// Contract: Peels expr_init wrapper; read-only.
static ASTNode* LambdaCaptureInitializerExpr(ASTNode* init) {
  if (init != NULL && init->op == AST_OP(expr_init)) {
    init = ((ExpressionInitializerASTNode*)init)->expr;
  }
  return init;
}

// Contract: Visitor; clears `kASTAnalyzed` on capture init subtrees before re-analysis.
static void ClearCaptureInitializerAnalyzed(ASTNode* node, void* data,
                                            int child_id, VisitorMode mode) {
  (void)data;
  (void)child_id;
  if (mode == kVisitPreChildren && node != NULL) {
    node->flags &= ~kASTAnalyzed;
  }
}

// Contract: Updates `auto` capture field types and relayout; may replace init nodes.
static void FinishAutoLambdaCaptureFields(CompoundLiteralASTNode* node) {
  if (node == NULL || (node->base.flags & kASTLambdaExpression) == 0 ||
      node->base.type == NULL || !TypeIsStructOrUnion(node->base.type) ||
      node->base.type->info.struct_info == NULL || node->initializer == NULL ||
      node->initializer->op != AST_OP(braced_init)) {
    return;
  }
  Struct* closure = node->base.type->info.struct_info;
  BracedInitializerASTNode* braced =
      (BracedInitializerASTNode*)node->initializer;
  bool changed = false;
  for (size_t i = 0; i < closure->members.length; i++) {
    StructMember* member = closure->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_member_function ||
        member->is_static || member->is_using_declaration ||
        !TypeContainsAuto(member->symbol->type)) {
      continue;
    }
    ASTNode* value = NULL;
    for (size_t j = 0; j < braced->initializers->length; j++) {
      ASTNode* initializer = braced->initializers->value.p[j];
      if (initializer == NULL || initializer->op != AST_OP(designated_init)) {
        continue;
      }
      DesignatedInitializerASTNode* designated =
          (DesignatedInitializerASTNode*)initializer;
      if (designated->designators == NULL ||
          designated->designators->length == 0) {
        continue;
      }
      Designator* designator = designated->designators->value.p[0];
      if (designator == NULL ||
          designator->designator_type != kDesignatorStruct ||
          designator->value.struct_member_name == NULL ||
          !StringEqualString(designator->value.struct_member_name,
                             &member->symbol->name)) {
        continue;
      }
      value = LambdaCaptureInitializerExpr(designated->init);
      break;
    }
    bool by_reference = false;
    if (value != NULL && value->op == AST_OP(address)) {
      by_reference = true;
      value = ((UnaryASTNode*)value)->sub;
    }
    if (value != NULL && value->op == AST_OP(cast)) {
      value = ((CastASTNode*)value)->expr;
    }
    if (value == NULL) {
      continue;
    }
    TypeRecord* concrete = NULL;
    if (value->op == AST_OP(identifier)) {
      Symbol* source = ((IdentifierASTNode*)value)->symbol;
      concrete = source != NULL ? source->type : NULL;
    } else {
      // Init-capture `[f = std::move(op)]`: the expression was analyzed while
      // `op` was still dependent, so re-analyze it now that the enclosing
      // instantiation is concrete.
      ASTNode* saved_parent = value->parent;
      int saved_child = value->child_id;
      ASTNodeVisit(value, ClearCaptureInitializerAnalyzed, 0, NULL);
      value->parent = NULL;
      ASTNode* analyzed = AnalyzeExpression(value);
      if (analyzed != NULL) {
        analyzed->parent = saved_parent;
        analyzed->child_id = saved_child;
        if (saved_parent != NULL && analyzed != value) {
          ASTNodeReplaceChild(saved_parent, saved_child, analyzed, false);
        }
        value = analyzed;
      }
      concrete = value != NULL ? value->type : NULL;
    }
    if (TypeIsReference(concrete)) {
      concrete = concrete->next;
    }
    if (concrete == NULL || TypeIsUnknown(concrete) ||
        TypeContainsAuto(concrete) ||
        TypeContainsTemplateParameter(concrete)) {
      continue;
    }
    TypeRecord* field_type = by_reference
                                 ? NewPointerTo(kQualPlain,
                                                TypeRecordCopy(concrete))
                                 : TypeRecordCopy(concrete);
    TypeRecordCalculateSize(field_type);
    TypeRecordDelete(member->symbol->type);
    member->symbol->type = field_type;
    changed = true;
  }
  if (!changed) {
    return;
  }
  RelayoutStruct(closure);
  node->base.type->size = closure->size;
  TypeRecordCalculateSize(node->base.type);
}

// A by-copy array capture copies each element.  The closure initializer's
// `.field = array` would instead decay the array and brace-elide the pointer
// into the first element, so spell the copy out as `{array[0], array[1], ...}`,
// nested per dimension.  Runs per instantiation, once the bound is concrete.
// Contract: Replaces designated array captures with elementwise copy AST; mutates braced init.
static void ExpandLambdaArrayCaptureCopies(CompoundLiteralASTNode* node) {
  if (node == NULL || (node->base.flags & kASTLambdaExpression) == 0 ||
      node->base.type == NULL || !TypeIsStructOrUnion(node->base.type) ||
      node->base.type->info.struct_info == NULL || node->initializer == NULL ||
      node->initializer->op != AST_OP(braced_init)) {
    return;
  }
  Struct* closure = node->base.type->info.struct_info;
  BracedInitializerASTNode* braced =
      (BracedInitializerASTNode*)node->initializer;
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* initializer = braced->initializers->value.p[i];
    if (initializer == NULL || initializer->op != AST_OP(designated_init)) {
      continue;
    }
    DesignatedInitializerASTNode* designated =
        (DesignatedInitializerASTNode*)initializer;
    if (designated->designators == NULL ||
        designated->designators->length != 1 || designated->init == NULL ||
        designated->init->op != AST_OP(expr_init)) {
      continue;
    }
    Designator* designator = designated->designators->value.p[0];
    if (designator == NULL ||
        designator->designator_type != kDesignatorStruct) {
      continue;
    }
    // A resolved designator in a cloned closure can still name the pattern's
    // member, whose bound is dependent; look the name up in this closure.
    String* name = NULL;
    if (designator->is_resolved_member) {
      StructMember* resolved = designator->value.struct_member;
      if (resolved != NULL && resolved->symbol != NULL) {
        name = &resolved->symbol->name;
      }
    } else {
      name = designator->value.struct_member_name;
    }
    StructMember* member = name != NULL ? FindStructMember(closure, name) : NULL;
    Symbol* field = member != NULL ? member->symbol : NULL;
    // By-reference capture fields are pointers and init-captures decay, so an
    // array-typed closure field is always a by-copy capture.
    if (field == NULL || field->lambda_capture_by_reference ||
        field->flags.is_parameter_pack) {
      continue;
    }
    int rank = FixedArrayRank(field->type);
    if (rank <= 0 || rank > kArrayElementwiseCopyMaxRank) {
      continue;
    }
    ASTNode* value = ((ExpressionInitializerASTNode*)designated->init)->expr;
    if (!LvaluePathIsRepeatable(value)) {
      continue;
    }
    int64_t indices[kArrayElementwiseCopyMaxRank];
    ASTNode* copy = NewArrayElementwiseCopy(value, field->type, indices, 0,
                                            value->location);
    ASTNodeReplaceChild(initializer, 0, copy, true);
  }
}

// Contract: Mutates closure layout/initializer (`FinishAutoLambdaCaptureFields`,
// array capture expansion, template temporary materialization) before
// `AnalyzeInitialization`; sets value category on `node->base`.
void AnalyzeCompoundLiteral(CompoundLiteralASTNode* node) {
  // Template substitution updates the cloned identifier node's type, but the
  // hidden temporary symbol can still carry the class-template pattern type.
  // Initialization reads the symbol type and would otherwise replace a
  // concrete `C<int>{}` literal with dependent `C<T>`.
  if (node->sym != NULL && node->sym->op == AST_OP(identifier) &&
      node->sym->type != NULL) {
    Symbol* temporary = ((IdentifierASTNode*)node->sym)->symbol;
    if (temporary != NULL && temporary->flags.is_temp &&
        temporary->type != node->sym->type) {
      SymbolSetType(temporary, TypeRecordCopy(node->sym->type));
    }
  }
  FinishAutoLambdaCaptureFields(node);
  ExpandLambdaArrayCaptureCopies(node);
  MaterializeBracedClassTemplateTemporary(node);
  node->initializer =
      AnalyzeInitialization(&node->base, node->sym,
                            node->initializer);
  // A C++ lambda-expression is a prvalue that materializes a temporary closure.
  // Keep that category so forwarding-reference deduction of `F&&` / `T&&` works.
  // A temporary materialized for reference binding is an xvalue.  Reanalyzing a
  // cloned template body clears kASTAnalyzed and would otherwise turn that
  // temporary into a C lvalue, which can no longer bind to an rvalue reference.
  if ((node->base.flags &
       (kASTLambdaExpression | kASTCXXBracedTemporary)) != 0) {
    node->base.value_category = kValueCategoryPrvalue;
  } else if (node->base.value_category != kValueCategoryXvalue) {
    node->base.value_category = kValueCategoryLvalue;
  }
}
