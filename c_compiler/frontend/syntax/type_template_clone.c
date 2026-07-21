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
static bool ASTNodeWithinUnresolvedPackExpansion(TemplateFunctionBodyClone* clone,
                                                 ASTNode* node);
static ASTNode* ReanalyzeClonedDependentFunctorCall(
    ASTNode* node, void* data, ASTNodeTransformAction* action);
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

static void ClearASTNodeType(ASTNode* node) {
  if (node != NULL && node->type != NULL) {
    TypeRecordDelete(node->type);
    node->type = NULL;
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
  if (!TypeIsStructOrUnion(target) || cast->expr == NULL ||
      TypeIsStructOrUnion(cast->expr->type)) {
    node->flags &= ~kASTDependentCast;
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
    ASTNodeSetType(result, NULL);
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
  if (receiver_type->template_origin == NULL ||
      strcmp(member_name->value.string->value,
             receiver_type->template_origin->name.value) != 0) {
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
static void RewriteTemplateBodyIdentifiers(ASTNode* node, Map* symbol_map) {
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
    return;
  }

  TypeRecord* type =
      SubstituteTemplateParameters(clone->parser, old_symbol->type, clone->args);
  RebaseTemplateParameterIndices(type, clone->rebase_template_parameter_base);
  Symbol* replacement =
      NewSymbol(old_symbol->name.value, type, old_symbol->storage);
  replacement->flags = old_symbol->flags;
  replacement->location = old_symbol->location;
  replacement->alignment = old_symbol->alignment;
  replacement->namespace_ = old_symbol->namespace_;
  replacement->value = old_symbol->value;

  MapKeyValue kv;
  kv.key.p = old_symbol;
  kv.value.p = replacement;
  MapInsert(&clone->symbol_map, kv);
  decl->symbol = replacement;
  ASTNodeSetType(node, replacement->type);
  RewriteTemplateBodyIdentifiers(decl->initializer, &clone->symbol_map);
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
      SubstituteTemplateParameters(clone->parser, old_symbol->type, clone->args);
  RebaseTemplateParameterIndices(type, clone->rebase_template_parameter_base);
  Symbol* replacement =
      NewSymbol(old_symbol->name.value, type, old_symbol->storage);
  replacement->flags = old_symbol->flags;
  replacement->location = old_symbol->location;
  replacement->alignment = old_symbol->alignment;
  replacement->namespace_ = old_symbol->namespace_;
  replacement->value = old_symbol->value;

  MapKeyValue kv;
  kv.key.p = old_symbol;
  kv.value.p = replacement;
  MapInsert(&clone->symbol_map, kv);
  return replacement;
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
    search->multiple_packs = true;
    return;
  }
  search->symbol = id->symbol;
}

/* Find the single parameter-pack symbol referenced inside a pack-expansion
 * pattern `node`. Sets `*multiple_packs` if more than one distinct pack appears
 * (which the simple expansion path cannot handle). */
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

static void InstantiateClonedFunctionTemplateCallVisitor(ASTNode* node,
                                                        void* data,
                                                        int child_id,
                                                        VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL) {
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
    if (replace->clone != NULL && id->template_arguments != NULL) {
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
    for (size_t i = 0; i < id->template_arguments->length; i++) {
      if (FindPackExpansionInTemplateArgument(
              id->template_arguments->value.p[i], clone->args, &pack_index,
              &pack_length)) {
        break;
      }
    }
    if (pack_index < 0 && id->template_arguments->length == 1 &&
        id->symbol != NULL && TypeIsFunction(id->symbol->type) &&
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
            element_args != NULL ? element_args : clone->args;
        concrete_args = SubstituteTemplateArgumentVector(
            clone->parser, id->template_arguments, substitution_args,
            clone->rebase_template_parameter_base);
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
  ASTNode* pattern_clone = ASTNodeClone(pattern, IdentityCloneNode, NULL, NULL);
  ASTNodeVisit(pattern_clone, DetachClonedPackExpansionCastTypes, 0, NULL);
  ReplacePackIdentifierData replace = {0};
  replace.clone = clone;
  replace.from = from;
  replace.to = to;
  replace.element_index = element_index;
  ASTNodeVisit(pattern_clone, ReplacePackIdentifierVisitor, 0, &replace);
  ASTNodeVisit(pattern_clone, InstantiateClonedFunctionTemplateCallVisitor, 0,
               clone);
  pattern_clone = ASTNodeVisitAndTransform(
      pattern_clone, ReanalyzeClonedDependentFunctorCall, NULL);
  return pattern_clone;
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

/* Expand pack-expansion call arguments (`f(args...)`) in a cloned call node
 * into the concrete sequence of per-element arguments. Handles three forms of
 * pack actuals: a bare pack identifier, an arbitrary pattern containing a pack,
 * and a captured-pack member access. Returns true if the argument list was
 * rewritten. */

static Struct* CloneLambdaClosureOwner(TemplateFunctionBodyClone* clone) {
  if (clone == NULL || clone->to_func == NULL ||
      !TypeIsFunction(clone->to_func)) {
    return NULL;
  }
  return clone->to_func->info.function.cxx_member_owner;
}

static bool ExpandClonedCallPackActuals(TemplateFunctionBodyClone* clone,
                                        ASTNode* node) {
  if (node->op != AST_OP(call)) {
    return false;
  }
  VectorASTNode* call = (VectorASTNode*)node;
  Vector* expanded = NewVector();
  bool changed = false;
  for (size_t i = 0; i < call->children->length; i++) {
    ASTNode* actual = call->children->value.p[i];
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
      if (multiple_packs) {
        SyntaxError(clone->parser->syntax,
                    "pack expansion with multiple parameter packs is not supported yet");
      }
      if (pack_symbol == NULL &&
          !ClonePatternReferencesUnresolvedPack(clone, actual)) {
        actual->flags &= ~kASTPackExpansion;
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

static void FindPackExpansionSubtreeVisitor(ASTNode* node, void* data,
                                            int child_id, VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL) {
    return;
  }
  bool* found = data;
  if ((node->flags & kASTPackExpansion) != 0) {
    *found = true;
  }
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
  bool found = false;
  ASTNodeVisit(node, FindPackExpansionSubtreeVisitor, 0, &found);
  return found;
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

/* In a cloned body, resolve a call whose callee is a function template to the
 * concrete instantiation deduced from the (now concrete) explicit template
 * arguments and actual arguments, updating the callee symbol and result type.
 * Skips calls whose actuals still contain pack expansions. */
static void InstantiateClonedFunctionTemplateCall(
    TemplateFunctionBodyClone* clone, ASTNode* node) {
  if (node->op != AST_OP(call)) {
    return;
  }
  VectorASTNode* call = (VectorASTNode*)node;
  if (call->left == NULL || call->left->op != AST_OP(identifier) ||
      CallActualsStillContainPackExpansion((ASTNode*)call)) {
    return;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)call->left;
  if (id->template_arguments != NULL && id->symbol != NULL &&
      id->symbol->type != NULL && TypeIsFunction(id->symbol->type) &&
      id->symbol->type->info.function.template_origin != NULL) {
    id->symbol = id->symbol->type->info.function.template_origin;
    ASTNodeSetType(call->left, id->symbol->type);
  }
  if (id->symbol == NULL || !id->symbol->flags.is_template ||
      !TypeIsFunction(id->symbol->type)) {
    return;
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
      if ((actual->flags & kASTLambdaExpression) != 0) {
        return;
      }
    }
  }
  Symbol* instantiated = NULL;
  if (id->symbol->overload_next != NULL) {
    // The callee names an *overloaded* function template.  During the first
    // (dependent) pass the identifier was bound to whichever overload the
    // lookup happened to return (the head of the set); binding the call to that
    // one and instantiating it would bypass overload resolution entirely.  Now
    // that the actuals are concrete, resolve across the whole overload set so
    // the best-matching overload wins (e.g. tag-dispatch on iterator category).
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
                        "pack expansion with multiple parameter packs is not supported yet");
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
                      "pack expansion with multiple parameter packs is not supported yet");
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

/* The identity value for an empty fold expression: `&&` folds to true, `||` to
 * false; any other operator over an empty pack is an error. */
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
  SyntaxError(parser->syntax, "Empty fold expression is not supported for this operator");
  return NewIntConstantASTNode(0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                               location);
}

typedef struct {
  TemplateFunctionBodyClone* clone;
  Symbol* symbol;
  TemplateArgument* argument;
  bool multiple_packs;
} TemplateArgumentPackSearch;

static void FindTemplateArgumentPack(ASTNode* node, void* data, int child_id,
                                     VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(identifier)) {
    return;
  }
  TemplateArgumentPackSearch* search = data;
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  if (id->symbol == NULL || !id->symbol->flags.is_parameter_pack) {
    return;
  }
  int pack_index = id->symbol->template_parameter_index;
  if (pack_index < 0) {
    TypeIsTemplateParameterPlaceholder(id->symbol->type, &pack_index);
  }
  if (pack_index < 0 ||
      (size_t)pack_index >= search->clone->args->length) {
    return;
  }
  TemplateArgument* argument =
      search->clone->args->value.p[pack_index];
  if (argument == NULL ||
      argument->kind != kTemplateParameterNonType ||
      argument->pack_arguments == NULL) {
    return;
  }
  if (search->symbol != NULL && search->symbol != id->symbol) {
    search->multiple_packs = true;
    return;
  }
  search->symbol = id->symbol;
  search->argument = argument;
}

static TemplateArgument* FindConcreteTemplateArgumentPack(
    TemplateFunctionBodyClone* clone, ASTNode* pattern, Symbol** symbol,
    bool* multiple_packs) {
  TemplateArgumentPackSearch search = {.clone = clone};
  ASTNodeVisit(pattern, FindTemplateArgumentPack, 0, &search);
  if (symbol != NULL) {
    *symbol = search.symbol;
  }
  if (multiple_packs != NULL) {
    *multiple_packs = search.multiple_packs;
  }
  return search.argument;
}

typedef struct {
  Symbol* symbol;
  TemplateArgument* argument;
} ReplaceTemplateArgumentPackData;

static ASTNode* ReplaceTemplateArgumentPackIdentifier(
    ASTNode* node, void* data, ASTNodeTransformAction* action) {
  ReplaceTemplateArgumentPackData* replace = data;
  if (node == NULL || node->op != AST_OP(identifier) ||
      ((IdentifierASTNode*)node)->symbol != replace->symbol) {
    return node;
  }
  *action = kASTTransformSkipChildren;
  ASTNode* value =
      TemplateArgumentMaterializeExpression(replace->argument, node->location);
  return value != NULL ? value : node;
}

static ASTNode* CloneTemplateArgumentPackPattern(
    TemplateFunctionBodyClone* clone, ASTNode* pattern, Symbol* pack_symbol,
    TemplateArgument* argument) {
  ASTNode* pattern_clone =
      ASTNodeClone(pattern, IdentityCloneNode, NULL, NULL);
  ReplaceTemplateArgumentPackData replace = {
      .symbol = pack_symbol,
      .argument = argument,
  };
  pattern_clone = ASTNodeVisitAndTransform(
      pattern_clone, ReplaceTemplateArgumentPackIdentifier, &replace);
  ASTNodeVisit(pattern_clone, InstantiateClonedFunctionTemplateCallVisitor, 0,
               clone);
  return ASTNodeVisitAndTransform(
      pattern_clone, ReanalyzeClonedDependentFunctorCall, NULL);
}

static ASTNode* CloneFoldPackElement(
    TemplateFunctionBodyClone* clone, ASTNode* pattern, Symbol* pack_symbol,
    Vector* replacements, size_t index, bool capture_pack,
    const char* capture_name, bool template_argument_pack) {
  if (template_argument_pack) {
    return CloneTemplateArgumentPackPattern(
        clone, pattern, pack_symbol, replacements->value.p[index]);
  }
  if (capture_pack) {
    return CloneLambdaCapturePackPattern(
        clone, pattern, capture_name, replacements->value.p[index], index);
  }
  return ClonePackExpansionPattern(
      clone, pattern, pack_symbol, replacements->value.p[index], index);
}

/* Expand a fold expression (`(... op pack)` / `(pack op ...)` / binary folds)
 * in a cloned body into a left- or right-associated chain of the operator over
 * the concrete pack elements, using NewFoldIdentity for the empty case. */
static ASTNode* ExpandClonedFoldExpression(TemplateFunctionBodyClone* clone,
                                           ASTNode* node) {
  if (node == NULL || (node->flags & kASTFoldExpression) == 0 ||
      (node->op != AST_OP(mult) && node->op != AST_OP(plus) &&
       node->op != AST_OP(minus) && node->op != AST_OP(div) &&
       node->op != AST_OP(mod) && node->op != AST_OP(lshift) &&
       node->op != AST_OP(rshift) && node->op != AST_OP(and) &&
       node->op != AST_OP(exor) && node->op != AST_OP(bitor) &&
       node->op != AST_OP(logand) && node->op != AST_OP(logor))) {
    return node;
  }

  BinaryASTNode* fold = (BinaryASTNode*)node;
  bool pack_on_left = (node->flags & kASTFoldPackOnLeft) != 0;
  ASTNode* pack_node = pack_on_left ? fold->left : fold->right;
  ASTNode* seed = pack_on_left ? fold->right : fold->left;
  bool multiple_packs = false;
  Symbol* pack_symbol =
      PackExpansionExpressionSymbol(clone, pack_node, &multiple_packs);
  if (multiple_packs) {
    SyntaxError(clone->parser->syntax,
                "pack expansion with multiple parameter packs is not supported yet");
  }
  Vector* replacements =
      pack_symbol != NULL
          ? MapFindPointerKey(&clone->pack_symbol_map, pack_symbol)
          : NULL;
  const char* capture_name = NULL;
  bool capture_pack = false;
  bool template_argument_pack = false;
  if (replacements == NULL) {
    bool multiple_template_packs = false;
    TemplateArgument* argument = FindConcreteTemplateArgumentPack(
        clone, pack_node, &pack_symbol, &multiple_template_packs);
    if (multiple_template_packs) {
      SyntaxError(clone->parser->syntax,
                  "fold expression with multiple template parameter packs is "
                  "not supported yet");
    }
    if (argument != NULL) {
      replacements = argument->pack_arguments;
      template_argument_pack = true;
    }
  }
  if (replacements == NULL) {
    replacements = LambdaCapturePackFieldReplacements(pack_node, CloneLambdaClosureOwner(clone));
    capture_name = LambdaCapturePackMemberName(pack_node);
    capture_pack = replacements != NULL && capture_name != NULL;
  }
  if (replacements == NULL) {
    return node;
  }
  if (replacements->length == 0) {
    if (capture_pack) {
      VectorDelete(replacements);
    }
    if (seed != NULL) {
      return seed;
    }
    return NewFoldIdentity(node->op, node->location, clone->parser);
  }

  if (pack_on_left) {
    ASTNode* result = seed != NULL
                          ? seed
                          : CloneFoldPackElement(
                                clone, pack_node, pack_symbol, replacements,
                                replacements->length - 1, capture_pack,
                                capture_name, template_argument_pack);
    size_t start = seed != NULL ? replacements->length
                                : replacements->length - 1;
    for (size_t i = start; i > 0; i--) {
      ASTNode* left = CloneFoldPackElement(
          clone, pack_node, pack_symbol, replacements, i - 1, capture_pack,
          capture_name, template_argument_pack);
      result = NewBinaryASTNode(node->op, NULL, node->location, left, result);
    }
    if (capture_pack) {
      VectorDelete(replacements);
    }
    return result;
  }

  ASTNode* result = seed != NULL
                        ? seed
                        : CloneFoldPackElement(
                              clone, pack_node, pack_symbol, replacements, 0,
                              capture_pack, capture_name,
                              template_argument_pack);
  size_t start = seed != NULL ? 0 : 1;
  for (size_t i = start; i < replacements->length; i++) {
    ASTNode* right = CloneFoldPackElement(
        clone, pack_node, pack_symbol, replacements, i, capture_pack,
        capture_name, template_argument_pack);
    result = NewBinaryASTNode(node->op, NULL, node->location, result, right);
  }
  if (capture_pack) {
    VectorDelete(replacements);
  }
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
                "pack expansion with multiple parameter packs is not supported yet");
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
                    "pack expansion with multiple parameter packs is not supported yet");
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
                  "pack expansion with multiple parameter packs is not supported yet");
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

ASTNode* CloneTemplateFunctionBodyNode(ASTNode* node, void* data) {
  TemplateFunctionBodyClone* clone = data;
  if (node->op == AST_OP(ptr_scale)) {
    PtrScaleASTNode* scale = (PtrScaleASTNode*)node;
    ASTNode* expr = scale->expr;
    scale->expr = NULL;
    ASTNodeDelete(node);
    return expr;
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
  if (node->op == AST_OP(structmember) && clone->from_owner != NULL &&
      clone->to_owner != NULL && clone->from_owner != clone->to_owner) {
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
        if (FindPackExpansionInTemplateArgument(member_args->value.p[i],
                                                clone->args, &pack_index,
                                                &pack_length)) {
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
  if ((node->op == AST_OP(dot) || node->op == AST_OP(arrow)) &&
      ((BinaryASTNode*)node)->right != NULL &&
      ((BinaryASTNode*)node)->right->op == AST_OP(structmember)) {
    StructMemberASTNode* member_node =
        (StructMemberASTNode*)((BinaryASTNode*)node)->right;
    if (member_node->member != NULL && member_node->member->symbol != NULL) {
      ASTNodeSetType(node, member_node->member->symbol->type);
      if (!member_node->member->is_member_function) {
        node->value_category = kValueCategoryLvalue;
      }
    }
  }
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
    ClearASTNodeType(node);
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
      TypeRecordCalculateSize(concrete);
      sizeof_node->base.value.ivalue =
          node->op == AST_OP(alignof) ? TypeRecordAlignment(concrete)
                                     : concrete->size;
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
         id->symbol->type->template_origin != NULL)) {
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
      if (concrete != NULL && TypeIsStructOrUnion(concrete) &&
          concrete->info.struct_info != NULL) {
        StructMember* member =
            FindStructMemberByName(concrete->info.struct_info,
                                   effective_member_name);
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
        if (member != NULL && member->symbol != NULL && !member->is_static &&
            clone->to_owner != NULL && clone->to_owner->tag_symbol != NULL &&
            clone->to_owner->tag_symbol->type != NULL &&
            (clone->to_owner == concrete->info.struct_info ||
             TypeIsDerivedFrom(clone->to_owner->tag_symbol->type, concrete))) {
          Symbol* this_symbol = NULL;
          if (clone->to_func != NULL && TypeIsFunction(clone->to_func) &&
              clone->to_func->info.function.prototype.length > 0) {
            Symbol* first = clone->to_func->info.function.prototype.value.p[0];
            if (first != NULL && strcmp(first->name.value, "this") == 0) {
              this_symbol = first;
            }
          }
          if (this_symbol != NULL) {
            ASTNode* left = NewIdentifierASTNode(this_symbol, node->location);
            ASTNode* right = NewStringConstantASTNode(
                NewString(member->symbol->name.value), NULL, node->location);
            ASTNode* access = NewBinaryASTNode(AST_OP(arrow), NULL,
                                               node->location, left, right);
            TypeRecordDelete(concrete);
            return access;
          }
        }
        if (member != NULL && member->symbol != NULL &&
            (member->is_static ||
             StorageIs(member->symbol->storage, STO(typedef)) ||
             member->symbol->flags.value_set)) {
          if (member->symbol->flags.value_set &&
              TypeIsIntegral(member->symbol->type)) {
            TypeRecord* value_type = TypeRecordCopy(member->symbol->type);
            TypeRecordDelete(concrete);
            return NewIntConstantASTNode(member->symbol->value.ivalue,
                                         value_type, node->location);
          }
          if (member->symbol->flags.value_set &&
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
    bool template_args_contain_pack = false;
    if (id->template_arguments != NULL) {
      int pack_index = -1;
      size_t pack_length = 0;
      for (size_t i = 0; i < id->template_arguments->length; i++) {
        if (FindPackExpansionInTemplateArgument(
                id->template_arguments->value.p[i], clone->args, &pack_index,
                &pack_length)) {
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
  if (node->op == AST_OP(cast)) {
    CastASTNode* cast = (CastASTNode*)node;
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
    if (pack_dependent_cast) {
      // Leave cast_type / node->type pack-dependent for per-element expansion.
      node_type_substituted = true;
    } else if (TypeContainsTemplateParameter(cast->cast_type) ||
               (clone->from_owner != NULL && clone->to_owner != NULL &&
                clone->from_owner != clone->to_owner &&
                TypeChainReferencesStruct(cast->cast_type,
                                          clone->from_owner))) {
      node->flags |= kASTDependentCast;
      TypeRecord* cast_type =
          SubstituteTemplateParameters(clone->parser, cast->cast_type,
                                       clone->args);
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
    TypeRecord* type =
        SubstituteTemplateParameters(clone->parser, node->type, clone->args);
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
  InstantiateClonedFunctionTemplateCall(clone, node);
  if (expanded_call_actuals && node->op == AST_OP(call)) {
    ASTNodeVisit(node, ReplaceSingleElementPackIdentifierVisitor, 0, clone);
    node->flags &= ~(kASTDependentFunctorCall | kASTAnalyzed);
    ASTNodeSetType(node, NULL);
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
  if (receiver_type->template_origin == NULL ||
      strcmp(member_name->value.string->value,
             receiver_type->template_origin->name.value) != 0) {
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
        ASTNodeTransformAction child_action = kASTTransformContinue;
        ASTNode* analyzed_actual = ReanalyzeClonedDependentFunctorCall(
            actual, data, &child_action);
        VectorSet(call->children, i, analyzed_actual);
        if (analyzed_actual != NULL) {
          analyzed_actual->parent = node;
          analyzed_actual->child_id = actual->child_id;
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
  ASTNodeSetType(node, NULL);
  if (call->left != NULL) {
    call->left->flags &= ~kASTAnalyzed;
    if ((call->left->op == AST_OP(dot) ||
         call->left->op == AST_OP(arrow)) &&
        ((BinaryASTNode*)call->left)->right != NULL &&
        ((BinaryASTNode*)call->left)->right->op == AST_OP(string)) {
      ClearASTNodeType(call->left);
    }
  }
  if (call->children != NULL) {
    for (size_t i = 0; i < call->children->length; i++) {
      ASTNode* actual = call->children->value.p[i];
      if (actual != NULL) {
        actual->flags &= ~kASTAnalyzed;
        ASTNodeSetType(actual, NULL);
      }
    }
  }
  ASTNode* parent = node->parent;
  int child_id = node->child_id;
  *action = kASTTransformSkipChildren;
  node->parent = NULL;
  ASTNode* analyzed = AnalyzeExpression(node);
  node->parent = parent;
  node->child_id = child_id;
  RestoreSymbolOverloadLinks(&overload_snapshots);
  if (parent != NULL && parent->op == AST_OP(call)) {
    parent->flags |= kASTDependentFunctorCall;
    parent->flags &= ~kASTAnalyzed;
  }
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
    if (ctor_owner != NULL && current_owner != NULL &&
        current_owner != ctor_owner &&
        !StructContainsTemplateParameter(current_owner)) {
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
  StructMember* member = FindStructMemberOverloadHead(ctor_owner, ctor_name);
  if (member == NULL || member->symbol == NULL) {
    return false;
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
  if (!if_node->is_constexpr || if_node->cond == NULL ||
      if_node->cond->op != AST_OP(number)) {
    return node;
  }
  int64_t value = ((ConstantASTNode*)if_node->cond)->value.ivalue;
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
         StructContainsTemplateParameter(concrete_type->info.struct_info))) {
      TypeRecordDelete(concrete_type);
      concrete_type =
          SubstituteTemplateParameters(clone->parser, id->symbol->type,
                                       clone->args);
      RebaseTemplateParameterIndices(concrete_type,
                                     clone->rebase_template_parameter_base);
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
      ASTNodeSetType(node, NULL);
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
    TypeRecordDelete(concrete_type);
  }
  bool is_constructor =
      id->symbol != NULL && id->symbol->type != NULL &&
      TypeIsFunction(id->symbol->type) &&
      id->symbol->type->info.function.is_constructor;
  if (!is_constructor) {
    return node;
  }
  Vector overload_snapshots;
  VectorInit(&overload_snapshots);
  RebindClonedConstructorCall(call, &overload_snapshots);
  node->flags &= ~(kASTDependentFunctorCall | kASTAnalyzed);
  ASTNodeSetType(node, NULL);
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
  clone.to_func = to;
  clone.rebase_template_parameter_base =
      from->info.function.template_parameter_base;
  clone.from_owner = CloneFunctionMemberOwner(from);
  clone.to_owner = CloneFunctionMemberOwner(to);
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
        VectorAppend(replacements,
                     to->info.function.prototype.value.p[to_index++]);
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
  Struct* saved_substitution_source = parser->template_substitution_source;
  Struct* saved_substitution_target = parser->template_substitution_target;
  Struct* saved_access_context = compiler->current_class_access_context;
  if (clone.from_owner != NULL && clone.to_owner != NULL) {
    parser->template_substitution_source = clone.from_owner;
    parser->template_substitution_target = clone.to_owner;
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
  ASTNode* body = ASTNodeClone(from->info.function.body,
                               CloneTemplateFunctionBodyNode, &clone, NULL);
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
  body = ASTNodeVisitAndTransform(body, ReanalyzeClonedResolvedCall, &clone);
  ASTNodeVisit(body, MarkClonedCastForReanalysis, 0, NULL);
  parser->template_substitution_source = saved_substitution_source;
  parser->template_substitution_target = saved_substitution_target;
  compiler->current_class_access_context = saved_access_context;
  MapDestructWithContents(&clone.pack_symbol_map, DeleteMappedVector);
  MapDestruct(&clone.symbol_map);
  return body;
}

/* True if a template instantiation with the given mangled asm name is already
 * queued for emission, so it is not instantiated/emitted twice. */
bool PendingTemplateInstantiationHasAsmName(const char* asm_name) {
  if (asm_name == NULL || *asm_name == '\0') {
    return false;
  }
  for (size_t i = 0; i < compiler->pending_template_instantiations.length; i++) {
    DeclarationListASTNode* decls =
        (DeclarationListASTNode*)compiler->pending_template_instantiations.value.p[i];
    if (decls == NULL || decls->base.op != AST_OP(decl_list) ||
        decls->declarations == NULL) {
      continue;
    }
    for (size_t j = 0; j < decls->declarations->length; j++) {
      VariableDeclarationASTNode* decl =
          (VariableDeclarationASTNode*)decls->declarations->value.p[j];
      if (decl == NULL || decl->symbol == NULL) {
        continue;
      }
      const char* queued_name = decl->symbol->asm_name.value;
      if (queued_name != NULL && strcmp(queued_name, asm_name) == 0) {
        return true;
      }
    }
  }
  return false;
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
    // template constructor (e.g. `duration`'s converting constructor) is instead
    // baked here, with the enclosing class arguments substituted, so the
    // init-list -- which may reference the enclosing class parameters -- is
    // resolved against concrete enclosing arguments (the per-call clone then
    // only substitutes the member's own parameters).
    if (symbol->type->info.function.is_constructor &&
        (symbol->type->info.function.template_parameter_count == 0 ||
         !FunctionTemplateHasOwnParameterPack(symbol->type))) {
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
  VectorAppend(&compiler->pending_template_instantiations,
               NewDeclarationListASTNode(declarations, symbol->location));
  VectorAppend(&compiler->declaration_asts, symbol->type->info.function.body);
}

void TypeEnsureTemplateMemberFunctionDefinition(Syntax* syntax, Symbol* symbol) {
  if (symbol == NULL || symbol->type == NULL ||
      !TypeIsFunction(symbol->type) ||
      symbol->flags.is_template ||
      symbol->type->info.function.cxx_member_owner == NULL ||
      symbol->type->info.function.body != NULL) {
    return;
  }
  if (symbol->type->info.function.is_implicitly_declared &&
      symbol->type->info.function.is_defaulted) {
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
  Struct* saved_source = parser.template_substitution_source;
  Struct* saved_target = parser.template_substitution_target;
  TypeRecord* source_owner =
      template_definition->type != NULL &&
              template_definition->type->info.function.cxx_member_owner != NULL &&
              template_definition->type->info.function.cxx_member_owner
                      ->tag_symbol != NULL
          ? template_definition->type->info.function.cxx_member_owner
                ->tag_symbol->type
          : NULL;
  parser.template_substitution_source =
      source_owner != NULL && TypeIsStructOrUnion(source_owner)
          ? source_owner->info.struct_info
          : NULL;
  parser.template_substitution_target =
      symbol->type->info.function.cxx_member_owner;
  QueueTemplateMemberFunctionDefinitionImpl(
      symbol, template_definition, &parser, template_arguments,
      /*allow_lazy=*/false);
  parser.template_substitution_source = saved_source;
  parser.template_substitution_target = saved_target;
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
  Struct* saved_substitution_source = parser->template_substitution_source;
  Struct* saved_substitution_target = parser->template_substitution_target;
  Struct* saved_access_context = compiler->current_class_access_context;
  compiler->current_function = func;
  if (clone.from_owner != NULL && clone.to_owner != NULL) {
    parser->template_substitution_source = clone.from_owner;
    parser->template_substitution_target = clone.to_owner;
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
    ASTNodeVisit(stmt, AnalyzeFunctionTemplateCallActualsVisitor, 0, NULL);
    ASTNodeVisit(stmt, InstantiateClonedFunctionTemplateCallVisitor, 0, &clone);
    AnalyzeStatement(stmt);
  }
  compiler->current_function = saved_function;
  parser->template_substitution_source = saved_substitution_source;
  parser->template_substitution_target = saved_substitution_target;
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
  ASTNodeSetType(node, NULL);
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
  Struct* saved_substitution_source = parser->template_substitution_source;
  Struct* saved_substitution_target = parser->template_substitution_target;
  parser->template_substitution_source = substitution_source;
  parser->template_substitution_target = owner;
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
  parser->template_substitution_source = saved_substitution_source;
  parser->template_substitution_target = saved_substitution_target;
}

/* Instantiate a member function of a class template into the concrete `owner`.
 * Sets up the source->target struct substitution (so self-type references in
 * the signature/body resolve to the instantiation), builds the concrete
 * function type, names constructors/destructors after the instantiated tag,
 * and mangles the symbol.  The body clone is deferred: the information needed
 * to clone it is appended to `pending` and run after all member signatures are
 * in place (so a body may reference later-declared members).  Returns the new
 * member. */
