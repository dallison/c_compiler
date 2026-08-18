//
//  type_template_clone.c
//  c_compiler
//

#include "type_template_internal.h"
#include "type_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <assert.h>
#include "ast.h"
#include "compiler.h"
#include "concepts.h"
#include "constexpr.h"
#include "dstring.h"
#include "expr_evaluator.h"
#include "expr_parser.h"
#include "expr_semantics.h"
#include "expansion_semantics.h"
#include "statement_semantics.h"
#include "statement_parser.h"
#include "symbol_table.h"
#include "syntax.h"
#include "semantics.h"
#include "errors.h"
#include "debug.h"
#include "member_pointer.h"
#include "rtti.h"
#include "set.h"

ASTNode* CloneTemplateFunctionBodyNode(ASTNode* node, void* data);
static void InstantiateClonedFunctionTemplateCall(TemplateFunctionBodyClone* clone,
                                                  ASTNode* node);
static bool ASTNodeWithinPackExpansion(ASTNode* node);
static bool ASTNodeWithinUnresolvedPackExpansion(TemplateFunctionBodyClone* clone,
                                                 ASTNode* node);
static ASTNode* ReanalyzeClonedDependentFunctorCall(
    ASTNode* node, void* data, ASTNodeTransformAction* action);
static ASTNode* ReanalyzeClonedConcreteMemberCall(
    ASTNode* node, void* data, ASTNodeTransformAction* action);
static ASTNode* ReanalyzeClonedUntypedExpression(
    ASTNode* node, void* data, ASTNodeTransformAction* action);
static ASTNode* RebuildClonedCallResultConversion(
    ASTNode* node, void* data, ASTNodeTransformAction* action);
static bool RebindClonedConstructorCall(VectorASTNode* call,
                                        Vector* overload_snapshots);
static void RestoreSymbolOverloadLinks(Vector* snapshots);
static void DeduceClonedAutoLocalVisitor(ASTNode* node, void* data,
                                         int child_id, VisitorMode mode);
static void RefreshClonedIdentifierTypeVisitor(ASTNode* node, void* data,
                                                int child_id,
                                                VisitorMode mode);
static void RefreshClonedCXXNewMetadataVisitor(ASTNode* node, void* data,
                                                int child_id,
                                                VisitorMode mode);
static void RestoreSymbolOverloadLinks(Vector* snapshots);
static bool RebindClonedConstructorCall(VectorASTNode* call, Vector* snapshots);
static StructMember* FindStructMemberOverloadHead(Struct* owner, String* name);
static void ClearAnalyzedFlagVisitor(ASTNode* node, void* data, int child_id,
                                     VisitorMode mode);
static void AnalyzeFunctionTemplateCallActualsVisitor(ASTNode* node, void* data,
                                                      int child_id,
                                                      VisitorMode mode);
static void AnalyzeInsertedConstructorPreamble(TypeParser* parser,
                                               TypeRecord* from_func,
                                               TypeRecord* func, Vector* body,
                                               size_t inserted_count,
                                               Vector* args);
static void ExpandClonedLocalDirectInitializerPack(
    TemplateFunctionBodyClone* clone, VariableDeclarationASTNode* decl,
    Symbol* replacement);
static void InstantiateClonedFunctionTemplateCallVisitor(ASTNode* node,
                                                           void* data,
                                                           int child_id,
                                                           VisitorMode mode);

ASTNode* CloneCXXDefaultMemberInitializer(ASTNode* initializer) {
  return ASTNodeClone(initializer, IdentityCloneNode, NULL, NULL);
}

void DeleteMappedVector(MapKeyValue* kv) {
  VectorDelete(kv->value.p);
}

static StructMember* FindStructMemberOverloadHead(Struct* owner, String* name) {
  StructMember* fallback = FindStructMember(owner, name);
  if (owner == NULL || name == NULL) {
    return fallback;
  }
  for (size_t i = 0; i < owner->members.length; i++) {
    StructMember* member = owner->members.value.p[i];
    if (member == NULL || member->symbol == NULL ||
        !StringEqual(&member->symbol->name, name->value)) {
      continue;
    }
    if (fallback == NULL) {
      fallback = member;
    }
    if (member->overload_next != NULL) {
      return member;
    }
  }
  return fallback;
}

static void ClearAnalyzedFlagVisitor(ASTNode* node, void* data,
                                     int child_id, VisitorMode mode) {
  (void)data;
  (void)child_id;
  if (mode == kVisitPreChildren && node != NULL) {
    // A compiler-synthesized pointer adjustment with a forced type (e.g. the
    // `(Base*)(this + byteoffset)` receiver of a __vbptr initializer) must keep
    // its analyzed state: re-analysis would recompute `this + offset` as the
    // receiver's own (element-scaled) pointer type and bind subobject members to
    // the wrong offset.  AnalyzeExpression short-circuits on the still-analyzed
    // node, so its (now-cleared) children are never re-analyzed either.
    if ((node->flags & kASTForcedTypeAdjustment) != 0) {
      return;
    }
    node->flags &= ~kASTAnalyzed;
  }
}

static void ClearClonedCastSubtreeFlagVisitor(ASTNode* node, void* data,
                                               int child_id,
                                               VisitorMode mode) {
  (void)data;
  (void)child_id;
  if (mode == kVisitPreChildren && node != NULL &&
      node->op != AST_OP(call)) {
    node->flags &= ~kASTAnalyzed;
  }
}

static void MarkClonedCastForReanalysis(ASTNode* node, void* data,
                                        int child_id, VisitorMode mode) {
  (void)data;
  (void)child_id;
  if (mode != kVisitPostChildren || node == NULL ||
      node->op != AST_OP(cast) || (node->flags & kASTDependentCast) == 0) {
    return;
  }
  CastASTNode* cast = (CastASTNode*)node;
  TypeRecord* target = cast->cast_type;
  if (TypeIsReference(target)) {
    target = target->next;
  }
  if (target == NULL || cast->expr == NULL || cast->expr->type == NULL) {
    node->flags &= ~kASTDependentCast;
    return;
  }
  // A dependent cast was initially analyzed without converting its operand.
  // Once both sides are concrete, every target category (not just class types)
  // must be re-analyzed so arithmetic conversions such as int-to-double are
  // materialized in the cloned tree.
  if (TypeContainsTemplateParameter(target) ||
      TypeContainsTemplateParameter(cast->expr->type)) {
    return;
  }
  // Cast lowering depends on its now-concrete target type. Clear the cast and
  // the dependent arithmetic below it, plus its enclosing expression/statement
  // path, so semantic analysis recomputes the concrete operand widths. Preserve
  // already-lowered calls to avoid repeating overload/template resolution.
  ASTNodeVisit(node, ClearClonedCastSubtreeFlagVisitor, 0, NULL);
  for (ASTNode* current = node; current != NULL; current = current->parent) {
    current->flags &= ~kASTAnalyzed;
  }
  node->flags &= ~kASTDependentCast;
}

static void AnalyzeFunctionTemplateCallActualsVisitor(ASTNode* node, void* data,
                                                      int child_id,
                                                      VisitorMode mode) {
  (void)data;
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL || node->op != AST_OP(call)) {
    return;
  }
  VectorASTNode* call = (VectorASTNode*)node;
  if (call->left == NULL || call->left->op != AST_OP(identifier)) {
    return;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)call->left;
  if (id->symbol == NULL || !id->symbol->flags.is_template ||
      !TypeIsFunction(id->symbol->type) ||
      CallActualsStillContainPackExpansion((ASTNode*)call)) {
    return;
  }
  for (size_t i = 0; call->children != NULL && i < call->children->length; i++) {
    ASTNode* actual = call->children->value.p[i];
    if (actual != NULL && (actual->flags & kASTAnalyzed) == 0) {
      call->children->value.p[i] = AnalyzeExpression(actual);
    }
  }
}

ASTNode* CloneDependentDecltypeNode(ASTNode* node, void* data) {
  bool was_dependent_call =
      node != NULL && (node->flags & kASTDependentFunctorCall) != 0;
  ASTNode* deferred =
      node != NULL && node->type != NULL
          ? node->type->dependent_decltype_expr
          : NULL;
  if (deferred != NULL) {
    node->type->dependent_decltype_expr = NULL;
  }
  ASTNode* result = CloneTemplateFunctionBodyNode(node, data);
  if (result != NULL &&
      (was_dependent_call ||
       (result->flags & kASTDependentFunctorCall) != 0)) {
    result->flags &= ~(kASTDependentFunctorCall | kASTAnalyzed);
    ASTNodeClearType(result);
    if (result->op == AST_OP(call)) {
      VectorASTNode* call = (VectorASTNode*)result;
      if (call->left != NULL) {
        call->left->flags &= ~kASTAnalyzed;
      }
    }
  }
  if (deferred != NULL) {
    node->type->dependent_decltype_expr = deferred;
  }
  return result;
}

/* Re-evaluate a value-dependent non-type template-argument expression (stored
 * unevaluated at parse time, e.g. `!is_integral<It>::value`) against the concrete
 * template arguments `args`.  The expression is cloned with template-parameter
 * substitution (which resolves dependent qualified names like `X<T>::value`
 * against the substituted scope) and then constant-folded.  On success writes the
 * folded value to *out and returns true; returns false when the expression is
 * still value-dependent (some referenced parameter is not yet concrete). */
/* Clone a value-dependent template-argument expression, substituting the
 * concrete actuals `args` into any template parameters they supply.  Parameters
 * not covered by `args` (e.g. a member function template's own parameter while
 * only its enclosing class's arguments are known) are left symbolic so a later,
 * more-concrete substitution can finish them.  The returned node is a fresh,
 * unanalyzed AST (arena-owned identifiers); the caller owns it. */
static ASTNode* FoldDependentStaticMemberReference(TemplateFunctionBodyClone* clone,
                                                   Symbol* symbol,
                                                   ASTNode* node) {
  if (clone == NULL || clone->parser == NULL || clone->args == NULL ||
      symbol == NULL || !CompilerIsCXX()) {
    return NULL;
  }
  // Concrete constants and template parameters are handled by other clone
  // paths; only value-dependent compile-time scalars are re-folded here.
  if (symbol->flags.value_set || symbol->flags.is_template_parameter) {
    return NULL;
  }
  TypeRecord* mtype = symbol->type;
  if (mtype == NULL ||
      (!symbol->flags.is_constexpr && !TypeIsConst(mtype)) ||
      (!TypeIsIntegral(mtype) && !TypeIsFloatingPoint(mtype))) {
    return NULL;
  }
  Struct* sources[2] = {
      clone->parser->template_substitution_source,
      clone->parser->enclosing_template_substitution_source,
  };
  StructMember* member = NULL;
  for (int i = 0; i < 2 && member == NULL; i++) {
    if (sources[i] == NULL) {
      continue;
    }
    StructMember* m = FindStructMember(sources[i], &symbol->name);
    if (m != NULL && m->is_static && !m->is_member_function &&
        m->default_initializer != NULL) {
      member = m;
    }
  }
  if (member == NULL) {
    return NULL;
  }
  ASTNode* init = ConstexprInitializerExpression(member->default_initializer);
  if (init == NULL) {
    return NULL;
  }
  if (TypeIsIntegral(mtype)) {
    int64_t value = 0;
    if (TryFoldDependentTemplateArgument(clone->parser, init, clone->args,
                                         &value)) {
      return NewIntConstantASTNode(value, TypeRecordCopy(mtype), node->location);
    }
    return NULL;
  }
  ASTNode* cloned =
      CloneDependentExpressionWithArgs(clone->parser, init, clone->args);
  if (cloned == NULL) {
    return NULL;
  }
  DiagnosticSuppressBegin();
  cloned = AnalyzeExpression(cloned);
  double dvalue = 0;
  bool ok = EvaluateFloatingPointExpression(cloned, &dvalue);
  DiagnosticSuppressEnd();
  ASTNodeDelete(cloned);
  if (ok) {
    return NewRealConstantASTNode(dvalue, TypeRecordCopy(mtype), node->location);
  }
  return NULL;
}

/* Instantiate a C++ variable template's initializer against concrete template
 * arguments `args` (a Vector of TemplateArgument*, positional by parameter
 * index) and constant-fold it to an integer.  Returns true and writes the value
 * to *out on success.  Used for constant-valued variable templates such as
 * `variant_size_v<T>`. */
static bool CXXRecordHasDefaultConstructorMember(TypeRecord* type) {
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL ||
      type->info.struct_info->tag_name == NULL ||
      type->info.struct_info->is_aggregate) {
    return false;
  }
  StructMember* ctor = FindStructMember(type->info.struct_info,
                                        type->info.struct_info->tag_name);
  for (StructMember* overload = ctor; overload != NULL;
       overload = overload->overload_next) {
    if (!overload->is_member_function || overload->symbol == NULL ||
        overload->symbol->type == NULL ||
        !TypeIsFunction(overload->symbol->type) ||
        !overload->symbol->type->info.function.is_constructor ||
        overload->symbol->type->info.function.is_deleted) {
      continue;
    }
    size_t explicit_parameters = 0;
    for (size_t i = 0;
         i < overload->symbol->type->info.function.prototype.length; i++) {
      Symbol* formal =
          overload->symbol->type->info.function.prototype.value.p[i];
      if (formal == NULL || StringEqual(&formal->name, "this") ||
          StringEqual(&formal->name, "__complete_object")) {
        continue;
      }
      explicit_parameters++;
    }
    if (explicit_parameters == 0) {
      return true;
    }
  }
  return false;
}

/* When a cloned member access (`obj.field`/`obj->field`) has no resolved type
 * yet, infer the member's type by looking the field name up in the enclosing
 * instantiated class (the function's member owner). */
static TypeRecord* InferClonedMemberReceiverType(TemplateFunctionBodyClone* clone,
                                                ASTNode* receiver) {
  if (clone == NULL || clone->to_func == NULL ||
      clone->to_func->info.function.cxx_member_owner == NULL ||
      receiver == NULL ||
      (receiver->op != AST_OP(dot) && receiver->op != AST_OP(arrow))) {
    return NULL;
  }
  BinaryASTNode* member_access = (BinaryASTNode*)receiver;
  if (member_access->right == NULL ||
      member_access->right->op != AST_OP(string)) {
    return NULL;
  }
  ConstantASTNode* member_name = (ConstantASTNode*)member_access->right;
  if (member_name->value.string == NULL) {
    return NULL;
  }
  StructMember* member = FindStructMember(
      clone->to_func->info.function.cxx_member_owner, member_name->value.string);
  if (member == NULL || member->symbol == NULL) {
    return NULL;
  }
  return member->symbol->type;
}

/* Fix up a cloned `receiver.member(...)` call where `member` still carries the
 * generic template's constructor name. After substitution the receiver's class
 * is concrete, so rewrite the member name to that class's constructor name and
 * rebind it to the instantiated member. */
static void RewriteClonedConstructorMemberCall(TemplateFunctionBodyClone* clone,
                                               ASTNode* node) {
  if (node->op != AST_OP(call)) {
    return;
  }
  VectorASTNode* call = (VectorASTNode*)node;
  if (call->left == NULL ||
      (call->left->op != AST_OP(dot) && call->left->op != AST_OP(arrow))) {
    return;
  }
  BinaryASTNode* member_access = (BinaryASTNode*)call->left;
  if (member_access->left == NULL || member_access->right == NULL ||
      member_access->right->op != AST_OP(string)) {
    return;
  }
  TypeRecord* receiver_type = member_access->left->type;
  if (receiver_type == NULL) {
    receiver_type = InferClonedMemberReceiverType(clone, member_access->left);
  }
  const char* constructor_name = CXXConstructorNameForRecord(receiver_type);
  if (constructor_name == NULL) {
    return;
  }
  ConstantASTNode* member_name = (ConstantASTNode*)member_access->right;
  if (member_name->value.string == NULL ||
      StringEqual(member_name->value.string, constructor_name)) {
    return;
  }
  if (TypeIsStructOrUnion(receiver_type) &&
      receiver_type->info.struct_info != NULL) {
    StructMember* existing = FindStructMember(receiver_type->info.struct_info,
                                              member_name->value.string);
    if (existing != NULL) {
      return;
    }
  }
  const char* source_name = member_name->value.string->value;
  const char* origin_name =
      receiver_type->template_origin != NULL
          ? receiver_type->template_origin->name.value
          : NULL;
  size_t origin_length = origin_name != NULL ? strlen(origin_name) : 0;
  if (origin_name == NULL || strncmp(source_name, origin_name, origin_length) != 0 ||
      (source_name[origin_length] != '\0' &&
       source_name[origin_length] != '<' &&
       source_name[origin_length] != '#')) {
    return;
  }
  StringSet(member_name->value.string, constructor_name);
}

/* Visitor: rewrite an identifier referencing an original symbol to point at
 * its cloned replacement (from `symbol_map`) and refresh its node type. */
static void RewriteTemplateBodyIdentifierVisitor(ASTNode* node, void* data,
                                                 int child_id,
                                                 VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node->op != AST_OP(identifier)) {
    return;
  }
  Map* symbol_map = data;
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  Symbol* replacement = MapFindPointerKey(symbol_map, id->symbol);
  if (replacement != NULL) {
    id->symbol = replacement;
    ASTNodeSetType(node, replacement->type);
  }
}

/* Rewrite all identifiers in `node` to their cloned symbols via `symbol_map`. */
void RewriteTemplateBodyIdentifiers(ASTNode* node, Map* symbol_map) {
  ASTNodeVisit(node, RewriteTemplateBodyIdentifierVisitor, 0, symbol_map);
}

/* Clone the symbol declared by a local `vardecl` in a template body: substitute
 * its type against the instantiation args, create a replacement symbol, record
 * the original->clone mapping, and point the declaration (and its initializer's
 * identifiers) at the new symbol. */
/* The owning C++ class of a member function type, whether static (recorded in
 * cxx_member_owner) or non-static (reached through the implicit `this`
 * parameter's pointee).  Returns NULL for non-member functions. */
static struct Struct* CloneFunctionMemberOwner(TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func)) {
    return NULL;
  }
  FunctionInfo* info = &func->info.function;
  if (info->cxx_member_owner != NULL) {
    return info->cxx_member_owner;
  }
  if (info->prototype.length > 0) {
    Symbol* this_sym = info->prototype.value.p[0];
    if (this_sym != NULL && this_sym->type != NULL &&
        StringEqual(&this_sym->name, "this") && this_sym->type->next != NULL &&
        TypeIsStructOrUnion(this_sym->type->next) &&
        this_sym->type->next->info.struct_info != NULL) {
      return this_sym->type->next->info.struct_info;
    }
  }
  return NULL;
}

static void AddOwnerMemberSymbolMappings(TemplateFunctionBodyClone* clone) {
  if (clone == NULL || clone->from_owner == NULL || clone->to_owner == NULL ||
      clone->from_owner == clone->to_owner) {
    return;
  }
  for (size_t i = 0; i < clone->from_owner->members.length; i++) {
    StructMember* from_member = clone->from_owner->members.value.p[i];
    if (from_member == NULL || from_member->symbol == NULL ||
        from_member->is_member_function) {
      continue;
    }
    StructMember* to_member =
        FindStructMemberByName(clone->to_owner, from_member->symbol->name.value);
    if (to_member == NULL || to_member->symbol == NULL) {
      continue;
    }
    MapKeyValue kv;
    kv.key.p = from_member->symbol;
    kv.value.p = to_member->symbol;
    MapInsert(&clone->symbol_map, kv);
  }
}

// True if any node in `type`'s declarator chain (the type itself, or a
// pointed-to / referenced / element type) is the class `str`.  Used to detect
// uses of the injected-class-name -- e.g. `static_cast<ClassName&&>(...)` or a
// local `ClassName tmp(...)` -- whose type names the *generic* primary-template
// struct directly and so contains no template *parameter*; such types must
// still be remapped to the current instantiation during member-body cloning,
// otherwise a cast to the generic self-type forces a spurious temporary built
// with the generic (unemitted) copy constructor.
static bool TypeChainReferencesStruct(TypeRecord* type, struct Struct* str) {
  if (str == NULL) {
    return false;
  }
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (TypeIsStructOrUnion(t) && t->info.struct_info == str) {
      return true;
    }
  }
  return false;
}

/* Substitute a type in a nested class-template member body.  Besides the
 * nested class itself, such a body can name the injected class name of its
 * enclosing specialization (for example `outer result;` inside
 * `outer<T>::promise_type`).  The normal source/target mapping covers the
 * nested class; temporarily use its lexical-parent mapping when this exact
 * type chain names the enclosing class. */
static TypeRecord* SubstituteTemplateBodyType(TemplateFunctionBodyClone* clone,
                                              TypeRecord* type) {
  Struct* source =
      clone->from_owner != NULL ? clone->from_owner->lexical_parent : NULL;
  Struct* target =
      clone->to_owner != NULL ? clone->to_owner->lexical_parent : NULL;
  if (source == NULL || target == NULL || source == target ||
      !TypeChainReferencesStruct(type, source)) {
    TypeSubstitutionScope substitution = TypeParserPushTemplateSubstitution(
        clone->parser, clone->substitution_source,
        clone->substitution_target);
    TypeRecord* substituted =
        SubstituteTemplateParameters(clone->parser, type, clone->args);
    TypeParserPopTemplateSubstitution(&substitution);
    return substituted;
  }

  TypeSubstitutionScope substitution =
      TypeParserPushTemplateSubstitution(clone->parser, source, target);
  TypeRecord* substituted =
      SubstituteTemplateParameters(clone->parser, type, clone->args);
  TypeParserPopTemplateSubstitution(&substitution);
  return substituted;
}

static bool ResolveClonedTemplateTemplateParameterCTAD(
    TemplateFunctionBodyClone* clone, VariableDeclarationASTNode* decl,
    Symbol* original, Symbol* replacement) {
  if (clone == NULL || decl == NULL || replacement == NULL ||
      replacement->type == NULL ||
      !TypeIsClassTemplatePlaceholder(replacement->type)) {
    return true;
  }
  if (decl->initializer == NULL ||
      decl->initializer->op != AST_OP(braced_init)) {
    SyntaxError(clone->parser->syntax,
                "Could not deduce template arguments for %s",
                replacement->name.value);
    return false;
  }

  BracedInitializerASTNode* braced =
      (BracedInitializerASTNode*)decl->initializer;
  Vector actuals;
  VectorInit(&actuals);
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* initializer = braced->initializers->value.p[i];
    ASTNode* expression = initializer;
    if (initializer != NULL && initializer->op == AST_OP(expr_init)) {
      ExpressionInitializerASTNode* expr_init =
          (ExpressionInitializerASTNode*)initializer;
      expr_init->expr = AnalyzeExpression(expr_init->expr);
      expression = expr_init->expr;
    } else if (initializer != NULL) {
      expression = AnalyzeExpression(initializer);
      VectorSet(braced->initializers, i, expression);
    }
    if (expression != NULL) {
      VectorAppend(&actuals, expression);
    }
  }

  bool alias_rejected = false;
  TypeRecord* deduced = TypeDeduceClassTemplateFromPlaceholder(
      clone->parser->syntax, replacement->type, &actuals,
      /*allow_explicit=*/true, &alias_rejected);
  VectorDestruct(&actuals);
  Symbol* template_parameter =
      original != NULL && original->type != NULL
              && original->type->template_origin != NULL
              && original->type->template_origin->flags
                     .is_template_template_parameter
          ? original->type->template_origin
          : NULL;
  if (deduced != NULL && template_parameter != NULL &&
      !TypeTemplateTemplateParameterAcceptsDeduced(template_parameter,
                                                   deduced)) {
    Symbol* argument_template =
        TypeClassTemplatePlaceholderOrigin(replacement->type);
    TypeRecord* adjusted = TypeTemplateTemplateParameterApplyDefaults(
        clone->parser, template_parameter, argument_template, deduced);
    TypeRecordDelete(deduced);
    deduced = adjusted;
  }
  bool accepted =
      deduced != NULL &&
      TypeClassTemplatePlaceholderAcceptsDeduced(replacement->type, deduced) &&
      (template_parameter == NULL ||
       TypeTemplateTemplateParameterAcceptsDeduced(template_parameter, deduced));
  if (!accepted) {
    if (deduced != NULL || alias_rejected) {
      SyntaxError(clone->parser->syntax,
                  "Deduced template arguments do not match template template "
                  "parameter");
    } else {
      SyntaxError(clone->parser->syntax,
                  "Could not deduce template arguments for %s",
                  replacement->name.value);
    }
    TypeRecordDelete(deduced);
    return false;
  }
  SymbolSetType(replacement, deduced);
  TypeRecordDelete(deduced);
  return true;
}

/* A direct initializer parsed while a local's type was dependent cannot be
 * classified as scalar initialization or a constructor call. Keep its
 * parenthesized arguments in a braced-initializer container until substitution,
 * then rebuild the concrete form before semantic analysis/code generation. */
static bool RewriteClonedConcreteLocalDirectInitializer(
    TemplateFunctionBodyClone* clone, VariableDeclarationASTNode* decl,
    Symbol* original, Symbol* replacement) {
  if (clone == NULL || decl == NULL || original == NULL ||
      replacement == NULL || original->type == NULL ||
      replacement->type == NULL ||
      !TypeContainsTemplateParameter(original->type) ||
      decl->initializer == NULL ||
      decl->initializer->op != AST_OP(braced_init)) {
    return false;
  }

  ASTNode* root = decl->initializer;
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)root;
  if (!TypeIsStructOrUnion(replacement->type)) {
    if (braced->initializers->length != 1) {
      return false;
    }
    ASTNode* initializer = braced->initializers->value.p[0];
    ASTNode* expression = initializer;
    if (initializer != NULL && initializer->op == AST_OP(expr_init)) {
      ExpressionInitializerASTNode* expr_init =
          (ExpressionInitializerASTNode*)initializer;
      expression = expr_init->expr;
      expr_init->expr = NULL;
    }
    braced->initializers->value.p[0] = NULL;
    ASTNode* parent = root->parent;
    int child_id = root->child_id;
    SourceLocation location = root->location;
    if (expression != NULL) {
      expression->parent = NULL;
    }
    if (initializer != expression) {
      ASTNodeDelete(initializer);
    }
    ASTNodeDelete(root);
    if (TypeIsReference(replacement->type) && expression != NULL) {
      expression->parent = parent;
      expression->child_id = child_id;
      decl->initializer = expression;
      return true;
    }
    ASTNode* target = NewIdentifierASTNode(replacement, location);
    target->flags |= kASTNeedAddress | kASTIsDeclaration;
    decl->initializer =
        NewBinaryASTNode(AST_OP(init), TypeRecordCopy(replacement->type),
                         location, target, expression);
    decl->initializer->parent = parent;
    decl->initializer->child_id = child_id;
    return true;
  }

  Vector* actuals = NewVector();
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* initializer = braced->initializers->value.p[i];
    ASTNode* actual = initializer;
    if (initializer != NULL && initializer->op == AST_OP(expr_init)) {
      ExpressionInitializerASTNode* expr_init =
          (ExpressionInitializerASTNode*)initializer;
      actual = expr_init->expr;
      expr_init->expr = NULL;
    }
    braced->initializers->value.p[i] = NULL;
    if (actual != NULL) {
      actual->parent = NULL;
      VectorAppend(actuals, actual);
    }
    if (initializer != actual) {
      ASTNodeDelete(initializer);
    }
  }

  ASTNode* parent = root->parent;
  int child_id = root->child_id;
  SourceLocation location = root->location;
  ASTNodeDelete(root);
  decl->initializer = SyntaxNewCXXConstructorCall(
      clone->parser->syntax, replacement, actuals, location);
  if (decl->initializer != NULL) {
    decl->initializer->parent = parent;
    decl->initializer->child_id = child_id;
  }
  return true;
}

static void CloneTemplateLocalDeclarationSymbol(TemplateFunctionBodyClone* clone,
                                                ASTNode* node) {
  if (node->op != AST_OP(vardecl)) {
    return;
  }
  VariableDeclarationASTNode* decl = (VariableDeclarationASTNode*)node;
  Symbol* old_symbol = decl->symbol;
  if (old_symbol == NULL || old_symbol->flags.is_argument) {
    return;
  }
  Symbol* existing = MapFindPointerKey(&clone->symbol_map, old_symbol);
  if (existing != NULL) {
    decl->symbol = existing;
    RewriteTemplateBodyIdentifiers(decl->initializer, &clone->symbol_map);
    ExpandClonedLocalDirectInitializerPack(clone, decl, existing);
    if (RewriteClonedConcreteLocalDirectInitializer(
            clone, decl, old_symbol, existing)) {
      node->flags &= ~kASTAnalyzed;
    }
    ASTNode* rewritten_initializer =
        SyntaxRewriteCXXCopyInitConstructorIfNeeded(
            clone->parser->syntax, existing, decl->initializer);
    if (rewritten_initializer != decl->initializer) {
      decl->initializer = rewritten_initializer;
      if (decl->initializer != NULL) {
        decl->initializer->parent = node;
        decl->initializer->child_id = 0;
      }
      node->flags &= ~kASTAnalyzed;
    }
    if (decl->initializer == NULL) {
      decl->initializer = SyntaxNewCXXDefaultConstructorCallIfNeeded(
          clone->parser->syntax, existing);
      if (decl->initializer != NULL) {
        decl->initializer->parent = node;
        decl->initializer->child_id = 0;
        node->flags &= ~kASTAnalyzed;
      }
    }
    if (TypeContainsAuto(existing->type) && decl->initializer != NULL) {
      ASTNodeVisit(decl->initializer, ClearAnalyzedFlagVisitor, 0, NULL);
      node->flags &= ~kASTAnalyzed;
    }
    return;
  }

  TypeRecord* type =
      SubstituteTemplateBodyType(clone, old_symbol->type);
  RebaseTemplateParameterIndices(type, clone->rebase_template_parameter_base);
  Symbol* replacement =
      NewSymbol(old_symbol->name.value, type, old_symbol->storage);
  replacement->flags = old_symbol->flags;
  replacement->location = old_symbol->location;
  replacement->alignment = old_symbol->alignment;
  replacement->namespace_ = old_symbol->namespace_;
  replacement->value = old_symbol->value;
  replacement->structured_binding_pack_size =
      old_symbol->structured_binding_pack_size;
  AttributeListDestruct(&replacement->attributes);
  AttributeListClone(&replacement->attributes, &old_symbol->attributes);

  MapKeyValue kv;
  kv.key.p = old_symbol;
  kv.value.p = replacement;
  MapInsert(&clone->symbol_map, kv);
  decl->symbol = replacement;
  RewriteTemplateBodyIdentifiers(decl->initializer, &clone->symbol_map);
  bool ctad_ok = ResolveClonedTemplateTemplateParameterCTAD(
      clone, decl, old_symbol, replacement);
  ASTNodeSetType(node, replacement->type);
  if (!ctad_ok) {
    node->flags &= ~kASTAnalyzed;
    return;
  }
  ExpandClonedLocalDirectInitializerPack(clone, decl, replacement);
  if (RewriteClonedConcreteLocalDirectInitializer(
          clone, decl, old_symbol, replacement)) {
    node->flags &= ~kASTAnalyzed;
  }
  ASTNode* rewritten_initializer =
      SyntaxRewriteCXXCopyInitConstructorIfNeeded(
          clone->parser->syntax, replacement, decl->initializer);
  if (rewritten_initializer != decl->initializer) {
    decl->initializer = rewritten_initializer;
    if (decl->initializer != NULL) {
      decl->initializer->parent = node;
      decl->initializer->child_id = 0;
    }
    node->flags &= ~kASTAnalyzed;
  }
  if (decl->initializer == NULL) {
    decl->initializer = SyntaxNewCXXDefaultConstructorCallIfNeeded(
        clone->parser->syntax, replacement);
    if (decl->initializer != NULL) {
      decl->initializer->parent = node;
      decl->initializer->child_id = 0;
      node->flags &= ~kASTAnalyzed;
    }
  }
  if (TypeContainsAuto(replacement->type) && decl->initializer != NULL) {
    // The primary template could not deduce this local while its initializer
    // was dependent. Re-analyze the concrete cloned initializer so ordinary
    // auto deduction runs for the instantiated function body.
    ASTNodeVisit(decl->initializer, ClearAnalyzedFlagVisitor, 0, NULL);
    node->flags &= ~kASTAnalyzed;
  }
}

/* Clone a compiler temporary whose type was template-dependent: substitute its
 * type and register the original->clone mapping (memoized in symbol_map). */
static Symbol* CloneTemplateDependentTemporarySymbol(
    TemplateFunctionBodyClone* clone, Symbol* old_symbol) {
  if (old_symbol == NULL || !old_symbol->flags.is_temp ||
      old_symbol->type == NULL) {
    return NULL;
  }
  Symbol* existing = MapFindPointerKey(&clone->symbol_map, old_symbol);
  if (existing != NULL) {
    return existing;
  }
  TypeRecord* type =
      SubstituteTemplateBodyType(clone, old_symbol->type);
  RebaseTemplateParameterIndices(type, clone->rebase_template_parameter_base);
  Symbol* replacement =
      NewSymbol(old_symbol->name.value, type, old_symbol->storage);
  replacement->flags = old_symbol->flags;
  replacement->location = old_symbol->location;
  replacement->alignment = old_symbol->alignment;
  replacement->namespace_ = old_symbol->namespace_;
  replacement->value = old_symbol->value;
  replacement->structured_binding_pack_size =
      old_symbol->structured_binding_pack_size;

  MapKeyValue kv;
  kv.key.p = old_symbol;
  kv.value.p = replacement;
  MapInsert(&clone->symbol_map, kv);
  return replacement;
}

static void RetargetClonedNestedConstructorReceiver(
    TemplateFunctionBodyClone* clone, VectorASTNode* call,
    Struct* source_nested_owner) {
  if (clone == NULL || call == NULL || source_nested_owner == NULL ||
      source_nested_owner->lexical_parent == NULL ||
      source_nested_owner->tag_name == NULL ||
      clone->substitution_source !=
          source_nested_owner->lexical_parent ||
      clone->substitution_target == NULL ||
      call->children == NULL || call->children->length == 0) {
    return;
  }
  StructMember* nested_member = FindStructMember(
      clone->substitution_target,
      source_nested_owner->tag_name);
  if (nested_member == NULL || nested_member->symbol == NULL ||
      nested_member->symbol->type == NULL ||
      !TypeIsStructOrUnion(nested_member->symbol->type)) {
    return;
  }
  ASTNode* receiver = call->children->value.p[0];
  if (receiver == NULL || receiver->op != AST_OP(address)) {
    return;
  }
  ASTNode* object = ((UnaryASTNode*)receiver)->sub;
  if (object == NULL || object->op != AST_OP(identifier)) {
    return;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)object;
  if (id->symbol == NULL) {
    return;
  }

  Struct* target_nested_owner =
      nested_member->symbol->type->info.struct_info;
  if (id->symbol->flags.is_temp) {
    TypeSubstitutionScope substitution = TypeParserPushTemplateSubstitution(
        clone->parser, source_nested_owner, target_nested_owner);
    Symbol* replacement =
        CloneTemplateDependentTemporarySymbol(clone, id->symbol);
    TypeParserPopTemplateSubstitution(&substitution);
    if (replacement != NULL) {
      id->symbol = replacement;
    }
  }
  ASTNodeSetType(object, nested_member->symbol->type);
  TypeRecord* pointer =
      NewPointerTo(kQualPlain, TypeRecordCopy(nested_member->symbol->type));
  ASTNodeSetType(receiver, pointer);
  TypeRecordDelete(pointer);
}

/* Like SubstituteTemplateArgumentVectorForTypes but additionally rebases the
 * parameter indices of the produced arguments by `rebase_base` (for nested /
 * member template instantiations). */
static bool IsIdentifierPackExpansion(ASTNode* node) {
  if (node == NULL || (node->flags & kASTPackExpansion) == 0 ||
      node->op != AST_OP(identifier)) {
    return false;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  return id->symbol != NULL && id->symbol->flags.is_parameter_pack;
}

typedef struct {
  TemplateFunctionBodyClone* clone;
  Symbol* symbol;
  bool multiple_packs;
} PackExpansionExpressionSearch;

static void FindPackExpansionExpressionSymbol(ASTNode* node, void* data,
                                              int child_id,
                                              VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(identifier)) {
    return;
  }
  PackExpansionExpressionSearch* search = data;
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  if (id->symbol == NULL || !id->symbol->flags.is_parameter_pack ||
      (MapFindPointerKey(&search->clone->pack_symbol_map, id->symbol) == NULL &&
       MapFindPointerKey(&search->clone->symbol_map, id->symbol) == NULL)) {
    return;
  }
  if (search->symbol != NULL && search->symbol != id->symbol) {
    Vector* first = MapFindPointerKey(&search->clone->pack_symbol_map,
                                      search->symbol);
    Vector* next =
        MapFindPointerKey(&search->clone->pack_symbol_map, id->symbol);
    if (first == NULL || next == NULL || first->length != next->length) {
      search->multiple_packs = true;
    }
    return;
  }
  search->symbol = id->symbol;
}

/* Find a parameter-pack symbol referenced inside a pack-expansion pattern.
 * Multiple function packs are supported when their concrete lengths match;
 * `multiple_packs` reports a length mismatch. */
static Symbol* PackExpansionExpressionSymbol(TemplateFunctionBodyClone* clone,
                                             ASTNode* node,
                                             bool* multiple_packs) {
  PackExpansionExpressionSearch search = {0};
  search.clone = clone;
  ASTNodeVisit(node, FindPackExpansionExpressionSymbol, 0, &search);
  if (multiple_packs != NULL) {
    *multiple_packs = search.multiple_packs;
  }
  return search.symbol;
}

typedef struct {
  TemplateFunctionBodyClone* clone;
  Vector symbols;
} PackExpansionFunctionPacks;

static void CollectPackExpansionFunctionPacks(ASTNode* node, void* data,
                                              int child_id,
                                              VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(identifier)) {
    return;
  }
  PackExpansionFunctionPacks* packs = data;
  Symbol* symbol = ((IdentifierASTNode*)node)->symbol;
  if (symbol == NULL || !symbol->flags.is_parameter_pack ||
      MapFindPointerKey(&packs->clone->pack_symbol_map, symbol) == NULL) {
    return;
  }
  for (size_t i = 0; i < packs->symbols.length; i++) {
    if (packs->symbols.value.p[i] == symbol) {
      return;
    }
  }
  VectorAppend(&packs->symbols, symbol);
}

typedef struct {
  TemplateFunctionBodyClone* clone;
  int pack_index;
  size_t pack_length;
  bool multiple_packs;
} TemplateArgumentPackExpressionSearch;

static void FindTemplateArgumentPackExpression(ASTNode* node, void* data,
                                               int child_id,
                                               VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(identifier)) {
    return;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  TemplateArgumentPackExpressionSearch* search = data;
  for (size_t i = 0; id->template_arguments != NULL &&
                     i < id->template_arguments->length; i++) {
    int pack_index = -1;
    size_t pack_length = 0;
    if (!FindPackExpansionInTemplateArgument(
            id->template_arguments->value.p[i], search->clone->args,
            &pack_index, &pack_length)) {
      continue;
    }
    if (search->pack_index >= 0 && search->pack_index != pack_index) {
      if (search->pack_length != pack_length) {
        search->multiple_packs = true;
      }
      continue;
    }
    search->pack_index = pack_index;
    search->pack_length = pack_length;
  }
}

static bool PackExpansionTemplateArgumentInfo(
    TemplateFunctionBodyClone* clone, ASTNode* node, size_t* pack_length,
    bool* multiple_packs) {
  TemplateArgumentPackExpressionSearch search = {
      .clone = clone,
      .pack_index = -1,
      .pack_length = 0,
      .multiple_packs = false,
  };
  ASTNodeVisit(node, FindTemplateArgumentPackExpression, 0, &search);
  if (pack_length != NULL) {
    *pack_length = search.pack_length;
  }
  if (multiple_packs != NULL) {
    *multiple_packs = search.multiple_packs;
  }
  return search.pack_index >= 0;
}

typedef struct {
  TemplateFunctionBodyClone* clone;
  bool found;
} UnresolvedPackSearch;

static void FindUnresolvedPackIdentifier(ASTNode* node, void* data,
                                         int child_id, VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(identifier)) {
    return;
  }
  UnresolvedPackSearch* search = data;
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  if (id->symbol != NULL && id->symbol->flags.is_parameter_pack &&
      MapFindPointerKey(&search->clone->pack_symbol_map, id->symbol) == NULL) {
    search->found = true;
  }
}

/* True if `node` (a pack-expansion pattern) still references a parameter pack
 * that this clone does not resolve: its pack symbol is not in the clone's
 * pack_symbol_map, so it belongs to a not-yet-instantiated nested/member
 * template (e.g. a variadic member template's pack while its enclosing class
 * template is being instantiated). Such a pattern must retain its
 * kASTPackExpansion marker for the later member-template instantiation. */
static bool ClonePatternReferencesUnresolvedPack(TemplateFunctionBodyClone* clone,
                                                 ASTNode* node) {
  UnresolvedPackSearch search;
  search.clone = clone;
  search.found = false;
  ASTNodeVisit(node, FindUnresolvedPackIdentifier, 0, &search);
  return search.found;
}

/* If `node` is (optionally cast/unary-wrapped) member access
 * `recv.name`/`recv->name` naming a captured field, return that access node;
 * otherwise NULL. Used to expand lambda capture packs stored as synthetic
 * struct members.  Patterns like `static_cast<T&&>(xs)...` must peel the cast
 * so the captured pack field can still be found and expanded. */
static ASTNode* LambdaCapturePackMemberAccess(ASTNode* node) {
  while (node != NULL) {
    if (node->op == AST_OP(cast)) {
      node = ((CastASTNode*)node)->expr;
      continue;
    }
    if (node->op == AST_OP(contents) || node->op == AST_OP(address) ||
        node->op == AST_OP(plus) || node->op == AST_OP(minus) ||
        node->op == AST_OP(not)) {
      node = ((UnaryASTNode*)node)->sub;
      continue;
    }
    break;
  }
  if (node == NULL || (node->op != AST_OP(arrow) &&
                       node->op != AST_OP(dot))) {
    return NULL;
  }
  BinaryASTNode* access = (BinaryASTNode*)node;
  if (access->right == NULL ||
      (access->right->op != AST_OP(string) &&
       access->right->op != AST_OP(structmember))) {
    return NULL;
  }
  return node;
}

/* Return the field name of a lambda-capture-pack member access, or NULL. */
static const char* LambdaCapturePackMemberName(ASTNode* node) {
  ASTNode* access_node = LambdaCapturePackMemberAccess(node);
  if (access_node == NULL) {
    return NULL;
  }
  BinaryASTNode* access = (BinaryASTNode*)access_node;
  if (access->right->op == AST_OP(string)) {
    ConstantASTNode* name = (ConstantASTNode*)access->right;
    return name->value.string != NULL ? name->value.string->value : NULL;
  }
  StructMemberASTNode* member = (StructMemberASTNode*)access->right;
  return member->member != NULL && member->member->symbol != NULL
             ? member->member->symbol->name.value
             : NULL;
}

/* Return the closure struct on the receiver side of a lambda-capture-pack
 * member access (looking through a pointer), or NULL.  When the access has not
 * yet been typed (common while cloning a body before re-analysis), fall back to
 * `fallback_owner` -- typically the call operator's cxx_member_owner, i.e. the
 * closure whose pack field was expanded into synthetic `$packN` elements. */
static Struct* LambdaCapturePackReceiverStruct(ASTNode* node,
                                               Struct* fallback_owner) {
  ASTNode* access_node = LambdaCapturePackMemberAccess(node);
  if (access_node == NULL) {
    return NULL;
  }
  BinaryASTNode* access = (BinaryASTNode*)access_node;
  TypeRecord* receiver_type = access->left != NULL ? access->left->type : NULL;
  if (receiver_type != NULL && TypeIsPointer(receiver_type)) {
    receiver_type = receiver_type->next;
  }
  if (receiver_type != NULL && TypeIsStructOrUnion(receiver_type)) {
    return receiver_type->info.struct_info;
  }
  if (access->left != NULL && access->left->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)access->left;
    if (id->symbol != NULL && id->symbol->type != NULL) {
      TypeRecord* sym_type = id->symbol->type;
      if (TypeIsPointer(sym_type)) {
        sym_type = sym_type->next;
      }
      if (sym_type != NULL && TypeIsStructOrUnion(sym_type)) {
        return sym_type->info.struct_info;
      }
    }
  }
  return fallback_owner;
}

/* Return the synthetic per-element fields (`base$pack0`, `base$pack1`, ...) of
 * a captured pack member, in declaration order, for expanding `field...`. */
static Vector* LambdaCapturePackFieldReplacements(ASTNode* node,
                                                  Struct* fallback_owner) {
  const char* base_name = LambdaCapturePackMemberName(node);
  Struct* receiver = LambdaCapturePackReceiverStruct(node, fallback_owner);
  if (base_name == NULL || receiver == NULL) {
    return NULL;
  }
  Vector* replacements = NewVector();
  for (size_t i = 0; i < receiver->members.length; i++) {
    StructMember* member = receiver->members.value.p[i];
    if (member == NULL || member->symbol == NULL) {
      continue;
    }
    if (LambdaCapturePackElementMatches(member->symbol->name.value,
                                        base_name)) {
      VectorAppend(replacements, member);
    }
  }
  return replacements;
}

/* Determine how many elements a captured pack field expands to, from the
 * length of the corresponding template argument pack. */
static bool LambdaCapturePackFieldArgumentLength(ASTNode* node, Vector* args,
                                                 size_t* length,
                                                 Struct* fallback_owner) {
  const char* base_name = LambdaCapturePackMemberName(node);
  Struct* receiver = LambdaCapturePackReceiverStruct(node, fallback_owner);
  if (base_name == NULL || receiver == NULL || length == NULL) {
    return false;
  }
  String name;
  StringInit(&name, base_name);
  StructMember* member = FindStructMember(receiver, &name);
  StringDestruct(&name);
  if (member == NULL || member->symbol == NULL ||
      !member->symbol->flags.is_parameter_pack) {
    return false;
  }
  int pack_index = FirstTemplateParameterIndexInType(member->symbol->type);
  if (pack_index < 0 || args == NULL || (size_t)pack_index >= args->length) {
    return false;
  }
  TemplateArgument* pack = args->value.p[pack_index];
  if (pack == NULL || pack->pack_arguments == NULL) {
    return false;
  }
  *length = pack->pack_arguments->length;
  return true;
}

/* Parameters for replacing, within one expanded pack element, references to a
 * pack symbol `from` with the element-specific symbol `to` (or pack-mapped
 * symbol at `element_index`). */
typedef struct {
  TemplateFunctionBodyClone* clone;
  Symbol* from;
  Symbol* to;
  size_t element_index;
} ReplacePackIdentifierData;

static Symbol* PackSourceForReplacement(TemplateFunctionBodyClone* clone,
                                        Symbol* replacement);

static void InstantiateClonedFunctionTemplateCallVisitor(ASTNode* node,
                                                        void* data,
                                                        int child_id,
                                                        VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPostChildren || node == NULL) {
    return;
  }
  InstantiateClonedFunctionTemplateCall(data, node);
}

/* Visitor that materializes one element of a pack expansion in a cloned body:
 * clears the pack-expansion flag, substitutes any per-identifier template
 * arguments for this element, rewrites pack-symbol references to the element's
 * symbol, and substitutes pack-dependent cast types. Invoked once per element
 * index while expanding `pattern...`. */
static void ReplacePackIdentifierVisitor(ASTNode* node, void* data,
                                         int child_id, VisitorMode mode) {
  (void)child_id;
  if (node != NULL) {
    node->flags &= ~kASTPackExpansion;
  }
  if (mode != kVisitPreChildren || node == NULL) {
    return;
  }
  ReplacePackIdentifierData* replace = data;
  if (node->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node;
    bool repaired_direct_pack_template_argument = false;
    if (replace->clone != NULL && id->template_arguments != NULL &&
        id->template_arguments->length == 1 && node->parent != NULL &&
        node->parent->op == AST_OP(call) &&
        ((VectorASTNode*)node->parent)->left == node) {
      VectorASTNode* parent_call = (VectorASTNode*)node->parent;
      ASTNode* first_actual =
          parent_call->children != NULL && parent_call->children->length > 0
              ? parent_call->children->value.p[0]
              : NULL;
      Symbol* actual_pack =
          first_actual != NULL && first_actual->op == AST_OP(identifier)
              ? ((IdentifierASTNode*)first_actual)->symbol
              : NULL;
      Symbol* actual_pack_source =
          PackSourceForReplacement(replace->clone, actual_pack);
      int pack_index =
          replace->from != NULL
              ? FirstTemplateParameterIndexInType(replace->from->type)
              : -1;
      if (pack_index >= (int)replace->clone->args->length &&
          replace->clone->rebase_template_parameter_base > 0) {
        pack_index -= replace->clone->rebase_template_parameter_base;
      }
      TemplateArgument* pack =
          (actual_pack == replace->from ||
           actual_pack_source == replace->from) &&
                  pack_index >= 0 &&
                  (size_t)pack_index < replace->clone->args->length
              ? replace->clone->args->value.p[pack_index]
              : NULL;
      TemplateArgument* element =
          pack != NULL && pack->pack_arguments != NULL &&
                  replace->element_index < pack->pack_arguments->length
              ? pack->pack_arguments->value.p[replace->element_index]
              : NULL;
      if (element != NULL) {
        VectorDeleteWithContents(
            id->template_arguments,
            (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
        id->template_arguments = NewVector();
        VectorAppend(id->template_arguments, TemplateArgumentCopy(element));
        repaired_direct_pack_template_argument = true;
      }
    }
    if (replace->clone != NULL && id->template_arguments != NULL &&
        !repaired_direct_pack_template_argument) {
      bool substituted_template_args = false;
      int pack_index = -1;
      size_t pack_length = 0;
      Vector* template_args_for_substitution = id->template_arguments;
      Vector* rebased_template_args = NULL;
      int substitution_rebase_base =
          replace->clone->rebase_template_parameter_base;
      for (size_t i = 0; i < id->template_arguments->length; i++) {
        if (FindPackExpansionInTemplateArgument(
                id->template_arguments->value.p[i], replace->clone->args,
                &pack_index, &pack_length)) {
          break;
        }
      }
      if (pack_index < 0 &&
          replace->clone->rebase_template_parameter_base > 0) {
        rebased_template_args =
            TemplateArgumentVectorCopy(id->template_arguments);
        for (size_t i = 0; i < rebased_template_args->length; i++) {
          RebaseTemplateArgumentParameterIndices(
              rebased_template_args->value.p[i],
              replace->clone->rebase_template_parameter_base);
        }
        for (size_t i = 0; i < rebased_template_args->length; i++) {
          if (FindPackExpansionInTemplateArgument(
                  rebased_template_args->value.p[i], replace->clone->args,
                  &pack_index, &pack_length)) {
            template_args_for_substitution = rebased_template_args;
            substitution_rebase_base = 0;
            break;
          }
        }
      }
      if (pack_index >= 0 && (size_t)pack_index < replace->clone->args->length) {
        TemplateArgument* pack = replace->clone->args->value.p[pack_index];
        if (pack != NULL && pack->pack_arguments != NULL &&
            replace->element_index < pack->pack_arguments->length) {
          Vector* element_args = TemplateArgumentVectorCopyWithPackElement(
              replace->clone->args, pack_index,
              pack->pack_arguments->value.p[replace->element_index]);
          Vector* concrete_args = SubstituteTemplateArgumentVector(
              replace->clone->parser, template_args_for_substitution,
              element_args, substitution_rebase_base);
          VectorDeleteWithContents(
              id->template_arguments,
              (VectorElementDestructor)TemplateArgumentDelete,
              /*free_element=*/false);
          id->template_arguments = concrete_args;
          if (id->symbol != NULL && TypeIsFunction(id->symbol->type) &&
              id->symbol->type->info.function.template_origin != NULL) {
            id->symbol = id->symbol->type->info.function.template_origin;
          }
          VectorDeleteWithContents(
              element_args, (VectorElementDestructor)TemplateArgumentDelete,
              /*free_element=*/false);
          substituted_template_args = true;
        }
      }
      if (rebased_template_args != NULL) {
        VectorDeleteWithContents(
            rebased_template_args,
            (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
      }
      if (!substituted_template_args) {
        Vector* concrete_args = SubstituteTemplateArgumentVector(
            replace->clone->parser, id->template_arguments,
            replace->clone->args,
            replace->clone->rebase_template_parameter_base);
        VectorDeleteWithContents(
            id->template_arguments,
            (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
        id->template_arguments = concrete_args;
      }
    }
    if (id->symbol != NULL && TypeIsFunction(id->symbol->type) &&
        id->symbol->type->info.function.template_origin != NULL &&
        node->parent != NULL && node->parent->op == AST_OP(call) &&
        ((VectorASTNode*)node->parent)->left == node) {
      id->symbol = id->symbol->type->info.function.template_origin;
      ASTNodeSetType(node, id->symbol->type);
      node->flags &= ~kASTAnalyzed;
      node->parent->flags &= ~kASTAnalyzed;
    }
    if (id->symbol == replace->from) {
      id->symbol = replace->to;
      ASTNodeSetType(node, TypeIsReference(replace->to->type)
                               ? replace->to->type->next
                               : replace->to->type);
      node->value_category = kValueCategoryLvalue;
    } else if (replace->clone != NULL && id->symbol != NULL) {
      Vector* replacements = MapFindPointerKey(&replace->clone->pack_symbol_map,
                                               id->symbol);
      if (replacements != NULL &&
          replace->element_index < replacements->length) {
        Symbol* replacement = replacements->value.p[replace->element_index];
        if (replacement != NULL) {
          id->symbol = replacement;
          ASTNodeSetType(node, TypeIsReference(replacement->type)
                                   ? replacement->type->next
                                   : replacement->type);
          node->value_category = kValueCategoryLvalue;
        }
      }
    }
  }
  if (node->op != AST_OP(cast) || replace->clone == NULL) {
    return;
  }
  CastASTNode* cast = (CastASTNode*)node;
  if (!TypeContainsTemplateParameter(cast->cast_type) &&
      !(replace->clone->from_owner != NULL &&
        replace->clone->to_owner != NULL &&
        replace->clone->from_owner != replace->clone->to_owner &&
        TypeChainReferencesStruct(cast->cast_type,
                                  replace->clone->from_owner))) {
    return;
  }
  int pack_index = -1;
  size_t pack_length = 0;
  TypeRecord* cast_type = NULL;
  if (FindPackExpansionInType(cast->cast_type, replace->clone->args,
                              &pack_index, &pack_length)) {
    if (pack_length > 0 && replace->element_index >= pack_length) {
      SyntaxError(replace->clone->parser->syntax,
                  "pack expansion argument packs have different lengths");
      return;
    }
    cast_type = SubstituteTemplateParametersForPackElement(
        replace->clone->parser, cast->cast_type, replace->clone->args,
        pack_index, replace->element_index);
  } else {
    cast_type = SubstituteTemplateParameters(replace->clone->parser,
                                             cast->cast_type,
                                             replace->clone->args);
  }
  RebaseTemplateParameterIndices(
      cast_type, replace->clone->rebase_template_parameter_base);
  TypeRecordCalculateSize(cast_type);
  TypeRecordDelete(cast->cast_type);
  cast->cast_type = cast_type;
  TypeRecordIncRef(cast->cast_type);
  if (TypeIsReference(cast_type)) {
    ASTNodeSetType(node, cast_type->next);
    node->value_category =
        cast_type->declarator == kDeclRValueReference
            ? kValueCategoryXvalue
            : kValueCategoryLvalue;
  } else {
    ASTNodeSetType(node, cast_type);
  }
}

typedef struct {
  Symbol* replacement;
  Symbol* source_pack;
} PackReplacementSearch;

static void FindPackSourceForReplacement(MapKeyValue* kv, void* data) {
  PackReplacementSearch* search = data;
  Vector* replacements = kv != NULL ? kv->value.p : NULL;
  for (size_t i = 0;
       search->source_pack == NULL && replacements != NULL &&
       i < replacements->length; i++) {
    Symbol* candidate = replacements->value.p[i];
    if (candidate == search->replacement ||
        (candidate != NULL && search->replacement != NULL &&
         strcmp(candidate->name.value, search->replacement->name.value) == 0)) {
      search->source_pack = kv->key.p;
    }
  }
}

static Symbol* PackSourceForReplacement(TemplateFunctionBodyClone* clone,
                                        Symbol* replacement) {
  if (clone == NULL || replacement == NULL) {
    return NULL;
  }
  PackReplacementSearch search = {
      .replacement = replacement,
      .source_pack = NULL,
  };
  MapTraverse(&clone->pack_symbol_map, FindPackSourceForReplacement, &search);
  return search.source_pack;
}

/* Visitor for the special case of a pack used in a non-expansion context where
 * exactly one element is expected (e.g. a single-element pack): substitute the
 * identifier's template arguments and bind to the concrete instantiation. */
static void ReplaceSingleElementPackIdentifierVisitor(ASTNode* node, void* data,
                                                      int child_id,
                                                      VisitorMode mode) {
  (void)child_id;
  TemplateFunctionBodyClone* clone = data;
  if (mode == kVisitPostChildren && node != NULL) {
    // Drop the pack-expansion marker only once the pack is resolved in this
    // clone.  A pattern still referencing a pack that this clone does not
    // resolve belongs to a not-yet-instantiated nested/member template (e.g. a
    // variadic member template of a class template during class instantiation)
    // and must keep its marker so the later member-template instantiation can
    // expand it.
    if ((node->flags & kASTPackExpansion) == 0 ||
        !ClonePatternReferencesUnresolvedPack(clone, node)) {
      node->flags &= ~kASTPackExpansion;
    }
    return;
  }
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(identifier)) {
    return;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  if (id->template_arguments != NULL &&
      !ASTNodeWithinUnresolvedPackExpansion(clone, node)) {
    int pack_index = -1;
    size_t pack_length = 0;
    Vector* template_args_for_substitution = id->template_arguments;
    Vector* rebased_template_args = NULL;
    int substitution_rebase_base =
        clone->rebase_template_parameter_base;
    for (size_t i = 0; i < id->template_arguments->length; i++) {
      if (FindPackExpansionInTemplateArgument(
              id->template_arguments->value.p[i], clone->args, &pack_index,
              &pack_length)) {
        break;
      }
    }
    VectorASTNode* parent_call =
        node->parent != NULL && node->parent->op == AST_OP(call)
            ? (VectorASTNode*)node->parent
            : NULL;
    ASTNode* first_actual =
        parent_call != NULL && parent_call->children != NULL &&
                parent_call->children->length > 0
            ? parent_call->children->value.p[0]
            : NULL;
    Symbol* actual_replacement =
        first_actual != NULL && first_actual->op == AST_OP(identifier)
            ? ((IdentifierASTNode*)first_actual)->symbol
            : NULL;
    Symbol* actual_source_pack =
        PackSourceForReplacement(clone, actual_replacement);
    if (pack_index < 0 && id->template_arguments->length == 1 &&
        id->symbol != NULL && TypeIsFunction(id->symbol->type) &&
        actual_source_pack != NULL &&
        (id->symbol->flags.is_template ||
         id->symbol->type->info.function.template_origin != NULL)) {
      for (size_t i = 0; i < clone->args->length; i++) {
        TemplateArgument* pack = clone->args->value.p[i];
        if (pack != NULL && pack->pack_arguments != NULL &&
            pack->pack_arguments->length == 1) {
          pack_index = (int)i;
          break;
        }
      }
    }
    if (pack_index < 0 && clone->rebase_template_parameter_base > 0) {
      rebased_template_args =
          TemplateArgumentVectorCopy(id->template_arguments);
      for (size_t i = 0; i < rebased_template_args->length; i++) {
        RebaseTemplateArgumentParameterIndices(
            rebased_template_args->value.p[i],
            clone->rebase_template_parameter_base);
      }
      for (size_t i = 0; i < rebased_template_args->length; i++) {
        if (FindPackExpansionInTemplateArgument(
                rebased_template_args->value.p[i], clone->args, &pack_index,
                &pack_length)) {
          template_args_for_substitution = rebased_template_args;
          substitution_rebase_base = 0;
          break;
        }
      }
    }
    if (pack_index >= 0 && (size_t)pack_index < clone->args->length) {
      Vector* element_args = NULL;
      TemplateArgument* pack = clone->args->value.p[pack_index];
      if (pack != NULL && pack->pack_arguments != NULL &&
          pack->pack_arguments->length == 1) {
        element_args = TemplateArgumentVectorCopyWithPackElement(
            clone->args, pack_index, pack->pack_arguments->value.p[0]);
      }
      Vector* concrete_args = NULL;
      bool use_single_pack_element =
          actual_source_pack != NULL &&
          id->template_arguments->length == 1 && pack != NULL &&
          pack->pack_arguments != NULL && pack->pack_arguments->length == 1 &&
          id->symbol != NULL && TypeIsFunction(id->symbol->type) &&
          (id->symbol->flags.is_template ||
           id->symbol->type->info.function.template_origin != NULL);
      if (use_single_pack_element) {
        concrete_args = NewVector();
        TemplateArgument* element = pack->pack_arguments->value.p[0];
        if (element != NULL && element->pack_arguments != NULL &&
            element->pack_arguments->length == 1) {
          element = element->pack_arguments->value.p[0];
        }
        VectorAppend(concrete_args, TemplateArgumentCopy(element));
      } else {
        Vector* substitution_args =
            actual_source_pack != NULL && element_args != NULL
                ? element_args
                : clone->args;
        concrete_args = SubstituteTemplateArgumentVector(
            clone->parser, template_args_for_substitution, substitution_args,
            substitution_rebase_base);
      }
      VectorDeleteWithContents(id->template_arguments,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      id->template_arguments = concrete_args;
      if (id->symbol != NULL && TypeIsFunction(id->symbol->type) &&
          id->symbol->type->info.function.template_origin != NULL) {
        id->symbol = id->symbol->type->info.function.template_origin;
      }
      if (node->parent != NULL && node->parent->op == AST_OP(call)) {
        VectorASTNode* parent_call = (VectorASTNode*)node->parent;
        if (parent_call->left == node) {
          node->parent->flags |= kASTDependentFunctorCall;
          node->parent->flags &= ~kASTAnalyzed;
        }
      }
      if (element_args != NULL) {
        VectorDeleteWithContents(
            element_args, (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
      }
    }
    if (rebased_template_args != NULL) {
      VectorDeleteWithContents(
          rebased_template_args,
          (VectorElementDestructor)TemplateArgumentDelete,
          /*free_element=*/false);
    }
  }
  Vector* replacements =
      id->symbol != NULL ? MapFindPointerKey(&clone->pack_symbol_map,
                                             id->symbol)
                         : NULL;
  if (replacements != NULL && replacements->length == 1) {
    Symbol* replacement = replacements->value.p[0];
    if (replacement != NULL) {
      id->symbol = replacement;
      ASTNodeSetType(node, TypeIsReference(replacement->type)
                               ? replacement->type->next
                               : replacement->type);
      if (TypeIsReference(replacement->type) &&
          replacement->type->declarator == kDeclReference) {
        node->value_category = kValueCategoryLvalue;
      } else if (node->value_category != kValueCategoryXvalue) {
        node->value_category = kValueCategoryLvalue;
      }
    }
  }
}

/* Produce one expanded copy of a pack-expansion `pattern` for element
 * `element_index`: clone the pattern, rewrite pack references to this element,
 * instantiate any nested function-template calls, then reanalyze dependent
 * functor calls. */
/* Deep-copy cast_type on every cast in a pack-expansion pattern clone so each
 * element can substitute its own pack-dependent cast type without mutating the
 * shared TypeRecord that ASTNodeClone only IncRefs. */
static void DetachClonedPackExpansionCastTypes(ASTNode* node, void* data,
                                               int child_id, VisitorMode mode) {
  (void)data;
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL || node->op != AST_OP(cast)) {
    return;
  }
  CastASTNode* cast = (CastASTNode*)node;
  if (cast->cast_type == NULL) {
    return;
  }
  TypeRecord* copy = TypeRecordCopy(cast->cast_type);
  TypeRecordDelete(cast->cast_type);
  cast->cast_type = copy;
  TypeRecordIncRef(cast->cast_type);
}

static ASTNode* ClonePackExpansionPattern(TemplateFunctionBodyClone* clone,
                                          ASTNode* pattern, Symbol* from,
                                          Symbol* to, size_t element_index) {
  PackExpansionFunctionPacks packs = {.clone = clone};
  VectorInit(&packs.symbols);
  ASTNodeVisit(pattern, CollectPackExpansionFunctionPacks, 0, &packs);
  ASTNode* pattern_clone = ASTNodeClone(pattern, IdentityCloneNode, NULL, NULL);
  ASTNodeVisit(pattern_clone, DetachClonedPackExpansionCastTypes, 0, NULL);
  ReplacePackIdentifierData replace = {0};
  replace.clone = clone;
  replace.from = from;
  replace.to = to;
  replace.element_index = element_index;
  ASTNodeVisit(pattern_clone, ReplacePackIdentifierVisitor, 0, &replace);
  for (size_t i = 0; i < packs.symbols.length; i++) {
    Symbol* symbol = packs.symbols.value.p[i];
    if (symbol == from) {
      continue;
    }
    Vector* replacements =
        MapFindPointerKey(&clone->pack_symbol_map, symbol);
    if (replacements == NULL || element_index >= replacements->length) {
      continue;
    }
    ReplacePackIdentifierData additional = {
        .clone = clone,
        .from = symbol,
        .to = replacements->value.p[element_index],
        .element_index = element_index,
    };
    ASTNodeVisit(pattern_clone, ReplacePackIdentifierVisitor, 0, &additional);
  }
  VectorDestruct(&packs.symbols);
  ASTNodeVisit(pattern_clone, InstantiateClonedFunctionTemplateCallVisitor, 0,
               clone);
  pattern_clone = ASTNodeVisitAndTransform(
      pattern_clone, ReanalyzeClonedDependentFunctorCall, NULL);
  return pattern_clone;
}

/* Expand a parameter pack used as the complete direct-initializer of a local
 * variable.  Dependent parsing represents `T value(args...)` as one scalar
 * initializer pattern because T is not known yet.  Once the enclosing function
 * template is instantiated, rebuild that pattern as either value-initialization
 * (empty pack), one scalar initializer, or a constructor call with all expanded
 * arguments. */
static void ExpandClonedLocalDirectInitializerPack(
    TemplateFunctionBodyClone* clone, VariableDeclarationASTNode* decl,
    Symbol* replacement) {
  if (clone == NULL || decl == NULL || replacement == NULL ||
      replacement->type == NULL || decl->initializer == NULL) {
    return;
  }

  ASTNode* root = decl->initializer;
  ASTNode** pattern_slot = &decl->initializer;
  if (root->op == AST_OP(init)) {
    pattern_slot = &((BinaryASTNode*)root)->right;
  } else if (root->op == AST_OP(expr_init)) {
    pattern_slot = &((ExpressionInitializerASTNode*)root)->expr;
  }
  ASTNode* pattern = *pattern_slot;
  if (pattern == NULL || (pattern->flags & kASTPackExpansion) == 0) {
    return;
  }

  bool multiple_packs = false;
  Symbol* pack_symbol =
      PackExpansionExpressionSymbol(clone, pattern, &multiple_packs);
  if (multiple_packs) {
    SyntaxError(clone->parser->syntax,
                "pack expansion argument packs have different lengths");
    return;
  }
  Vector* replacements =
      pack_symbol != NULL
          ? MapFindPointerKey(&clone->pack_symbol_map, pack_symbol)
          : NULL;
  if (replacements == NULL) {
    return;
  }

  if (!TypeIsStructOrUnion(replacement->type)) {
    if (replacements->length > 1) {
      SyntaxError(clone->parser->syntax,
                  "too many initializers for scalar object");
      return;
    }
    ASTNode* expanded =
        replacements->length == 1
            ? ClonePackExpansionPattern(clone, pattern, pack_symbol,
                                        replacements->value.p[0], 0)
            : NewIntConstantASTNode(0, TypeRecordCopy(replacement->type),
                                    pattern->location);
    ASTNode* parent = pattern->parent;
    int child_id = pattern->child_id;
    ASTNodeDelete(pattern);
    if (root == pattern) {
      ASTNode* decl_id =
          NewIdentifierASTNode(replacement, expanded->location);
      decl_id->flags |= kASTNeedAddress | kASTIsDeclaration;
      ASTNode* init = NewBinaryASTNode(
          AST_OP(init), TypeRecordCopy(replacement->type), expanded->location,
          decl_id, expanded);
      init->parent = parent;
      init->child_id = child_id;
      decl->initializer = init;
    } else {
      *pattern_slot = expanded;
      expanded->parent = parent;
      expanded->child_id = child_id;
    }
    return;
  }

  Vector* actuals = NewVector();
  for (size_t i = 0; i < replacements->length; i++) {
    VectorAppend(actuals,
                 ClonePackExpansionPattern(clone, pattern, pack_symbol,
                                           replacements->value.p[i], i));
  }
  SourceLocation location = root->location;
  ASTNode* parent = root->parent;
  int child_id = root->child_id;
  ASTNodeDelete(root);
  decl->initializer = SyntaxNewCXXConstructorCall(
      clone->parser->syntax, replacement, actuals, location);
  if (decl->initializer != NULL) {
    decl->initializer->parent = parent;
    decl->initializer->child_id = child_id;
  }
}

/* Parameters for rewriting, in one expanded element, a captured-pack member
 * access named `from_name` to the synthetic element field `to`. */
typedef struct {
  TemplateFunctionBodyClone* clone;
  const char* from_name;
  StructMember* to;
  size_t element_index;
} ReplaceLambdaCapturePackFieldData;

/* Visitor: rewrite a `closure.field`/`closure->field` access naming a captured
 * pack to the synthetic per-element field, fixing the member, offset, and node
 * type (and the parent dereference's type). */
static void ReplaceLambdaCapturePackFieldVisitor(ASTNode* node, void* data,
                                                 int child_id,
                                                 VisitorMode mode) {
  (void)child_id;
  if (node != NULL) {
    node->flags &= ~kASTPackExpansion;
  }
  if (mode != kVisitPreChildren || node == NULL) {
    return;
  }
  ReplaceLambdaCapturePackFieldData* replace = data;
  if (node->op == AST_OP(cast) && replace->clone != NULL) {
    ReplacePackIdentifierData cast_replace = {0};
    cast_replace.clone = replace->clone;
    cast_replace.element_index = replace->element_index;
    ReplacePackIdentifierVisitor(node, &cast_replace, child_id, mode);
  }
  ASTNode* access_node = LambdaCapturePackMemberAccess(node);
  if (access_node != node) {
    return;
  }
  BinaryASTNode* access = (BinaryASTNode*)access_node;
  if (access->right->op == AST_OP(string)) {
    ConstantASTNode* name = (ConstantASTNode*)access->right;
    if (name->value.string == NULL ||
        strcmp(name->value.string->value, replace->from_name) != 0) {
      return;
    }
    StringSet(name->value.string, replace->to->symbol->name.value);
  } else if (access->right->op == AST_OP(structmember)) {
    StructMemberASTNode* member = (StructMemberASTNode*)access->right;
    if (member->member == NULL || member->member->symbol == NULL ||
        strcmp(member->member->symbol->name.value, replace->from_name) != 0) {
      return;
    }
    member->member = replace->to;
    member->access = replace->to->access;
    member->byte_offset = replace->to->byte_offset;
    ASTNodeSetType(access->right, replace->to->symbol->type);
  } else {
    return;
  }
  ASTNodeSetType(node, replace->to->symbol->type);
  if (node->parent != NULL && node->parent->op == AST_OP(contents) &&
      TypeIsPointer(replace->to->symbol->type)) {
    ASTNodeSetType(node->parent, replace->to->symbol->type->next);
  }
}

/* Produce one expanded copy of a pattern that references a captured pack field,
 * rewriting `from_name` to the synthetic element field `to`. */
static ASTNode* CloneLambdaCapturePackPattern(TemplateFunctionBodyClone* clone,
                                              ASTNode* pattern,
                                              const char* from_name,
                                              StructMember* to,
                                              size_t element_index) {
  ASTNode* pattern_clone = ASTNodeClone(pattern, IdentityCloneNode, NULL, NULL);
  ASTNodeVisit(pattern_clone, DetachClonedPackExpansionCastTypes, 0, NULL);
  ReplaceLambdaCapturePackFieldData replace = {0};
  replace.clone = clone;
  replace.from_name = from_name;
  replace.to = to;
  replace.element_index = element_index;
  ASTNodeVisit(pattern_clone, ReplaceLambdaCapturePackFieldVisitor, 0, &replace);
  return pattern_clone;
}

/* Expand pack-expansion arguments (`f(args...)` or `value[args...]`) in a
 * cloned call-like vector node into the concrete sequence of per-element
 * arguments. Handles three forms of pack actuals: a bare pack identifier, an
 * arbitrary pattern containing a pack, and a captured-pack member access.
 * Returns true if the argument list was rewritten. */

static Struct* CloneLambdaClosureOwner(TemplateFunctionBodyClone* clone) {
  if (clone == NULL || clone->to_func == NULL ||
      !TypeIsFunction(clone->to_func)) {
    return NULL;
  }
  return clone->to_func->info.function.cxx_member_owner;
}

/* Expand `pack...[Indices]...` in a call argument list.  The outer expansion
 * is driven by the index pack, while the indexed pack must remain whole until
 * each concrete index has been folded. */
static bool ExpandClonedPackIndexActual(TemplateFunctionBodyClone* clone,
                                        ASTNode* parent, ASTNode* actual,
                                        Vector* expanded) {
  if (actual == NULL || actual->op != AST_OP(pack_index) ||
      (actual->flags & kASTPackExpansion) == 0) {
    return false;
  }
  BinaryASTNode* indexed = (BinaryASTNode*)actual;
  if (indexed->left == NULL || indexed->left->op != AST_OP(identifier) ||
      indexed->right == NULL) {
    return false;
  }

  int index_pack_parameter = -1;
  size_t index_pack_length = 0;
  if (!FindPackExpansionInExpression(indexed->right, clone->args,
                                     &index_pack_parameter,
                                     &index_pack_length) ||
      index_pack_parameter < 0 ||
      (size_t)index_pack_parameter >= clone->args->length) {
    return false;
  }
  TemplateArgument* index_pack =
      clone->args->value.p[index_pack_parameter];
  if (index_pack == NULL || index_pack->pack_arguments == NULL) {
    return false;
  }

  Symbol* indexed_symbol = ((IdentifierASTNode*)indexed->left)->symbol;
  Vector* function_elements =
      MapFindPointerKey(&clone->pack_symbol_map, indexed_symbol);
  int value_pack_parameter =
      indexed_symbol != NULL ? indexed_symbol->template_parameter_index : -1;
  TemplateArgument* value_pack =
      function_elements == NULL && value_pack_parameter >= 0 &&
              (size_t)value_pack_parameter < clone->args->length
          ? clone->args->value.p[value_pack_parameter]
          : NULL;
  if (function_elements == NULL &&
      (value_pack == NULL || value_pack->pack_arguments == NULL)) {
    return false;
  }

  for (size_t i = 0; i < index_pack_length; i++) {
    Vector* element_args = TemplateArgumentVectorCopyWithPackElement(
        clone->args, index_pack_parameter,
        index_pack->pack_arguments->value.p[i]);
    int64_t selected = 0;
    bool folded = TryFoldDependentTemplateArgument(
        clone->parser, indexed->right, element_args, &selected);
    VectorDeleteWithContents(
        element_args, (VectorElementDestructor)TemplateArgumentDelete,
        /*free_element=*/false);

    size_t indexed_length = function_elements != NULL
                                ? function_elements->length
                                : value_pack->pack_arguments->length;
    ASTNode* replacement = NULL;
    if (!folded) {
      SemanticError(indexed->right,
                    "pack index must be a constant expression");
    } else if (selected < 0 || (uint64_t)selected >= indexed_length) {
      SemanticError(indexed->right,
                    "pack index %" PRId64
                    " is out of bounds for a pack of length %zu",
                    selected, indexed_length);
    } else if (function_elements != NULL) {
      replacement = NewIdentifierASTNode(
          function_elements->value.p[(size_t)selected], actual->location);
    } else {
      replacement = TemplateArgumentMaterializeExpression(
          value_pack->pack_arguments->value.p[(size_t)selected],
          actual->location);
    }
    if (replacement == NULL) {
      replacement = NewIntConstantASTNode(
          0, NewTypeRecordWithSize(kTypeInt, kQualPlain), actual->location);
    }
    replacement->flags &= ~kASTPackExpansion;
    replacement->parent = parent;
    replacement->child_id = (int)expanded->length;
    VectorAppend(expanded, replacement);
  }
  return true;
}

static bool ExpandClonedCallPackActuals(TemplateFunctionBodyClone* clone,
                                        ASTNode* node) {
  if (node->op != AST_OP(call) &&
      !(node->op == AST_OP(subscript) &&
        ASTNodeGetShape(node) == kASTShapeVector)) {
    return false;
  }
  VectorASTNode* call = (VectorASTNode*)node;
  Vector* expanded = NewVector();
  bool changed = false;
  for (size_t i = 0; i < call->children->length; i++) {
    ASTNode* actual = call->children->value.p[i];
    if (ExpandClonedPackIndexActual(clone, node, actual, expanded)) {
      ASTNodeDelete(actual);
      changed = true;
      continue;
    }
    if (IsIdentifierPackExpansion(actual)) {
      IdentifierASTNode* id = (IdentifierASTNode*)actual;
      Vector* replacements = MapFindPointerKey(&clone->pack_symbol_map,
                                               id->symbol);
      if (replacements != NULL) {
        for (size_t j = 0; j < replacements->length; j++) {
          Symbol* replacement = replacements->value.p[j];
          ASTNode* replacement_id =
              NewIdentifierASTNode(replacement, actual->location);
          replacement_id->parent = node;
          replacement_id->child_id = (int)expanded->length;
          VectorAppend(expanded, replacement_id);
        }
        ASTNodeDelete(actual);
        changed = true;
        continue;
      }
    }
    if ((actual->flags & kASTPackExpansion) != 0) {
      bool multiple_packs = false;
      Symbol* pack_symbol =
          PackExpansionExpressionSymbol(clone, actual, &multiple_packs);
      size_t template_argument_pack_length = 0;
      bool template_argument_multiple_packs = false;
      bool has_template_argument_pack = PackExpansionTemplateArgumentInfo(
          clone, actual, &template_argument_pack_length,
          &template_argument_multiple_packs);
      multiple_packs =
          multiple_packs || template_argument_multiple_packs;
      if (multiple_packs) {
        SyntaxError(clone->parser->syntax,
                    "pack expansion argument packs have different lengths");
      }
      if (pack_symbol == NULL &&
          !has_template_argument_pack &&
          !ClonePatternReferencesUnresolvedPack(clone, actual)) {
        actual->flags &= ~kASTPackExpansion;
      }
      if (pack_symbol == NULL && has_template_argument_pack) {
        for (size_t j = 0; j < template_argument_pack_length; j++) {
          ASTNode* expanded_actual =
              ClonePackExpansionPattern(clone, actual, NULL, NULL, j);
          expanded_actual->parent = node;
          expanded_actual->child_id = (int)expanded->length;
          VectorAppend(expanded, expanded_actual);
        }
        ASTNodeDelete(actual);
        changed = true;
        continue;
      }
      Vector* replacements =
          pack_symbol != NULL
              ? MapFindPointerKey(&clone->pack_symbol_map, pack_symbol)
              : NULL;
      bool replacements_belong_to_function = replacements != NULL;
      for (size_t replacement_index = 0;
           replacements_belong_to_function &&
           replacement_index < replacements->length;
           replacement_index++) {
        Symbol* replacement = replacements->value.p[replacement_index];
        bool found = false;
        for (size_t formal_index = 0;
             clone->to_func != NULL && TypeIsFunction(clone->to_func) &&
             formal_index <
                 clone->to_func->info.function.prototype.length;
             formal_index++) {
          if (clone->to_func->info.function.prototype.value.p[formal_index] ==
              replacement) {
            found = true;
            break;
          }
        }
        replacements_belong_to_function = found;
      }
      Vector* matched_formals = NULL;
      if (pack_symbol != NULL &&
          (replacements == NULL || replacements->length == 0 ||
           !replacements_belong_to_function) &&
          clone->to_func != NULL && TypeIsFunction(clone->to_func)) {
        matched_formals = NewVector();
        for (size_t formal_index = 0;
             formal_index <
             clone->to_func->info.function.prototype.length;
             formal_index++) {
          Symbol* formal =
              clone->to_func->info.function.prototype.value.p[formal_index];
          if (formal != NULL && !formal->flags.is_parameter_pack &&
              StringEqualString(&formal->name, &pack_symbol->name)) {
            VectorAppend(matched_formals, formal);
          }
        }
        if (matched_formals->length != 0) {
          replacements = matched_formals;
        } else {
          VectorDelete(matched_formals);
          matched_formals = NULL;
        }
      }
      if (replacements == NULL && pack_symbol != NULL) {
        Symbol* replacement =
            MapFindPointerKey(&clone->symbol_map, pack_symbol);
        if (replacement != NULL) {
          ASTNode* expanded_actual =
              ClonePackExpansionPattern(clone, actual, pack_symbol,
                                        replacement, 0);
          expanded_actual->parent = node;
          expanded_actual->child_id = (int)expanded->length;
          VectorAppend(expanded, expanded_actual);
          ASTNodeDelete(actual);
          changed = true;
          continue;
        }
      }
      if (replacements != NULL) {
        for (size_t j = 0; j < replacements->length; j++) {
          Symbol* replacement = replacements->value.p[j];
          ASTNode* expanded_actual =
              ClonePackExpansionPattern(clone, actual, pack_symbol,
                                        replacement, j);
          expanded_actual->parent = node;
          expanded_actual->child_id = (int)expanded->length;
          VectorAppend(expanded, expanded_actual);
        }
        if (matched_formals != NULL) {
          VectorDelete(matched_formals);
        }
        ASTNodeDelete(actual);
        changed = true;
        continue;
      }
      Vector* capture_replacements = LambdaCapturePackFieldReplacements(actual, CloneLambdaClosureOwner(clone));
      const char* capture_name = LambdaCapturePackMemberName(actual);
      if (capture_replacements != NULL && capture_name != NULL) {
        for (size_t j = 0; j < capture_replacements->length; j++) {
          StructMember* replacement = capture_replacements->value.p[j];
          ASTNode* expanded_actual =
              CloneLambdaCapturePackPattern(clone, actual, capture_name,
                                            replacement, j);
          expanded_actual->parent = node;
          expanded_actual->child_id = (int)expanded->length;
          VectorAppend(expanded, expanded_actual);
        }
        VectorDelete(capture_replacements);
        ASTNodeDelete(actual);
        changed = true;
        continue;
      }
    }
    Vector* capture_replacements = LambdaCapturePackFieldReplacements(actual, CloneLambdaClosureOwner(clone));
    const char* capture_name = LambdaCapturePackMemberName(actual);
    if (capture_replacements != NULL && capture_name != NULL &&
        capture_replacements->length > 0) {
      for (size_t j = 0; j < capture_replacements->length; j++) {
        StructMember* replacement = capture_replacements->value.p[j];
        ASTNode* expanded_actual =
            CloneLambdaCapturePackPattern(clone, actual, capture_name,
                                          replacement, j);
        expanded_actual->parent = node;
        expanded_actual->child_id = (int)expanded->length;
        VectorAppend(expanded, expanded_actual);
      }
      VectorDelete(capture_replacements);
      ASTNodeDelete(actual);
      changed = true;
      continue;
    }
    if (capture_replacements != NULL) {
      VectorDelete(capture_replacements);
    }
    actual->parent = node;
    actual->child_id = (int)expanded->length;
    VectorAppend(expanded, actual);
  }
  if (changed) {
    VectorDelete(call->children);
    call->children = expanded;
  } else {
    VectorDelete(expanded);
  }
  return changed;
}

static bool ASTNodeIsPackExpansion(ASTNode* node, void* data) {
  (void)data;
  return node != NULL && (node->flags & kASTPackExpansion) != 0;
}

/* True if any node anywhere in `node`'s subtree is still an unexpanded pack
 * expansion.  A call cannot be resolved to a concrete overload while a pack
 * expansion survives *anywhere* beneath it -- not just among its direct actuals
 * -- because the surviving pack makes the effective argument count (and types)
 * unknown.  For example `value_type(key, mapped_type(static_cast<Args&&>(args)
 * ...))` inside `unordered_map::try_emplace` has no direct pack actual (its
 * arguments are `key` and the inner `mapped_type(...)` call), yet resolving the
 * outer construction while the member's own pack `Args` is still unbound picks a
 * spurious 0-or-N-argument constructor overload (a phantom `pair<const K,V>()`
 * ambiguity).  Such a construction must stay deferred until the member template
 * is instantiated per call, when the pack length is known. */
static bool ASTNodeSubtreeContainsPackExpansion(ASTNode* node) {
  return ASTNodeAny(node, ASTNodeIsPackExpansion, NULL);
}

/* True if any actual argument of `call` is still an unexpanded pack expansion
 * (so the call cannot yet be resolved to a concrete overload). */
bool CallActualsStillContainPackExpansion(struct ASTNode* call_node) {
  VectorASTNode* call = (VectorASTNode*)call_node;
  for (size_t i = 0; call != NULL && call->children != NULL &&
                    i < call->children->length;
       i++) {
    ASTNode* actual = call->children->value.p[i];
    if (actual != NULL && (actual->flags & kASTPackExpansion) != 0) {
      return true;
    }
    Vector* replacements = LambdaCapturePackFieldReplacements(actual, NULL);
    if (replacements != NULL) {
      bool still_contains_pack = replacements->length > 0;
      VectorDelete(replacements);
      if (still_contains_pack) {
        return true;
      }
    }
  }
  return false;
}

/* Set a cloned call node's result type and value category from the resolved
 * function type `func`. A reference return yields an lvalue/xvalue of the
 * referenced type; otherwise a prvalue of the return type. */
static void SetClonedCallReturnType(VectorASTNode* call, TypeRecord* func) {
  if (call == NULL || func == NULL) {
    return;
  }
  if (TypeIsPointer(func)) {
    func = func->next;
  }
  if (!TypeIsFunction(func)) {
    return;
  }
  TypeRecord* return_type = func->next;
  if (return_type == NULL) {
    return;
  }
  if (TypeIsReference(return_type)) {
    ASTNodeSetType((ASTNode*)call, return_type->next);
    call->base.value_category =
        return_type->declarator == kDeclRValueReference
            ? kValueCategoryXvalue
            : kValueCategoryLvalue;
  } else {
    ASTNodeSetType((ASTNode*)call, return_type);
  }
}

static void RebindClonedLoweredDependentMemberCall(VectorASTNode* call) {
  if (call == NULL || call->left == NULL ||
      call->left->op != AST_OP(identifier) || call->children == NULL ||
      call->children->length == 0) {
    return;
  }
  IdentifierASTNode* callee = (IdentifierASTNode*)call->left;
  if (callee->symbol == NULL || callee->symbol->type == NULL ||
      !TypeIsFunction(callee->symbol->type) ||
      callee->symbol->type->info.function.cxx_member_owner == NULL) {
    return;
  }
  Struct* original_owner =
      callee->symbol->type->info.function.cxx_member_owner;
  StructMember* original_member =
      FindStructMember(original_owner, &callee->symbol->name);
  bool has_nonstatic_overload = false;
  for (StructMember* overload = original_member; overload != NULL;
       overload = overload->overload_next) {
    if (!overload->is_member_function) {
      continue;
    }
    if (!overload->is_static) {
      has_nonstatic_overload = true;
    }
    if (overload->symbol == callee->symbol && overload->is_static) {
      // Static member calls already carry their first declared argument in
      // slot zero. Treating that argument as an object receiver can
      // accidentally rebind `allocator_traits<A>::construct(a, ...)` to
      // `A::construct(...)`, dropping the required address-of lowering.
      return;
    }
  }
  if (!has_nonstatic_overload) {
    return;
  }
  ASTNode* receiver = call->children->value.p[0];
  TypeRecord* receiver_type = receiver != NULL ? receiver->type : NULL;
  if (receiver_type == NULL) {
    return;
  }
  if (TypeIsReference(receiver_type)) {
    receiver_type = receiver_type->next;
  }
  if (TypeIsPointer(receiver_type)) {
    receiver_type = receiver_type->next;
  }
  if (!TypeIsStructOrUnion(receiver_type) ||
      receiver_type->info.struct_info == NULL) {
    return;
  }
  StructMember* member = FindStructMember(receiver_type->info.struct_info,
                                          &callee->symbol->name);
  if (member == NULL || !member->is_member_function || member->is_static ||
      member->overload_next != NULL || member->symbol == NULL ||
      !TypeIsFunction(member->symbol->type)) {
    return;
  }
  callee->symbol = member->symbol;
  ASTNodeSetType(call->left, member->symbol->type);
  SetClonedCallReturnType(call, member->symbol->type);
  TypeEnsureTemplateMemberFunctionDefinition(&compiler->syntax,
                                             member->symbol);
}

static void RebindClonedLoweredDependentMemberCallVisitor(
    ASTNode* node, void* data, int child_id, VisitorMode mode) {
  (void)data;
  (void)child_id;
  if (mode == kVisitPostChildren && node != NULL &&
      node->op == AST_OP(call)) {
    VectorASTNode* call = (VectorASTNode*)node;
    if (call->left != NULL && call->left->op == AST_OP(identifier) &&
        call->left->type != NULL && TypeIsStructOrUnion(call->left->type)) {
      node->flags &= ~kASTAnalyzed;
      call->left->flags &= ~kASTAnalyzed;
    }
    RebindClonedLoweredDependentMemberCall((VectorASTNode*)node);
  }
}

static void RestoreClonedInlineReferenceActualTypes(VectorASTNode* call) {
  for (size_t i = 0; call != NULL && call->children != NULL &&
                     i < call->children->length;
       i++) {
    ASTNode* actual = call->children->value.p[i];
    if (actual == NULL || actual->op != AST_OP(inline_call) ||
        actual->value_category == kValueCategoryPrvalue) {
      continue;
    }
    InlineCallASTNode* inline_call = (InlineCallASTNode*)actual;
    TypeRecord* stored_result =
        inline_call->ret_value != NULL ? inline_call->ret_value->type : NULL;
    if (TypeIsPointer(stored_result)) {
      ASTNodeSetType(actual, stored_result->next);
    }
  }
}

/* In a cloned body, resolve a call whose callee is a function template to the
 * concrete instantiation deduced from the (now concrete) explicit template
 * arguments and actual arguments, updating the callee symbol and result type.
 * Skips calls whose actuals still contain pack expansions. */
static void InstantiateClonedFunctionTemplateCall(
    TemplateFunctionBodyClone* clone, ASTNode* node) {
  if (node->op != AST_OP(call) || ASTNodeWithinPackExpansion(node) ||
      PackExpansionExpressionSymbol(clone, node, NULL) != NULL) {
    return;
  }
  VectorASTNode* call = (VectorASTNode*)node;
  if (call->left == NULL || call->left->op != AST_OP(identifier) ||
      CallActualsStillContainPackExpansion((ASTNode*)call)) {
    return;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)call->left;
  if (id->symbol == NULL || !TypeIsFunction(id->symbol->type)) {
    return;
  }
  if (id->symbol->namespace_ != NULL) {
    Symbol* overload_head =
        NamespaceFindSymbol(id->symbol->namespace_, &id->symbol->name);
    if (overload_head != NULL && overload_head->flags.is_template &&
        overload_head->type != NULL && TypeIsFunction(overload_head->type)) {
      id->symbol = overload_head;
      ASTNodeSetType(call->left, overload_head->type);
    }
  }
  if (!id->symbol->flags.is_template) {
    Symbol* origin = id->symbol->type->info.function.template_origin;
    if (origin == NULL || id->symbol->type->info.function.body != NULL ||
        !origin->flags.is_template || !TypeIsFunction(origin->type)) {
      return;
    }
    // A dependent pass can attach a speculative specialization before the
    // enclosing template's arguments are concrete.  If that specialization
    // has no body, deduction never had enough information to materialize it.
    // Rebind only this incomplete case to its primary template; completed
    // specializations must remain intact (notably for overload sets such as
    // variant's converting constructors).
    id->symbol = origin;
    ASTNodeSetType(call->left, origin->type);
  }
  if (TemplateArgumentVectorContainsTemplateParameter(id->template_arguments)) {
    return;
  }
  // A deleted function template reached by ordinary unqualified lookup is the
  // standard-library "poison pill" idiom (e.g. `template <class T> void
  // swap(T&, T&) = delete;` inside `std::ranges::__swap`): it exists only so
  // that a real, non-template `swap` found by argument-dependent lookup wins
  // overload resolution.  Eagerly deducing and instantiating it here would fail
  // ("use of deleted"/"definition required") during substitution, before the
  // full, ADL-aware re-analysis of the cloned expression runs.  Leave such a
  // call unresolved so later overload resolution (which combines ordinary
  // lookup with ADL) selects the correct function.
  if (id->symbol->type->info.function.is_deleted &&
      id->symbol->overload_next == NULL &&
      (call->left->flags & kASTQualifiedName) == 0) {
    return;
  }
  // An actual argument whose type still contains `auto` has not been re-deduced
  // yet in this instantiation (its declaration is re-analyzed later, at compile
  // time).  A classic case is a local closure variable, `auto l = [..]{..};`,
  // passed to a function template: at this early post-clone pass `l` is still an
  // undeduced placeholder, so deducing the callee now would bind it against the
  // wrong (placeholder) argument type.  Leave the call for the full re-analysis
  // that runs once `auto` is resolved.
  //
  // The same deferral applies to a temporary lambda-expression passed directly
  // (`f([&]{ ... })`): its closure type may already look concrete after nested
  // struct substitution, but its `operator()` body is only fully re-analyzed
  // later.  Instantiating the callee against that temporary now binds a
  // specialization to a half-lowered call operator (e.g. an indirect call of a
  // captured functor) and never recovers.
  if (call->children != NULL) {
    for (size_t i = 0; i < call->children->length; i++) {
      ASTNode* actual = call->children->value.p[i];
      if (actual == NULL) {
        continue;
      }
      if (actual->type != NULL && TypeContainsAuto(actual->type)) {
        return;
      }
      // The call is still type-dependent: this clone only substituted the
      // enclosing template's arguments, so a member function template's own
      // parameters (and any type built from them) survive here.  Deduction
      // against such an argument would bind the callee to the placeholder type
      // itself, producing a specialization whose parameters are unusable -- and
      // that bogus specialization is cached, so the later, concrete
      // instantiation never gets a chance to replace it.
      if (actual->type != NULL && TypeContainsTemplateParameter(actual->type)) {
        return;
      }
      if ((actual->flags & kASTLambdaExpression) != 0) {
        return;
      }
    }
  }
  RestoreClonedInlineReferenceActualTypes(call);
  Symbol* instantiated = NULL;
  if (id->symbol->overload_next != NULL) {
    instantiated = CXXResolveOverloadedFunctionTemplateCall(
        id->symbol, id->template_arguments, call->children);
  }
  if (instantiated == NULL) {
    instantiated = TypeDeduceFunctionTemplateFromCallWithExplicitArgs(
        clone->parser->syntax, id->symbol, id->template_arguments,
        call->children);
  }
  if (instantiated != NULL && instantiated != id->symbol) {
    id->symbol = instantiated;
    ASTNodeSetInstantiatedCalleeType(call->left, instantiated->type);
    SetClonedCallReturnType(call, instantiated->type);
  }
}

/* True if `node` is an expression-initializer wrapping a bare pack-identifier
 * expansion (`xs...`); reports the pack symbol and source location. */
static bool ExpressionInitializerIsIdentifierPackExpansion(ASTNode* node,
                                                           Symbol** symbol,
                                                           SourceLocation* loc) {
  if (node == NULL || node->op != AST_OP(expr_init)) {
    return false;
  }
  ExpressionInitializerASTNode* init = (ExpressionInitializerASTNode*)node;
  if (!IsIdentifierPackExpansion(init->expr)) {
    return false;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)init->expr;
  if (symbol != NULL) {
    *symbol = id->symbol;
  }
  if (loc != NULL) {
    *loc = init->expr->location;
  }
  return true;
}

/* Expand pack expansions appearing as elements of a braced initializer
 * (`{xs...}`), including designated-initializer member packs, into the concrete
 * per-element initializers. */
static void ExpandClonedBracedInitializerPackElements(
    TemplateFunctionBodyClone* clone, ASTNode* node) {
  if (node->op != AST_OP(braced_init)) {
    return;
  }
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)node;
  Vector* expanded = NewVector();
  bool changed = false;
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* initializer = braced->initializers->value.p[i];
    Symbol* pack_symbol = NULL;
    SourceLocation location = initializer != NULL ? initializer->location
                                                  : node->location;
    if (ExpressionInitializerIsIdentifierPackExpansion(initializer,
                                                       &pack_symbol,
                                                       &location)) {
      Vector* replacements = MapFindPointerKey(&clone->pack_symbol_map,
                                               pack_symbol);
      if (replacements != NULL) {
        for (size_t j = 0; j < replacements->length; j++) {
          ASTNode* replacement =
              NewIdentifierASTNode(replacements->value.p[j], location);
          ASTNode* expr_init =
              NewExpressionInitializerASTNode(replacement, location);
          expr_init->parent = node;
          expr_init->child_id = (int)expanded->length;
          VectorAppend(expanded, expr_init);
        }
        ASTNodeDelete(initializer);
        changed = true;
        continue;
      }
      int pack_index =
          pack_symbol != NULL ? pack_symbol->template_parameter_index : -1;
      if (pack_index < 0 && pack_symbol != NULL) {
        TypeIsTemplateParameterPlaceholder(pack_symbol->type, &pack_index);
      }
      TemplateArgument* value_pack =
          pack_index >= 0 && (size_t)pack_index < clone->args->length
              ? clone->args->value.p[(size_t)pack_index]
              : NULL;
      if (value_pack != NULL && value_pack->pack_arguments != NULL) {
        size_t expanded_start = expanded->length;
        bool materialized = true;
        for (size_t j = 0; j < value_pack->pack_arguments->length; j++) {
          ASTNode* replacement = TemplateArgumentMaterializeExpression(
              value_pack->pack_arguments->value.p[j], location);
          if (replacement == NULL) {
            materialized = false;
            break;
          }
          replacement->flags &= ~kASTPackExpansion;
          ASTNode* expr_init =
              NewExpressionInitializerASTNode(replacement, location);
          expr_init->parent = node;
          expr_init->child_id = (int)expanded->length;
          VectorAppend(expanded, expr_init);
        }
        if (materialized) {
          ASTNodeDelete(initializer);
          changed = true;
          continue;
        }
        while (expanded->length > expanded_start) {
          ASTNodeDelete(VectorLast(expanded));
          VectorPop(expanded);
        }
      }
    }
    if (initializer != NULL && initializer->op == AST_OP(designated_init)) {
      DesignatedInitializerASTNode* designated =
          (DesignatedInitializerASTNode*)initializer;
      ASTNode* init = designated->init;
      if (init != NULL && init->op == AST_OP(expr_init)) {
        ExpressionInitializerASTNode* expr_init =
            (ExpressionInitializerASTNode*)init;
        if (expr_init->expr != NULL &&
            (expr_init->expr->flags & kASTPackExpansion) != 0 &&
            designated->designators != NULL &&
            designated->designators->length == 1) {
          Designator* designator = designated->designators->value.p[0];
          const char* member_name =
              designator != NULL &&
                      designator->designator_type == kDesignatorStruct &&
                      !designator->is_resolved_member &&
                      designator->value.struct_member_name != NULL
                  ? designator->value.struct_member_name->value
                  : NULL;
          bool multiple_packs = false;
          pack_symbol = PackExpansionExpressionSymbol(clone, expr_init->expr,
                                                      &multiple_packs);
          if (multiple_packs) {
            SyntaxError(clone->parser->syntax,
                        "pack expansion argument packs have different lengths");
          }
          Vector* replacements =
              pack_symbol != NULL
                  ? MapFindPointerKey(&clone->pack_symbol_map, pack_symbol)
                  : NULL;
          if (member_name != NULL && replacements != NULL) {
            for (size_t j = 0; j < replacements->length; j++) {
              ASTNode* replacement = ClonePackExpansionPattern(
                  clone, expr_init->expr, pack_symbol,
                  replacements->value.p[j], j);
              ASTNode* expanded_init =
                  NewExpressionInitializerASTNode(replacement, location);
              Vector* designators = NewVector();
              String field_name;
              StringInit(&field_name, NULL);
              LambdaCapturePackElementName(&field_name, member_name, j);
              VectorAppend(designators,
                           NewStructDesignator(NewString(field_name.value)));
              StringDestruct(&field_name);
              ASTNode* expanded_designated =
                  NewDesignatedInitializerASTNode(designators, expanded_init,
                                                  location);
              expanded_designated->parent = node;
              expanded_designated->child_id = (int)expanded->length;
              VectorAppend(expanded, expanded_designated);
            }
            ASTNodeDelete(initializer);
            changed = true;
            continue;
          }
        }
      }
    }
    if (initializer != NULL && initializer->op == AST_OP(expr_init)) {
      ExpressionInitializerASTNode* expr_init_node =
          (ExpressionInitializerASTNode*)initializer;
      if (expr_init_node->expr != NULL &&
          (expr_init_node->expr->flags & kASTPackExpansion) != 0) {
        bool multiple_packs = false;
        pack_symbol = PackExpansionExpressionSymbol(clone, expr_init_node->expr,
                                                    &multiple_packs);
        if (multiple_packs) {
          SyntaxError(clone->parser->syntax,
                      "pack expansion argument packs have different lengths");
        }
        Vector* replacements =
            pack_symbol != NULL
                ? MapFindPointerKey(&clone->pack_symbol_map, pack_symbol)
                : NULL;
        if (replacements != NULL) {
          for (size_t j = 0; j < replacements->length; j++) {
            ASTNode* replacement = ClonePackExpansionPattern(
                clone, expr_init_node->expr, pack_symbol,
                replacements->value.p[j], j);
            ASTNode* expanded_init =
                NewExpressionInitializerASTNode(replacement, location);
            expanded_init->parent = node;
            expanded_init->child_id = (int)expanded->length;
            VectorAppend(expanded, expanded_init);
          }
          ASTNodeDelete(initializer);
          changed = true;
          continue;
        }
        Vector* capture_replacements =
            LambdaCapturePackFieldReplacements(expr_init_node->expr, CloneLambdaClosureOwner(clone));
        const char* capture_name =
            LambdaCapturePackMemberName(expr_init_node->expr);
        if (capture_replacements != NULL && capture_name != NULL) {
          for (size_t j = 0; j < capture_replacements->length; j++) {
            ASTNode* replacement = CloneLambdaCapturePackPattern(
                clone, expr_init_node->expr, capture_name,
                capture_replacements->value.p[j], j);
            ASTNode* expanded_init =
                NewExpressionInitializerASTNode(replacement, location);
            expanded_init->parent = node;
            expanded_init->child_id = (int)expanded->length;
            VectorAppend(expanded, expanded_init);
          }
          VectorDelete(capture_replacements);
          ASTNodeDelete(initializer);
          changed = true;
          continue;
        }
      }
    }
    if (initializer != NULL) {
      initializer->parent = node;
      initializer->child_id = (int)expanded->length;
    }
    VectorAppend(expanded, initializer);
  }
  if (changed) {
    VectorDelete(braced->initializers);
    braced->initializers = expanded;
  } else {
    VectorDelete(expanded);
  }
}

/* The identity value for an empty unary fold expression: `&&` folds to true,
 * `||` to false, and `,` to void(). Any other operator is ill-formed. */
static ASTNode* NewFoldIdentity(ASTOpcode op, SourceLocation location,
                                TypeParser* parser) {
  if (op == AST_OP(logand)) {
    return NewIntConstantASTNode(1, NewTypeRecordWithSize(kTypeBool, kQualPlain),
                                 location);
  }
  if (op == AST_OP(logor)) {
    return NewIntConstantASTNode(0, NewTypeRecordWithSize(kTypeBool, kQualPlain),
                                 location);
  }
  if (op == AST_OP(comma)) {
    ASTNode* zero = NewIntConstantASTNode(
        0, NewTypeRecordWithSize(kTypeInt, kQualPlain), location);
    ASTNode* result = NewCastASTNode(
        NewTypeRecordWithSize(kTypeVoid, kQualPlain), location, zero);
    ((CastASTNode*)result)->kind = kCastStatic;
    return result;
  }
  SyntaxError(parser->syntax, "Empty fold expression is not supported for this operator");
  return NewIntConstantASTNode(0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                               location);
}

static bool IsFoldOperatorOpcode(ASTOpcode op) {
  switch (op) {
    case AST_OP(mult):
    case AST_OP(div):
    case AST_OP(mod):
    case AST_OP(plus):
    case AST_OP(minus):
    case AST_OP(lshift):
    case AST_OP(rshift):
    case AST_OP(and):
    case AST_OP(exor):
    case AST_OP(bitor):
    case AST_OP(logand):
    case AST_OP(logor):
    case AST_OP(equal):
    case AST_OP(noteq):
    case AST_OP(less):
    case AST_OP(greater):
    case AST_OP(lesseq):
    case AST_OP(greatereq):
    case AST_OP(assign):
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
    case AST_OP(comma):
    case AST_OP(dotstar):
    case AST_OP(arrowstar):
      return true;
    default:
      return false;
  }
}

typedef enum {
  kFoldPackFunction,
  kFoldPackTemplateArgument,
  kFoldPackLambdaCapture,
} FoldPackKind;

typedef struct {
  FoldPackKind kind;
  Symbol* symbol;
  int template_parameter_index;
  const char* capture_name;
  Vector* replacements;
  bool owns_replacements;
} FoldPackBinding;

typedef struct {
  TemplateFunctionBodyClone* clone;
  Vector bindings;
  size_t length;
  bool has_length;
  bool mismatched_lengths;
  bool unresolved_pack;
} FoldPackSearch;

static void DeleteFoldPackBindings(FoldPackSearch* search) {
  for (size_t i = 0; i < search->bindings.length; i++) {
    FoldPackBinding* binding = search->bindings.value.p[i];
    if (binding->owns_replacements) {
      VectorDelete(binding->replacements);
    }
    free(binding);
  }
  VectorDestruct(&search->bindings);
}

static bool FoldPackBindingMatches(FoldPackBinding* binding, FoldPackKind kind,
                                   Symbol* symbol,
                                   int template_parameter_index,
                                   const char* capture_name) {
  if (binding->kind != kind) {
    return false;
  }
  if (kind == kFoldPackTemplateArgument) {
    return binding->template_parameter_index == template_parameter_index;
  }
  if (kind != kFoldPackLambdaCapture) {
    return binding->symbol == symbol;
  }
  return binding->capture_name != NULL && capture_name != NULL &&
         strcmp(binding->capture_name, capture_name) == 0;
}

static void AddFoldPackBinding(FoldPackSearch* search, FoldPackKind kind,
                               Symbol* symbol, int template_parameter_index,
                               const char* capture_name,
                               Vector* replacements, size_t length,
                               bool owns_replacements) {
  for (size_t i = 0; i < search->bindings.length; i++) {
    FoldPackBinding* existing = search->bindings.value.p[i];
    if (FoldPackBindingMatches(existing, kind, symbol,
                               template_parameter_index, capture_name)) {
      if (owns_replacements) {
        VectorDelete(replacements);
      }
      return;
    }
  }
  FoldPackBinding* binding = calloc(1, sizeof(FoldPackBinding));
  binding->kind = kind;
  binding->symbol = symbol;
  binding->template_parameter_index = template_parameter_index;
  binding->capture_name = capture_name;
  binding->replacements = replacements;
  binding->owns_replacements = owns_replacements;
  VectorAppend(&search->bindings, binding);
  if (!search->has_length) {
    search->length = length;
    search->has_length = true;
  } else if (search->length != length) {
    search->mismatched_lengths = true;
  }
}

static TemplateArgument* ConcreteTemplateArgumentPack(
    TemplateFunctionBodyClone* clone, Symbol* symbol) {
  if (clone == NULL || symbol == NULL || clone->args == NULL) {
    return NULL;
  }
  int pack_index = symbol->template_parameter_index;
  if (pack_index < 0) {
    TypeIsTemplateParameterPlaceholder(symbol->type, &pack_index);
  }
  if (pack_index < 0 || (size_t)pack_index >= clone->args->length) {
    return NULL;
  }
  TemplateArgument* argument = clone->args->value.p[pack_index];
  return argument != NULL && argument->pack_arguments != NULL
             ? argument
             : NULL;
}

static void FindFoldPacks(ASTNode* node, void* data, int child_id,
                          VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL) {
    return;
  }
  FoldPackSearch* search = data;
  if (node->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node;
    if (id->symbol != NULL && id->symbol->flags.is_parameter_pack) {
      Vector* replacements =
          MapFindPointerKey(&search->clone->pack_symbol_map, id->symbol);
      if (replacements != NULL) {
        AddFoldPackBinding(search, kFoldPackFunction, id->symbol, -1, NULL,
                           replacements, replacements->length, false);
        return;
      }
      TemplateArgument* argument =
          ConcreteTemplateArgumentPack(search->clone, id->symbol);
      if (argument != NULL) {
        AddFoldPackBinding(search, kFoldPackTemplateArgument, id->symbol,
                           id->symbol->template_parameter_index, NULL,
                           argument->pack_arguments,
                           argument->pack_arguments->length, false);
        return;
      }
      search->unresolved_pack = true;
      return;
    }
  }
  Vector* template_arguments = NULL;
  ASTNodeShape shape = ASTNodeGetShape(node);
  if (shape == kASTShapeIdentifier) {
    template_arguments = ((IdentifierASTNode*)node)->template_arguments;
  } else if (shape == kASTShapeStructMember) {
    template_arguments = ((StructMemberASTNode*)node)->template_arguments;
  } else if (shape == kASTShapeConstant) {
    template_arguments = ((ConstantASTNode*)node)->template_arguments;
  }
  for (size_t i = 0;
       template_arguments != NULL && i < template_arguments->length; i++) {
    int pack_index = -1;
    size_t pack_length = 0;
    if (!FindPackExpansionInTemplateArgument(
            template_arguments->value.p[i], search->clone->args, &pack_index,
            &pack_length)) {
      continue;
    }
    if (pack_index < 0 || search->clone->args == NULL ||
        (size_t)pack_index >= search->clone->args->length) {
      search->unresolved_pack = true;
      continue;
    }
    TemplateArgument* argument =
        search->clone->args->value.p[pack_index];
    if (argument == NULL || argument->pack_arguments == NULL) {
      search->unresolved_pack = true;
      continue;
    }
    AddFoldPackBinding(search, kFoldPackTemplateArgument, NULL, pack_index,
                       NULL, argument->pack_arguments, pack_length, false);
  }
}

static void CollectFoldPacks(TemplateFunctionBodyClone* clone, ASTNode* pattern,
                             FoldPackSearch* search) {
  memset(search, 0, sizeof(*search));
  search->clone = clone;
  VectorInit(&search->bindings);
  ASTNodeVisit(pattern, FindFoldPacks, 0, search);
}

static ASTNode* CloneFoldPackElement(TemplateFunctionBodyClone* clone,
                                    ASTNode* pattern, FoldPackSearch* packs,
                                    size_t index) {
  Vector* element_args = TemplateArgumentVectorCopy(clone->args);
  for (size_t i = 0; i < packs->bindings.length; i++) {
    FoldPackBinding* binding = packs->bindings.value.p[i];
    if (binding->kind != kFoldPackTemplateArgument ||
        binding->template_parameter_index < 0 ||
        (size_t)binding->template_parameter_index >= element_args->length) {
      continue;
    }
    TemplateArgumentDelete(
        element_args->value.p[binding->template_parameter_index]);
    VectorSet(element_args, (size_t)binding->template_parameter_index,
              TemplateArgumentCopy(binding->replacements->value.p[index]));
  }
  ASTNode* pattern_clone =
      CloneDependentExpressionWithArgs(clone->parser, pattern, element_args);
  VectorDeleteWithContents(
      element_args, (VectorElementDestructor)TemplateArgumentDelete,
      /*free_element=*/false);
  if (pattern_clone == NULL) {
    pattern_clone = ASTNodeClone(pattern, IdentityCloneNode, NULL, NULL);
  }
  ASTNodeVisit(pattern_clone, DetachClonedPackExpansionCastTypes, 0, NULL);

  bool only_capture_packs = true;
  for (size_t i = 0; i < packs->bindings.length; i++) {
    FoldPackBinding* binding = packs->bindings.value.p[i];
    if (binding->kind == kFoldPackFunction) {
      only_capture_packs = false;
      ReplacePackIdentifierData replace = {
          .clone = clone,
          .from = binding->symbol,
          .to = binding->replacements->value.p[index],
          .element_index = index,
      };
      ASTNodeVisit(pattern_clone, ReplacePackIdentifierVisitor, 0, &replace);
    } else if (binding->kind == kFoldPackTemplateArgument) {
      only_capture_packs = false;
    } else if (binding->kind == kFoldPackLambdaCapture) {
      ReplaceLambdaCapturePackFieldData replace = {
          .clone = clone,
          .from_name = binding->capture_name,
          .to = binding->replacements->value.p[index],
          .element_index = index,
      };
      ASTNodeVisit(pattern_clone, ReplaceLambdaCapturePackFieldVisitor, 0,
                   &replace);
    }
  }
  if (only_capture_packs) {
    return pattern_clone;
  }
  ASTNodeVisit(pattern_clone, InstantiateClonedFunctionTemplateCallVisitor, 0,
               clone);
  return ASTNodeVisitAndTransform(
      pattern_clone, ReanalyzeClonedDependentFunctorCall, NULL);
}

/* Expand a fold expression (`(... op pack)` / `(pack op ...)` / binary folds)
 * in a cloned body into a left- or right-associated chain of the operator over
 * the concrete pack elements, using NewFoldIdentity for the empty case. */
static ASTNode* ExpandClonedFoldExpression(TemplateFunctionBodyClone* clone,
                                           ASTNode* node) {
  if (node == NULL || (node->flags & kASTFoldExpression) == 0 ||
      !IsFoldOperatorOpcode(node->op)) {
    return node;
  }

  BinaryASTNode* fold = (BinaryASTNode*)node;
  ASTOpcode fold_op =
      node->op == AST_OP(rshifteql) || node->op == AST_OP(rshifteqa)
          ? AST_OP(rshifteq)
          : node->op;
  bool pack_on_left = (node->flags & kASTFoldPackOnLeft) != 0;
  ASTNode* pack_node = pack_on_left ? fold->left : fold->right;
  ASTNode* seed = pack_on_left ? fold->right : fold->left;
  FoldPackSearch packs;
  CollectFoldPacks(clone, pack_node, &packs);
  if (packs.bindings.length == 0) {
    const char* capture_name = LambdaCapturePackMemberName(pack_node);
    Vector* replacements = LambdaCapturePackFieldReplacements(
        pack_node, CloneLambdaClosureOwner(clone));
    if (capture_name != NULL && replacements != NULL) {
      AddFoldPackBinding(&packs, kFoldPackLambdaCapture, NULL, -1,
                         capture_name, replacements, replacements->length,
                         true);
      packs.unresolved_pack = false;
    } else if (replacements != NULL) {
      VectorDelete(replacements);
    }
  }
  if (packs.bindings.length == 0 || packs.unresolved_pack) {
    DeleteFoldPackBindings(&packs);
    return node;
  }
  if (packs.mismatched_lengths) {
    SyntaxError(clone->parser->syntax,
                "pack expansion argument packs have different lengths");
    DeleteFoldPackBindings(&packs);
    return NewIntConstantASTNode(
        0, NewTypeRecordWithSize(kTypeInt, kQualPlain), node->location);
  }
  if (packs.length == 0) {
    DeleteFoldPackBindings(&packs);
    if (seed != NULL) {
      return seed;
    }
    return NewFoldIdentity(fold_op, node->location, clone->parser);
  }

  if (pack_on_left) {
    ASTNode* result = seed != NULL
                          ? seed
                          : CloneFoldPackElement(clone, pack_node, &packs,
                                                 packs.length - 1);
    size_t start = seed != NULL ? packs.length : packs.length - 1;
    for (size_t i = start; i > 0; i--) {
      ASTNode* left =
          CloneFoldPackElement(clone, pack_node, &packs, i - 1);
      result = NewBinaryASTNode(fold_op, NULL, node->location, left, result);
    }
    DeleteFoldPackBindings(&packs);
    return result;
  }

  ASTNode* result = seed != NULL
                        ? seed
                        : CloneFoldPackElement(clone, pack_node, &packs, 0);
  size_t start = seed != NULL ? 0 : 1;
  for (size_t i = start; i < packs.length; i++) {
    ASTNode* right = CloneFoldPackElement(clone, pack_node, &packs, i);
    result = NewBinaryASTNode(fold_op, NULL, node->location, result, right);
  }
  DeleteFoldPackBindings(&packs);
  return result;
}

/* Expand a scalar member-initializer whose initializer is a parameter-pack
 * expansion into a concrete assignment.  A variadic constructor initializing a
 * scalar member -- e.g. `__node(Args&&... args) : __value(forward<Args>(args)...)`
 * with `__value` an `int` -- is lowered by the constructor-preamble builder to a
 * plain assignment `this->__value = forward<Args>(args)...` rather than a
 * constructor call, so it never passes through RewriteClonedDependentNewInitializer.
 * Its pack must still be expanded against this instantiation: an empty pack
 * value-initializes the scalar, a single element supplies the value, and more
 * than one element is ill-formed.  Without this the unexpanded pattern (with
 * `args` still of pack type) survives, leaving the instantiated constructor body
 * with an unexpanded pack -- which code generation then silently skips, yielding
 * an undefined constructor symbol at link/run time. */
static void ExpandClonedScalarMemberInitPack(TemplateFunctionBodyClone* clone,
                                              ASTNode* node) {
  if (node == NULL || node->op != AST_OP(assign) ||
      (node->flags & kASTDependentNewInitializer) != 0) {
    return;
  }
  BinaryASTNode* assign = (BinaryASTNode*)node;
  if (assign->right == NULL ||
      (assign->right->flags & kASTPackExpansion) == 0) {
    return;
  }
  bool multiple_packs = false;
  Symbol* pack_symbol =
      PackExpansionExpressionSymbol(clone, assign->right, &multiple_packs);
  if (multiple_packs) {
    SyntaxError(clone->parser->syntax,
                "pack expansion argument packs have different lengths");
  }
  Vector* replacements =
      pack_symbol != NULL
          ? MapFindPointerKey(&clone->pack_symbol_map, pack_symbol)
          : NULL;
  if (replacements == NULL) {
    // The pack belongs to an enclosing, not-yet-instantiated template: leave the
    // pattern intact for the later (member-template) clone to expand.
    return;
  }
  if (replacements->length > 1) {
    SyntaxError(clone->parser->syntax,
                "too many initializers for scalar member");
    return;
  }
  TypeRecord* scalar_type =
      node->type != NULL
          ? node->type
          : (assign->left != NULL ? assign->left->type : NULL);
  ASTNode* pattern = ASTNodeMove(assign->right);
  ASTNode* expanded =
      replacements->length == 1
          ? ClonePackExpansionPattern(clone, pattern, pack_symbol,
                                      replacements->value.p[0], 0)
          : NewIntConstantASTNode(0, TypeRecordCopy(scalar_type),
                                  node->location);
  ASTNodeDelete(pattern);
  expanded->parent = node;
  expanded->child_id = 1;
  assign->right = expanded;
  node->flags &= ~kASTPackExpansion;
}

/* Build the member-access constructor call `receiver.Tag(actuals)` for a
 * dependent `new` initializer whose type resolved to the class `record`. */
static ASTNode* NewClonedDependentConstructorCall(TypeRecord* record,
                                                  ASTNode* receiver,
                                                  Vector* actuals,
                                                  SourceLocation location) {
  ASTNode* member = NewStringConstantASTNode(
      NewString(record->info.struct_info->tag_name->value), NULL, location);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, member);
  return NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals);
}

/* Rewrite a deferred `new` initializer (parsed as an assignment while T was
 * dependent) now that T has resolved to a concrete type.
 *
 *   * `new T(expr)` copy/direct-init becomes a constructor call
 *     `receiver.T(expr)` when T is a class, and stays a scalar assignment
 *     otherwise.
 *   * `new T()` value-init (flagged kASTDependentNewValueInit) becomes a
 *     default constructor call `receiver.T()` when T is a class with a
 *     constructor, an empty-compound-literal zero-init `*receiver = (T){}` when
 *     T is an aggregate class, and keeps its `*receiver = 0` scalar zero-init
 *     otherwise. */
static ASTNode* RewriteClonedDependentNewInitializer(
    TemplateFunctionBodyClone* clone, ASTNode* node) {
  if (node == NULL || node->op != AST_OP(assign) ||
      (node->flags & kASTDependentNewInitializer) == 0 || node->type == NULL) {
    return node;
  }
  BinaryASTNode* assign = (BinaryASTNode*)node;
  if (assign->left == NULL || assign->right == NULL) {
    return node;
  }
  // If the allocated type is still dependent (e.g. a `typename X<Target,T>::type`
  // whose member-template parameter `Target` is not yet concrete during the
  // enclosing class-template instantiation), leave the deferred initializer
  // intact.  Rewriting it now against the unresolved placeholder would bake in a
  // wrong scalar zero-init and lose the dependent-type information the later
  // member-template instantiation needs to resolve the real class type.
  if (TypeContainsTemplateParameter(node->type) ||
      node->type->dependent_member_name != NULL ||
      (node->type->type & kTypeUnknown) != 0) {
    return node;
  }
  bool is_class = TypeIsStructOrUnion(node->type) &&
                  node->type->info.struct_info != NULL &&
                  node->type->info.struct_info->tag_name != NULL;

  if ((node->flags & kASTDependentNewValueInit) != 0) {
    if (is_class && CXXRecordHasDefaultConstructorMember(node->type)) {
      // Class with a constructor: value-init runs the default constructor.
      return NewClonedDependentConstructorCall(node->type,
                                               ASTNodeMove(assign->left),
                                               NewVector(), node->location);
    }
    if (is_class) {
      // Aggregate class with no constructor: zero-initialize the object with an
      // empty compound literal `*receiver = (T){}`.
      ASTNode* receiver = ASTNodeMove(assign->left);
      Symbol* storage =
          SyntaxNewTemporary(clone->parser->syntax, node->type);
      ASTNode* literal = NewCompoundLiteralASTNode(
          NewIdentifierASTNode(storage, node->location), node->location,
          NewBracedInitializerASTNode(NewVector(), NULL, node->location));
      return NewBinaryASTNode(AST_OP(assign), node->type, node->location,
                              receiver, literal);
    }
    // Scalar: the placeholder `*receiver = 0` already zero-initializes it.
    node->flags &= ~(kASTDependentNewInitializer | kASTDependentNewValueInit);
    return node;
  }

  if (!is_class) {
    if (assign->right != NULL && assign->right->op == AST_OP(braced_init)) {
      BracedInitializerASTNode* braced =
          (BracedInitializerASTNode*)assign->right;
      bool dependent_initializer = false;
      for (size_t i = 0; i < braced->initializers->length; i++) {
        ASTNode* init = braced->initializers->value.p[i];
        if (init != NULL &&
            ((init->flags & kASTPackExpansion) != 0 ||
             DependentExpressionContainsTemplateParameter(init))) {
          dependent_initializer = true;
          break;
        }
      }
      if (dependent_initializer) {
        return node;
      }
      if (braced->initializers->length > 1) {
        SyntaxError(clone->parser->syntax,
                    "too many initializers for scalar new-expression");
        return node;
      }
      ASTNode* replacement = braced->initializers->length == 1
          ? ASTNodeMove(braced->initializers->value.p[0])
          : NewIntConstantASTNode(0, TypeRecordCopy(node->type),
                                  node->location);
      replacement->parent = node;
      replacement->child_id = 1;
      ASTNodeDelete(assign->right);
      assign->right = replacement;
      return node;
    }
    // `new T(expr)` where T resolved to a scalar keeps its scalar assignment.
    // When the initializer is a parameter-pack expansion (e.g. the allocator's
    // `new (ptr) U(std::forward<Args>(args)...)` with U deduced to a scalar),
    // expand it here against this instantiation's pack: an empty pack
    // value-initializes the scalar (`*receiver = 0`), a single-element pack
    // supplies the value, and more than one element is ill-formed.  Otherwise
    // the unexpanded pack would survive as a stray `*receiver = forward(args)`
    // whose callee template arguments are never substituted.
    if (assign->right != NULL &&
        (assign->right->flags & kASTPackExpansion) != 0) {
      bool multiple_packs = false;
      Symbol* pack_symbol =
          PackExpansionExpressionSymbol(clone, assign->right, &multiple_packs);
      if (multiple_packs) {
        SyntaxError(clone->parser->syntax,
                    "pack expansion argument packs have different lengths");
      }
      Vector* replacements =
          pack_symbol != NULL
              ? MapFindPointerKey(&clone->pack_symbol_map, pack_symbol)
              : NULL;
      if (replacements != NULL) {
        if (replacements->length > 1) {
          SyntaxError(clone->parser->syntax,
                      "too many initializers for scalar new-expression");
          return node;
        }
        ASTNode* pattern = ASTNodeMove(assign->right);
        ASTNode* expanded =
            replacements->length == 1
                ? ClonePackExpansionPattern(clone, pattern, pack_symbol,
                                            replacements->value.p[0], 0)
                : NewIntConstantASTNode(0, TypeRecordCopy(node->type),
                                        node->location);
        ASTNodeDelete(pattern);
        expanded->parent = node;
        expanded->child_id = 1;
        assign->right = expanded;
        node->flags &= ~kASTPackExpansion;
      }
    }
    return node;
  }
  Vector* actuals = NewVector();
  if (assign->right != NULL && assign->right->op == AST_OP(braced_init)) {
    BracedInitializerASTNode* braced =
        (BracedInitializerASTNode*)assign->right;
    for (size_t i = 0; i < braced->initializers->length; i++) {
      VectorAppend(actuals, ASTNodeMove(braced->initializers->value.p[i]));
    }
    ASTNodeDelete(assign->right);
  } else if (assign->right != NULL &&
             (assign->right->flags & kASTPackExpansion) != 0) {
    // `new (ptr) T(std::forward<Args>(args)...)`: expand the initializer pack
    // against this instantiation before it becomes constructor arguments.  An
    // empty pack value-initializes T (handled below for an aggregate); a
    // non-empty pack supplies one actual per element.  Without this the raw
    // pack-expansion pattern would survive as a single bogus argument whose
    // callee template arguments are never substituted.
    bool multiple_packs = false;
    Symbol* pack_symbol =
        PackExpansionExpressionSymbol(clone, assign->right, &multiple_packs);
    if (multiple_packs) {
      SyntaxError(clone->parser->syntax,
                  "pack expansion argument packs have different lengths");
    }
    Vector* replacements =
        pack_symbol != NULL
            ? MapFindPointerKey(&clone->pack_symbol_map, pack_symbol)
            : NULL;
    if (replacements == NULL) {
      // The pack is not resolvable at this level (e.g. it belongs to an
      // enclosing template): leave the deferred initializer for a later clone.
      VectorDelete(actuals);
      return node;
    }
    ASTNode* pattern = ASTNodeMove(assign->right);
    for (size_t i = 0; i < replacements->length; i++) {
      VectorAppend(actuals,
                   ClonePackExpansionPattern(clone, pattern, pack_symbol,
                                             replacements->value.p[i], i));
    }
    ASTNodeDelete(pattern);
  } else {
    VectorAppend(actuals, ASTNodeMove(assign->right));
  }

  ASTNode* receiver = ASTNodeMove(assign->left);
  if (actuals->length == 0 &&
      !CXXRecordHasDefaultConstructorMember(node->type)) {
    // Value-initialize an aggregate class with no constructor by zero-init'ing
    // the object with an empty compound literal `*receiver = (T){}`.
    VectorDelete(actuals);
    Symbol* storage = SyntaxNewTemporary(clone->parser->syntax, node->type);
    ASTNode* literal = NewCompoundLiteralASTNode(
        NewIdentifierASTNode(storage, node->location), node->location,
        NewBracedInitializerASTNode(NewVector(), NULL, node->location));
    return NewBinaryASTNode(AST_OP(assign), node->type, node->location,
                            receiver, literal);
  }
  return NewClonedDependentConstructorCall(node->type, receiver, actuals,
                                           node->location);
}

/* True if `node` or any ancestor is itself a pack expansion (so it will be
 * materialized per-element by the surrounding expansion, not here). */
static bool ASTNodeWithinPackExpansion(ASTNode* node) {
  for (ASTNode* current = node; current != NULL; current = current->parent) {
    if ((current->flags & kASTPackExpansion) != 0) {
      return true;
    }
  }
  return false;
}

/* True if `node` sits inside a pack-expansion pattern (`pattern...`) whose pack
 * is NOT resolved by this clone -- i.e. it belongs to a not-yet-instantiated
 * nested/member template (e.g. `std::forward<Args>(args)...` in a variadic
 * member template while its enclosing class template is being instantiated).
 * Sub-expressions of such a pattern must be left untouched: their per-element
 * expansion happens later, at the member template's own instantiation, so
 * concretizing their template arguments now (against the enclosing class's
 * arguments) would bind the wrong pack. */
static bool ASTNodeWithinUnresolvedPackExpansion(TemplateFunctionBodyClone* clone,
                                                 ASTNode* node) {
  for (ASTNode* current = node; current != NULL; current = current->parent) {
    if ((current->flags & kASTPackExpansion) != 0 &&
        ClonePatternReferencesUnresolvedPack(clone, current)) {
      return true;
    }
  }
  return false;
}

static StructMember* FindClonedConcreteMember(Struct* receiver,
                                               StructMember* source) {
  if (receiver == NULL || source == NULL || source->symbol == NULL) {
    return NULL;
  }
  TypeRecord* source_type = source->symbol->type;
  String* lookup_name = &source->symbol->name;
  if (source_type != NULL && TypeIsFunction(source_type) &&
      source_type->info.function.is_constructor &&
      receiver->tag_name != NULL) {
    lookup_name = receiver->tag_name;
  }
  StructMember* head = FindStructMember(receiver, lookup_name);
  if (head == NULL || source_type == NULL || !TypeIsFunction(source_type)) {
    return head;
  }

  StructMember* match = NULL;
  for (StructMember* candidate = head; candidate != NULL;
       candidate = candidate->overload_next) {
    TypeRecord* candidate_type =
        candidate->symbol != NULL ? candidate->symbol->type : NULL;
    if (candidate_type == NULL || !TypeIsFunction(candidate_type) ||
        candidate_type->info.function.prototype.length !=
            source_type->info.function.prototype.length) {
      continue;
    }
    CXXSpecialMemberKind source_kind =
        source_type->info.function.cxx_special_member_kind;
    if (source_kind != kCXXSpecialMemberNone &&
        candidate_type->info.function.cxx_special_member_kind != source_kind) {
      continue;
    }
    bool parameter_match = true;
    for (size_t i = 1; i < source_type->info.function.prototype.length; i++) {
      Symbol* source_parameter =
          source_type->info.function.prototype.value.p[i];
      Symbol* candidate_parameter =
          candidate_type->info.function.prototype.value.p[i];
      if (source_parameter == NULL || candidate_parameter == NULL) {
        parameter_match = source_parameter == candidate_parameter;
      } else if (!TypeContainsTemplateParameter(source_parameter->type) &&
                 !TypeEqual(source_parameter->type,
                            candidate_parameter->type)) {
        parameter_match = false;
      }
      if (!parameter_match) {
        break;
      }
    }
    if (parameter_match) {
      if (match != NULL) {
        return NULL;
      }
      match = candidate;
    }
  }
  return match;
}

static Symbol* FindClonedOwnerMemberSymbol(TemplateFunctionBodyClone* clone,
                                           Symbol* source) {
  if (clone == NULL || source == NULL || clone->from_owner == NULL ||
      clone->to_owner == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < clone->from_owner->members.length; i++) {
    StructMember* original = clone->from_owner->members.value.p[i];
    if (original == NULL || original->symbol != source) {
      continue;
    }
    StructMember* concrete =
        i < clone->to_owner->members.length
            ? clone->to_owner->members.value.p[i] : NULL;
    if (concrete != NULL && concrete->symbol != NULL &&
        concrete->is_static == original->is_static &&
        concrete->is_member_function == original->is_member_function) {
      return concrete->symbol;
    }
    concrete = FindClonedConcreteMember(clone->to_owner, original);
    return concrete != NULL ? concrete->symbol : NULL;
  }
  return NULL;
}

static void RebindClonedConcreteMemberAccess(
    TemplateFunctionBodyClone* clone, ASTNode* node) {
  if (node == NULL ||
      (node->op != AST_OP(dot) && node->op != AST_OP(arrow))) {
    return;
  }
  BinaryASTNode* access = (BinaryASTNode*)node;
  if (access->right == NULL ||
      access->right->op != AST_OP(structmember)) {
    return;
  }
  StructMemberASTNode* member_node =
      (StructMemberASTNode*)access->right;
  TypeRecord* receiver_type =
      access->left != NULL ? access->left->type : NULL;
  if (receiver_type != NULL && node->op == AST_OP(arrow) &&
      TypeIsPointer(receiver_type)) {
    receiver_type = receiver_type->next;
  }
  Struct* receiver_owner =
      receiver_type != NULL && TypeIsStructOrUnion(receiver_type)
          ? receiver_type->info.struct_info
          : NULL;
  bool rebind_owner_by_clone =
      clone != NULL && clone->from_owner != NULL &&
      clone->from_owner->tag_symbol != NULL &&
      !clone->from_owner->tag_symbol->flags.invented &&
      clone->to_owner != NULL;
  if (member_node->member != NULL && clone != NULL &&
      rebind_owner_by_clone &&
      receiver_owner == clone->from_owner &&
      member_node->member->index < clone->to_owner->members.length) {
    StructMember* indexed =
        clone->to_owner->members.value.p[member_node->member->index];
    if (indexed != NULL && indexed->symbol != NULL &&
        indexed->is_member_function == member_node->member->is_member_function &&
        indexed->is_static == member_node->member->is_static) {
      StructMemberASTNodeSetMember(member_node, indexed);
      node->flags &= ~kASTAnalyzed;
    }
  }
  if (rebind_owner_by_clone &&
      receiver_owner == clone->from_owner) {
    receiver_owner = clone->to_owner;
  }
  if (receiver_owner != NULL &&
      member_node->member != NULL && member_node->member->symbol != NULL) {
    StructMember* concrete = FindClonedConcreteMember(
        receiver_owner, member_node->member);
    if (concrete != NULL && concrete->symbol != NULL) {
      StructMemberASTNodeSetMember(member_node, concrete);
      node->flags &= ~kASTAnalyzed;
    }
  }
  if (member_node->member != NULL && member_node->member->symbol != NULL) {
    ASTNodeSetType(node, member_node->member->symbol->type);
    if (!member_node->member->is_member_function) {
      node->value_category = kValueCategoryLvalue;
    }
  }
}

static void RebindClonedConcreteMemberAccessVisitor(
    ASTNode* node, void* data, int child_id, VisitorMode mode) {
  (void)child_id;
  if (mode == kVisitPostChildren) {
    RebindClonedConcreteMemberAccess(data, node);
  }
}

/* Match the source spelling of a class-valued member alias without the
 * permissive canonical type equality used by overload resolution.  Distinct
 * aliases of the same primary template (allocator_traits<allocator_type> and
 * allocator_traits<node_allocator>) must not collapse here. */
static bool TypeRecordHasSameAliasTemplateId(TypeRecord* left,
                                             TypeRecord* right) {
  if (left == NULL || right == NULL || left->declarator != right->declarator ||
      left->type != right->type || !TypeIsStructOrUnion(left) ||
      !TypeIsStructOrUnion(right) ||
      left->info.struct_info != right->info.struct_info ||
      left->template_origin != right->template_origin) {
    return false;
  }
  return TemplateArgumentVectorEqual(left->template_arguments,
                                     right->template_arguments);
}

/* Per-node transform applied while cloning a template function body for one
 * instantiation. It turns the dependent generic AST into a concrete AST by:
 *   - evaluating `sizeof...(pack)` to the concrete pack length;
 *   - expanding fold expressions and dependent `delete`;
 *   - substituting identifier template arguments and rebinding symbols;
 *   - substituting cast/declaration types against the instantiation args;
 *   - rewriting deferred dependent `new` initializers and member calls.
 * Returns the (possibly replacement) node for this position. */
static ASTNode* RewriteClonedDependentMemberAddress(ASTNode* node) {
  if (node == NULL || node->op != AST_OP(address)) {
    return node;
  }
  UnaryASTNode* address = (UnaryASTNode*)node;
  if (address->sub == NULL || address->sub->op != AST_OP(identifier) ||
      (address->sub->flags & kASTDependentQualifiedName) == 0) {
    return node;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)address->sub;
  if (id->symbol == NULL || id->symbol->type == NULL ||
      !TypeIsFunction(id->symbol->type)) {
    return node;
  }
  Struct* owner = id->symbol->type->info.function.cxx_member_owner;
  StructMember* member =
      owner != NULL ? FindStructMember(owner, &id->symbol->name) : NULL;
  if (member == NULL || member->symbol != id->symbol || member->is_static ||
      !member->is_member_function || member->overload_next != NULL) {
    return node;
  }

  TypeRecord* member_type = TypeMemberPointerPointeeFromMember(member);
  TypeRecord* member_pointer =
      NewMemberPointerTypeRecord(owner, kQualPlain);
  TypeRecordChain(member_pointer, member_type);
  TypeRecordCalculateSize(member_pointer);
  StructMemberASTNode* member_node =
      (StructMemberASTNode*)NewStructMemberASTNode(member, node->location);
  ASTNode* result = NewUnaryASTNode(
      AST_OP(member_ptr), member_pointer, node->location,
      (ASTNode*)member_node);
  result->value_category = kValueCategoryPrvalue;
  ASTNodeDelete(node);
  return result;
}

static void QueueODRUsedStaticDataMember(StructMember* member) {
  if (member == NULL || member->symbol == NULL || !member->is_static ||
      member->is_member_function || member->default_initializer == NULL) {
    return;
  }
  Symbol* symbol = member->symbol;
  if (symbol->asm_name.length == 0) {
    SymbolSetCXXMangledAsmName(symbol);
  }
  if (PendingTemplateInstantiationHasAsmName(symbol->asm_name.value)) {
    return;
  }

  symbol->flags.address_taken = true;
  symbol->flags.is_defined = true;
  symbol->flags.is_template = false;
  symbol->flags.is_weak = true;
  Vector* declarations = NewVector();
  VectorAppend(
      declarations,
      NewVariableDeclarationASTNode(
          symbol, CloneCXXDefaultMemberInitializer(member->default_initializer),
          symbol->location));
  CompilerQueuePendingTemplateInstantiation(
      NewDeclarationListASTNode(declarations, symbol->location));
}

static Struct* FindOwnerBaseStructMatchingType(TemplateFunctionBodyClone* clone,
                                               Struct* str,
                                               TypeRecord* target);

static void MaterializeClonedQualifiedBaseStructMember(
    TemplateFunctionBodyClone* clone, StructMemberASTNode* member_node,
    ASTNode* node) {
  if (clone == NULL || member_node == NULL ||
      member_node->owner_type == NULL || member_node->member == NULL ||
      member_node->member->symbol == NULL) {
    return;
  }
  TypeRecord* owner_type = member_node->owner_type;
  if (!TypeContainsTemplateParameter(owner_type) &&
      owner_type->template_origin == NULL) {
    return;
  }
  TypeRecord* scope = TypeRecordCopy(owner_type);
  TypeRecord* concrete =
      SubstituteTemplateParameters(clone->parser, scope, clone->args);
  RebaseTemplateParameterIndices(concrete, clone->rebase_template_parameter_base);
  concrete = TypeMaterializeClassTemplateSpecialization(
      clone->parser->syntax, concrete);
  TypeRecordDelete(scope);
  if (concrete == NULL || !TypeIsStructOrUnion(concrete) ||
      concrete->info.struct_info == NULL) {
    TypeRecordDelete(concrete);
    return;
  }
  Struct* lookup_struct = concrete->info.struct_info;
  if (clone->to_owner != NULL) {
    Struct* owner_base = FindOwnerBaseStructMatchingType(clone, clone->to_owner,
                                                         concrete);
    if (owner_base != NULL) {
      lookup_struct = owner_base;
    }
  }
  StructMember* resolved =
      FindStructMemberByName(lookup_struct,
                             member_node->member->symbol->name.value);
  if (resolved == NULL || resolved->symbol == NULL) {
    TypeRecordDelete(concrete);
    return;
  }
  member_node->member = resolved;
  TypeRecordDelete(member_node->owner_type);
  member_node->owner_type = concrete;
  TypeRecordIncRef(member_node->owner_type);
  ASTNodeSetType(node, resolved->symbol->type);
}

static Struct* FindOwnerBaseStructMatchingType(TemplateFunctionBodyClone* clone,
                                               Struct* str,
                                               TypeRecord* target) {
  if (clone == NULL || str == NULL || target == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base->type == NULL || !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    TypeRecord* materialized = TypeMaterializeClassTemplateSpecialization(
        clone->parser->syntax, base->type);
    TypeRecord* compare_type =
        materialized != NULL ? materialized : base->type;
    if (TypeEqual(compare_type, target)) {
      Struct* result = compare_type->info.struct_info;
      if (materialized != NULL && materialized != base->type) {
        TypeRecordDelete(materialized);
      }
      return result;
    }
    if (materialized != NULL && materialized != base->type) {
      TypeRecordDelete(materialized);
    }
    Struct* nested = FindOwnerBaseStructMatchingType(
        clone, base->type->info.struct_info, target);
    if (nested != NULL) {
      return nested;
    }
  }
  return NULL;
}

static ASTNode* FoldClonedPackIndex(TemplateFunctionBodyClone* clone,
                                    ASTNode* node) {
  if (node == NULL || node->op != AST_OP(pack_index)) {
    return node;
  }
  BinaryASTNode* pack_index = (BinaryASTNode*)node;
  if (pack_index->left == NULL ||
      pack_index->left->op != AST_OP(identifier) ||
      pack_index->right == NULL) {
    return node;
  }

  int64_t index = 0;
  if (!EvaluateIntegerExpression(pack_index->right, &index)) {
    if (DependentExpressionContainsTemplateParameter(pack_index->right)) {
      return node;
    }
    SemanticError(pack_index->right,
                  "pack index must be a constant expression");
    return NewIntConstantASTNode(
        0, NewTypeRecordWithSize(kTypeInt, kQualPlain), node->location);
  }
  IdentifierASTNode* id = (IdentifierASTNode*)pack_index->left;
  Symbol* pack_symbol = id->symbol;

  Vector* replacements =
      MapFindPointerKey(&clone->pack_symbol_map, pack_symbol);
  if (replacements != NULL) {
    if (index < 0 || (uint64_t)index >= replacements->length) {
      SemanticError(pack_index->right,
                    "pack index %" PRId64
                    " is out of bounds for a pack of length %zu",
                    index, replacements->length);
      return NewIntConstantASTNode(
          0, NewTypeRecordWithSize(kTypeInt, kQualPlain), node->location);
    }
    Symbol* replacement = replacements->value.p[(size_t)index];
    ASTNode* result =
        NewIdentifierASTNode(replacement, node->location);
    result->flags = node->flags & ~kASTAnalyzed;
    return result;
  }

  int parameter_index =
      pack_symbol != NULL ? pack_symbol->template_parameter_index : -1;
  if (parameter_index < 0 || clone->args == NULL ||
      (size_t)parameter_index >= clone->args->length) {
    return node;
  }
  TemplateArgument* pack = clone->args->value.p[parameter_index];
  if (pack == NULL || pack->pack_arguments == NULL) {
    return node;
  }
  if (index < 0 || (uint64_t)index >= pack->pack_arguments->length) {
    SemanticError(pack_index->right,
                  "pack index %" PRId64
                  " is out of bounds for a pack of length %zu",
                  index, pack->pack_arguments->length);
    return NewIntConstantASTNode(
        0, NewTypeRecordWithSize(kTypeInt, kQualPlain), node->location);
  }
  ASTNode* result = TemplateArgumentMaterializeExpression(
      pack->pack_arguments->value.p[(size_t)index], node->location);
  return result != NULL ? result : node;
}

static TypeRecord* StructuredBindingInitializerType(
    StructuredBindingASTNode* binding) {
  if (binding == NULL || binding->initializer == NULL) {
    return NULL;
  }
  ASTNode* initializer = binding->initializer;
  if (initializer->op == AST_OP(expr_init)) {
    ASTNode* expr = ((ExpressionInitializerASTNode*)initializer)->expr;
    return expr != NULL ? expr->type : NULL;
  }
  return initializer->type;
}

static bool ClonedStructuredBindingStillDependent(
    StructuredBindingASTNode* binding) {
  TypeRecord* type = StructuredBindingInitializerType(binding);
  return type == NULL || TypeIsUnknown(type) || TypeContainsAuto(type) ||
         TypeContainsTemplateParameter(type) ||
         ExpressionIsTemplateDependent(binding->initializer);
}

ASTNode* CloneTemplateFunctionBodyNode(ASTNode* node, void* data) {
  TemplateFunctionBodyClone* clone = data;
  if (node->op == AST_OP(structured_binding)) {
    StructuredBindingASTNode* binding = (StructuredBindingASTNode*)node;
    TypeRecord* declared =
        SubstituteTemplateBodyType(clone, binding->declared_type);
    RebaseTemplateParameterIndices(
        declared, clone->rebase_template_parameter_base);
    TypeRecordDelete(binding->declared_type);
    binding->declared_type = declared;
    ASTNodeSetType(node, declared);
    node->flags &= ~kASTAnalyzed;
    if (ClonedStructuredBindingStillDependent(binding)) {
      return node;
    }
    ASTNodeVisit(binding->initializer, ClearAnalyzedFlagVisitor, 0, NULL);
    return SemanticMaterializeClonedStructuredBinding(
        node, &clone->symbol_map, &clone->pack_symbol_map);
  }
  if (node->op == AST_OP(expansion_for)) {
    ExpansionStatementASTNode* expansion = (ExpansionStatementASTNode*)node;
    if (expansion->binding_type != NULL) {
      TypeRecord* declared =
          SubstituteTemplateBodyType(clone, expansion->binding_type);
      RebaseTemplateParameterIndices(
          declared, clone->rebase_template_parameter_base);
      TypeRecordDelete(expansion->binding_type);
      expansion->binding_type = declared;
    }
    if (expansion->item_symbol != NULL) {
      Symbol* mapped =
          MapFindPointerKey(&clone->symbol_map, expansion->item_symbol);
      if (mapped != NULL) {
        expansion->item_symbol = mapped;
      }
    }
    if (expansion->binding_symbols != NULL) {
      for (size_t i = 0; i < expansion->binding_symbols->length; i++) {
        Symbol* source = expansion->binding_symbols->value.p[i];
        Symbol* mapped = MapFindPointerKey(&clone->symbol_map, source);
        if (mapped != NULL) {
          expansion->binding_symbols->value.p[i] = mapped;
        }
      }
    }
    node->flags &= ~kASTAnalyzed;
    return node;
  }
  ASTNode* indexed = FoldClonedPackIndex(clone, node);
  if (indexed != node) {
    return indexed;
  }
  if (node->op == AST_OP(ptr_scale)) {
    PtrScaleASTNode* scale = (PtrScaleASTNode*)node;
    TypeRecord* ref_type =
        SubstituteTemplateBodyType(clone, scale->ref_type);
    RebaseTemplateParameterIndices(ref_type,
                                   clone->rebase_template_parameter_base);
    TypeRecordCalculateSize(ref_type);
    TypeRecordDelete(scale->ref_type);
    scale->ref_type = ref_type;
  }
  if (node->op == AST_OP(requires_expr)) {
    RequiresExpressionASTNode* requires_node =
        (RequiresExpressionASTNode*)node;
    SourceLocation location = node->location;
    int64_t value = 0;
    bool ok = ConceptsEvaluateConstraintWithArguments(
        requires_node->constraint, clone->args, &value);
    ConstraintExprDelete(requires_node->constraint);
    requires_node->constraint = NULL;
    ASTNodeDelete(node);
    return NewIntConstantASTNode(ok && value != 0 ? 1 : 0,
                                 NewTypeRecordWithSize(kTypeBool, kQualPlain),
                                 location);
  }
  if (node->op == AST_OP(builtin_type_trait)) {
    VectorASTNode* trait = (VectorASTNode*)node;
    if (trait->children != NULL && trait->children->length > 1) {
      Vector* new_children = NewVector();
      VectorAppend(new_children, trait->children->value.p[0]);
      bool expanded_pack = false;
      for (size_t i = 1; i < trait->children->length; i++) {
        ASTNode* arg = trait->children->value.p[i];
        if (arg != NULL && (arg->flags & kASTPackExpansion) != 0 &&
            arg->type != NULL) {
          int pack_index = arg->type->template_parameter_index;
          if (pack_index < 0) {
            TypeIsTemplateParameterPlaceholder(arg->type, &pack_index);
          }
          if (pack_index >= 0 && (size_t)pack_index < clone->args->length) {
            TemplateArgument* pack = clone->args->value.p[pack_index];
            if (pack != NULL && pack->pack_arguments != NULL &&
                pack->pack_arguments->length > 0) {
              expanded_pack = true;
              for (size_t j = 0; j < pack->pack_arguments->length; j++) {
                TemplateArgument* element = pack->pack_arguments->value.p[j];
                if (element != NULL && element->type != NULL) {
                  TypeRecord* concrete = TypeRecordCopy(element->type);
                  concrete = TypeMaterializeClassTemplateSpecialization(
                      clone->parser->syntax, concrete);
                  VectorAppend(
                      new_children,
                      (ASTNode*)NewIntConstantASTNode(0, concrete, arg->location));
                }
              }
              continue;
            }
          }
          size_t param_index = i - 1;
          if (param_index < clone->args->length) {
            expanded_pack = true;
            for (size_t j = param_index; j < clone->args->length; j++) {
              TemplateArgument* element = clone->args->value.p[j];
              if (element != NULL && element->type != NULL) {
                TypeRecord* concrete = TypeRecordCopy(element->type);
                concrete = TypeMaterializeClassTemplateSpecialization(
                    clone->parser->syntax, concrete);
                VectorAppend(
                    new_children,
                    (ASTNode*)NewIntConstantASTNode(0, concrete, arg->location));
              }
            }
            continue;
          }
          expanded_pack = true;
          continue;
        }
        if (arg != NULL && arg->type != NULL) {
          TypeRecord* subst = SubstituteTemplateParameters(
              clone->parser, arg->type, clone->args);
          if (subst != NULL) {
            subst = TypeMaterializeClassTemplateSpecialization(
                clone->parser->syntax, subst);
            ASTNodeSetType(arg, subst);
          }
          arg->flags &= ~kASTPackExpansion;
        }
        if (arg != NULL) {
          VectorAppend(new_children, arg);
        }
      }
      if (expanded_pack) {
        VectorDelete(trait->children);
        trait->children = new_children;
      }
      node->flags &= ~kASTAnalyzed;
    }
  }
  if (node->op == AST_OP(structmember) &&
      (node->flags & kASTQualifiedName) != 0) {
    MaterializeClonedQualifiedBaseStructMember(
        clone, (StructMemberASTNode*)node, node);
  }
  if (node->op == AST_OP(structmember) && clone->from_owner != NULL &&
      clone->to_owner != NULL && clone->from_owner != clone->to_owner &&
      (node->flags & kASTQualifiedName) == 0) {
    StructMemberASTNode* member_node = (StructMemberASTNode*)node;
    if (member_node->member != NULL && member_node->member->symbol != NULL) {
      StructMember* concrete =
          FindStructMemberByName(clone->to_owner,
                                 member_node->member->symbol->name.value);
      if (concrete != NULL && concrete->symbol != NULL) {
        member_node->member = concrete;
        member_node->access = concrete->access;
        member_node->byte_offset = concrete->byte_offset;
        ASTNodeSetType(node, concrete->symbol->type);
      }
    }
  }
  // Explicit template arguments on an accessed member function template, e.g.
  // `obj.template mfn<I>()`, are held on the StructMemberASTNode rather than on
  // an identifier. Substitute the instantiation's arguments into them (mirroring
  // the identifier handling below) so a dependent explicit argument like `I`
  // becomes concrete and the member call can be resolved during instantiation.
  // Explicit template arguments on an accessed member function template, e.g.
  // `obj.template mfn<I>()`, are held on the member-name node (a StructMember
  // node once resolved, or an as-yet-unanalyzed string constant) rather than on
  // an ordinary identifier. Substitute the instantiation's arguments into them
  // (mirroring the identifier handling below) so a dependent explicit argument
  // like `I` becomes concrete and the member call resolves during instantiation.
  {
    Vector** member_template_args = NULL;
    if (node->op == AST_OP(structmember)) {
      member_template_args = &((StructMemberASTNode*)node)->template_arguments;
    } else if (node->op == AST_OP(string)) {
      member_template_args = &((ConstantASTNode*)node)->template_arguments;
    }
    if (member_template_args != NULL && *member_template_args != NULL) {
      Vector* member_args = *member_template_args;
      bool member_template_args_contain_pack = false;
      int pack_index = -1;
      size_t pack_length = 0;
      for (size_t i = 0; i < member_args->length; i++) {
        TemplateArgument* member_arg = member_args->value.p[i];
        if (member_arg != NULL && member_arg->is_pack_expansion &&
            FindPackExpansionInTemplateArgument(
                member_arg, clone->args, &pack_index, &pack_length)) {
          member_template_args_contain_pack = true;
          break;
        }
      }
      if (!member_template_args_contain_pack &&
          !ASTNodeWithinPackExpansion(node)) {
        Vector* concrete_args = SubstituteTemplateArgumentVector(
            clone->parser, member_args, clone->args,
            clone->rebase_template_parameter_base);
        VectorDeleteWithContents(
            member_args, (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
        *member_template_args = concrete_args;
      } else if (clone->rebase_template_parameter_base > 0) {
        for (size_t i = 0; i < member_args->length; i++) {
          RebaseTemplateArgumentParameterIndices(
              member_args->value.p[i],
              clone->rebase_template_parameter_base);
        }
      }
    }
  }
  RebindClonedConcreteMemberAccess(clone, node);
  // A dependent member access `recv.name` / `recv->name` whose member is still an
  // unresolved *string* name (its receiver's type was dependent when the
  // template was defined) must be rebound after substitution.  Clear its
  // analyzed state and placeholder type so semantic analysis re-resolves `name`
  // against the now-concrete receiver.  Without this a member-initializer such
  // as `p(o.p)` in a member-template constructor -- whose preamble is baked and
  // analyzed as dependent while the enclosing class template is instantiated
  // (the member's own parameters, and hence `o`'s sibling-specialization type,
  // are still unbound) -- keeps its placeholder type and is never rebound at
  // per-call instantiation, yielding a bogus "cannot convert from <receiver>"
  // error.  A member access already resolved to a concrete `structmember` (see
  // above) is untouched.
  if ((node->op == AST_OP(dot) || node->op == AST_OP(arrow)) &&
      ((BinaryASTNode*)node)->right != NULL &&
      ((BinaryASTNode*)node)->right->op == AST_OP(string)) {
    node->flags &= ~kASTAnalyzed;
    ASTNodeClearType(node);
  }
  if (node->op == AST_OP(sizeof) || node->op == AST_OP(alignof)) {
    SizeofASTNode* sizeof_node = (SizeofASTNode*)node;
    if (node->op == AST_OP(sizeof) && sizeof_node->is_pack_size &&
        sizeof_node->expr != NULL) {
      if (sizeof_node->expr->op == AST_OP(identifier)) {
        IdentifierASTNode* id = (IdentifierASTNode*)sizeof_node->expr;
        Vector* replacements = MapFindPointerKey(&clone->pack_symbol_map,
                                                 id->symbol);
        if (replacements != NULL) {
          return NewIntConstantASTNode(
              (int64_t)replacements->length, NewSizeTypeRecord(),
              node->location);
        }
        int pack_index =
            id->symbol != NULL ? id->symbol->template_parameter_index : -1;
        if ((pack_index < 0) && id->symbol != NULL) {
          TypeIsTemplateParameterPlaceholder(id->symbol->type, &pack_index);
        }
        if (id->symbol != NULL && pack_index >= 0 &&
            (size_t)pack_index < clone->args->length) {
          TemplateArgument* arg = clone->args->value.p[pack_index];
          if (arg != NULL && arg->pack_arguments != NULL) {
            return NewIntConstantASTNode(
                (int64_t)arg->pack_arguments->length, NewSizeTypeRecord(),
                node->location);
          }
        }
      }
      Vector* capture_replacements =
          LambdaCapturePackFieldReplacements(sizeof_node->expr, CloneLambdaClosureOwner(clone));
      if (capture_replacements != NULL) {
        size_t length = capture_replacements->length;
        VectorDelete(capture_replacements);
        size_t argument_length = 0;
        if (LambdaCapturePackFieldArgumentLength(sizeof_node->expr,
                                                 clone->args,
                                                 &argument_length,
                                                 CloneLambdaClosureOwner(clone))) {
          length = argument_length;
        }
        return NewIntConstantASTNode((int64_t)length, NewSizeTypeRecord(),
                                     node->location);
      }
    }
    if (sizeof_node->expr != NULL && !sizeof_node->is_pack_size &&
        sizeof_node->expr->type != NULL &&
        !TypeContainsTemplateParameter(sizeof_node->expr->type)) {
      TypeRecord* concrete = sizeof_node->expr->type;
      if (CompilerIsCXX() && TypeIsReference(concrete)) {
        concrete = concrete->next;
      }
      TypeRecordCalculateSize(concrete);
      if (!TypeIsVLA(concrete)) {
        sizeof_node->base.value.ivalue =
            node->op == AST_OP(alignof) ? TypeRecordAlignment(concrete)
                                       : concrete->size;
      }
    }
    // `sizeof(dependent-type)`: substitute the retained operand type and
    // recompute the size for this instantiation.
    if (sizeof_node->type_operand != NULL) {
      TypeRecord* concrete = sizeof_node->type_operand;
      if (TypeContainsTemplateParameter(concrete)) {
        concrete = SubstituteTemplateParameters(
            clone->parser, concrete, clone->args);
        RebaseTemplateParameterIndices(
            concrete, clone->rebase_template_parameter_base);
        TypeRecordDelete(sizeof_node->type_operand);
        sizeof_node->type_operand = concrete;
      }
      TypeRecord* measured = concrete;
      if (CompilerIsCXX() && TypeIsReference(measured)) {
        measured = measured->next;
      }
      TypeRecordCalculateSize(measured);
      sizeof_node->base.value.ivalue =
          node->op == AST_OP(alignof) ? TypeRecordAlignment(measured)
                                     : measured->size;
    }
  }
  if (node->op == AST_OP(typeid)) {
    TypeidASTNode* typeid_node = (TypeidASTNode*)node;
    if (typeid_node->operand_type != NULL &&
        TypeContainsTemplateParameter(typeid_node->operand_type)) {
      TypeRecord* concrete = SubstituteTemplateParameters(
          clone->parser, typeid_node->operand_type, clone->args);
      RebaseTemplateParameterIndices(
          concrete, clone->rebase_template_parameter_base);
      TypeRecordCalculateSize(concrete);
      TypeRecordDelete(typeid_node->operand_type);
      typeid_node->operand_type = concrete;
    }
  }
  if (node->op == AST_OP(reflect)) {
    ReflectionASTNode* reflection = (ReflectionASTNode*)node;
    node->flags &= ~kASTAnalyzed;
    if (reflection->operand_type != NULL &&
        TypeContainsTemplateParameter(reflection->operand_type)) {
      TypeRecord* concrete = SubstituteTemplateParameters(
          clone->parser, reflection->operand_type, clone->args);
      RebaseTemplateParameterIndices(
          concrete, clone->rebase_template_parameter_base);
      TypeRecordCalculateSize(concrete);
      TypeRecordDelete(reflection->operand_type);
      reflection->operand_type = concrete;
    }
  }
  node = RewriteClonedDependentMemberAddress(node);
  if (node->op == AST_OP(member_ptr)) {
    return node;
  }
  if ((node->flags & kASTFoldExpression) != 0) {
    return ExpandClonedFoldExpression(clone, node);
  }
  if ((node->flags & kASTDependentDelete) != 0 &&
      node->op == AST_OP(call)) {
    VectorASTNode* call = (VectorASTNode*)node;
    if (call->children != NULL && call->children->length == 1) {
      ASTNode* expr = call->children->value.p[0];
      if (expr->type != NULL && TypeContainsTemplateParameter(expr->type)) {
        TypeRecord* concrete = SubstituteTemplateParameters(
            clone->parser, expr->type, clone->args);
        RebaseTemplateParameterIndices(
            concrete, clone->rebase_template_parameter_base);
        TypeRecordCalculateSize(concrete);
        ASTNodeSetType(expr, concrete);
        TypeRecordDelete(concrete);
      }
      bool is_array_delete = (node->flags & kASTDependentArrayDelete) != 0;
      return NewCXXDeleteExpressionForPointer(clone->parser->syntax, expr,
                                              is_array_delete,
                                              node->location,
                                              /*global_scope=*/false);
    }
  }
  // Injected-class-name as a functional-cast callee: `ClassName(args)` inside a
  // member body of the primary template must construct a temporary of *this*
  // instantiation, not the generic template.  We only rewrite the call's callee
  // identifier (not other uses of the name, e.g. as a type in a cast or a
  // declared object's type, which must stay pure type references); otherwise the
  // tag would be misused as runtime storage.  Children are cloned before this
  // callback runs, so `call->left` is already the cloned callee here.
  if (node->op == AST_OP(call) && clone->from_owner != NULL &&
      clone->to_owner != NULL && clone->from_owner != clone->to_owner &&
      clone->to_owner->tag_symbol != NULL &&
      clone->to_owner->tag_symbol->type != NULL) {
    VectorASTNode* call = (VectorASTNode*)node;
    if (call->left != NULL && call->left->op == AST_OP(identifier)) {
      IdentifierASTNode* callee = (IdentifierASTNode*)call->left;
      if (callee->symbol != NULL && callee->symbol->type != NULL &&
          TypeIsStructOrUnion(callee->symbol->type) &&
          callee->symbol->type->info.struct_info == clone->from_owner) {
        callee->symbol = clone->to_owner->tag_symbol;
        ASTNodeSetType(call->left, clone->to_owner->tag_symbol->type);
      }
    }
  }
  if (node->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node;
    if (clone->from_owner != NULL && clone->to_owner != NULL &&
        clone->from_owner != clone->to_owner && id->symbol != NULL &&
        id->symbol->type != NULL && TypeIsFunction(id->symbol->type)) {
      StructMember* from_member =
          FindStructMember(clone->from_owner, &id->symbol->name);
      bool belongs_to_source =
          id->symbol->type->info.function.cxx_member_owner ==
              clone->from_owner ||
          (from_member != NULL && from_member->symbol == id->symbol);
      if (belongs_to_source) {
        StructMember* to_member =
            FindStructMember(clone->to_owner, &id->symbol->name);
        if (to_member != NULL && to_member->symbol != NULL &&
            TypeIsFunction(to_member->symbol->type)) {
          id->symbol = to_member->symbol;
          ASTNodeSetType(node, to_member->symbol->type);
          if ((node->flags & kASTNeedAddress) != 0 ||
              to_member->symbol->flags.address_taken) {
            TypeEnsureTemplateMemberFunctionDefinition(
                clone->parser->syntax, to_member->symbol);
          }
        }
      }
    }
    if (clone->from_owner != NULL && clone->to_owner != NULL &&
        clone->from_owner != clone->to_owner && id->symbol != NULL &&
        StorageIs(id->symbol->storage, STO(typedef))) {
      StructMember* from_member =
          FindStructMember(clone->from_owner, &id->symbol->name);
      if (from_member != NULL && from_member->symbol == id->symbol) {
        StructMember* to_member =
            FindStructMember(clone->to_owner, &id->symbol->name);
        if (to_member != NULL && to_member->symbol != NULL &&
            StorageIs(to_member->symbol->storage, STO(typedef))) {
          if (to_member->symbol->type != NULL &&
              TypeContainsTemplateParameter(to_member->symbol->type)) {
            TypeRecord* concrete_type = SubstituteTemplateParameters(
                clone->parser, to_member->symbol->type, clone->args);
            RebaseTemplateParameterIndices(
                concrete_type, clone->rebase_template_parameter_base);
            Symbol* concrete_symbol =
                NewSymbol(to_member->symbol->name.value, concrete_type,
                          to_member->symbol->storage);
            concrete_symbol->namespace_ = to_member->symbol->namespace_;
            concrete_symbol->flags = to_member->symbol->flags;
            concrete_symbol->location = to_member->symbol->location;
            id->symbol = concrete_symbol;
            ASTNodeSetType(node, concrete_type);
          } else {
            id->symbol = to_member->symbol;
            ASTNodeSetType(node, to_member->symbol->type);
          }
        }
      }
    }
    if (id->symbol != NULL && StorageIs(id->symbol->storage, STO(typedef)) &&
        id->symbol->type != NULL &&
        TypeContainsTemplateParameter(id->symbol->type)) {
      TypeRecord* concrete_type = SubstituteTemplateParameters(
          clone->parser, id->symbol->type, clone->args);
      RebaseTemplateParameterIndices(concrete_type,
                                     clone->rebase_template_parameter_base);
      if (concrete_type != NULL &&
          !TypeContainsTemplateParameter(concrete_type)) {
        Symbol* concrete_symbol =
            NewSymbol(id->symbol->name.value, concrete_type,
                      id->symbol->storage);
        concrete_symbol->namespace_ = id->symbol->namespace_;
        concrete_symbol->flags = id->symbol->flags;
        concrete_symbol->location = id->symbol->location;
        id->symbol = concrete_symbol;
        ASTNodeSetType(node, concrete_type);
      } else {
        TypeRecordDelete(concrete_type);
      }
    }
    // Dependent qualified value name like `T::member` or `Trait<T>::member`:
    // substitute the scope placeholder to its concrete class (a bare template
    // parameter or a dependent template specialization), then resolve the named
    // static/enum member against the resulting concrete type.
    if ((node->flags & kASTDependentQualifiedName) != 0 && id->symbol != NULL &&
        id->symbol->type != NULL &&
        id->symbol->type->dependent_member_name != NULL &&
        (id->symbol->type->template_parameter_index >= 0 ||
         id->symbol->type->template_origin != NULL ||
         TypeContainsTemplateParameter(id->symbol->type))) {
      TypeRecord* scope = TypeRecordCopy(id->symbol->type);
      StringDelete(scope->dependent_member_name);
      scope->dependent_member_name = NULL;
      TypeRecord* concrete = NULL;
      // A qualified name can use a member alias whose definition itself
      // depends on another member alias (for example
      // `node_traits::allocate`, where `node_traits` wraps
      // `value_traits::rebind_alloc<node>`).  The aliases on `to_owner` have
      // already been instantiated in declaration order.  Reuse that canonical
      // concrete alias instead of independently re-expanding the source alias,
      // which can retain the nested alias template's rebased parameter index.
      if (clone->from_owner != NULL && clone->to_owner != NULL &&
          clone->from_owner != clone->to_owner) {
        for (size_t i = 0; i < clone->from_owner->members.length; i++) {
          StructMember* from_member = clone->from_owner->members.value.p[i];
          if (!StructMemberIsNestedType(from_member) ||
              from_member->symbol == NULL ||
              !TypeRecordHasSameAliasTemplateId(scope,
                                                from_member->symbol->type)) {
            continue;
          }
          StructMember* to_member = FindStructMember(
              clone->to_owner, &from_member->symbol->name);
          if (StructMemberIsNestedType(to_member) &&
              to_member->symbol != NULL && to_member->symbol->type != NULL) {
            concrete = TypeRecordCopy(to_member->symbol->type);
          }
          break;
        }
      }
      if (concrete == NULL) {
        concrete =
            SubstituteTemplateParameters(clone->parser, scope, clone->args);
      }
      RebaseTemplateParameterIndices(concrete,
                                     clone->rebase_template_parameter_base);
      concrete = TypeMaterializeClassTemplateSpecialization(
          clone->parser->syntax, concrete);
      TypeRecordDelete(scope);
      // The qualified name may reference a member through a chain of member
      // typedefs, e.g. `W::period::num` (encoded as a single `::`-joined
      // dependent-member path).  Advance `concrete` through every leading
      // component (each a member type alias, materialized to its concrete
      // specialization) so the final component is looked up in the right class.
      // `effective_member_name` points into the persistent dependent-member
      // string (the substring after the final `::`); it stays valid because the
      // owning type outlives this resolution.
      const char* effective_member_name =
          id->symbol->type->dependent_member_name->value;
      for (const char* c = id->symbol->type->dependent_member_name->value;
           c != NULL && *c != '\0'; c++) {
        if (c[0] == ':' && c[1] == ':') {
          effective_member_name = c + 2;
        }
      }
      {
        Vector* path =
            SplitDependentMemberPath(id->symbol->type->dependent_member_name);
        bool path_ok = concrete != NULL && TypeIsStructOrUnion(concrete) &&
                       concrete->info.struct_info != NULL;
        for (size_t pi = 0; path_ok && pi + 1 < path->length; pi++) {
          StructMember* seg =
              FindStructMember(concrete->info.struct_info, path->value.p[pi]);
          if (seg == NULL || seg->symbol == NULL ||
              !StorageIs(seg->symbol->storage, STO(typedef))) {
            break;
          }
          TypeRecord* next = TypeRecordCopy(seg->symbol->type);
          next = TypeMaterializeClassTemplateSpecialization(
              clone->parser->syntax, next);
          TypeRecordDelete(concrete);
          concrete = next;
          if (concrete == NULL || !TypeIsStructOrUnion(concrete) ||
              concrete->info.struct_info == NULL) {
            break;
          }
        }
        DeleteStringVector(path);
      }
      if (concrete != NULL && !TypeContainsTemplateParameter(concrete) &&
          TypeIsStructOrUnion(concrete) &&
          concrete->info.struct_info != NULL) {
        Struct* lookup_struct = concrete->info.struct_info;
        if (clone->to_owner != NULL) {
          Struct* owner_base = FindOwnerBaseStructMatchingType(
              clone, clone->to_owner, concrete);
          if (owner_base != NULL) {
            lookup_struct = owner_base;
          }
        }
        StructMember* member =
            FindStructMemberByName(lookup_struct, effective_member_name);
        if (member != NULL && member->symbol != NULL && !member->is_static &&
            clone->to_owner != NULL) {
          Symbol* this_symbol = NULL;
          if (clone->to_func != NULL && TypeIsFunction(clone->to_func)) {
            for (size_t pi = 0;
                 pi < clone->to_func->info.function.prototype.length; pi++) {
              Symbol* formal =
                  clone->to_func->info.function.prototype.value.p[pi];
              if (formal != NULL && strcmp(formal->name.value, "this") == 0) {
                this_symbol = formal;
                break;
              }
            }
          }
          if (this_symbol != NULL) {
            ASTNode* left = NewIdentifierASTNode(this_symbol, node->location);
            ASTNode* right =
                NewStructMemberASTNode(member, node->location);
            right->flags |= kASTQualifiedName;
            StructMemberASTNode* member_node = (StructMemberASTNode*)right;
            member_node->owner_type = concrete;
            TypeRecordIncRef(member_node->owner_type);
            ASTNode* access = NewBinaryASTNode(AST_OP(arrow), NULL,
                                               node->location, left, right);
            TypeRecordDelete(concrete);
            return access;
          }
        }
        if (member != NULL && member->symbol != NULL && !member->is_static &&
            member->is_member_function && member->overload_next == NULL &&
            (node->flags & kASTNeedAddress) != 0) {
          id->symbol = member->symbol;
          id->symbol->type->info.function.cxx_member_owner =
              concrete->info.struct_info;
          ASTNodeSetType(node, member->symbol->type);
          node->value_category = kValueCategoryLvalue;
          TypeRecordDelete(concrete);
          // Retain kASTDependentQualifiedName until the enclosing address node
          // rewrites `&T::member` to the dedicated pointer-to-member AST.
          return node;
        }
        if (member != NULL && member->symbol != NULL &&
            (member->is_static ||
             StorageIs(member->symbol->storage, STO(typedef)) ||
             member->symbol->flags.value_set)) {
          bool address_operand =
              (node->flags & kASTNeedAddress) != 0 ||
              (node->parent != NULL &&
               node->parent->op == AST_OP(address));
          if (address_operand && member->is_static) {
            QueueODRUsedStaticDataMember(member);
          }
          if (member->symbol->flags.value_set && !address_operand &&
              TypeIsIntegral(member->symbol->type)) {
            TypeRecord* value_type = TypeRecordCopy(member->symbol->type);
            TypeRecordDelete(concrete);
            return NewIntConstantASTNode(member->symbol->value.ivalue,
                                         value_type, node->location);
          }
          if (member->symbol->flags.value_set && !address_operand &&
              TypeIsFloatingPoint(member->symbol->type)) {
            TypeRecord* value_type = TypeRecordCopy(member->symbol->type);
            double value = member->symbol->value.fvalue;
            TypeRecordDelete(concrete);
            return NewRealConstantASTNode(value, value_type, node->location);
          }
          id->symbol = member->symbol;
          if (TypeIsFunction(id->symbol->type)) {
            id->symbol->type->info.function.cxx_member_owner =
                concrete->info.struct_info;
            StringClear(&id->symbol->asm_name);
            SymbolSetCXXMangledAsmName(id->symbol);
            if ((node->flags & kASTNeedAddress) != 0 ||
                id->symbol->flags.address_taken) {
              TypeEnsureTemplateMemberFunctionDefinition(
                  clone->parser->syntax, id->symbol);
            }
          }
          ASTNodeSetType(node, member->symbol->type);
          node->flags &= ~kASTDependentQualifiedName;
          node->value_category = kValueCategoryLvalue;
          TypeRecordDelete(concrete);
          return node;
        }
      }
      // The scope is still dependent (e.g. `__index_of<T, Types...>::value`
      // where the member template's own `T` is unknown but the enclosing class
      // pack `Types` is now concrete).  Bake the partial substitution back into
      // the identifier's scope type -- re-attaching the member name -- so a
      // later, more-concrete instantiation (which supplies `T`) can finish
      // resolving the qualified name.  Without this the enclosing class's
      // arguments would be dropped and the name would stay forever dependent.
      if (concrete != NULL && TypeContainsTemplateParameter(concrete)) {
        concrete->dependent_member_name =
            NewString(id->symbol->type->dependent_member_name->value);
        Symbol* copy = NewSymbol(id->symbol->name.value, concrete,
                                 id->symbol->storage);
        copy->namespace_ = id->symbol->namespace_;
        copy->flags = id->symbol->flags;
        copy->template_parameter_index = id->symbol->template_parameter_index;
        copy->dependent_value_template_parameter_index =
            id->symbol->dependent_value_template_parameter_index;
        id->symbol = copy;
        ASTNodeSetType(node, concrete);
        return node;
      }
      if (concrete != NULL) {
        concrete->dependent_member_name =
            NewString(id->symbol->type->dependent_member_name->value);
        Symbol* copy =
            NewSymbol(id->symbol->name.value, concrete, id->symbol->storage);
        copy->namespace_ = id->symbol->namespace_;
        copy->flags = id->symbol->flags;
        copy->template_parameter_index = id->symbol->template_parameter_index;
        copy->dependent_value_template_parameter_index =
            id->symbol->dependent_value_template_parameter_index;
        id->symbol = copy;
        ASTNodeSetType(node, concrete);
        return node;
      }
      TypeRecordDelete(concrete);
    }
    if (id->symbol != NULL &&
        id->symbol->flags.is_template_template_parameter &&
        id->symbol->template_parameter_index >= 0 && clone->args != NULL &&
        (size_t)id->symbol->template_parameter_index < clone->args->length) {
      TemplateArgument* actual =
          clone->args->value.p[id->symbol->template_parameter_index];
      if (actual != NULL && actual->kind == kTemplateParameterTemplate &&
          actual->template_symbol != NULL) {
        id->symbol = actual->template_symbol;
      }
    }
    bool template_args_contain_pack = false;
    if (id->template_arguments != NULL) {
      int pack_index = -1;
      size_t pack_length = 0;
      for (size_t i = 0; i < id->template_arguments->length; i++) {
        TemplateArgument* template_arg = id->template_arguments->value.p[i];
        if (template_arg != NULL && template_arg->is_pack_expansion &&
            FindPackExpansionInTemplateArgument(
                template_arg, clone->args, &pack_index, &pack_length)) {
          template_args_contain_pack = true;
          break;
        }
      }
    }
    if (id->template_arguments != NULL && !template_args_contain_pack &&
        !ASTNodeWithinPackExpansion(node)) {
      Vector* concrete_args = SubstituteTemplateArgumentVector(
          clone->parser, id->template_arguments, clone->args,
          clone->rebase_template_parameter_base);
      VectorDeleteWithContents(id->template_arguments,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      id->template_arguments = concrete_args;
      if (id->symbol != NULL && id->symbol->variable_template != NULL &&
          !TemplateArgumentVectorContainsTemplateParameter(concrete_args) &&
          (node->flags & kASTNeedAddress) == 0 &&
          (node->parent == NULL || node->parent->op != AST_OP(address))) {
        TypeRecord* concrete_type = TypeInstantiateVariableTemplateType(
            clone->parser->syntax, id->symbol, concrete_args);
        int64_t value = 0;
        if (concrete_type != NULL && TypeIsIntegral(concrete_type) &&
            TypeInstantiateVariableTemplateConstant(
                clone->parser->syntax, id->symbol, concrete_args, &value)) {
          return NewIntConstantASTNode(value, concrete_type, node->location);
        }
        double floating_value = 0;
        if (concrete_type != NULL && TypeIsFloatingPoint(concrete_type) &&
            TypeInstantiateVariableTemplateFloatingConstant(
                clone->parser->syntax, id->symbol, concrete_args,
                &floating_value)) {
          return NewRealConstantASTNode(floating_value, concrete_type,
                                        node->location);
        }
        TypeRecordDelete(concrete_type);
      }
      if (id->symbol != NULL && TypeIsFunction(id->symbol->type) &&
          id->symbol->type->info.function.template_origin != NULL) {
        id->symbol = id->symbol->type->info.function.template_origin;
      }
    } else if (id->template_arguments != NULL &&
               clone->rebase_template_parameter_base > 0) {
      // The full substitution above is deferred when this template-id is inside
      // a pack expansion (e.g. `std::forward<Args>(args)...`), because the pack
      // elements are not known until the pattern is expanded later. However, if
      // this body belongs to a member template of an enclosing template (so the
      // member's own parameters are numbered after the enclosing ones), we must
      // still renumber those explicit template arguments down by the enclosing
      // parameter count now. Otherwise the deferred per-element expansion, which
      // runs against the member's own zero-based arguments, would look the pack
      // up at the wrong (enclosing-offset) index and fail to substitute it.
      for (size_t i = 0; i < id->template_arguments->length; i++) {
        RebaseTemplateArgumentParameterIndices(
            id->template_arguments->value.p[i],
            clone->rebase_template_parameter_base);
      }
    }
    if ((node->flags & kASTPackExpansion) != 0 &&
        id->symbol != NULL && id->symbol->flags.is_parameter_pack &&
        MapFindPointerKey(&clone->pack_symbol_map, id->symbol) != NULL) {
      return node;
    }
    Symbol* replacement = MapFindPointerKey(&clone->symbol_map, id->symbol);
    if (replacement != NULL) {
      id->symbol = replacement;
      ASTNodeSetType(node, TypeIsReference(replacement->type)
                               ? replacement->type->next
                               : replacement->type);
      node->value_category = kValueCategoryLvalue;
      return node;
    }
    replacement = FindClonedOwnerMemberSymbol(clone, id->symbol);
    if (replacement != NULL) {
      id->symbol = replacement;
      ASTNodeSetType(node, replacement->type);
      node->value_category = kValueCategoryLvalue;
    }
    replacement = CloneTemplateDependentTemporarySymbol(clone, id->symbol);
    if (replacement != NULL) {
      id->symbol = replacement;
      ASTNodeSetType(node, TypeIsReference(replacement->type)
                               ? replacement->type->next
                               : replacement->type);
      node->value_category = kValueCategoryLvalue;
      return node;
    }
    if (id->symbol != NULL && id->symbol->flags.is_template_parameter &&
        !id->symbol->flags.is_template_type_parameter &&
        id->symbol->template_parameter_index >= 0 &&
        (size_t)id->symbol->template_parameter_index < clone->args->length) {
      TemplateArgument* arg =
          clone->args->value.p[id->symbol->template_parameter_index];
      if (arg != NULL && arg->kind == kTemplateParameterNonType &&
          arg->pack_arguments == NULL && arg->template_parameter_index < 0) {
        ASTNode* value =
            TemplateArgumentMaterializeExpression(arg, node->location);
        if (value != NULL) {
          return value;
        }
      }
    }
    // A bare reference to a value-dependent static data member of the template
    // currently being instantiated (e.g. `num` inside `using type =
    // ratio<num, den>;`, where `num` is itself computed from the template
    // parameters).  Such a member is not a template parameter, so it is not
    // resolved by the substitution above; fold its own initializer against the
    // same concrete arguments so the enclosing non-type argument can fold.
    {
      ASTNode* folded_member =
          FoldDependentStaticMemberReference(clone, id->symbol, node);
      if (folded_member != NULL) {
        return folded_member;
      }
    }
    // Deferred reference to a member template's own template parameter (e.g.
    // `Target` inside a member function template of a class template): during
    // the lazy first-pass clone that bakes in the enclosing class arguments,
    // such a parameter is numbered after the enclosing ones and is not among
    // `clone->args`. It must be renumbered down by the enclosing parameter
    // count so the subsequent per-call instantiation (which supplies the
    // member's own zero-based arguments) can substitute it. Copy the symbol so
    // the shared template definition is not mutated.
    if (clone->rebase_template_parameter_base > 0 && id->symbol != NULL &&
        id->symbol->flags.is_template_parameter &&
        (id->symbol->template_parameter_index >=
             clone->rebase_template_parameter_base ||
         id->symbol->dependent_value_template_parameter_index >=
             clone->rebase_template_parameter_base)) {
      Symbol* old = id->symbol;
      TypeRecord* type = old->type != NULL ? TypeRecordCopy(old->type) : NULL;
      RebaseTemplateParameterIndices(type,
                                     clone->rebase_template_parameter_base);
      Symbol* copy = NewSymbol(old->name.value, type, old->storage);
      copy->namespace_ = old->namespace_;
      copy->flags = old->flags;
      copy->alignment = old->alignment;
      copy->template_parameter_index = old->template_parameter_index;
      if (copy->template_parameter_index >=
          clone->rebase_template_parameter_base) {
        copy->template_parameter_index -=
            clone->rebase_template_parameter_base;
      }
      copy->dependent_value_template_parameter_index =
          old->dependent_value_template_parameter_index;
      if (copy->dependent_value_template_parameter_index >=
          clone->rebase_template_parameter_base) {
        copy->dependent_value_template_parameter_index -=
            clone->rebase_template_parameter_base;
      }
      copy->location = old->location;
      copy->value = old->value;
      copy->stack_offset = old->stack_offset;
      copy->alias_target = old->alias_target;
      id->symbol = copy;
      ASTNodeSetType(node, copy->type);
      return node;
    }
    if (id->symbol != NULL && id->symbol->type != NULL &&
        TypeIsFunction(id->symbol->type) &&
        id->symbol->type->info.function.cxx_member_owner != NULL) {
      Struct* owner = id->symbol->type->info.function.cxx_member_owner;
      TypeRecord* owner_type =
          owner->tag_symbol != NULL ? owner->tag_symbol->type : NULL;
      if (owner_type != NULL &&
          (TypeContainsTemplateParameter(owner_type) ||
           StructContainsTemplateParameter(owner))) {
        TypeRecord* concrete_owner =
            SubstituteTemplateParameters(clone->parser, owner_type,
                                         clone->args);
        RebaseTemplateParameterIndices(concrete_owner,
                                       clone->rebase_template_parameter_base);
        if (concrete_owner != NULL && TypeIsStructOrUnion(concrete_owner) &&
            concrete_owner->info.struct_info != NULL) {
          StructMember* member =
              FindStructMember(concrete_owner->info.struct_info,
                               &id->symbol->name);
          if (member != NULL && member->is_member_function &&
              member->symbol != NULL) {
            id->symbol = member->symbol;
            ASTNodeSetType(node, member->symbol->type);
            TypeRecordDelete(concrete_owner);
            return node;
          }
        }
        TypeRecordDelete(concrete_owner);
      }
    }
  }
  bool node_type_substituted = false;
  if (node->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node;
    if (id->symbol != NULL && id->symbol->type != NULL &&
        TypeIsFunction(id->symbol->type) &&
        id->symbol->type->info.function.cxx_member_owner == NULL &&
        (id->symbol->flags.is_template ||
         id->symbol->type->info.function.template_origin != NULL)) {
      Symbol* pattern = id->symbol->flags.is_template
                            ? id->symbol
                            : id->symbol->type->info.function.template_origin;
      if (pattern != NULL && pattern->namespace_ != NULL) {
        Symbol* overload_head =
            NamespaceFindSymbol(pattern->namespace_, &pattern->name);
        if (overload_head != NULL && overload_head->flags.is_template &&
            overload_head->type != NULL &&
            TypeIsFunction(overload_head->type)) {
          id->symbol = overload_head;
          ASTNodeSetType(node, overload_head->type);
        }
      }
      // A named function template that is not being instantiated here (a callee
      // such as `ranges::partition(first, last, ...)` inside another template's
      // body).  Its type mentions *its own* template parameters, which have
      // nothing to do with the enclosing template's, so substituting them
      // positionally against `clone->args` would silently bind, say, the
      // callee's `I` to the caller's `R`.  Leave the pattern type alone;
      // overload resolution deduces the callee's arguments from the cloned
      // actuals when the instantiated body is analyzed.
      node_type_substituted = true;
    }
  }
  if (node->op == AST_OP(cast)) {
    CastASTNode* cast = (CastASTNode*)node;
    if (cast->expr != NULL && cast->expr->op == AST_OP(identifier) &&
        clone->from_func != NULL) {
      IdentifierASTNode* operand = (IdentifierASTNode*)cast->expr;
      int arg_number =
          operand->symbol != NULL && operand->symbol->flags.is_argument
              ? operand->symbol->value.arg_number : -1;
      if (arg_number >= 0 &&
          (size_t)arg_number <
              clone->from_func->info.function.prototype.length) {
        Symbol* formal =
            clone->from_func->info.function.prototype.value.p[arg_number];
        int formal_index =
            formal != NULL
                ? FirstTemplateParameterIndexInType(formal->type) : -1;
        int cast_index = FirstTemplateParameterIndexInType(cast->cast_type);
        if (formal_index >= 0 && cast_index >= 0 &&
            formal_index != cast_index) {
          TypeRecord* repaired = TypeRecordCloneSpine(cast->cast_type);
          TypeRecordDelete(cast->cast_type);
          cast->cast_type = repaired;
          for (TypeRecord* t = cast->cast_type; t != NULL; t = t->next) {
            if (t->template_parameter_index == cast_index) {
              t->template_parameter_index = formal_index;
            }
          }
        }
      }
    }
    // A dependent reference cast in an imported template can share its
    // referent with the function return type.  If that referent was released
    // before the module graph was archived, the imported cast retains only the
    // reference head.  A return statement still has an authoritative concrete
    // target on the instantiated function type, so restore the cast from it
    // before deciding whether template substitution is needed.
    bool invalid_return_reference =
        TypeIsReference(cast->cast_type) && cast->cast_type->next == NULL &&
        node->parent != NULL && node->parent->op == AST_OP(return) &&
        clone->to_func != NULL && TypeIsFunction(clone->to_func) &&
        TypeIsReference(clone->to_func->next) &&
        clone->to_func->next->next != NULL &&
        clone->to_func->next->declarator == cast->cast_type->declarator;
    if (invalid_return_reference) {
      TypeRecordDelete(cast->cast_type);
      cast->cast_type = TypeRecordCopy(clone->to_func->next);
      TypeRecordIncRef(cast->cast_type);
      ASTNodeSetType(node, cast->cast_type->next);
      node->value_category =
          cast->cast_type->declarator == kDeclRValueReference
              ? kValueCategoryXvalue
              : kValueCategoryLvalue;
      node_type_substituted = true;
    }
    // A cast whose type still names a pack (e.g. `static_cast<Ts&&>(args)...`)
    // must keep the pack-dependent cast type until ExpandClonedCallPackActuals
    // clones the pattern per element.  Substituting the whole pack here would
    // collapse `Ts` to a single element (or leave a broken type) and poison
    // every expanded copy that shares the cast_type pointer.
    int pack_index = -1;
    size_t pack_length = 0;
    bool pack_dependent_cast =
        FindPackExpansionInType(cast->cast_type, clone->args, &pack_index,
                                &pack_length) &&
        ASTNodeWithinPackExpansion(node);
    if (invalid_return_reference) {
      // The concrete function return type above is authoritative.
    } else if (pack_dependent_cast) {
      // Leave cast_type / node->type pack-dependent for per-element expansion.
      node_type_substituted = true;
    } else if (TypeContainsTemplateParameter(cast->cast_type) ||
               (clone->from_owner != NULL && clone->to_owner != NULL &&
                clone->from_owner != clone->to_owner &&
                TypeChainReferencesStruct(cast->cast_type,
                                          clone->from_owner))) {
      node->flags |= kASTDependentCast;
      TypeRecord* cast_pattern = TypeRecordCloneSpine(cast->cast_type);
      TypeRecord* cast_type =
          SubstituteTemplateBodyType(clone, cast_pattern);
      TypeRecordDelete(cast_pattern);
      TypeRecord* independent_cast_type = TypeRecordCloneSpine(cast_type);
      TypeRecordDelete(cast_type);
      cast_type = independent_cast_type;
      // A member-function template can clone its saved constructor
      // initializer after the enclosing class has already been specialized.
      // Any surviving dependent-member type at that point is indexed against
      // the class template, not the member function's own argument vector.
      // Resolve it from the concrete owner's template arguments before
      // rebasing the member-template parameters.
      if (cast_type != NULL && cast_type->declarator == kDeclPrimitive &&
          cast_type->template_parameter_index >= 0 &&
          cast_type->dependent_member_name != NULL &&
          clone->to_owner != NULL && clone->to_owner->tag_symbol != NULL &&
          clone->to_owner->tag_symbol->type != NULL &&
          clone->to_owner->tag_symbol->type->template_arguments != NULL &&
          (size_t)cast_type->template_parameter_index <
              clone->to_owner->tag_symbol->type->template_arguments->length) {
        TypeRecord* owner_resolved = SubstituteTemplateParameters(
            clone->parser, cast_type,
            clone->to_owner->tag_symbol->type->template_arguments);
        if (owner_resolved != NULL) {
          TypeRecordDelete(cast_type);
          cast_type = owner_resolved;
        }
      }
      RebaseTemplateParameterIndices(cast_type,
                                     clone->rebase_template_parameter_base);
      TypeRecordCalculateSize(cast_type);
      TypeRecordDelete(cast->cast_type);
      cast->cast_type = cast_type;
      TypeRecordIncRef(cast->cast_type);
      if (TypeIsReference(cast_type)) {
        ASTNodeSetType(node, cast_type->next);
        node->value_category =
            cast_type->declarator == kDeclRValueReference
                ? kValueCategoryXvalue
                : kValueCategoryLvalue;
      } else {
        ASTNodeSetType(node, cast_type);
      }
      /* The cast block produced the authoritative result type and kept it in
       * sync with cast->cast_type. Re-substituting node->type below would both
       * be redundant and desync node->type from cast->cast_type (yielding a
       * separate, dangling pointee after later type-record churn). */
      node_type_substituted = true;
    }
  }
  if (node->type != NULL && !node_type_substituted) {
    TypeRecord* type = SubstituteTemplateBodyType(clone, node->type);
    RebaseTemplateParameterIndices(type, clone->rebase_template_parameter_base);
    ASTNodeSetType(node, type);
  }
  if (node->op == AST_OP(cast) &&
      (node->flags & kASTDependentNewAllocation) != 0) {
    CastASTNode* cast = (CastASTNode*)node;
    TypeRecord* allocated_type =
        TypeIsPointer(cast->cast_type) ? cast->cast_type->next : NULL;
    if (allocated_type != NULL &&
        !TypeContainsTemplateParameter(allocated_type) &&
        cast->expr != NULL && cast->expr->op == AST_OP(call)) {
      VectorASTNode* allocation = (VectorASTNode*)cast->expr;
      if (allocation->children != NULL && allocation->children->length > 0) {
        ASTNode* old_size = allocation->children->value.p[0];
        // A scalar dependent new-expression has a single sizeof operand
        // (possibly wrapped in an integer conversion).  Array new uses an
        // arithmetic size expression and retains its element sizeof node.
        if (old_size != NULL && old_size->op != AST_OP(plus)) {
          TypeRecordCalculateSize(allocated_type);
          ASTNode* concrete_size = NewSizeofASTNodeWithKnownSize(
              allocated_type->size, old_size->location);
          ASTNodeReplaceChild((ASTNode*)allocation, old_size->child_id,
                              concrete_size, true);
          allocation->base.flags &= ~kASTAnalyzed;
        }
      }
      if (allocation->children != NULL &&
          allocation->children->length == 2 &&
          allocation->left != NULL &&
          allocation->left->op == AST_OP(identifier)) {
        Symbol* allocation_symbol =
            ((IdentifierASTNode*)allocation->left)->symbol;
        TypeRecord* allocation_type =
            allocation_symbol != NULL ? allocation_symbol->type : NULL;
        Symbol* placement_formal =
            allocation_type != NULL && TypeIsFunction(allocation_type) &&
                    allocation_type->info.function.prototype.length >= 2
                ? allocation_type->info.function.prototype.value.p[1] : NULL;
        TypeRecord* placement_type =
            placement_formal != NULL ? placement_formal->type : NULL;
        if (placement_type != NULL && TypeIsPointer(placement_type) &&
            placement_type->next != NULL &&
            TypeIsVoid(placement_type->next)) {
          node->flags |= kASTCXXPlacementNew;
        }
      }
      node->flags &= ~kASTDependentNewAllocation;
    }
  }
  if (node->op == AST_OP(call)) {
    VectorASTNode* call = (VectorASTNode*)node;
    if (call->left != NULL && call->left->type != NULL) {
      SetClonedCallReturnType(call, call->left->type);
    }
  }
  ASTNode* rewritten_new = RewriteClonedDependentNewInitializer(clone, node);
  if (rewritten_new != node) {
    // A dependent `new T(args)` initializer is rewritten into a constructor
    // call here.  When the initializer is a pack expansion, e.g.
    // `new T(std::forward<Args>(args)...)`, the freshly built call still holds
    // the unexpanded pack argument (with its per-element explicit template
    // arguments unsubstituted).  Route it through the ordinary call
    // pack-expansion + instantiation path below rather than returning
    // immediately, which would leave the pack unexpanded and fail to apply the
    // element template arguments to callees like `std::forward<Args>`.
    if (rewritten_new->op == AST_OP(call) &&
        CallActualsStillContainPackExpansion(rewritten_new)) {
      node = rewritten_new;
    } else {
      return rewritten_new;
    }
  }
  ExpandClonedScalarMemberInitPack(clone, node);
  bool expanded_call_actuals = ExpandClonedCallPackActuals(clone, node);
  Vector nested_constructor_overload_snapshots;
  bool nested_constructor_rebound = false;
  if (node->op == AST_OP(call)) {
    VectorASTNode* call = (VectorASTNode*)node;
    if (call->left != NULL &&
        (call->left->op == AST_OP(dot) ||
         call->left->op == AST_OP(arrow))) {
      BinaryASTNode* access = (BinaryASTNode*)call->left;
      if (access->right != NULL &&
          access->right->op == AST_OP(structmember)) {
        StructMember* member =
            ((StructMemberASTNode*)access->right)->member;
        if (member != NULL && member->symbol != NULL &&
            member->symbol->type != NULL &&
            TypeIsFunction(member->symbol->type) &&
            (member->symbol->flags.is_template ||
             TypeFunctionReturnContainsAuto(member->symbol->type))) {
          // A dependent pass may have resolved this member-template call
          // against placeholder arguments and marked the whole expression
          // analyzed.  Its cloned actuals are concrete now, so force normal
          // member overload resolution and return-type deduction to run again.
          node->flags &= ~kASTAnalyzed;
          call->left->flags &= ~kASTAnalyzed;
          access->right->flags &= ~kASTAnalyzed;
          ASTNodeClearType(node);
        }
      }
    }
    RebindClonedLoweredDependentMemberCall(call);
    if (call->left != NULL && call->left->op == AST_OP(identifier)) {
      IdentifierASTNode* callee = (IdentifierASTNode*)call->left;
      TypeRecord* callee_type =
          callee->symbol != NULL ? callee->symbol->type : NULL;
      Struct* constructor_owner =
          callee_type != NULL && TypeIsFunction(callee_type) &&
                  callee_type->info.function.is_constructor
              ? callee_type->info.function.cxx_member_owner
              : NULL;
      if (constructor_owner != NULL &&
          constructor_owner->lexical_parent != NULL &&
          constructor_owner->lexical_parent ==
              clone->substitution_source &&
          clone->substitution_target != NULL) {
        // A nested class can be non-dependent in layout while still belonging
        // to an enclosing class-template specialization. Its parsed
        // constructor symbol names the primary nested class. The cloned
        // temporary receiver has already been substituted, so rebind through
        // that concrete receiver before member-template deduction.
        RetargetClonedNestedConstructorReceiver(
            clone, call, constructor_owner);
        VectorInit(&nested_constructor_overload_snapshots);
        RebindClonedConstructorCall(
            call, &nested_constructor_overload_snapshots);
        nested_constructor_rebound = true;
        node->flags &= ~kASTAnalyzed;
        call->left->flags &= ~kASTAnalyzed;
        ASTNodeClearType(node);
      }
    }
  }
  InstantiateClonedFunctionTemplateCall(clone, node);
  if (nested_constructor_rebound) {
    RestoreSymbolOverloadLinks(&nested_constructor_overload_snapshots);
  }
  if (expanded_call_actuals && node->op == AST_OP(call)) {
    ASTNodeVisit(node, ReplaceSingleElementPackIdentifierVisitor, 0, clone);
    node->flags &= ~(kASTDependentFunctorCall | kASTAnalyzed);
    ASTNodeClearType(node);
    VectorASTNode* call = (VectorASTNode*)node;
    if (call->left != NULL) {
      call->left->flags &= ~kASTAnalyzed;
    }
    /* This clone callback runs bottom-up, so an enclosing declaration's symbol
     * type may not yet be substituted to its instantiated class. Re-analyzing a
     * member-access call here (e.g. a constructor call whose receiver is the
     * declared object) would resolve overloads against the not-yet-instantiated
     * generic class. Defer such calls to the post-clone reanalysis passes, which
     * run after all declaration types have been substituted. */
    bool is_member_access_call =
        call->left != NULL && (call->left->op == AST_OP(dot) ||
                               call->left->op == AST_OP(arrow));
    if (is_member_access_call) {
      node->flags |= kASTDependentFunctorCall;
      return node;
    }
    // This callback runs before ASTNodeClone installs the cloned parent.
    // ASTNodeBaseCopy therefore still leaves `node->parent` pointing into the
    // source template. AnalyzeExpression may replace the call in its parent;
    // detach it first so expanding one specialization cannot mutate the
    // template body used by later specializations.
    node->parent = NULL;
    return AnalyzeExpression(node);
  }
  ExpandClonedBracedInitializerPackElements(clone, node);
  CloneTemplateLocalDeclarationSymbol(clone, node);
  RewriteClonedConstructorMemberCall(clone, node);
  return node;
}

/* A deferred member-access constructor call may still name the generic class
 * template's constructor (e.g. "Foo") even though its receiver is now the
 * instantiated class ("Foo<int>"), whose constructor member is named after the
 * instantiated tag. Rewrite the member-name string to the instantiated
 * constructor name so the member lookup succeeds during re-analysis. */
static void RewriteDeferredConstructorMemberName(VectorASTNode* call) {
  if (call->left == NULL ||
      (call->left->op != AST_OP(dot) && call->left->op != AST_OP(arrow))) {
    return;
  }
  BinaryASTNode* member_access = (BinaryASTNode*)call->left;
  if (member_access->left == NULL || member_access->right == NULL ||
      member_access->right->op != AST_OP(string)) {
    return;
  }
  TypeRecord* receiver_type = member_access->left->type;
  if (receiver_type != NULL && TypeIsStructOrUnionPointer(receiver_type)) {
    receiver_type = receiver_type->next;
  }
  const char* constructor_name = CXXConstructorNameForRecord(receiver_type);
  if (constructor_name == NULL || receiver_type->info.struct_info == NULL) {
    return;
  }
  ConstantASTNode* member_name = (ConstantASTNode*)member_access->right;
  if (member_name->value.string == NULL ||
      StringEqual(member_name->value.string, constructor_name)) {
    return;
  }
  if (FindStructMember(receiver_type->info.struct_info,
                       member_name->value.string) != NULL) {
    return;
  }
  const char* source_name = member_name->value.string->value;
  const char* origin_name =
      receiver_type->template_origin != NULL
          ? receiver_type->template_origin->name.value
          : NULL;
  size_t origin_length = origin_name != NULL ? strlen(origin_name) : 0;
  if (origin_name == NULL || strncmp(source_name, origin_name, origin_length) != 0 ||
      (source_name[origin_length] != '\0' &&
       source_name[origin_length] != '<' &&
       source_name[origin_length] != '#')) {
    return;
  }
  StringSet(member_name->value.string, constructor_name);
}

static ASTNode* ReanalyzeClonedDependentFunctorCall(
    ASTNode* node, void* data, ASTNodeTransformAction* action) {
  (void)data;
  if ((node->flags & kASTDependentFunctorCall) == 0 ||
      node->op != AST_OP(call)) {
    return node;
  }
  // Keep a construction/call deferred while any pack expansion survives beneath
  // it: a nested member-template pack (e.g. the `args...` of
  // `value_type(key, mapped_type(static_cast<Args&&>(args)...))`) is still
  // unbound during the enclosing class instantiation, so resolving the outer
  // construction now mis-selects an overload against the (empty-expanded) pack.
  // The per-call instantiation, which binds the member's own pack, re-runs this
  // pass with the pack expanded and resolves it correctly.
  if (ASTNodeSubtreeContainsPackExpansion(node)) {
    return node;
  }
  VectorASTNode* call = (VectorASTNode*)node;
  if (call->children != NULL) {
    for (size_t i = 0; i < call->children->length; i++) {
      ASTNode* actual = call->children->value.p[i];
      if (actual != NULL && actual->op == AST_OP(call) &&
          (actual->flags & kASTDependentFunctorCall) != 0) {
        int actual_child_id = actual->child_id;
        ASTNodeTransformAction child_action = kASTTransformContinue;
        ASTNode* analyzed_actual = ReanalyzeClonedDependentFunctorCall(
            actual, data, &child_action);
        VectorSet(call->children, i, analyzed_actual);
        if (analyzed_actual != NULL) {
          analyzed_actual->parent = node;
          analyzed_actual->child_id = actual_child_id;
        }
      }
    }
  }
  /* The cloned body may still reference the generic template's constructor
   * symbol. Rebind it to the instantiated class's constructor (derived from
   * the receiver) before re-analysis, while the receiver type is still intact.
   */
  RewriteDeferredConstructorMemberName(call);
  Vector overload_snapshots;
  VectorInit(&overload_snapshots);
  RebindClonedConstructorCall(call, &overload_snapshots);
  node->flags &= ~(kASTDependentFunctorCall | kASTAnalyzed);
  ASTNodeClearType(node);
  if (call->left != NULL) {
    call->left->flags &= ~kASTAnalyzed;
    if ((call->left->op == AST_OP(dot) ||
         call->left->op == AST_OP(arrow)) &&
        ((BinaryASTNode*)call->left)->right != NULL &&
        ((BinaryASTNode*)call->left)->right->op == AST_OP(string)) {
      ASTNodeClearType(call->left);
    }
  }
  if (call->children != NULL) {
    for (size_t i = 0; i < call->children->length; i++) {
      ASTNode* actual = call->children->value.p[i];
      if (actual != NULL) {
        if (actual->op == AST_OP(inline_call) &&
            actual->value_category != kValueCategoryPrvalue) {
          continue;
        }
        actual->flags &= ~kASTAnalyzed;
        ASTNodeClearType(actual);
      }
    }
  }
  ASTNode* parent = node->parent;
  int child_id = node->child_id;
  *action = kASTTransformSkipChildren;
  node->parent = NULL;
  ASTNode* analyzed = AnalyzeExpression(node);
  if (analyzed != NULL) {
    analyzed->parent = parent;
    analyzed->child_id = child_id;
  }
  RestoreSymbolOverloadLinks(&overload_snapshots);
  if (parent != NULL && parent->op == AST_OP(call)) {
    parent->flags |= kASTDependentFunctorCall;
    parent->flags &= ~kASTAnalyzed;
  }
  return analyzed;
}

// Constructor preambles can acquire a fresh concrete member-call subtree while
// a formerly dependent functional construction is being re-analyzed.  Such a
// call was not present when the surrounding statement was first analyzed, so
// resolve and lower it explicitly before handing the statement to codegen.
static ASTNode* ReanalyzeClonedConcreteMemberCall(
    ASTNode* node, void* data, ASTNodeTransformAction* action) {
  (void)data;
  if (node == NULL || node->op != AST_OP(call)) {
    return node;
  }
  VectorASTNode* call = (VectorASTNode*)node;
  if (call->left == NULL ||
      (call->left->op != AST_OP(dot) &&
       call->left->op != AST_OP(arrow))) {
    return node;
  }
  BinaryASTNode* access = (BinaryASTNode*)call->left;
  if (access->right == NULL || access->right->op != AST_OP(structmember) ||
      access->left == NULL || access->left->type == NULL ||
      TypeContainsTemplateParameter(access->left->type) ||
      ASTNodeSubtreeContainsPackExpansion(node)) {
    return node;
  }
  StructMember* member = ((StructMemberASTNode*)access->right)->member;
  if (member == NULL || member->symbol == NULL ||
      member->symbol->type == NULL ||
      !TypeIsFunction(member->symbol->type) ||
      !member->symbol->type->info.function.is_constructor) {
    return node;
  }
  ASTNode* receiver = ASTNodeMove(access->left);
  ASTNode* old_callee = call->left;
  call->left = NewIdentifierASTNode(member->symbol, old_callee->location);
  call->left->parent = (ASTNode*)call;
  call->left->child_id = 0;
  ASTNodeDelete(old_callee);
  VectorInsertBefore(call->children, 0, receiver);
  for (size_t i = 0; i < call->children->length; i++) {
    ASTNode* child = call->children->value.p[i];
    child->parent = (ASTNode*)call;
    child->child_id = i;
  }

  ASTNode* parent = node->parent;
  int child_id = node->child_id;
  Vector overload_snapshots;
  VectorInit(&overload_snapshots);
  RebindClonedConstructorCall(call, &overload_snapshots);
  ASTNodeVisit(node, ClearAnalyzedFlagVisitor, 0, NULL);
  node->flags &= ~kASTDependentFunctorCall;
  node->parent = NULL;
  ASTNode* analyzed = AnalyzeExpression(node);
  RestoreSymbolOverloadLinks(&overload_snapshots);
  if (analyzed != NULL) {
    analyzed->parent = parent;
    analyzed->child_id = child_id;
  }
  *action = kASTTransformSkipChildren;
  return analyzed;
}

/* Saved overload-link state for one symbol so it can be restored after a
 * temporary rewiring (see SaveAndLinkStructMemberOverloadSymbols). */
typedef struct {
  Symbol* symbol;
  Symbol* overload_next;
  bool is_overloaded;
} SymbolOverloadLinkSnapshot;

/* Temporarily rebuild the symbol-level overload chain from a struct member's
 * overload list (member overloads link via StructMember, but overload
 * resolution walks Symbol->overload_next). Each touched symbol's prior link is
 * snapshotted into `snapshots` for later restoration. */
static void SaveAndLinkStructMemberOverloadSymbols(StructMember* first,
                                                   Vector* snapshots) {
  bool overloaded = first != NULL && first->overload_next != NULL;
  for (StructMember* member = first; member != NULL;
       member = member->overload_next) {
    if (member->symbol == NULL) {
      continue;
    }
    SymbolOverloadLinkSnapshot* snapshot = malloc(sizeof(*snapshot));
    snapshot->symbol = member->symbol;
    snapshot->overload_next = member->symbol->overload_next;
    snapshot->is_overloaded = member->symbol->flags.is_overloaded;
    VectorAppend(snapshots, snapshot);
    member->symbol->flags.is_overloaded = overloaded;
    StructMember* next = member->overload_next;
    while (next != NULL && next->symbol == NULL) {
      next = next->overload_next;
    }
    member->symbol->overload_next = next != NULL ? next->symbol : NULL;
  }
}

/* Restore the symbol overload links saved by SaveAndLinkStructMemberOverloadSymbols
 * and free the snapshot vector's contents. */
static void RestoreSymbolOverloadLinks(Vector* snapshots) {
  if (snapshots == NULL) {
    return;
  }
  for (size_t i = 0; i < snapshots->length; i++) {
    SymbolOverloadLinkSnapshot* snapshot = snapshots->value.p[i];
    if (snapshot == NULL || snapshot->symbol == NULL) {
      continue;
    }
    snapshot->symbol->overload_next = snapshot->overload_next;
    snapshot->symbol->flags.is_overloaded = snapshot->is_overloaded;
  }
  VectorDestructWithContents(snapshots, free, /*free_element=*/false);
}

/* Find the head of the overload chain for member `name` in `owner`, preferring
 * a member that actually has overloads linked; falls back to any match. */
static bool RebindClonedConstructorCall(VectorASTNode* call,
                                        Vector* overload_snapshots) {
  if (call == NULL || call->left == NULL ||
      call->left->op != AST_OP(identifier)) {
    return false;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)call->left;
  if (id->symbol == NULL || id->symbol->type == NULL ||
      !TypeIsFunction(id->symbol->type) ||
      !id->symbol->type->info.function.is_constructor) {
    return false;
  }
  Struct* ctor_owner = NULL;
  String* ctor_name = NULL;
  if (call->children != NULL && call->children->length > 0) {
    ASTNode* receiver = call->children->value.p[0];
    TypeRecord* receiver_type = receiver != NULL ? receiver->type : NULL;
    /* Look through an address-of so this still works if the top-level
     * receiver type has already been cleared for re-analysis. */
    if (receiver_type == NULL && receiver != NULL &&
        receiver->op == AST_OP(address)) {
      UnaryASTNode* addr = (UnaryASTNode*)receiver;
      if (addr->sub != NULL && TypeIsStructOrUnion(addr->sub->type)) {
        ctor_owner = addr->sub->type->info.struct_info;
      }
    } else if (receiver_type != NULL &&
               TypeIsStructOrUnionPointer(receiver_type) &&
               receiver_type->next != NULL &&
               TypeIsStructOrUnion(receiver_type->next)) {
      ctor_owner = receiver_type->next->info.struct_info;
    }
    if (ctor_owner != NULL && ctor_owner->tag_name != NULL) {
      ctor_name = ctor_owner->tag_name;
    } else {
      ctor_owner = NULL;
    }
    /* A base- or member-subobject initializer inside a cloned constructor body
     * has the *derived* (enclosing) object as its receiver, yet the call
     * actually targets the subobject's own, already-concrete constructor. The
     * receiver-based detection above would hijack the owner to the derived
     * class in that case. When the call already resolves to a constructor of a
     * concrete (non-dependent) class that differs from the receiver's class,
     * trust that binding rather than the receiver: only a still-generic owner
     * needs rebinding from the receiver. */
    Struct* current_owner = id->symbol->type->info.function.cxx_member_owner;
    bool current_owner_is_template_pattern =
        current_owner != NULL &&
        (StructContainsTemplateParameter(current_owner) ||
         (current_owner->lexical_parent != NULL &&
          current_owner->lexical_parent->is_template));
    if (ctor_owner != NULL && current_owner != NULL &&
        current_owner != ctor_owner &&
        !current_owner_is_template_pattern) {
      ctor_owner = NULL;
      ctor_name = NULL;
    }
  }
  if (ctor_owner == NULL &&
      id->symbol->type->info.function.cxx_member_owner != NULL) {
    ctor_owner = id->symbol->type->info.function.cxx_member_owner;
    ctor_name = &id->symbol->name;
  }
  if (ctor_owner == NULL &&
      id->symbol->type->info.function.prototype.length > 0) {
    Symbol* this_formal = id->symbol->type->info.function.prototype.value.p[0];
    TypeRecord* this_type = this_formal != NULL ? this_formal->type : NULL;
    if (TypeIsPointer(this_type) && TypeIsStructOrUnion(this_type->next) &&
        this_type->next->info.struct_info != NULL &&
        this_type->next->info.struct_info->tag_name != NULL) {
      ctor_owner = this_type->next->info.struct_info;
      ctor_name = ctor_owner->tag_name;
    }
  }
  if (ctor_owner == NULL || ctor_name == NULL) {
    return false;
  }
  size_t original_arity =
      id->symbol->type->info.function.prototype.length;
  bool provisional_function_template =
      id->symbol->flags.is_template ||
      id->symbol->type->info.function.template_origin != NULL;
  if (original_arity == call->children->length) {
    if (!provisional_function_template &&
        !TypeContainsTemplateParameter(id->symbol->type)) {
      ASTNodeSetType(call->left, id->symbol->type);
      return true;
    }
    bool compatible = true;
    for (size_t i = 0; i < call->children->length; i++) {
      ASTNode* actual = call->children->value.p[i];
      Symbol* formal =
          id->symbol->type->info.function.prototype.value.p[i];
      if (actual == NULL || actual->type == NULL || formal == NULL ||
          formal->type == NULL ||
          !TypeAssignmentCompatible(actual->type, formal->type)) {
        compatible = false;
        break;
      }
    }
    if (compatible) {
      ASTNodeSetType(call->left, id->symbol->type);
      return true;
    }
  }
  StructMember* member = FindStructMemberOverloadHead(ctor_owner, ctor_name);
  if (member == NULL || member->symbol == NULL) {
    return false;
  }
  if (provisional_function_template) {
    // A dependent first pass may choose a converting constructor template for
    // same-type copy initialization. Implicit copy/move constructors can be
    // stored as separate same-named members rather than in that template's
    // overload chain, so recover an exact concrete candidate from the owner.
    StructMember* concrete_candidate = NULL;
    for (size_t member_index = 0;
         member_index < ctor_owner->members.length; member_index++) {
      for (StructMember* candidate = ctor_owner->members.value.p[member_index];
           candidate != NULL; candidate = candidate->overload_next) {
        if (candidate->symbol == NULL ||
            candidate->symbol->flags.is_template ||
            !StringEqual(&candidate->symbol->name, ctor_name->value) ||
            candidate->symbol->type == NULL ||
            !TypeIsFunction(candidate->symbol->type) ||
            candidate->symbol->type->info.function.template_origin != NULL ||
            candidate->symbol->type->info.function.prototype.length !=
                call->children->length) {
          continue;
        }
        bool compatible = true;
        for (size_t i = 1; i < call->children->length; i++) {
          ASTNode* actual = call->children->value.p[i];
          Symbol* formal =
              candidate->symbol->type->info.function.prototype.value.p[i];
          if (actual == NULL || actual->type == NULL || formal == NULL ||
              formal->type == NULL ||
              !TypeAssignmentCompatible(actual->type, formal->type)) {
            compatible = false;
            break;
          }
        }
        if (compatible) {
          concrete_candidate = candidate;
          break;
        }
      }
      if (concrete_candidate != NULL) {
        break;
      }
    }
    if (concrete_candidate != NULL) {
      member = concrete_candidate;
    }
  }
  if (original_arity != call->children->length) {
    StructMember* arity_fallback = NULL;
    for (StructMember* candidate = member; candidate != NULL;
         candidate = candidate->overload_next) {
      if (candidate->symbol != NULL &&
          !candidate->symbol->flags.is_template &&
          candidate->symbol->type != NULL &&
          TypeIsFunction(candidate->symbol->type) &&
          candidate->symbol->type->info.function.prototype.length ==
              call->children->length) {
        if (arity_fallback == NULL) {
          arity_fallback = candidate;
        }
        bool compatible = true;
        for (size_t i = 1; i < call->children->length; i++) {
          ASTNode* actual = call->children->value.p[i];
          Symbol* formal =
              candidate->symbol->type->info.function.prototype.value.p[i];
          if (actual == NULL || actual->type == NULL || formal == NULL ||
              formal->type == NULL ||
              !TypeAssignmentCompatible(actual->type, formal->type)) {
            compatible = false;
            break;
          }
        }
        if (compatible) {
          member = candidate;
          arity_fallback = NULL;
          break;
        }
      }
    }
    if (arity_fallback != NULL) {
      member = arity_fallback;
    }
    if (member->symbol->type->info.function.prototype.length !=
        call->children->length) {
      StructMember* owner_fallback = NULL;
      for (size_t member_index = 0;
           member_index < ctor_owner->members.length; member_index++) {
        StructMember* candidate = ctor_owner->members.value.p[member_index];
        if (candidate == NULL || candidate->symbol == NULL ||
            candidate->symbol->flags.is_template ||
            !StringEqual(&candidate->symbol->name, ctor_name->value) ||
            candidate->symbol->type == NULL ||
            !TypeIsFunction(candidate->symbol->type) ||
            candidate->symbol->type->info.function.prototype.length !=
                call->children->length) {
          continue;
        }
        if (owner_fallback == NULL) {
          owner_fallback = candidate;
        }
        bool compatible = true;
        for (size_t i = 1; i < call->children->length; i++) {
          ASTNode* actual = call->children->value.p[i];
          Symbol* formal =
              candidate->symbol->type->info.function.prototype.value.p[i];
          if (actual == NULL || actual->type == NULL || formal == NULL ||
              formal->type == NULL ||
              !TypeAssignmentCompatible(actual->type, formal->type)) {
            compatible = false;
            break;
          }
        }
        if (compatible) {
          member = candidate;
          owner_fallback = NULL;
          break;
        }
      }
      if (owner_fallback != NULL) {
        member = owner_fallback;
      }
    }
  }
  SaveAndLinkStructMemberOverloadSymbols(member, overload_snapshots);
  id->symbol = member->symbol;
  if (id->symbol != NULL && id->symbol->type != NULL &&
      TypeIsFunction(id->symbol->type) &&
      id->symbol->type->info.function.is_constructor) {
    id->symbol->type->info.function.cxx_member_owner = ctor_owner;
    StringClear(&id->symbol->asm_name);
    SymbolSetCXXMangledAsmName(id->symbol);
  }
  ASTNodeSetType(call->left, id->symbol->type);
  return true;
}

// Normalize pointer differences after all cloned-expression reanalysis has
// finished, when parent links reliably show whether a scale already exists.
// Dependent iterator subtraction can otherwise retain a raw byte difference;
// conversely, reanalysis can add a second division around a retained scale.
static ASTNode* NormalizeClonedPointerDifferenceScale(
    ASTNode* node, void* data, ASTNodeTransformAction* action) {
  (void)data;
  (void)action;
  if (node == NULL) {
    return NULL;
  }
  if (node->op == AST_OP(ptr_scale)) {
    PtrScaleASTNode* scale = (PtrScaleASTNode*)node;
    if (scale->scale_op == AST_OP(div) && scale->expr != NULL &&
        scale->expr->op == AST_OP(ptr_scale) &&
        ((PtrScaleASTNode*)scale->expr)->scale_op == AST_OP(div)) {
      ASTNode* replacement = ASTNodeMove(scale->expr);
      ASTNodeDelete(node);
      return replacement;
    }
    return node;
  }
  if (node->op != AST_OP(minus) || node->parent == NULL ||
      node->parent->op != AST_OP(cast)) {
    return node;
  }
  BinaryASTNode* minus = (BinaryASTNode*)node;
  if (minus->left == NULL || minus->right == NULL ||
      minus->left->type == NULL || minus->right->type == NULL ||
      !TypeIsPointerOrArray(minus->left->type) ||
      !TypeIsPointerOrArray(minus->right->type) ||
      TypeContainsTemplateParameter(minus->left->type) ||
      TypeContainsTemplateParameter(minus->right->type) ||
      minus->right->type->next == NULL) {
    return node;
  }
  ASTNode* scale = NewPtrScaleASTNode(
      minus->right->type->next, AST_OP(div), node, node->location);
  ASTNodeSetType(scale, NewTypeRecordWithSize(kTypeLong, kQualPlain));
  return scale;
}

/* Post-clone pass: prune the discarded branch of an `if constexpr` whose
 * condition has already been folded to a constant during the body clone (a
 * `requires`-expression condition is evaluated and replaced with 0/1 by
 * CloneTemplateFunctionBodyNode).  Statement-level analysis of `if constexpr`
 * (AnalyzeIfStatement) would eventually drop the not-taken branch, but the
 * intervening re-analysis passes below (ReanalyzeClonedResolvedCall, etc.)
 * would first walk that dead branch and re-resolve its construction calls --
 * e.g. `owning_view(v)` in the false branch of `views::all`'s
 * `if constexpr (requires { ref_view(v); }) ... else ...`.  Re-analyzing a
 * discarded branch can raise spurious errors (its constraints legitimately
 * fail for this argument), so eliminate it here, before those passes run. */
static ASTNode* PruneClonedConstexprIf(ASTNode* node, void* data,
                                       ASTNodeTransformAction* action) {
  (void)action;
  if (node == NULL || node->op != AST_OP(if)) {
    return node;
  }
  IfStatementASTNode* if_node = (IfStatementASTNode*)node;
  if (!if_node->is_constexpr || if_node->cond == NULL) {
    return node;
  }
  int64_t value = 0;
  bool folded = false;
  if (if_node->cond->op == AST_OP(number)) {
    value = ((ConstantASTNode*)if_node->cond)->value.ivalue;
    folded = true;
  } else {
    if (EvaluateIntegerExpression(if_node->cond, &value)) {
      folded = true;
    } else {
      ASTNodeVisit(if_node->cond, ClearAnalyzedFlagVisitor, 0, NULL);
      bool saved_trap = DiagnosticErrorTrapBegin();
      DiagnosticSuppressBegin();
      ASTNode* analyzed = AnalyzeExpression(if_node->cond);
      bool trapped = analyzed == NULL || DiagnosticErrorTrapped();
      DiagnosticSuppressEnd();
      DiagnosticErrorTrapEnd(saved_trap);
      if (!trapped) {
        if_node->cond = analyzed;
        folded = EvaluateIntegerExpression(if_node->cond, &value);
      }
    }
  }
  if (!folded) {
    return node;
  }
  ASTNode** taken_slot = value != 0 ? &if_node->if_part : &if_node->else_part;
  ASTNode* taken = *taken_slot;
  SourceLocation location = node->location;
  // Detach the surviving branch so deleting the `if` node does not free it.
  *taken_slot = NULL;
  ASTNodeDelete(node);
  if (taken == NULL) {
    taken = NewCompoundStatementASTNode(NewVector(), location);
  } else {
    taken->parent = NULL;
  }
  // Recurse so nested `if constexpr` statements inside the surviving branch are
  // pruned too (the driver does not descend into a replaced node).
  return ASTNodeVisitAndTransform(taken, PruneClonedConstexprIf, data);
}

static ASTNode* PruneConstexprIfBeforeBodyClone(
    ASTNode* node, void* data, ASTNodeTransformAction* action) {
  (void)action;
  if (node == NULL || node->op != AST_OP(if)) {
    return node;
  }
  IfStatementASTNode* if_node = (IfStatementASTNode*)node;
  if (!if_node->is_constexpr || if_node->cond == NULL) {
    return node;
  }
  TemplateFunctionBodyClone* clone = data;
  ASTNode* condition = ASTNodeClone(if_node->cond,
                                    CloneTemplateFunctionBodyNode, clone, NULL);
  if (condition == NULL) {
    return node;
  }
  int64_t value = 0;
  if (!EvaluateIntegerExpression(condition, &value)) {
    ASTNodeVisit(condition, ClearAnalyzedFlagVisitor, 0, NULL);
    bool saved_trap = DiagnosticErrorTrapBegin();
    DiagnosticSuppressBegin();
    ASTNode* analyzed = AnalyzeExpression(condition);
    bool trapped = analyzed == NULL || DiagnosticErrorTrapped();
    DiagnosticSuppressEnd();
    DiagnosticErrorTrapEnd(saved_trap);
    if (trapped || !EvaluateIntegerExpression(analyzed, &value)) {
      ASTNodeDelete(analyzed != NULL ? analyzed : condition);
      return node;
    }
    ASTNodeDelete(analyzed);
  } else {
    ASTNodeDelete(condition);
  }
  ASTNode** taken_slot = value != 0 ? &if_node->if_part : &if_node->else_part;
  ASTNode* taken = *taken_slot;
  SourceLocation location = node->location;
  *taken_slot = NULL;
  ASTNodeDelete(node);
  if (taken == NULL) {
    return NewCompoundStatementASTNode(NewVector(), location);
  }
  taken->parent = NULL;
  return ASTNodeVisitAndTransform(
      taken, PruneConstexprIfBeforeBodyClone, data);
}

static TypeRecord* ResolveClonedDependentTypedef(
    TemplateFunctionBodyClone* clone, TypeRecord* type) {
  if (clone == NULL || type == NULL || type->dependent_member_name == NULL) {
    return NULL;
  }
  TypeRecord* scope = TypeRecordCopy(type);
  StringDelete(scope->dependent_member_name);
  scope->dependent_member_name = NULL;
  TypeRecord* owner =
      SubstituteTemplateParameters(clone->parser, scope, clone->args);
  RebaseTemplateParameterIndices(owner,
                                 clone->rebase_template_parameter_base);
  owner = TypeMaterializeClassTemplateSpecialization(
      clone->parser->syntax, owner);
  TypeRecordDelete(scope);

  TypeRecord* result = NULL;
  if (owner != NULL && TypeIsStructOrUnion(owner) &&
      owner->info.struct_info != NULL) {
    StructMember* member =
        FindStructMember(owner->info.struct_info,
                         type->dependent_member_name);
    if (member != NULL && member->symbol != NULL &&
        member->symbol->type != NULL &&
        StorageIs(member->symbol->storage, STO(typedef))) {
      result = TypeRecordCopy(member->symbol->type);
      result->qualifiers |= type->qualifiers;
      result = TypeMaterializeClassTemplateSpecialization(
          clone->parser->syntax, result);
    }
  }
  TypeRecordDelete(owner);
  return result;
}

/* Post-clone pass: re-resolve already-typed construction calls in the cloned
 * body. Constructor calls may still reference the generic template's overload
 * set, and typedef class functional casts (e.g. `alias(args)`) may still carry
 * a dependent alias type even after the enclosing class is concrete. Keep this
 * narrow so ordinary resolved calls are not disturbed. */
static ASTNode* ReanalyzeClonedResolvedCall(
    ASTNode* node, void* data, ASTNodeTransformAction* action) {
  TemplateFunctionBodyClone* clone = data;
  if (node == NULL || node->op != AST_OP(call)) {
    return node;
  }
  VectorASTNode* call = (VectorASTNode*)node;
  if (call->left == NULL || call->left->op != AST_OP(identifier)) {
    return node;
  }
  if (CallActualsStillContainPackExpansion((ASTNode*)call) ||
      ASTNodeSubtreeContainsPackExpansion((ASTNode*)call)) {
    return node;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)call->left;
  if (clone != NULL && clone->to_func != NULL &&
      TypeIsFunction(clone->to_func) && id->symbol != NULL &&
      StorageIs(id->symbol->storage, STO(typedef)) &&
      id->symbol->type != NULL) {
    int parameter_index =
        FirstTemplateParameterIndexInType(id->symbol->type);
    int parameter_base =
        clone->to_func->info.function.template_parameter_base;
    int parameter_count =
        clone->to_func->info.function.template_parameter_count;
    if (parameter_index >= parameter_base &&
        parameter_index < parameter_base + parameter_count) {
      // This functional construction still names one of the cloned function
      // template's own type parameters. Its index has already been rebased for
      // the standalone function template, so substituting it again with the
      // enclosing class's argument vector would bind it to an unrelated class
      // parameter. Leave it dependent until the per-call instantiation.
      return node;
    }
  }
  if (clone != NULL && clone->to_owner != NULL && id->symbol != NULL &&
      StorageIs(id->symbol->storage, STO(typedef)) &&
      TypeContainsTemplateParameter(id->symbol->type)) {
    StructMember* concrete_member =
        FindStructMember(clone->to_owner, &id->symbol->name);
    if (concrete_member != NULL && concrete_member->symbol != NULL &&
        StorageIs(concrete_member->symbol->storage, STO(typedef)) &&
        concrete_member->symbol->type != NULL &&
        !TypeContainsTemplateParameter(concrete_member->symbol->type)) {
      id->symbol = concrete_member->symbol;
      ASTNodeSetType(call->left, concrete_member->symbol->type);
    }
  }
  bool is_typedef_class_construction =
      clone != NULL && id->template_arguments == NULL &&
      id->symbol != NULL && id->symbol->type != NULL &&
      StorageIs(id->symbol->storage, STO(typedef)) &&
      (TypeIsStructOrUnion(id->symbol->type) ||
       TypeContainsTemplateParameter(id->symbol->type));
  if (is_typedef_class_construction) {
    TypeRecord* concrete_type = TypeRecordCopy(id->symbol->type);
    if (TypeContainsTemplateParameter(concrete_type) ||
        (TypeIsStructOrUnion(concrete_type) &&
         (StructContainsTemplateParameter(concrete_type->info.struct_info) ||
          (clone->substitution_source != NULL &&
           concrete_type->info.struct_info->lexical_parent ==
               clone->substitution_source)))) {
      TypeRecordDelete(concrete_type);
      concrete_type = SubstituteTemplateBodyType(clone, id->symbol->type);
      RebaseTemplateParameterIndices(concrete_type,
                                     clone->rebase_template_parameter_base);
      concrete_type = TypeMaterializeClassTemplateSpecialization(
          clone->parser->syntax, concrete_type);
    }
    if (concrete_type != NULL &&
        TypeContainsTemplateParameter(concrete_type)) {
      TypeRecord* resolved =
          ResolveClonedDependentTypedef(clone, id->symbol->type);
      if (resolved != NULL) {
        TypeRecordDelete(concrete_type);
        concrete_type = resolved;
      }
    }
    if (!TypeIsStructOrUnion(concrete_type) &&
        !TypeContainsTemplateParameter(concrete_type) &&
        call->children != NULL && call->children->length == 0) {
      ASTNode* zero = NewIntConstantASTNode(0, concrete_type, node->location);
      if (node->parent != NULL) {
        ASTNodeReplaceChild(node->parent, node->child_id, zero, true);
      }
      return AnalyzeExpression(zero);
    }
    if (TypeIsStructOrUnion(concrete_type) &&
        !TypeContainsTemplateParameter(concrete_type)) {
      Symbol* concrete = NewSymbol(id->symbol->name.value, concrete_type,
                                   id->symbol->storage);
      concrete->flags = id->symbol->flags;
      concrete->location = id->symbol->location;
      concrete->alignment = id->symbol->alignment;
      concrete->namespace_ = id->symbol->namespace_;
      concrete->value = id->symbol->value;
      concrete->stack_offset = id->symbol->stack_offset;
      id->symbol = concrete;
      ASTNodeSetType(call->left, concrete->type);
      node->flags &= ~(kASTDependentFunctorCall | kASTAnalyzed);
      ASTNodeClearType(node);
      call->left->flags &= ~kASTAnalyzed;
      if (call->children != NULL) {
        for (size_t i = 0; i < call->children->length; i++) {
          ASTNode* actual = call->children->value.p[i];
          if (actual != NULL) {
            actual->flags &= ~kASTAnalyzed;
          }
        }
      }
      if (action != NULL) {
        *action = kASTTransformSkipChildren;
      }
      ASTNode* parent = node->parent;
      int child_id = node->child_id;
      node->parent = NULL;
      ASTNode* analyzed = AnalyzeExpression(node);
      if (analyzed == node) {
        node->parent = parent;
        node->child_id = child_id;
      }
      return analyzed;
    }
    if (TypeContainsTemplateParameter(concrete_type)) {
      Symbol* partial = NewSymbol(id->symbol->name.value, concrete_type,
                                  id->symbol->storage);
      partial->flags = id->symbol->flags;
      partial->location = id->symbol->location;
      partial->alignment = id->symbol->alignment;
      partial->namespace_ = id->symbol->namespace_;
      id->symbol = partial;
      ASTNodeSetType(call->left, partial->type);
      return node;
    }
    TypeRecordDelete(concrete_type);
  }
  bool is_constructor =
      id->symbol != NULL && id->symbol->type != NULL &&
      TypeIsFunction(id->symbol->type) &&
      id->symbol->type->info.function.is_constructor;
  if (!is_constructor) {
    return node;
  }
  if (call->children != NULL && call->children->length > 0) {
    ASTNode* receiver = call->children->value.p[0];
    if (receiver != NULL && receiver->op == AST_OP(address)) {
      ASTNode* object = ((UnaryASTNode*)receiver)->sub;
      if (object != NULL && object->type != NULL &&
          !TypeContainsTemplateParameter(object->type) &&
          (receiver->type == NULL || !TypeIsPointer(receiver->type) ||
           receiver->type->next == NULL ||
           !TypeEqual(receiver->type->next, object->type))) {
        TypeRecord* pointer =
            NewPointerTo(kQualPlain, TypeRecordCopy(object->type));
        ASTNodeSetType(receiver, pointer);
        TypeRecordDelete(pointer);
        receiver->flags &= ~kASTAnalyzed;
      }
    }
  }
  Struct* constructor_owner =
      id->symbol->type->info.function.cxx_member_owner;
  if (constructor_owner != NULL &&
      constructor_owner->lexical_parent == clone->substitution_source &&
      clone->substitution_target != NULL) {
    RetargetClonedNestedConstructorReceiver(
        clone, call, constructor_owner);
  }
  Vector overload_snapshots;
  VectorInit(&overload_snapshots);
  RebindClonedConstructorCall(call, &overload_snapshots);
  if (call->children != NULL && call->children->length > 0 &&
      id->symbol != NULL && id->symbol->type != NULL &&
      TypeIsFunction(id->symbol->type) &&
      id->symbol->type->info.function.prototype.length ==
          call->children->length) {
    ASTNode* receiver = call->children->value.p[0];
    Symbol* this_formal =
        id->symbol->type->info.function.prototype.value.p[0];
    if (receiver != NULL && receiver->op == AST_OP(address) &&
        this_formal != NULL && TypeIsPointer(this_formal->type) &&
        this_formal->type->next != NULL &&
        (receiver->type == NULL || !TypeIsPointer(receiver->type) ||
         receiver->type->next == NULL)) {
      ASTNodeSetType(receiver, this_formal->type);
      ASTNode* object = ((UnaryASTNode*)receiver)->sub;
      if (object != NULL) {
        ASTNodeSetType(object, this_formal->type->next);
      }
      receiver->flags &= ~kASTAnalyzed;
    }
  }
  node->flags &= ~(kASTDependentFunctorCall | kASTAnalyzed);
  ASTNodeClearType(node);
  if (call->left != NULL) {
    call->left->flags &= ~kASTAnalyzed;
  }
  if (action != NULL) {
    *action = kASTTransformSkipChildren;
  }
  ASTNode* parent = node->parent;
  int child_id = node->child_id;
  node->parent = NULL;
  ASTNode* analyzed = AnalyzeExpression(node);
  if (analyzed == node) {
    node->parent = parent;
    node->child_id = child_id;
  }
  RestoreSymbolOverloadLinks(&overload_snapshots);
  return analyzed;
}

/* A template-dependent expression can initially have a placeholder type,
 * causing semantic analysis to wrap it in the wrong scalar conversion. Once
 * cloning resolves concrete types, rebuild stale conversions whose source and
 * target now have the same scalar kind, as well as conversions around calls. */
static ASTNode* RebuildClonedCallResultConversion(
    ASTNode* node, void* data, ASTNodeTransformAction* action) {
  (void)data;
  if (node == NULL || node->op < AST_OP(i2s) ||
      node->op > AST_OP(b2ld)) {
    return node;
  }
  UnaryASTNode* conversion = (UnaryASTNode*)node;
  if (conversion->sub == NULL || conversion->sub->type == NULL ||
      node->type == NULL ||
      TypeContainsTemplateParameter(conversion->sub->type) ||
      TypeContainsTemplateParameter(node->type)) {
    return node;
  }
  TypeRecord* source = conversion->sub->type;
  TypeRecord* target_type = node->type;
  bool same_scalar_kind =
      (TypeIsInt(source) && TypeIsInt(target_type)) ||
      (TypeIsCharFamily(source) && TypeIsCharFamily(target_type)) ||
      (TypeIsShort(source) && TypeIsShort(target_type)) ||
      (TypeIsLong(source) && TypeIsLong(target_type)) ||
      (TypeIsLongLong(source) && TypeIsLongLong(target_type)) ||
      (TypeIsFloat(source) && TypeIsFloat(target_type)) ||
      (TypeIsFloat32(source) && TypeIsFloat32(target_type)) ||
      (TypeIsDouble(source) && TypeIsDouble(target_type)) ||
      (TypeIsFloat64(source) && TypeIsFloat64(target_type)) ||
      (TypeIsLongDouble(source) && TypeIsLongDouble(target_type)) ||
      (TypeIsBool(source) && TypeIsBool(target_type));
  if (conversion->sub->op != AST_OP(call) && !same_scalar_kind) {
    return node;
  }

  ASTNode* expression = conversion->sub;
  conversion->sub = NULL;
  expression->parent = NULL;
  TypeRecord* target = node->type;
  TypeRecordIncRef(target);
  SourceLocation location = node->location;
  ASTNodeDelete(node);

  ASTNode* holder =
      NewUnaryASTNode(AST_OP(uplus), NULL, location, expression);
  SemanticConvertType(expression, target, kConvertNormal);
  ASTNode* rebuilt = ((UnaryASTNode*)holder)->sub;
  ((UnaryASTNode*)holder)->sub = NULL;
  if (rebuilt != NULL) {
    rebuilt->parent = NULL;
  }
  ASTNodeDelete(holder);
  TypeRecordDelete(target);
  if (action != NULL) {
    *action = kASTTransformSkipChildren;
  }
  return rebuilt;
}

static bool IsReanalyzableClonedExpressionOpcode(ASTOpcode op) {
  switch (op) {
    case AST_OP(postinc):
    case AST_OP(postdec):
    case AST_OP(uminus):
    case AST_OP(uplus):
    case AST_OP(contents):
    case AST_OP(address):
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
    case AST_OP(question):
    case AST_OP(assign):
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
    case AST_OP(comma):
    case AST_OP(logand):
    case AST_OP(logor):
    case AST_OP(dotstar):
    case AST_OP(arrowstar):
      return true;
    default:
      return false;
  }
}

/* A class-template body may contain a non-dependent expression nested in an
 * otherwise dependent statement.  The template parse deliberately leaves that
 * expression untyped, but after cloning both operands can be concrete.  Analyze
 * the expression itself at that point so code generation never has to infer a
 * missing semantic type. */
static ASTNode* ReanalyzeClonedUntypedExpression(
    ASTNode* node, void* data, ASTNodeTransformAction* action) {
  (void)data;
  if (node != NULL && node->op == AST_OP(expansion_for)) {
    // The item declaration's type is deduced separately for every materialized
    // iteration.  Expressions in the retained pattern cannot be reanalyzed
    // until that declaration has been instantiated.
    *action = kASTTransformSkipChildren;
    return node;
  }
  bool stale_floating_arithmetic = false;
  if (node != NULL &&
      (node->op == AST_OP(plus) || node->op == AST_OP(minus) ||
       node->op == AST_OP(mult) || node->op == AST_OP(div)) &&
      ASTNodeGetShape(node) == kASTShapeBinary) {
    BinaryASTNode* binary = (BinaryASTNode*)node;
    stale_floating_arithmetic =
        binary->left != NULL && binary->right != NULL &&
        binary->left->type != NULL && binary->right->type != NULL &&
        (TypeIsFloatingPoint(binary->left->type) ||
         TypeIsFloatingPoint(binary->right->type)) &&
        (node->type == NULL || !TypeIsFloatingPoint(node->type));
  }
  if (node == NULL ||
      ExpressionIsTemplateDependent(node) ||
      (!stale_floating_arithmetic && node->type != NULL &&
       !TypeContainsAuto(node->type)) ||
      !IsReanalyzableClonedExpressionOpcode(node->op)) {
    return node;
  }
  ASTNodeShape shape = ASTNodeGetShape(node);
  if (shape == kASTShapeUnary) {
    ASTNode* sub = ((UnaryASTNode*)node)->sub;
    if (sub == NULL || sub->type == NULL ||
        TypeContainsTemplateParameter(sub->type)) {
      return node;
    }
  } else if (shape == kASTShapeBinary) {
    BinaryASTNode* binary = (BinaryASTNode*)node;
    if (binary->left == NULL || binary->right == NULL ||
        binary->left->type == NULL || binary->right->type == NULL ||
        TypeContainsTemplateParameter(binary->left->type) ||
        TypeContainsTemplateParameter(binary->right->type)) {
      return node;
    }
  } else {
    return node;
  }
  *action = kASTTransformSkipChildren;
  node->flags &= ~kASTAnalyzed;
  if (stale_floating_arithmetic) {
    ASTNodeClearType(node);
  }
  ASTNode* parent = node->parent;
  int child_id = node->child_id;
  node->parent = NULL;
  ASTNode* analyzed = AnalyzeExpression(node);
  if (analyzed != NULL) {
    analyzed->parent = parent;
    analyzed->child_id = child_id;
  }
  return analyzed;
}

static void DeduceClonedAutoLocalVisitor(ASTNode* node, void* data,
                                         int child_id, VisitorMode mode) {
  (void)data;
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(vardecl)) {
    return;
  }
  VariableDeclarationASTNode* decl = (VariableDeclarationASTNode*)node;
  if (decl->symbol == NULL || !TypeContainsAuto(decl->symbol->type) ||
      decl->initializer == NULL) {
    return;
  }
  node->flags &= ~kASTAnalyzed;
  AnalyzeStatement(node);
}

typedef struct {
  TemplateFunctionBodyClone* clone;
  bool materialized;
  Vector condition_symbols;  // Hidden objects for deferred binding conditions.
} DeferredStructuredBindingContext;

static ASTNode* MaterializeDeferredStructuredBinding(
    ASTNode* node, void* data, ASTNodeTransformAction* action) {
  if (node == NULL || node->op != AST_OP(structured_binding)) {
    return node;
  }
  DeferredStructuredBindingContext* context = data;
  TemplateFunctionBodyClone* clone = context->clone;
  StructuredBindingASTNode* binding = (StructuredBindingASTNode*)node;
  if (ClonedStructuredBindingStillDependent(binding)) {
    return node;
  }
  ASTNodeVisit(binding->initializer, ClearAnalyzedFlagVisitor, 0, NULL);
  Symbol* source_condition = binding->condition_symbol;
  context->materialized = true;
  *action = kASTTransformSkipChildren;
  ASTNode* result = SemanticMaterializeClonedStructuredBinding(
      node, &clone->symbol_map, &clone->pack_symbol_map);
  if (source_condition != NULL) {
    Symbol* replacement =
        MapFindPointerKey(&clone->symbol_map, source_condition);
    if (replacement != NULL) {
      VectorAppend(&context->condition_symbols, replacement);
    }
  }
  return result;
}

static ASTNode* ExpandDeferredStructuredBindingPackUse(
    ASTNode* node, void* data, ASTNodeTransformAction* action) {
  (void)action;
  if (node == NULL) {
    return NULL;
  }
  DeferredStructuredBindingContext* context = data;
  TemplateFunctionBodyClone* clone = context->clone;
  if (node->op == AST_OP(pack_index)) {
    return FoldClonedPackIndex(clone, node);
  }
  if ((node->flags & kASTFoldExpression) != 0) {
    return ExpandClonedFoldExpression(clone, node);
  }
  if (node->op == AST_OP(sizeof)) {
    SizeofASTNode* sizeof_node = (SizeofASTNode*)node;
    if (sizeof_node->is_pack_size && sizeof_node->expr != NULL &&
        sizeof_node->expr->op == AST_OP(identifier)) {
      Symbol* pack = ((IdentifierASTNode*)sizeof_node->expr)->symbol;
      Vector* replacements =
          MapFindPointerKey(&clone->pack_symbol_map, pack);
      if (replacements != NULL) {
        return NewIntConstantASTNode((int64_t)replacements->length,
                                     NewSizeTypeRecord(), node->location);
      }
    }
  }
  bool expanded_call = ExpandClonedCallPackActuals(clone, node);
  ExpandClonedBracedInitializerPackElements(clone, node);
  if (expanded_call) {
    ASTNodeVisit(node, ClearAnalyzedFlagVisitor, 0, NULL);
  }
  return node;
}

static bool NodeNamesDeferredBindingCondition(ASTNode* node, void* data) {
  if (node == NULL || node->op != AST_OP(identifier)) {
    return false;
  }
  Symbol* symbol = ((IdentifierASTNode*)node)->symbol;
  Vector* condition_symbols = data;
  for (size_t i = 0; i < condition_symbols->length; i++) {
    if (condition_symbols->value.p[i] == symbol) {
      return true;
    }
  }
  return false;
}

typedef struct {
  TemplateFunctionBodyClone* clone;
  bool materialized;
} DeferredExpansionContext;

static ASTNode* MaterializeDeferredExpansionStatement(
    ASTNode* node, void* data, ASTNodeTransformAction* action) {
  if (node == NULL || node->op != AST_OP(expansion_for)) {
    return node;
  }
  DeferredExpansionContext* context = data;
  TemplateFunctionBodyClone* clone = context->clone;
  ExpansionStatementASTNode* expansion = (ExpansionStatementASTNode*)node;
  if (ExpansionStatementIsDependent(expansion, &clone->symbol_map,
                                      &clone->pack_symbol_map)) {
    return node;
  }
  if (expansion->init_stmt != NULL) {
    ASTNodeVisit(expansion->init_stmt, ClearAnalyzedFlagVisitor, 0, NULL);
  }
  if (expansion->initializer != NULL) {
    ASTNodeVisit(expansion->initializer, ClearAnalyzedFlagVisitor, 0, NULL);
  }
  if (expansion->stmt != NULL) {
    ASTNodeVisit(expansion->stmt, ClearAnalyzedFlagVisitor, 0, NULL);
  }
  context->materialized = true;
  *action = kASTTransformSkipChildren;
  return SemanticMaterializeExpansionStatement(
      expansion, &clone->symbol_map, &clone->pack_symbol_map);
}

static ASTNode* ReanalyzeDeferredStructuredBindingCondition(
    ASTNode* node, void* data, ASTNodeTransformAction* action) {
  DeferredStructuredBindingContext* context = data;
  ASTNode* condition = NULL;
  if (node != NULL && node->op == AST_OP(if)) {
    condition = ((IfStatementASTNode*)node)->cond;
  } else if (node != NULL && node->op == AST_OP(while)) {
    condition = ((CombinedStatementASTNode*)node)->cond;
  } else if (node != NULL && node->op == AST_OP(switch)) {
    condition = ((SwitchStatementASTNode*)node)->expr;
  }
  if (condition == NULL ||
      !ASTNodeAny(condition, NodeNamesDeferredBindingCondition,
                  &context->condition_symbols)) {
    return node;
  }
  ASTNodeVisit(node, ClearAnalyzedFlagVisitor, 0, NULL);
  if (node->op == AST_OP(switch)) {
    SwitchStatementASTNode* switch_node = (SwitchStatementASTNode*)node;
    VectorClear(&switch_node->cases);
    switch_node->default_node = NULL;
  }
  *action = kASTTransformSkipChildren;
  AnalyzeStatement(node);
  return node;
}

static void RefreshClonedIdentifierTypeVisitor(ASTNode* node, void* data,
                                                int child_id,
                                                VisitorMode mode) {
  (void)data;
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(identifier)) {
    return;
  }
  Symbol* symbol = ((IdentifierASTNode*)node)->symbol;
  if (symbol != NULL && symbol->type != NULL &&
      !TypeContainsAuto(symbol->type) &&
      (node->type == NULL || TypeContainsAuto(node->type))) {
    ASTNodeSetType(node, symbol->type);
    node->flags &= ~kASTAnalyzed;
  }
}

static void RefreshClonedCXXNewMetadataVisitor(ASTNode* node, void* data,
                                                int child_id,
                                                VisitorMode mode) {
  (void)data;
  (void)child_id;
  if (mode != kVisitPostChildren || node == NULL ||
      node->op != AST_OP(cast) ||
      (node->flags & kASTCXXNewExpression) == 0) {
    return;
  }
  CastASTNode* cast = (CastASTNode*)node;
  if (cast->expr == NULL || cast->expr->op != AST_OP(call)) {
    return;
  }
  VectorASTNode* allocation = (VectorASTNode*)cast->expr;
  if (allocation->children == NULL || allocation->children->length != 2 ||
      allocation->children->value.p[1] == NULL) {
    node->flags &= ~kASTCXXPlacementNew;
    return;
  }
  ASTNode* placement = allocation->children->value.p[1];
  if (TypeIsPointer(placement->type)) {
    node->flags |= kASTCXXPlacementNew;
  } else {
    node->flags &= ~kASTCXXPlacementNew;
  }
}

static void RebindClonedDesignatorMemberVisitor(ASTNode* node, void* data,
                                                int child_id,
                                                VisitorMode mode) {
  (void)data;
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(designated_init) || node->parent == NULL ||
      node->parent->op != AST_OP(braced_init) ||
      !TypeIsStructOrUnion(node->parent->type)) {
    return;
  }
  DesignatedInitializerASTNode* initializer =
      (DesignatedInitializerASTNode*)node;
  TypeRecord* aggregate_type = node->parent->type;
  for (size_t i = 0; initializer->designators != NULL &&
                     i < initializer->designators->length; i++) {
    Designator* designator = initializer->designators->value.p[i];
    if (designator == NULL ||
        designator->designator_type != kDesignatorStruct ||
        !designator->is_resolved_member ||
        designator->value.struct_member == NULL ||
        designator->value.struct_member->symbol == NULL ||
        !TypeIsStructOrUnion(aggregate_type)) {
      break;
    }
    const char* name =
        designator->value.struct_member->symbol->name.value;
    StructMember* concrete =
        FindStructMemberByName(aggregate_type->info.struct_info, name);
    if (concrete == NULL || concrete->symbol == NULL) {
      break;
    }
    designator->value.struct_member = concrete;
    aggregate_type = concrete->symbol->type;
  }
}

/* Clone the body of function template `from` into the concrete instantiation
 * `to`, substituting template arguments `args`. First builds the original->
 * clone symbol map (mapping each generic formal to its instantiated formal, and
 * each parameter pack to the vector of its expanded formals), then clones the
 * AST via CloneTemplateFunctionBodyNode and runs the post-clone reanalysis
 * passes (pack-element rewrites, nested template-call instantiation, and
 * dependent/constructor call re-resolution). Returns the new body. */
ASTNode* CloneTemplateFunctionBody(TypeParser* parser,
                                          TypeRecord* from,
                                          TypeRecord* to,
                                          Vector* args) {
  if (from == NULL || to == NULL || from->info.function.body == NULL) {
    return NULL;
  }
  TemplateFunctionBodyClone clone;
  MapInitForPointerKeys(&clone.symbol_map);
  MapInitForPointerKeys(&clone.pack_symbol_map);
  clone.parser = parser;
  clone.args = args;
  clone.from_func = from;
  clone.to_func = to;
  clone.rebase_template_parameter_base =
      from->info.function.template_parameter_base;
  clone.from_owner = CloneFunctionMemberOwner(from);
  clone.to_owner = CloneFunctionMemberOwner(to);
  clone.substitution_source =
      clone.from_owner != NULL && clone.to_owner != NULL
          ? clone.from_owner
          : parser->template_substitution_source;
  clone.substitution_target =
      clone.from_owner != NULL && clone.to_owner != NULL
          ? clone.to_owner
          : parser->template_substitution_target;
  size_t to_index = 0;
  for (size_t i = 0; i < from->info.function.prototype.length; i++) {
    Symbol* from_formal = from->info.function.prototype.value.p[i];
    if (from_formal == NULL) {
      continue;
    }
    if (from_formal->flags.is_parameter_pack) {
      int pack_index = -1;
      size_t pack_length = 0;
      bool found_pack = FindPackExpansionInType(from_formal->type, args,
                                                &pack_index, &pack_length);
      TemplateArgument* pack =
          found_pack && pack_index >= 0 && (size_t)pack_index < args->length
              ? args->value.p[pack_index]
              : NULL;
      bool expandable = pack != NULL && pack->pack_arguments != NULL;
      if (!expandable) {
        // The pack's argument is not part of this clone's `args`, so it belongs
        // to an outer/nested template that is not being instantiated here (e.g.
        // a member function template's own pack while its enclosing class
        // template is instantiated).  Map it 1:1 to the corresponding still-pack
        // formal in `to` so the pack expansion in the body is preserved verbatim
        // for the later member-template instantiation instead of being expanded
        // to an empty sequence.
        if (to_index < to->info.function.prototype.length) {
          Symbol* replacement =
              to->info.function.prototype.value.p[to_index++];
          if (replacement != NULL &&
              !TypeContainsTemplateParameter(replacement->type)) {
            replacement->flags.is_parameter_pack = false;
            Vector* replacements = NewVector();
            VectorAppend(replacements, replacement);
            MapKeyValue kv;
            kv.key.p = from_formal;
            kv.value.p = replacements;
            MapInsert(&clone.pack_symbol_map, kv);
            continue;
          }
          MapKeyValue kv;
          kv.key.p = from_formal;
          kv.value.p = replacement;
          MapInsert(&clone.symbol_map, kv);
        }
        continue;
      }
      if (pack_length == 0) {
        pack_length = pack->pack_arguments->length;
      }
      Vector* replacements = NewVector();
      for (size_t j = 0;
           j < pack_length && to_index < to->info.function.prototype.length;
           j++) {
        Symbol* replacement =
            to->info.function.prototype.value.p[to_index++];
        VectorAppend(replacements, replacement);
      }
      MapKeyValue kv;
      kv.key.p = from_formal;
      kv.value.p = replacements;
      MapInsert(&clone.pack_symbol_map, kv);
      continue;
    }
    if (to_index >= to->info.function.prototype.length) {
      break;
    }
    MapKeyValue kv;
    kv.key.p = from_formal;
    kv.value.p = to->info.function.prototype.value.p[to_index++];
    MapInsert(&clone.symbol_map, kv);
  }
  // Make self-type references inside the body (the injected-class-name used as a
  // local variable's type, a `static_cast<T&&>` target, etc.) resolve to *this*
  // instantiation rather than the generic primary template.  The deferred body
  // clone runs long after InstantiateTemplateMemberFunction restored these, so
  // re-establish the source->target struct mapping that
  // SubstituteTemplateParameters consults; otherwise such types stay generic and
  // member calls (e.g. the move ctor/assignment used by `swap`) target an
  // unemitted, generic-mangled symbol.
  Struct* saved_access_context = compiler->current_class_access_context;
  TypeSubstitutionScope substitution = TypeParserPushTemplateSubstitution(
      parser, clone.substitution_source, clone.substitution_target);
  if (clone.from_owner != NULL && clone.to_owner != NULL) {
    compiler->current_class_access_context = clone.to_owner;
  }
  AddOwnerMemberSymbolMappings(&clone);
  // Default arguments live on the function prototype rather than in the body,
  // but they can contain the same dependent member aliases as the body (for
  // example `key_compare()` and `allocator_type()` on a class-template
  // constructor).  Re-clone them with the owner/source mappings instead of
  // retaining the identity clone made while the prototype was built.
  for (size_t i = 0; i < from->info.function.prototype.length; i++) {
    Symbol* from_formal = from->info.function.prototype.value.p[i];
    if (from_formal == NULL || from_formal->default_argument == NULL ||
        from_formal->flags.is_parameter_pack) {
      continue;
    }
    Symbol* to_formal = MapFindPointerKey(&clone.symbol_map, from_formal);
    if (to_formal == NULL) {
      continue;
    }
    ASTNode* default_argument =
        ASTNodeClone(from_formal->default_argument,
                     CloneTemplateFunctionBodyNode, &clone, NULL);
    default_argument = ASTNodeVisitAndTransform(
        default_argument, ReanalyzeClonedDependentFunctorCall, NULL);
    default_argument = ASTNodeVisitAndTransform(
        default_argument, ReanalyzeClonedResolvedCall, &clone);
    ASTNodeDelete(to_formal->default_argument);
    to_formal->default_argument = default_argument;
  }
  VectorDestructWithContents(
      &to->info.function.contract_assertions,
      (VectorElementDestructor)ContractAssertionDelete,
      /*free_element=*/false);
  VectorInit(&to->info.function.contract_assertions);
  for (size_t i = 0;
       i < from->info.function.contract_assertions.length; i++) {
    ContractAssertion* source =
        from->info.function.contract_assertions.value.p[i];
    Symbol* result_binding = NULL;
    if (source->result_binding != NULL) {
      result_binding =
          NewSymbol(source->result_binding->name.value,
                    source->result_binding->type,
                    source->result_binding->storage);
      result_binding->flags = source->result_binding->flags;
      result_binding->location = source->result_binding->location;
      MapKeyValue kv;
      kv.key.p = source->result_binding;
      kv.value.p = result_binding;
      MapInsert(&clone.symbol_map, kv);
    }
    ASTNode* predicate =
        ASTNodeClone(source->predicate, CloneTemplateFunctionBodyNode,
                     &clone, NULL);
    predicate = ASTNodeVisitAndTransform(
        predicate, ReanalyzeClonedDependentFunctorCall, NULL);
    predicate = ASTNodeVisitAndTransform(
        predicate, ReanalyzeClonedResolvedCall, &clone);
    Vector attrs = {0};
    AttributeListClone(&attrs, &source->attributes);
    VectorAppend(&to->info.function.contract_assertions,
                 NewContractAssertion(source->kind, predicate,
                                      result_binding, &attrs,
                                      source->location));
  }
  ASTNode* clone_source =
      ASTNodeClone(from->info.function.body, IdentityCloneNode, NULL, NULL);
  clone_source = ASTNodeVisitAndTransform(
      clone_source, PruneConstexprIfBeforeBodyClone, &clone);
  ASTNode* body = ASTNodeClone(clone_source, CloneTemplateFunctionBodyNode,
                               &clone, NULL);
  ASTNodeDelete(clone_source);
  ASTNodeVisit(body, RebindClonedConcreteMemberAccessVisitor, 0, &clone);
  ASTNodeVisit(body, RebindClonedLoweredDependentMemberCallVisitor, 0, NULL);
  ASTNodeVisit(body, RebindClonedDesignatorMemberVisitor, 0, NULL);
  // Drop discarded `if constexpr` branches (whose condition the clone above has
  // already folded to a constant) before the re-analysis passes can walk them.
  body = ASTNodeVisitAndTransform(body, PruneClonedConstexprIf, NULL);
  body = ASTNodeVisitAndTransform(body, ReanalyzeClonedDependentFunctorCall,
                                  NULL);
  ASTNodeVisit(body, ReplaceSingleElementPackIdentifierVisitor, 0, &clone);
  ASTNodeVisit(body, InstantiateClonedFunctionTemplateCallVisitor, 0, &clone);
  body = ASTNodeVisitAndTransform(body, ReanalyzeClonedDependentFunctorCall,
                                  NULL);
  body = ASTNodeVisitAndTransform(body, ReanalyzeClonedDependentFunctorCall,
                                  NULL);
  ASTNodeVisit(body, RefreshClonedIdentifierTypeVisitor, 0, NULL);
  body = ASTNodeVisitAndTransform(body, ReanalyzeClonedResolvedCall, &clone);
  body = ASTNodeVisitAndTransform(body, RebuildClonedCallResultConversion,
                                  NULL);
  ASTNodeVisit(body, MarkClonedCastForReanalysis, 0, NULL);
  TypeRecord* saved_function = compiler->current_function;
  compiler->current_function = to;
  ASTNodeVisit(body, DeduceClonedAutoLocalVisitor, 0, NULL);
  ASTNodeVisit(body, RefreshClonedIdentifierTypeVisitor, 0, NULL);
  body = ASTNodeVisitAndTransform(body, ReanalyzeClonedUntypedExpression, NULL);
  DeferredStructuredBindingContext deferred_binding = {
      .clone = &clone, .materialized = false};
  VectorInit(&deferred_binding.condition_symbols);
  body = ASTNodeVisitAndTransform(
      body, MaterializeDeferredStructuredBinding, &deferred_binding);
  if (deferred_binding.materialized) {
    RewriteTemplateBodyIdentifiers(body, &clone.symbol_map);
    body = ASTNodeVisitAndTransform(
        body, ExpandDeferredStructuredBindingPackUse, &deferred_binding);
    ASTNodeVisit(body, DeduceClonedAutoLocalVisitor, 0, NULL);
    ASTNodeVisit(body, RefreshClonedIdentifierTypeVisitor, 0, NULL);
    body =
        ASTNodeVisitAndTransform(body, ReanalyzeClonedUntypedExpression, NULL);
    body = ASTNodeVisitAndTransform(
        body, ReanalyzeDeferredStructuredBindingCondition, &deferred_binding);
  }
  DeferredExpansionContext deferred_expansion = {.clone = &clone,
                                                 .materialized = false};
  body = ASTNodeVisitAndTransform(
      body, MaterializeDeferredExpansionStatement, &deferred_expansion);
  if (deferred_expansion.materialized) {
    ASTNodeVisit(body, ClearAnalyzedFlagVisitor, 0, NULL);
    ASTNodeVisit(body, DeduceClonedAutoLocalVisitor, 0, NULL);
    ASTNodeVisit(body, RefreshClonedIdentifierTypeVisitor, 0, NULL);
    body =
        ASTNodeVisitAndTransform(body, ReanalyzeClonedUntypedExpression, NULL);
    to->info.function.body = body;
    AnalyzeStatement(body);
  }
  VectorDestruct(&deferred_binding.condition_symbols);
  body = ASTNodeVisitAndTransform(body, RebuildClonedCallResultConversion,
                                  NULL);
  body = ASTNodeVisitAndTransform(
      body, NormalizeClonedPointerDifferenceScale, NULL);
  ASTNodeVisit(body, RefreshClonedCXXNewMetadataVisitor, 0, NULL);
  compiler->current_function = saved_function;
  TypeParserPopTemplateSubstitution(&substitution);
  compiler->current_class_access_context = saved_access_context;
  MapDestructWithContents(&clone.pack_symbol_map, DeleteMappedVector);
  MapDestruct(&clone.symbol_map);
  return body;
}

/* True if a template instantiation with the given mangled asm name is already
 * queued for emission, so it is not instantiated/emitted twice. */
bool PendingTemplateInstantiationHasAsmName(const char* asm_name) {
  return CompilerPendingTemplateInstantiationHasAsmName(asm_name);
}

// True if a function template has a template parameter pack among its own
// parameters (e.g. `template <class... Args>`).  Such member constructor
// templates must defer member-initializer preamble insertion until per-call
// instantiation, when the pack length is known (see the caller).
static bool FunctionTemplateHasOwnParameterPack(TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func)) {
    return false;
  }
  for (size_t i = 0; i < func->info.function.template_parameters.length; i++) {
    TemplateParameter* param =
        func->info.function.template_parameters.value.p[i];
    if (param != NULL && param->is_parameter_pack) {
      return true;
    }
  }
  for (size_t i = 0; i < func->info.function.prototype.length; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    if (formal != NULL && formal->flags.is_parameter_pack) {
      return true;
    }
  }
  return false;
}

// Defer class-typed member initializers in a member constructor template until
// the constructor's own arguments are known.  Their constructor overload set
// can depend on those arguments even when the outer initializer node's cached
// type no longer appears dependent after the enclosing class was substituted.
static bool ConstructorHasClassMemberInitializer(
    Symbol* template_definition, Symbol* symbol) {
  if (template_definition == NULL || symbol == NULL || symbol->type == NULL ||
      symbol->type->info.function.cxx_member_owner == NULL) {
    return false;
  }
  CXXConstructorInitList* initializers =
      FindTemplateConstructorInitializers(template_definition);
  if (initializers == NULL) {
    return false;
  }
  Struct* owner = symbol->type->info.function.cxx_member_owner;
  for (size_t i = 0; i < initializers->deferred_initializers.length; i++) {
    CXXDeferredConstructorInitializer* initializer =
        initializers->deferred_initializers.value.p[i];
    if (initializer == NULL || initializer->actuals == NULL) {
      continue;
    }
    StructMember* member = FindStructMember(owner, &initializer->name);
    if (member == NULL || member->symbol == NULL ||
        !TypeIsStructOrUnion(member->symbol->type)) {
      continue;
    }
    return true;
  }
  return false;
}

void SyntaxInsertClonedTemplateConstructorPreamble(TypeParser* parser,
                                                    Symbol* template_definition,
                                                    Symbol* symbol,
                                                    Vector* args) {
  if (symbol == NULL || symbol->type == NULL ||
      !symbol->type->info.function.is_constructor ||
      symbol->type->info.function.body == NULL ||
      symbol->type->info.function.body->op != AST_OP(compound)) {
    return;
  }
  CXXConstructorInitList* stored =
      FindTemplateConstructorInitializers(template_definition);
  CXXConstructorInitList* initializers =
      SyntaxCXXConstructorInitListCloneDeferred(stored);
  if (initializers == NULL) {
    return;
  }
  CompoundStatementASTNode* body =
      (CompoundStatementASTNode*)symbol->type->info.function.body;
  size_t first_new_statement = body->statements->length;
  Struct* saved_access_context = compiler->current_class_access_context;
  compiler->current_class_access_context =
      symbol->type->info.function.cxx_member_owner;
  SyntaxInsertCXXConstructorPreamble(parser->syntax, symbol->type,
                                     body->statements, initializers,
                                     symbol->location);
  AnalyzeInsertedConstructorPreamble(
      parser, template_definition->type, symbol->type, body->statements,
      body->statements->length - first_new_statement, args);
  compiler->current_class_access_context = saved_access_context;
  SyntaxCXXConstructorInitListDestruct(initializers);
  free(initializers);
}

/* Instantiate the body of a member function template into `symbol` by cloning
 * `template_definition`'s body with `args`, mark it defined, set inline/weak
 * linkage as appropriate, and queue the instantiation for code emission (unless
 * it is still dependent or already queued). */
void QueueTemplateMemberFunctionDefinitionImpl(Symbol* symbol,
                                                      Symbol* template_definition,
                                                      TypeParser* parser,
                                                      Vector* args,
                                                      bool allow_lazy) {
  if (symbol == NULL || symbol->type == NULL ||
      template_definition == NULL || template_definition->type == NULL ||
      template_definition->type->info.function.body == NULL) {
    return;
  }
  if (allow_lazy && !symbol->flags.is_template && args != NULL) {
    // A nested non-generic lambda inside a generic lambda / function template
    // still needs its body cloned against the rebuilt closure: capture field
    // types and dependent functor calls (e.g. `vis(x)` on a captured template
    // parameter) are only concrete after this substitution.  The plain lazy
    // path would keep the template-level body and mis-lower those calls.
    bool nested_lambda_operator =
        symbol->name.value != NULL &&
        strcmp(symbol->name.value, "operator()") == 0 &&
        symbol->type->info.function.cxx_member_owner != NULL &&
        symbol->type->info.function.cxx_member_owner->tag_symbol != NULL &&
        symbol->type->info.function.cxx_member_owner->tag_symbol->flags.invented;
    if (!nested_lambda_operator) {
      symbol->value.func_defn = template_definition;
      if (symbol->type->template_arguments == NULL) {
        symbol->type->template_arguments = TemplateArgumentVectorCopy(args);
      }
      return;
    }
  }
  if (PendingTemplateInstantiationHasAsmName(symbol->asm_name.value)) {
    symbol->type->info.function.definition = true;
    symbol->flags.is_defined = true;
    symbol->value.func_defn = symbol;
    return;
  }
  if (allow_lazy && symbol->flags.is_template && args != NULL) {
    symbol->type->info.function.body =
        CloneTemplateFunctionBody(parser, template_definition->type,
                                  symbol->type, args);
    // A member function TEMPLATE constructor with a *parameter pack* must not
    // have its member-initializer list inserted and analyzed now: with its own
    // parameters still unbound, a member pack such as an in-place variadic
    // constructor `v(static_cast<Args&&>(args)...)` mis-expands to a
    // value-initialization `v()` that then fails for a type (e.g. a lambda)
    // whose value-init is not viable here.  For those, the preamble is deferred
    // to per-call instantiation (its init-list is re-keyed onto this class-level
    // symbol so the per-call clone can rediscover it).  A non-pack member
    // template constructor is inserted now, but any still-dependent initializer
    // is left unanalyzed until the per-call body clone binds its parameters.
    bool defer_constructor_preamble =
        symbol->type->info.function.is_constructor &&
        (FunctionTemplateHasOwnParameterPack(symbol->type) ||
         ConstructorHasClassMemberInitializer(template_definition, symbol));
    if (symbol->type->info.function.is_constructor &&
        (symbol->type->info.function.template_parameter_count == 0 ||
         !defer_constructor_preamble)) {
      SyntaxInsertClonedTemplateConstructorPreamble(parser, template_definition,
                                                    symbol, args);
    } else if (symbol->type->info.function.is_constructor) {
      CopyTemplateConstructorInitializersKey(template_definition, symbol);
    }
    symbol->type->info.function.definition = true;
    symbol->flags.is_defined = true;
    symbol->value.func_defn = symbol;
    return;
  }
  symbol->type->info.function.body =
      CloneTemplateFunctionBody(parser, template_definition->type,
                                symbol->type, args);
  SyntaxInsertClonedTemplateConstructorPreamble(parser, template_definition,
                                                symbol, args);
  symbol->type->info.function.definition = true;
  symbol->flags.is_defined = true;
  symbol->value.func_defn = symbol;
  if (symbol->type->info.function.is_inline) {
    symbol->flags.is_inline_defn = true;
    if (!StorageIs(symbol->storage, STO(static))) {
      symbol->flags.is_weak = true;
    }
  }
  if (TypeContainsTemplateParameter(symbol->type)) {
    return;
  }
  if (symbol->flags.is_template) {
    VectorAppend(&compiler->declaration_asts, symbol->type->info.function.body);
    return;
  }
  Vector* declarations = NewVector();
  VectorAppend(declarations,
               NewVariableDeclarationASTNode(symbol, NULL, symbol->location));
  CompilerQueuePendingTemplateInstantiation(
      NewDeclarationListASTNode(declarations, symbol->location));
  VectorAppend(&compiler->declaration_asts, symbol->type->info.function.body);
}

void TypeEnsureTemplateMemberFunctionDefinition(Syntax* syntax, Symbol* symbol) {
  if (symbol == NULL || symbol->type == NULL ||
      !TypeIsFunction(symbol->type) ||
      symbol->type->info.function.body != NULL) {
    return;
  }
  if (!symbol->flags.is_template &&
      symbol->type->info.function.template_origin != NULL &&
      symbol->type->template_arguments != NULL) {
    Symbol* template_definition =
        symbol->type->info.function.template_origin;
    if (symbol->type->info.function.cxx_member_owner != NULL) {
      if ((template_definition->type == NULL ||
           template_definition->type->info.function.body == NULL) &&
          template_definition->value.func_defn != NULL) {
        template_definition = template_definition->value.func_defn;
      }
      TypeParser parser;
      TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                     syntax->context);
      QueueTemplateMemberFunctionDefinitionImpl(
          symbol, template_definition, &parser,
          symbol->type->template_arguments, /*allow_lazy=*/false);
      TypeParserDestruct(&parser);
    } else {
      TypeInstantiateFunctionTemplate(
          syntax, template_definition, symbol->type->template_arguments);
    }
    return;
  }
  if (symbol->flags.is_template ||
      symbol->type->info.function.cxx_member_owner == NULL) {
    return;
  }
  if (symbol->type->info.function.is_defaulted &&
      (symbol->type->info.function.is_implicitly_declared ||
       symbol->type->info.function.is_explicitly_defaulted)) {
    if (symbol->type->info.function.is_deleted) {
      return;
    }
    TypeParser parser;
    TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                   syntax->context);
    SynthesizeDefaultedMemberFunctionBody(&parser, symbol);
    TypeParserDestruct(&parser);
    return;
  }
  if (symbol->value.func_defn == NULL) {
    return;
  }
  Vector* template_arguments = symbol->type->template_arguments;
  Struct* owner = symbol->type->info.function.cxx_member_owner;
  if (template_arguments == NULL && owner != NULL &&
      owner->tag_symbol != NULL && owner->tag_symbol->type != NULL) {
    template_arguments = owner->tag_symbol->type->template_arguments;
  }
  if (template_arguments == NULL) {
    return;
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  Symbol* template_definition = symbol->value.func_defn;
  TypeRecord* source_owner =
      template_definition->type != NULL &&
              template_definition->type->info.function.cxx_member_owner != NULL &&
              template_definition->type->info.function.cxx_member_owner
                      ->tag_symbol != NULL
          ? template_definition->type->info.function.cxx_member_owner
                ->tag_symbol->type
          : NULL;
  TypeSubstitutionScope substitution = TypeParserPushTemplateSubstitution(
      &parser,
      source_owner != NULL && TypeIsStructOrUnion(source_owner)
          ? source_owner->info.struct_info
          : NULL,
      symbol->type->info.function.cxx_member_owner);
  QueueTemplateMemberFunctionDefinitionImpl(
      symbol, template_definition, &parser, template_arguments,
      /*allow_lazy=*/false);
  TypeParserPopTemplateSubstitution(&substitution);
  TypeParserDestruct(&parser);
}

/* Build the concrete function type for a function-template instantiation:
 * copy all function attributes from the template type `from`, substitute the
 * return type and each parameter (expanding parameter packs) against `args`,
 * and renumber argument slots. (The body is cloned separately.) */
static void AnalyzeInsertedConstructorPreamble(TypeParser* parser,
                                               TypeRecord* from_func,
                                               TypeRecord* func, Vector* body,
                                               size_t inserted_count,
                                               Vector* args) {
  if (func == NULL || body == NULL) {
    return;
  }
  TemplateFunctionBodyClone clone;
  MapInitForPointerKeys(&clone.symbol_map);
  MapInitForPointerKeys(&clone.pack_symbol_map);
  clone.parser = parser;
  clone.args = args;
  clone.from_func = from_func;
  clone.to_func = func;
  // Match CloneTemplateFunctionBody: for a member function *template* the
  // member's own template parameters are numbered after the enclosing class
  // parameters, so any explicit template arguments in the member-initializer
  // list (e.g. `value(std::forward<Arg>(arg))`) reference the member's own
  // parameters at their absolute index.  Rebasing by the template-parameter
  // base renumbers those remaining references down to the member's zero-based
  // arguments, exactly as the body clone does; without it the reference keeps
  // its enclosing-offset index and the later substitution silently drops the
  // explicit template argument (turning `forward<Arg>` into an un-deducible
  // `forward`).  For a non-member template constructor the base is 0, so this
  // is a no-op there.
  clone.rebase_template_parameter_base =
      from_func != NULL && TypeIsFunction(from_func)
          ? from_func->info.function.template_parameter_base
          : 0;
  clone.from_owner = CloneFunctionMemberOwner(from_func);
  clone.to_owner = TypeIsFunction(func) ? func->info.function.cxx_member_owner
                                        : NULL;
  clone.substitution_source =
      clone.from_owner != NULL && clone.to_owner != NULL
          ? clone.from_owner
          : parser->template_substitution_source;
  clone.substitution_target =
      clone.from_owner != NULL && clone.to_owner != NULL
          ? clone.to_owner
          : parser->template_substitution_target;
  if (from_func != NULL && TypeIsFunction(from_func) && TypeIsFunction(func)) {
    size_t to_index = 0;
    for (size_t i = 0; i < from_func->info.function.prototype.length; i++) {
      Symbol* from_formal = from_func->info.function.prototype.value.p[i];
      if (from_formal == NULL) {
        continue;
      }
      if (from_formal->flags.is_parameter_pack) {
        int pack_index = -1;
        size_t pack_length = 0;
        bool found_pack = FindPackExpansionInType(from_formal->type, args,
                                                  &pack_index, &pack_length);
        TemplateArgument* pack =
            found_pack && pack_index >= 0 && (size_t)pack_index < args->length
                ? args->value.p[pack_index]
                : NULL;
        bool expandable = pack != NULL && pack->pack_arguments != NULL;
        if (!expandable) {
          if (to_index < func->info.function.prototype.length) {
            MapKeyValue kv;
            kv.key.p = from_formal;
            kv.value.p = func->info.function.prototype.value.p[to_index++];
            MapInsert(&clone.symbol_map, kv);
          }
          continue;
        }
        if (pack_length == 0) {
          pack_length = pack->pack_arguments->length;
        }
        Vector* replacements = NewVector();
        for (size_t j = 0; j < pack_length &&
                           to_index < func->info.function.prototype.length;
             j++) {
          VectorAppend(replacements,
                       func->info.function.prototype.value.p[to_index++]);
        }
        MapKeyValue kv;
        kv.key.p = from_formal;
        kv.value.p = replacements;
        MapInsert(&clone.pack_symbol_map, kv);
        continue;
      }
      if (to_index >= func->info.function.prototype.length) {
        break;
      }
      Symbol* to_formal = func->info.function.prototype.value.p[to_index++];
      MapKeyValue kv;
      kv.key.p = from_formal;
      kv.value.p = to_formal;
      MapInsert(&clone.symbol_map, kv);
    }
  }
  TypeRecord* saved_function = compiler->current_function;
  Struct* saved_access_context = compiler->current_class_access_context;
  compiler->current_function = func;
  TypeSubstitutionScope substitution = TypeParserPushTemplateSubstitution(
      parser,
      clone.from_owner != NULL && clone.to_owner != NULL
          ? clone.from_owner
          : parser->template_substitution_source,
      clone.from_owner != NULL && clone.to_owner != NULL
          ? clone.to_owner
          : parser->template_substitution_target);
  if (clone.from_owner != NULL && clone.to_owner != NULL) {
    compiler->current_class_access_context = clone.to_owner;
  }
  AddOwnerMemberSymbolMappings(&clone);
  if (inserted_count > body->length) {
    inserted_count = body->length;
  }
  for (size_t i = 0; i < inserted_count; i++) {
    ASTNode* stmt = body->value.p[i];
    ASTNode* substituted =
        ASTNodeClone(stmt, CloneTemplateFunctionBodyNode, &clone, NULL);
    if (substituted != NULL) {
      ASTNodeDelete(stmt);
      stmt = substituted;
      body->value.p[i] = stmt;
    }
    ASTNodeVisit(stmt, ClearAnalyzedFlagVisitor, 0, NULL);
    RewriteTemplateBodyIdentifiers(stmt, &clone.symbol_map);
    stmt = ASTNodeVisitAndTransform(
        stmt, ReanalyzeClonedDependentFunctorCall, NULL);
    stmt = ASTNodeVisitAndTransform(
        stmt, ReanalyzeClonedResolvedCall, &clone);
    body->value.p[i] = stmt;
    ASTNodeVisit(stmt, AnalyzeFunctionTemplateCallActualsVisitor, 0, NULL);
    ASTNodeVisit(stmt, InstantiateClonedFunctionTemplateCallVisitor, 0, &clone);
    AnalyzeStatement(stmt);
    ASTNodeVisit(stmt, RefreshClonedCXXNewMetadataVisitor, 0, NULL);
    stmt = ASTNodeVisitAndTransform(
        stmt, ReanalyzeClonedConcreteMemberCall, NULL);
    body->value.p[i] = stmt;
  }
  compiler->current_function = saved_function;
  TypeParserPopTemplateSubstitution(&substitution);
  compiler->current_class_access_context = saved_access_context;
  MapDestruct(&clone.symbol_map);
  MapDestructWithContents(&clone.pack_symbol_map, DeleteMappedVector);
}

/* True if a template argument is still dependent and therefore the
 * instantiation it belongs to cannot be fully realized yet. Differs from
 * TemplateArgumentContainsTemplateParameter in that it inspects every pack
 * element regardless of kind. */
static ASTNode* ReanalyzeDeferredDependentAssignNode(
    ASTNode* node, void* data, ASTNodeTransformAction* action) {
  (void)data;
  if (node == NULL || (node->flags & kASTDeferredDependentAssign) == 0) {
    return node;
  }
  *action = kASTTransformSkipChildren;
  ASTNodeVisit(node, ClearAnalyzedFlagVisitor, 0, NULL);
  node->flags &= ~(kASTDeferredDependentAssign | kASTAnalyzed);
  ASTNodeClearType(node);
  ASTNode* parent = node->parent;
  int child_id = node->child_id;
  node->parent = NULL;
  ASTNode* analyzed = AnalyzeExpression(node);
  if (analyzed != NULL) {
    analyzed->parent = parent;
    analyzed->child_id = child_id;
  }
  return analyzed;
}

/* Walk an instantiated function body and re-analyze any deferred dependent
 * assignments (constructor mem-initializers whose right-hand side was still
 * dependent at the first instantiation stage).  Establishes the concrete
 * function as the current function so `this`/member references resolve. */
void ReanalyzeDeferredDependentAssignments(TypeParser* parser,
                                                  TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.body == NULL) {
    return;
  }
  TypeRecord* saved_function = compiler->current_function;
  Struct* saved_access_context = compiler->current_class_access_context;
  compiler->current_function = func;
  if (func->info.function.cxx_member_owner != NULL) {
    compiler->current_class_access_context =
        func->info.function.cxx_member_owner;
  }
  func->info.function.body = ASTNodeVisitAndTransform(
      func->info.function.body, ReanalyzeDeferredDependentAssignNode, parser);
  compiler->current_function = saved_function;
  compiler->current_class_access_context = saved_access_context;
}

/* Instantiate a function template `templ` with explicit/deduced arguments
 * `args`. Completes defaulted/deduced arguments, builds the concrete function
 * type and body (caching to avoid duplicate instantiations), queues it for
 * emission, and returns the instantiation symbol. Returns `templ` unchanged if
 * arguments are still dependent or the definition is unavailable. */
void CloneInstantiatedMemberFunctionBody(TypeParser* parser,
                                                Struct* owner, Symbol* symbol,
                                                Symbol* template_definition,
                                                Struct* substitution_source,
                                                Vector* args) {
  TypeSubstitutionScope substitution = TypeParserPushTemplateSubstitution(
      parser, substitution_source, owner);
  // A virtual member function is referenced from the class's vtable, not (only)
  // from direct call sites, so the lazy "define on first direct call" scheme
  // would leave it undefined and the vtable slot pointing at a missing symbol.
  // Emit its body eagerly, matching how a normal class always defines its
  // virtuals.  (A pure virtual has no body and is filtered out inside the queue
  // helper.)
  bool is_virtual = symbol != NULL && symbol->type != NULL &&
                    TypeIsFunction(symbol->type) &&
                    (symbol->type->info.function.is_virtual ||
                     symbol->type->info.function.is_pure_virtual);
  QueueTemplateMemberFunctionDefinitionImpl(symbol, template_definition, parser,
                                            args, /*allow_lazy=*/!is_virtual);
  TypeParserPopTemplateSubstitution(&substitution);
}

/* Instantiate a member function of a class template into the concrete `owner`.
 * Sets up the source->target struct substitution (so self-type references in
 * the signature/body resolve to the instantiation), builds the concrete
 * function type, names constructors/destructors after the instantiated tag,
 * and mangles the symbol.  The body clone is deferred: the information needed
 * to clone it is appended to `pending` and run after all member signatures are
 * in place (so a body may reference later-declared members).  Returns the new
 * member. */
