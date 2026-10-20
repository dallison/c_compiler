//
//  syntax_ctor_init.c
//  c_compiler
//
//  C++ constructor mem-initializer lists: parsing, ordering, base and member
//  lookup, default member initializers and template formal remapping.
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

static CXXBaseSpecifier* FindCXXDirectBaseByNameSkipping(
    Struct* owner, const char* name, Vector* already_used,
    size_t pack_element_index);

// Initializes all vectors in `init_list` and clears its delegating statement.
void SyntaxCXXConstructorInitListInit(CXXConstructorInitList* init_list) {
  VectorInit(&init_list->virtual_base_specs);
  VectorInit(&init_list->virtual_base_statements);
  VectorInit(&init_list->base_specs);
  VectorInit(&init_list->base_statements);
  VectorInit(&init_list->member_specs);
  VectorInit(&init_list->member_statements);
  init_list->delegating_statement = NULL;
  VectorInit(&init_list->raw_initializers);
  VectorInit(&init_list->deferred_initializers);
  init_list->last_initializer_order = -1;
}

// Frees every vector and AST node owned by `init_list`.
void SyntaxCXXConstructorInitListDestruct(CXXConstructorInitList* init_list) {
  VectorDestruct(&init_list->virtual_base_specs);
  VectorDestruct(&init_list->virtual_base_statements);
  VectorDestruct(&init_list->base_specs);
  VectorDestruct(&init_list->base_statements);
  VectorDestruct(&init_list->member_specs);
  VectorDestruct(&init_list->member_statements);
  ASTNodeDelete(init_list->delegating_statement);
  VectorDestructWithContents(
      &init_list->raw_initializers,
      (VectorElementDestructor)CXXDeferredConstructorInitializerDelete,
      /*free_element=*/false);
  VectorDestructWithContents(
      &init_list->deferred_initializers,
      (VectorElementDestructor)CXXDeferredConstructorInitializerDelete,
      /*free_element=*/false);
}

// Allocates a fresh init list whose deferred initializers are clones of
// those in `initializers`.  Returns an empty list when `initializers` is
// NULL.  Caller owns the returned list.
static CXXConstructorInitList* SyntaxCXXConstructorInitListCloneVector(
    Vector* initializers) {
  CXXConstructorInitList* clone = malloc(sizeof(CXXConstructorInitList));
  SyntaxCXXConstructorInitListInit(clone);
  if (initializers == NULL) {
    return clone;
  }
  for (size_t i = 0; i < initializers->length; i++) {
    CXXDeferredConstructorInitializer* init = initializers->value.p[i];
    CXXDeferredConstructorInitializer* cloned =
        CloneCXXDeferredConstructorInitializer(init);
    if (cloned != NULL) {
      VectorAppend(&clone->deferred_initializers, cloned);
    }
  }
  return clone;
}

// Returns a copy of `init_list`'s unbound raw mem-initializers, or NULL when
// `init_list` is NULL.  Caller owns the returned list.
CXXConstructorInitList* SyntaxCXXConstructorInitListCloneRaw(
    CXXConstructorInitList* init_list) {
  if (init_list == NULL) {
    return NULL;
  }
  return SyntaxCXXConstructorInitListCloneVector(&init_list->raw_initializers);
}

// Returns a copy of `init_list`'s deferred mem-initializers, or NULL when
// `init_list` is NULL.  Caller owns the returned list.
CXXConstructorInitList* SyntaxCXXConstructorInitListCloneDeferred(
    CXXConstructorInitList* init_list) {
  if (init_list == NULL) {
    return NULL;
  }
  return SyntaxCXXConstructorInitListCloneVector(
      &init_list->deferred_initializers);
}

typedef struct {
  TypeRecord* from_func;
  TypeRecord* to_func;
  int rebase_base;
  // Class-template arguments that correspond to indices below `rebase_base`.
  // Borrowed.  NULL keeps the rebase-only behavior.
  Vector* enclosing_args;
} ConstructorInitFormalRemap;

/* Visitor: within a cloned member-initializer actual, (1) rewrite an identifier
 * that names one of `from_func`'s parameters to the parameter at the same
 * position in `to_func`, and (2) bind explicit template arguments that name
 * the enclosing class (`make_unique<S>` inside `Box<S>::Box<It>`) to
 * `enclosing_args`, then renumber the member template's own parameters down
 * by `rebase_base`.  Rebase alone leaves the class parameter at index 0, which
 * is the same slot the member's first parameter occupies after renumbering, so
 * the later per-call clone would substitute the class parameter with the
 * member argument. */
// A fresh copy of `type` with the enclosing class arguments bound and the
// member template's own parameters renumbered from 0.  A type that also names
// the member's own parameters is only renumbered: the class arguments alone
// cannot substitute it.
static TypeRecord* RemapEnclosingTemplateType(ConstructorInitFormalRemap* remap,
                                              TypeRecord* type) {
  TypeRecord* result =
      remap->enclosing_args != NULL && remap->enclosing_args->length > 0 &&
              !TypeNamesTemplateParameterAtOrAbove(type, remap->rebase_base)
          ? TypeSubstituteTemplateType(&compiler->syntax, type,
                                       remap->enclosing_args)
          : TypeRecordCopy(type);
  if (result != NULL) {
    RebaseTemplateParameterIndices(result, remap->rebase_base);
  }
  return result;
}

// Pre-order visitor: rewrites parameter identifiers and template arguments
// in a cloned mem-initializer actual per `ConstructorInitFormalRemap`.
static void RemapConstructorInitFormalVisitor(ASTNode* node, void* data,
                                              int child_id, VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL) {
    return;
  }
  ConstructorInitFormalRemap* remap = data;
  if (node->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node;
    Vector* from = &remap->from_func->info.function.prototype;
    Vector* to = &remap->to_func->info.function.prototype;
    for (size_t i = 0; i < from->length && i < to->length; i++) {
      if (from->value.p[i] == id->symbol) {
        Symbol* replacement = to->value.p[i];
        id->symbol = replacement;
        if (replacement != NULL) {
          ASTNodeSetType(node, replacement->type);
        }
        break;
      }
    }
    if (remap->enclosing_args != NULL && remap->enclosing_args->length > 0 &&
        remap->rebase_base > 0 && id->template_arguments != NULL) {
      Vector* concrete = TypeSubstituteTemplateArgumentVectorAndRebase(
          &compiler->syntax, id->template_arguments, remap->enclosing_args,
          remap->rebase_base);
      if (concrete != NULL) {
        VectorDeleteWithContents(
            id->template_arguments,
            (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
        id->template_arguments = concrete;
      }
    } else if (remap->rebase_base > 0 && id->template_arguments != NULL) {
      for (size_t i = 0; i < id->template_arguments->length; i++) {
        RebaseTemplateArgumentParameterIndices(
            id->template_arguments->value.p[i], remap->rebase_base);
      }
    }
    // `Period::den` names its scope by the class parameter's absolute index.
    // Once rebased, index 0 is the member template's first parameter.
    if ((node->flags & kASTDependentQualifiedName) != 0 &&
        remap->rebase_base > 0 && id->symbol != NULL &&
        id->symbol->type != NULL &&
        TypeDependentOf(id->symbol->type)->dependent_member_name != NULL &&
        TypeContainsTemplateParameter(id->symbol->type)) {
      TypeRecord* scope = TypeRecordCopy(id->symbol->type);
      StringDelete(TypeDependentOf(scope)->dependent_member_name);
      TypeMutableDependent(scope)->dependent_member_name = NULL;
      TypeRecord* concrete = RemapEnclosingTemplateType(remap, scope);
      TypeRecordDelete(scope);
      if (concrete != NULL) {
        TypeMutableDependent(concrete)->dependent_member_name = NewString(
            TypeDependentOf(id->symbol->type)->dependent_member_name->value);
        Symbol* copy =
            NewSymbol(id->symbol->name.value, concrete, id->symbol->storage);
        copy->flags = id->symbol->flags;
        id->symbol = copy;
        ASTNodeClearType(node);
      }
    } else if (remap->rebase_base > 0 && id->symbol != NULL &&
               StorageIs(id->symbol->storage, STO(typedef)) &&
               !id->symbol->flags.is_template &&
               id->template_arguments == NULL && id->symbol->type != NULL &&
               TypeContainsTemplateParameter(id->symbol->type)) {
      // A class parameter used as a value, as in `Allocator()`.  An alias
      // template-id (`index_sequence_for<Args...>{}`) keeps its arguments on
      // the identifier; its symbol's type is the alias pattern.
      TypeRecord* concrete = RemapEnclosingTemplateType(remap, id->symbol->type);
      if (concrete != NULL) {
        Symbol* copy =
            NewSymbol(id->symbol->name.value, concrete, id->symbol->storage);
        copy->flags = id->symbol->flags;
        id->symbol = copy;
        ASTNodeClearType(node);
      }
    } else if (remap->rebase_base > 0 && id->symbol != NULL &&
               id->symbol->flags.is_template_parameter &&
               !id->symbol->flags.is_template_type_parameter &&
               (id->symbol->template_parameter_index >= remap->rebase_base ||
                SymbolDependentValueTemplateParameterIndex(id->symbol) >=
                    remap->rebase_base)) {
      // The member template's own non-type parameter used as a value, as in
      // `s(N)` for `template <class U, size_t N> S(U (&)[N])`.  Copy the
      // symbol so the primary's init-list keeps its absolute numbering.
      Symbol* old = id->symbol;
      TypeRecord* type = old->type != NULL ? TypeRecordCopy(old->type) : NULL;
      RebaseTemplateParameterIndices(type, remap->rebase_base);
      Symbol* copy = NewSymbol(old->name.value, type, old->storage);
      copy->namespace_ = old->namespace_;
      copy->flags = old->flags;
      copy->alignment = old->alignment;
      copy->template_parameter_index = old->template_parameter_index;
      if (copy->template_parameter_index >= remap->rebase_base) {
        copy->template_parameter_index -= remap->rebase_base;
      }
      SymbolSetDependentValueTemplateParameterIndex(
          copy, SymbolDependentValueTemplateParameterIndex(old));
      if (SymbolDependentValueTemplateParameterIndex(copy) >=
          remap->rebase_base) {
        SymbolSetDependentValueTemplateParameterIndex(
            copy, SymbolDependentValueTemplateParameterIndex(copy) -
                      remap->rebase_base);
      }
      copy->location = old->location;
      copy->value = old->value;
      id->symbol = copy;
      ASTNodeSetType(node, copy->type);
    }
  } else if (node->op == AST_OP(cast) && remap->rebase_base > 0 &&
             ((CastASTNode*)node)->cast_type != NULL &&
             TypeContainsTemplateParameter(((CastASTNode*)node)->cast_type)) {
    // The clone shares `cast_type` with the primary's init-list; renumbering
    // it in place would renumber the primary for every later instantiation.
    CastASTNode* cast = (CastASTNode*)node;
    TypeRecord* remapped = RemapEnclosingTemplateType(remap, cast->cast_type);
    if (remapped != NULL) {
      TypeRecordIncRef(remapped);
      TypeRecordDelete(cast->cast_type);
      cast->cast_type = remapped;
    }
  }
}

// The enclosing class's non-type parameter used as a value, as in `e(E)`.
// Must run before the rebase: once renumbered, the member's own parameters
// occupy the enclosing parameters' indices.
static ASTNode* SubstituteEnclosingNonTypeParameter(
    ASTNode* node, void* data, ASTNodeTransformAction* action) {
  ConstructorInitFormalRemap* remap = data;
  if (node->op != AST_OP(identifier) || remap->enclosing_args == NULL) {
    return node;
  }
  Symbol* symbol = ((IdentifierASTNode*)node)->symbol;
  if (symbol == NULL || !symbol->flags.is_template_parameter ||
      symbol->flags.is_template_type_parameter ||
      symbol->template_parameter_index < 0 ||
      symbol->template_parameter_index >= remap->rebase_base ||
      (size_t)symbol->template_parameter_index >=
          remap->enclosing_args->length) {
    return node;
  }
  TemplateArgument* arg =
      remap->enclosing_args->value.p[symbol->template_parameter_index];
  if (arg == NULL || arg->kind != kTemplateParameterNonType ||
      arg->pack_arguments != NULL || arg->template_parameter_index >= 0) {
    return node;
  }
  ASTNode* value = TemplateArgumentMaterializeExpression(arg, node->location);
  if (value == NULL) {
    return node;
  }
  *action = kASTTransformSkipChildren;
  return value;
}

/* Rewrite every reference to one of `from_func`'s parameters inside the deferred
 * member-initializer actuals of `init_list` to the correspondingly-positioned
 * parameter of `to_func`.  `enclosing_args` (indices below `rebase_base`) are
 * substituted into explicit template arguments, then the member template's own
 * parameters are rebased by `rebase_base`.  Used when a class-template
 * instantiation clones a member function template constructor: the deferred
 * init-list is shared from the primary and still names the primary's parameters
 * and numbers the member's own template parameters relative to the enclosing
 * class, but the per-call preamble insertion keys its clone maps off the cloned
 * (class-level) prototype (whose parameters are fresh and whose
 * template_parameter_base is 0).  Aligning both lets pack initializers such as
 * `value(std::forward<Args>(args)...)` expand against the concrete arguments
 * instead of dropping `forward`'s explicit template argument, and lets
 * `make_unique<S>` keep the class argument rather than the member's first
 * parameter. */
void SyntaxCXXConstructorInitListRemapFormals(CXXConstructorInitList* init_list,
                                              TypeRecord* from_func,
                                              TypeRecord* to_func,
                                              int rebase_base,
                                              Vector* enclosing_args) {
  if (init_list == NULL || from_func == NULL || to_func == NULL ||
      !TypeIsFunction(from_func) || !TypeIsFunction(to_func)) {
    return;
  }
  ConstructorInitFormalRemap remap = {from_func, to_func, rebase_base,
                                      enclosing_args};
  for (size_t i = 0; i < init_list->deferred_initializers.length; i++) {
    CXXDeferredConstructorInitializer* init =
        init_list->deferred_initializers.value.p[i];
    if (init == NULL || init->actuals == NULL) {
      continue;
    }
    for (size_t j = 0; j < init->actuals->length; j++) {
      ASTNode* actual = init->actuals->value.p[j];
      if (rebase_base > 0) {
        ASTNode* substituted = ASTNodeVisitAndTransform(
            actual, SubstituteEnclosingNonTypeParameter, &remap);
        if (substituted != actual) {
          ASTNodeDelete(actual);
          actual = substituted;
          init->actuals->value.p[j] = actual;
        }
      }
      ASTNodeVisit(actual, RemapConstructorInitFormalVisitor, 0, &remap);
    }
  }
}

// Inserts `value` before `index` in `vec`, or appends when `index` is past
// the end.
void VectorInsertOrAppend(Vector* vec, size_t index, void* value) {
  if (index >= vec->length) {
    VectorAppend(vec, value);
  } else {
    VectorInsertBefore(vec, index, value);
  }
}

// Returns true when `vec` holds a pointer equal to `value`.
bool VectorContainsPointer(Vector* vec, void* value) {
  for (size_t i = 0; i < vec->length; i++) {
    if (vec->value.p[i] == value) {
      return true;
    }
  }
  return false;
}

// Returns the mem-initializer declaration-order index of direct base `base`
// within `owner`, or -1 when `base` is not a direct base of `owner`.
static int CXXDirectBaseOrder(Struct* owner, CXXBaseSpecifier* base) {
  if (owner == NULL || base == NULL) {
    return -1;
  }
  for (size_t i = 0; i < owner->bases.length; i++) {
    if (owner->bases.value.p[i] == base) {
      return (int)owner->virtual_bases.length + (int)i;
    }
  }
  return -1;
}

// Returns the mem-initializer declaration-order index of virtual base `base`
// within `owner`, or -1 when `base` is not a virtual base of `owner`.
static int CXXVirtualBaseOrder(Struct* owner, CXXVirtualBaseInfo* base) {
  if (owner == NULL || base == NULL) {
    return -1;
  }
  for (size_t i = 0; i < owner->virtual_bases.length; i++) {
    if (owner->virtual_bases.value.p[i] == base) {
      return (int)i;
    }
  }
  return -1;
}

/* True if `member` is one of the members an anonymous struct or union member
 * `anon` injects into its enclosing class. */
static bool CXXAnonymousAggregateDeclaresMember(StructMember* anon,
                                                StructMember* member) {
  return anon != NULL && anon != member && anon->is_anon &&
         anon->symbol != NULL && anon->symbol->type != NULL &&
         TypeIsStructOrUnion(anon->symbol->type) && member != NULL &&
         member->symbol != NULL &&
         FindStructMember(anon->symbol->type->info.struct_info,
                          &member->symbol->name) != NULL;
}

// Returns the mem-initializer declaration-order index of data member
// `member` in `owner`, treating members injected by anonymous aggregates as
// occupying their aggregate's slot, or -1 when `member` is invalid.
static int CXXDirectMemberOrder(Struct* owner, StructMember* member) {
  if (owner == NULL || member == NULL) {
    return -1;
  }
  // A member injected by an anonymous aggregate has that aggregate's position;
  // its own index counts within the aggregate.
  size_t index = member->index;
  for (size_t i = 0; i < owner->members.length; i++) {
    if (CXXAnonymousAggregateDeclaresMember(owner->members.value.p[i],
                                            member)) {
      index = i;
      break;
    }
  }
  return (int)owner->virtual_bases.length + (int)owner->bases.length +
         (int)index;
}

// Warns when mem-initializer `name` at `order` appears before an earlier
// initializer in declaration order, then updates `init_list`'s last order.
static void CheckCXXConstructorInitializerOrder(
    Syntax* syntax, CXXConstructorInitList* init_list, const char* name,
    int order) {
  if (order < 0) {
    return;
  }
  if (init_list->last_initializer_order > order) {
    SyntaxWarning(syntax, "reorder-ctor-init",
                  "constructor initializer for %s does not match declaration order",
                  name);
  }
  init_list->last_initializer_order = order;
}

/* The primary-template name of a class type, or NULL if it is not a template
 * instantiation.  A template-id base is named in a mem-initializer by its bare
 * template name (`Base<T>()` records the base as "Base"), whereas the
 * instantiated base's tag_name is the specialized form ("Base<int>"), so base
 * matching against a mem-initializer name must also consider it. */
static String* CXXPrimaryTemplateName(TypeRecord* type) {
  if (type == NULL || !TypeIsStructOrUnion(type)) {
    return NULL;
  }
  Symbol* origin = type->template_origin;
  if (origin == NULL && type->info.struct_info != NULL &&
      type->info.struct_info->tag_symbol != NULL &&
      type->info.struct_info->tag_symbol->type != NULL) {
    origin = type->info.struct_info->tag_symbol->type->template_origin;
  }
  return origin != NULL ? &origin->name : NULL;
}

/* A mem-initializer may name a direct base through a typedef that is not a
 * member of the class (`using HashSetIteratorGenerationInfo = ...Disabled;`
 * then `iterator() : HashSetIteratorGenerationInfo(...)`).  The typedef is
 * visible from the class, its enclosing classes, or their namespaces. */
static TypeRecord* CXXTypedefTypeVisibleFromClass(Struct* owner,
                                                  const char* name) {
  if (owner == NULL || name == NULL) {
    return NULL;
  }
  String lookup;
  StringInit(&lookup, name);
  TypeRecord* found = NULL;
  for (Struct* current = owner; current != NULL && found == NULL;
       current = current->lexical_parent) {
    StructMember* member = FindStructMemberByName(current, name);
    if (member != NULL && member->symbol != NULL &&
        StorageIs(member->symbol->storage, STO(typedef)) &&
        member->symbol->type != NULL) {
      found = member->symbol->type;
      break;
    }
    Symbol* tag = current->tag_symbol;
    Namespace* ns = tag != NULL ? tag->namespace_ : NULL;
    while (ns != NULL && found == NULL) {
      Symbol* symbol = NamespaceFindSymbol(ns, &lookup);
      if (symbol != NULL && StorageIs(symbol->storage, STO(typedef)) &&
          symbol->type != NULL) {
        found = symbol->type;
        break;
      }
      ns = ns->parent;
    }
  }
  StringDestruct(&lookup);
  return found;
}

// Returns the direct base of `owner` whose name matches `name`, or NULL when
// none match.
static CXXBaseSpecifier* FindCXXDirectBaseByName(Struct* owner,
                                                 const char* name) {
  return FindCXXDirectBaseByNameSkipping(owner, name, NULL, 0);
}

/* A mem-initializer may name a direct base by the class template's parameter
 * (`struct Storage<T, I, Tag, true> : T { Storage(...) : T(v) {} }`).  After
 * instantiation the base's tag is the argument type (`Hash<unsigned long>`),
 * not the parameter name `T`. */
static TypeRecord* CXXTemplateParameterBaseType(Struct* owner, const char* name,
                                                size_t pack_element_index) {
  if (owner == NULL || name == NULL || owner->tag_symbol == NULL ||
      owner->tag_symbol->type == NULL) {
    return NULL;
  }
  TypeRecord* owner_type = owner->tag_symbol->type;
  Symbol* origin = owner_type->template_origin;
  Vector* args = owner_type->template_arguments;
  if (origin == NULL || origin->type == NULL ||
      !TypeIsStructOrUnion(origin->type) ||
      origin->type->info.struct_info == NULL || args == NULL) {
    return NULL;
  }
  Vector* parameters = &origin->type->info.struct_info->template_parameters;
  for (size_t i = 0; i < parameters->length; i++) {
    TemplateParameter* param = parameters->value.p[i];
    if (param == NULL || param->kind != kTemplateParameterType ||
        param->name.value == NULL || strcmp(param->name.value, name) != 0) {
      continue;
    }
    if (param->index < 0 || (size_t)param->index >= args->length) {
      return NULL;
    }
    TemplateArgument* arg = args->value.p[param->index];
    if (arg == NULL || arg->kind != kTemplateParameterType) {
      return NULL;
    }
    if (param->is_parameter_pack) {
      if (arg->pack_arguments == NULL ||
          pack_element_index >= arg->pack_arguments->length) {
        return NULL;
      }
      TemplateArgument* element =
          arg->pack_arguments->value.p[pack_element_index];
      return element != NULL ? element->type : NULL;
    }
    if (pack_element_index != 0) {
      return NULL;
    }
    return arg->type;
  }
  return NULL;
}

// Like FindCXXDirectBaseByName but skips bases listed in `already_used` and
// resolves template parameter names using `pack_element_index` for pack bases.
// Returns NULL when no unused matching direct base remains.
static CXXBaseSpecifier* FindCXXDirectBaseByNameSkipping(
    Struct* owner, const char* name, Vector* already_used,
    size_t pack_element_index) {
  if (owner == NULL || name == NULL) {
    return NULL;
  }
  TypeRecord* alias_type = CXXTypedefTypeVisibleFromClass(owner, name);
  TypeRecord* parameter_type =
      CXXTemplateParameterBaseType(owner, name, pack_element_index);
  for (size_t i = 0; i < owner->bases.length; i++) {
    CXXBaseSpecifier* base = owner->bases.value.p[i];
    if (base->type == NULL || !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    if (already_used != NULL && VectorContainsPointer(already_used, base)) {
      continue;
    }
    if (alias_type != NULL && TypeEqual(alias_type, base->type)) {
      return base;
    }
    if (parameter_type != NULL && TypeEqual(parameter_type, base->type)) {
      return base;
    }
    if (base->type->info.struct_info->tag_name != NULL &&
        StringEqual(base->type->info.struct_info->tag_name, name)) {
      return base;
    }
    String* template_name = CXXPrimaryTemplateName(base->type);
    if (template_name != NULL && StringEqual(template_name, name)) {
      return base;
    }
  }
  return NULL;
}

// Returns the virtual base of `owner` whose name matches `name` (including
// typedef and template-parameter aliases), or NULL when none match.
static CXXVirtualBaseInfo* FindCXXVirtualBaseByName(Struct* owner,
                                                    const char* name) {
  if (owner == NULL || name == NULL) {
    return NULL;
  }
  TypeRecord* alias_type = CXXTypedefTypeVisibleFromClass(owner, name);
  TypeRecord* parameter_type = CXXTemplateParameterBaseType(owner, name, 0);
  for (size_t i = 0; i < owner->virtual_bases.length; i++) {
    CXXVirtualBaseInfo* base = owner->virtual_bases.value.p[i];
    if (base->type == NULL || !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    if (alias_type != NULL && TypeEqual(alias_type, base->type)) {
      return base;
    }
    if (parameter_type != NULL && TypeEqual(parameter_type, base->type)) {
      return base;
    }
    if (base->type->info.struct_info->tag_name != NULL &&
        StringEqual(base->type->info.struct_info->tag_name, name)) {
      return base;
    }
    String* template_name = CXXPrimaryTemplateName(base->type);
    if (template_name != NULL && StringEqual(template_name, name)) {
      return base;
    }
  }
  return NULL;
}

// Returns true when mem-initializer name `name` names class `owner` itself
// (tag, primary template name, or a typedef of the class type) rather than a
// base or member.
static bool CXXConstructorInitializerNamesOwner(Struct* owner,
                                                const char* name) {
  if (owner == NULL || name == NULL) {
    return false;
  }
  if (owner->tag_name != NULL && StringEqual(owner->tag_name, name)) {
    return true;
  }
  if (owner->tag_symbol != NULL && owner->tag_symbol->type != NULL) {
    String* template_name = CXXPrimaryTemplateName(owner->tag_symbol->type);
    if (template_name != NULL && StringEqual(template_name, name)) {
      return true;
    }
  }
  StructMember* alias = FindStructMemberByName(owner, name);
  if (alias == NULL || alias->symbol == NULL ||
      !StorageIs(alias->symbol->storage, STO(typedef)) ||
      alias->symbol->type == NULL || owner->tag_symbol == NULL ||
      owner->tag_symbol->type == NULL ||
      !TypeEqual(alias->symbol->type, owner->tag_symbol->type)) {
    return false;
  }
  // `using Base = DoAllAction<Other...>` inside `DoAllAction<Initial, Other...>`
  // still shares the injected-class-name Struct, so TypeEqual against this
  // class succeeds.  The same typedef matches the direct base by template
  // arguments; that initializer is a base initializer, not a delegating one.
  if (FindCXXDirectBaseByName(owner, name) != NULL ||
      FindCXXVirtualBaseByName(owner, name) != NULL) {
    return false;
  }
  return true;
}

// Builds an expression statement that delegates from constructor `func` to
// another constructor of the same class with `actuals`.  Consumes `actuals`
// on success; deletes them and returns NULL on failure.
static ASTNode* NewCXXDelegatingConstructorCall(
    Syntax* syntax, TypeRecord* func, Vector* actuals,
    SourceLocation location) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.cxx_member_owner == NULL) {
    VectorDelete(actuals);
    return NULL;
  }
  Struct* owner = func->info.function.cxx_member_owner;
  // The constructor may be instantiated while another member function is the
  // current scope (`Time::In` constructing a `civil_time`).  The `this` in
  // that scope is the other class.  The delegating call has to use this
  // constructor's own object parameter.
  Symbol* this_symbol = NULL;
  if (func->info.function.prototype.length > 0) {
    Symbol* param = func->info.function.prototype.value.p[0];
    if (param != NULL && StringEqual(&param->name, "this")) {
      this_symbol = param;
    }
  }
  if (this_symbol == NULL) {
    this_symbol = FindThisSymbol(syntax);
  }
  if (this_symbol == NULL || owner->tag_name == NULL) {
    VectorDelete(actuals);
    return NULL;
  }
  if (StructHasVirtualBases(owner) &&
      func->info.function.prototype.length > 1) {
    Symbol* complete_object = func->info.function.prototype.value.p[1];
    ASTNode* argument = NewIdentifierASTNode(complete_object, location);
    if (actuals->length == 0) {
      VectorAppend(actuals, argument);
    } else {
      VectorInsertBefore(actuals, 0, argument);
    }
  }
  ASTNode* member = NewStringConstantASTNode(
      NewString(owner->tag_name->value), NULL, location);
  ASTNode* access = NewBinaryASTNode(
      AST_OP(arrow), NULL, location,
      NewIdentifierASTNode(this_symbol, location), member);
  ASTNode* call =
      NewVectorASTNode(AST_OP(call), NULL, location, access, actuals);
  return NewExpressionStatementASTNode(call, location);
}

// Returns the non-static, non-function data member of `owner` named `name`,
// including members injected by anonymous aggregates, or NULL when the name
// is not a direct data member.
static StructMember* FindCXXDirectDataMemberByName(Struct* owner,
                                                   const char* name) {
  if (owner == NULL || name == NULL) {
    return NULL;
  }
  // Anonymous union/struct members are injected into the lookup tables, not
  // the layout `members` vector.  Mem-inits such as `Rep() : data{...}` and
  // `Rep(p) : as_tree(p)` name those injected members.
  String name_str;
  StringInit(&name_str, name);
  StructMember* member = FindStructMember(owner, &name_str);
  StringDestruct(&name_str);
  if (member != NULL && !member->is_static && !member->is_member_function &&
      !StorageIs(member->symbol->storage, STO(typedef))) {
    return member;
  }
  return NULL;
}

// Returns the implicit `this` parameter symbol of function type `func`, or
// NULL when `func` is not a function or has no parameters.
Symbol* CXXThisSymbolFromFunction(TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.prototype.length == 0) {
    return NULL;
  }
  return func->info.function.prototype.value.p[0];
}

// Returns a `this->member_name` access expression for constructor `func`, or
// NULL when `func` has no `this` parameter.
ASTNode* NewCXXThisMemberAccess(TypeRecord* func,
                                const char* member_name,
                                SourceLocation location) {
  Symbol* this_symbol = CXXThisSymbolFromFunction(func);
  if (this_symbol == NULL) {
    return NULL;
  }
  return NewBinaryASTNode(
      AST_OP(arrow), NULL, location,
      NewIdentifierASTNode(this_symbol, location),
      NewStringConstantASTNode(NewString(member_name), NULL, location));
}

// Returns the source object parameter of copy/move constructor `func` (the
// last parameter), or NULL when `func` is not a suitable member function.
Symbol* CXXSourceObjectParameter(TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.prototype.length < 2) {
    return NULL;
  }
  return func->info.function.prototype.value.p[func->info.function.prototype.length - 1];
}

// Returns a `source.member_name` access expression, or NULL when `source` is
// NULL.
ASTNode* NewCXXSourceMemberAccess(Symbol* source,
                                  const char* member_name,
                                  SourceLocation location) {
  if (source == NULL) {
    return NULL;
  }
  return NewBinaryASTNode(
      AST_OP(dot), NULL, location, NewIdentifierASTNode(source, location),
      NewStringConstantASTNode(NewString(member_name), NULL, location));
}

// A default member initializer is parsed before any constructor exists, so its
// implicit member accesses go through an invented `this`
// (NewMemberAccessFromEnclosingClass).  Bind them to the `this` parameter of
// the constructor the initializer is being inserted into.
static ASTNode* BindDefaultMemberInitializerThis(ASTNode* node, void* data) {
  Symbol* this_formal = data;
  if (node->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node;
    if (this_formal != NULL && id->symbol != NULL &&
        id->symbol != this_formal &&
        id->symbol->flags.invented && !id->symbol->flags.is_argument &&
        id->symbol->storage == STO(implicit) &&
        StringEqual(&id->symbol->name, "this")) {
      id->symbol = this_formal;
    }
  }
  return node;
}

// Clones default-member-initializer AST `node`, binding invented `this`
// identifiers to the `this` parameter of constructor `func`.
static ASTNode* CloneDefaultMemberInitializerFor(TypeRecord* func,
                                                 ASTNode* node) {
  return ASTNodeClone(node, BindDefaultMemberInitializerThis,
                      CXXThisSymbolFromFunction(func), NULL);
}

// Builds the constructor-argument vector for mem-initializing from default
// member initializer `initializer`, cloning expressions for `func`.  Returns
// NULL when `initializer` is NULL; caller owns the returned vector.
static Vector* CXXDefaultMemberInitializerActuals(TypeRecord* func,
                                                  ASTNode* initializer) {
  if (initializer == NULL) {
    return NULL;
  }
  Vector* actuals = NewVector();
  if (initializer->op == AST_OP(expr_init)) {
    ExpressionInitializerASTNode* expr_init =
        (ExpressionInitializerASTNode*)initializer;
    VectorAppend(actuals,
                 CloneDefaultMemberInitializerFor(func, expr_init->expr));
    return actuals;
  }
  if (initializer->op == AST_OP(braced_init)) {
    BracedInitializerASTNode* braced = (BracedInitializerASTNode*)initializer;
    for (size_t i = 0; i < braced->initializers->length; i++) {
      ASTNode* init = braced->initializers->value.p[i];
      if (init != NULL && init->op == AST_OP(expr_init)) {
        ExpressionInitializerASTNode* expr_init =
            (ExpressionInitializerASTNode*)init;
        VectorAppend(actuals,
                     CloneDefaultMemberInitializerFor(func, expr_init->expr));
      } else {
        VectorAppend(actuals, CloneDefaultMemberInitializerFor(func, init));
      }
    }
    return actuals;
  }
  VectorAppend(actuals, CloneDefaultMemberInitializerFor(func, initializer));
  return actuals;
}

// True if constructor `func` default-initializing `member`, of class `type`,
// is ill-formed because the class declares constructors but none is a usable
// default constructor.  The call is then still built so that overload
// resolution diagnoses it.
static bool CXXMemberLacksUsableDefaultConstructor(TypeRecord* func,
                                                   StructMember* member,
                                                   TypeRecord* type) {
  if (func == NULL || !TypeIsFunction(func) || func->info.function.is_deleted ||
      member->is_anon || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL || type->info.struct_info->is_union ||
      type->info.struct_info->tag_name == NULL ||
      TypeContainsTemplateParameter(type)) {
    return false;
  }
  return FindCXXConstructor(type) != NULL;
}

// Returns a mem-initializer statement for `member` from its default member
// initializer or synthesized default construction, or NULL when no initializer
// is needed (e.g. copy/move special members or trivial members).
ASTNode* NewCXXDefaultMemberInitializerStatement(Syntax* syntax,
                                                       TypeRecord* func,
                                                       StructMember* member) {
  if (member == NULL || member->symbol == NULL) {
    return NULL;
  }
  if (member->default_initializer != NULL) {
    // `member{a, b}` is list-initialization.  When the class has an
    // initializer-list constructor, the braced list is one argument of that
    // constructor ([over.match.list]); flattening it into separate arguments
    // turns `map{{k, v}}` into a two-argument call.  An empty list still
    // value-initializes.
    // An aggregate member is aggregate-initialized from the whole list, as for
    // an explicit `member{a, b}` mem-initializer.
    ASTNode* initializer = member->default_initializer;
    TypeRecord* member_type = member->symbol->type;
    if (initializer->op == AST_OP(braced_init) &&
        ((BracedInitializerASTNode*)initializer)->initializers != NULL &&
        ((BracedInitializerASTNode*)initializer)->initializers->length > 0 &&
        member_type != NULL && TypeIsStructOrUnion(member_type) &&
        (TypeHasCXXInitializerListConstructor(member_type) ||
         (member_type->info.struct_info != NULL &&
          member_type->info.struct_info->is_aggregate &&
          !TypeContainsTemplateParameter(member_type)))) {
      Vector* actuals = NewVector();
      VectorAppend(actuals, CloneDefaultMemberInitializerFor(func, initializer));
      return NewCXXMemberInitializerStatement(
          syntax, func, member, actuals, true, initializer->location);
    }
    Vector* actuals = CXXDefaultMemberInitializerActuals(func, initializer);
    return NewCXXMemberInitializerStatement(
        syntax, func, member, actuals, true, initializer->location);
  }
  // No default member initializer and no explicit mem-initializer: a class-type
  // member that has a default constructor must still be default-constructed by
  // the enclosing constructor.  A scalar member is left uninitialized, matching
  // C++ default initialization, so only synthesize a call for a record member
  // that has a constructor.  This does not apply to the memberwise copy/move
  // special members, whose members are copied/moved from the source object (by
  // AppendCXXMemberwiseAssignments) rather than default-constructed.
  if (func != NULL && TypeIsFunction(func) &&
      !func->info.function.is_user_provided) {
    switch (func->info.function.cxx_special_member_kind) {
      case kCXXSpecialMemberCopyConstructor:
      case kCXXSpecialMemberMoveConstructor:
      case kCXXSpecialMemberCopyAssignment:
      case kCXXSpecialMemberMoveAssignment:
        return NULL;
      default:
        break;
    }
  }
  TypeRecord* member_type = member->symbol->type;
  if (TypeIsFixedArray(member_type)) {
    // Each element of a class array member is default-constructed.  Skip the
    // calls when that constructor is the trivial implicit one.
    TypeRecord* element_type = member_type->next;
    if (!CXXTypeHasDefaultConstructor(element_type) &&
        !CXXMemberLacksUsableDefaultConstructor(func, member, element_type)) {
      return NULL;
    }
    StructMember* ctor = FindCXXConstructor(element_type);
    if (ctor != NULL && ctor->overload_next == NULL &&
        ctor->symbol->flags.invented &&
        !ctor->symbol->type->info.function.is_deleted &&
        ctor->symbol->type->info.function.is_trivial_special_member) {
      return NULL;
    }
    return NewCXXMemberInitializerStatement(
        syntax, func, member, NewVector(), false, member->symbol->location);
  }
  if (!CXXTypeHasDefaultConstructor(member_type) &&
      !CXXMemberLacksUsableDefaultConstructor(func, member, member_type)) {
    return NULL;
  }
  return NewCXXMemberInitializerStatement(
      syntax, func, member, NewVector(), false, member->symbol->location);
}

// Returns the bound mem-initializer statement for data member `member` in
// `init_list`, or NULL when `member` has no explicit initializer there.
ASTNode* FindCXXExplicitMemberInitializer(CXXConstructorInitList* init_list,
                                                StructMember* member) {
  for (size_t i = 0; i < init_list->member_specs.length; i++) {
    if (init_list->member_specs.value.p[i] == member) {
      return init_list->member_statements.value.p[i];
    }
  }
  return NULL;
}

/* Mem-initializers name the members an anonymous aggregate injects
 * (`union { slot_type* slot_; }` initialized by `slot_(slot)`), not the
 * aggregate itself.  Insert them at the aggregate's declaration position; the
 * other members of an anonymous struct keep their default member initializers,
 * while an anonymous union activates only the named member.  Returns false,
 * inserting nothing, if no mem-initializer names a member of `anon`. */
bool InsertCXXAnonymousAggregateInitializers(
    Syntax* syntax, TypeRecord* func, CXXConstructorInitList* init_list,
    StructMember* anon, Vector* body, size_t* insert_at) {
  bool named = false;
  for (size_t i = 0; i < init_list->member_specs.length && !named; i++) {
    named = CXXAnonymousAggregateDeclaresMember(
        anon, init_list->member_specs.value.p[i]);
  }
  if (!named) {
    return false;
  }
  Struct* aggregate = anon->symbol->type->info.struct_info;
  for (size_t i = 0; i < aggregate->members.length; i++) {
    StructMember* member = aggregate->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function ||
        StorageIs(member->symbol->storage, STO(typedef))) {
      continue;
    }
    if (member->is_anon) {
      if (!InsertCXXAnonymousAggregateInitializers(
              syntax, func, init_list, member, body, insert_at) &&
          !aggregate->is_union) {
        ASTNode* stmt =
            NewCXXDefaultMemberInitializerStatement(syntax, func, member);
        if (stmt != NULL) {
          VectorInsertOrAppend(body, *insert_at, stmt);
          (*insert_at)++;
        }
      }
      continue;
    }
    ASTNode* stmt = NULL;
    for (size_t j = 0; j < init_list->member_specs.length; j++) {
      StructMember* injected = init_list->member_specs.value.p[j];
      if (injected != NULL && injected->symbol != NULL &&
          StringEqualString(&injected->symbol->name, &member->symbol->name)) {
        stmt = init_list->member_statements.value.p[j];
        break;
      }
    }
    if (stmt == NULL && !aggregate->is_union) {
      stmt = NewCXXDefaultMemberInitializerStatement(syntax, func, member);
    }
    if (stmt != NULL) {
      VectorInsertOrAppend(body, *insert_at, stmt);
      (*insert_at)++;
    }
  }
  return true;
}

// Returns the bound mem-initializer statement for virtual base `base` in
// `init_list`, or NULL when `base` has no explicit initializer there.
ASTNode* FindCXXExplicitVirtualBaseInitializer(
    CXXConstructorInitList* init_list, CXXVirtualBaseInfo* base) {
  for (size_t i = 0; i < init_list->virtual_base_specs.length; i++) {
    if (init_list->virtual_base_specs.value.p[i] == base) {
      return init_list->virtual_base_statements.value.p[i];
    }
  }
  return NULL;
}

// Returns the type mem-initializer name `name` initializes in class `owner`
// (the class itself, a base, or a direct data member), or NULL when the
// name does not resolve to an initializer target.
static TypeRecord* CXXConstructorInitializerTargetType(Struct* owner,
                                                       const char* name) {
  if (owner == NULL || name == NULL) {
    return NULL;
  }
  if (CXXConstructorInitializerNamesOwner(owner, name)) {
    return owner->tag_symbol != NULL ? owner->tag_symbol->type : NULL;
  }
  CXXBaseSpecifier* base = FindCXXDirectBaseByName(owner, name);
  if (base != NULL) {
    return base->type;
  }
  CXXVirtualBaseInfo* virtual_base = FindCXXVirtualBaseByName(owner, name);
  if (virtual_base != NULL) {
    return virtual_base->type;
  }
  StructMember* member = FindCXXDirectDataMemberByName(owner, name);
  return member != NULL && member->symbol != NULL ? member->symbol->type : NULL;
}

// When `actuals` is a single braced initializer and the target is neither an
// aggregate nor an initializer-list constructor, expands the braced list into
// separate constructor arguments.  Otherwise returns `actuals` unchanged;
// may replace and delete the original vector.
static Vector* ResolveCXXBracedConstructorInitializerActuals(
    TypeRecord* target_type, Vector* actuals) {
  if (target_type == NULL || actuals == NULL || actuals->length != 1) {
    return actuals;
  }
  ASTNode* root = actuals->value.p[0];
  StructMember* constructor = TypeIsStructOrUnion(target_type)
                                  ? FindCXXConstructor(target_type)
                                  : NULL;
  if (root == NULL || root->op != AST_OP(braced_init) ||
      (TypeIsStructOrUnion(target_type) &&
       target_type->info.struct_info != NULL &&
       target_type->info.struct_info->is_aggregate) ||
      CXXConstructorSetHasInitializerList(constructor)) {
    return actuals;
  }

  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)root;
  Vector* expanded = NewVector();
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* initializer = braced->initializers->value.p[i];
    ASTNode* expression = initializer;
    if (initializer != NULL && initializer->op == AST_OP(expr_init)) {
      ExpressionInitializerASTNode* expr_init =
          (ExpressionInitializerASTNode*)initializer;
      expression = expr_init->expr;
      expr_init->expr = NULL;
    }
    braced->initializers->value.p[i] = NULL;
    if (expression != NULL) {
      expression->parent = NULL;
      VectorAppend(expanded, expression);
    }
    if (initializer != expression) {
      ASTNodeDelete(initializer);
    }
  }
  actuals->value.p[0] = NULL;
  ASTNodeDelete(root);
  VectorDelete(actuals);
  return expanded;
}

// Marks mem-initializer statement `stmt` and its call expression as pack
// expansions for template instantiation.
static void MarkCXXConstructorInitPackExpansion(ASTNode* stmt) {
  if (stmt == NULL) {
    return;
  }
  stmt->flags |= kASTPackExpansion;
  if (ASTNodeGetShape(stmt) == kASTShapeExprStmt) {
    ASTNode* expr = ((ExpressionStatementASTNode*)stmt)->expr;
    if (expr != NULL) {
      expr->flags |= kASTPackExpansion;
    }
  }
}

/* A qualified mem-initializer such as `std::ostream(...)` names a typedef of
 * a class template.  The last component ("ostream") is not the base's tag
 * (`basic_ostream`).  Match the resolved, materialized class instead. */
static bool CXXMemInitializerTypesMatch(TypeRecord* left, TypeRecord* right) {
  if (left == NULL || right == NULL) {
    return false;
  }
  if (TypeIsStructOrUnion(left) && TypeIsStructOrUnion(right) &&
      left->info.struct_info != NULL &&
      left->info.struct_info == right->info.struct_info) {
    return true;
  }
  return TypeEqual(left, right);
}

// Returns the first unused direct base of `owner` whose materialized type
// matches resolved class type `type`, or NULL when none match.
static CXXBaseSpecifier* FindCXXDirectBaseByResolvedType(
    Syntax* syntax, Struct* owner, TypeRecord* type, Vector* already_used) {
  if (owner == NULL || type == NULL) {
    return NULL;
  }
  TypeRecord* want =
      TypeMaterializeClassTemplateSpecialization(syntax, type);
  CXXBaseSpecifier* found = NULL;
  for (size_t i = 0; i < owner->bases.length; i++) {
    CXXBaseSpecifier* base = owner->bases.value.p[i];
    if (base == NULL || base->type == NULL || base->is_virtual) {
      continue;
    }
    if (already_used != NULL && VectorContainsPointer(already_used, base)) {
      continue;
    }
    TypeRecord* have =
        TypeMaterializeClassTemplateSpecialization(syntax, base->type);
    if (CXXMemInitializerTypesMatch(want, have)) {
      found = base;
    }
    if (have != base->type) {
      TypeRecordDelete(have);
    }
    if (found != NULL) {
      break;
    }
  }
  if (want != type) {
    TypeRecordDelete(want);
  }
  return found;
}

// Returns the virtual base of `owner` whose materialized type matches
// resolved class type `type`, or NULL when none match.
static CXXVirtualBaseInfo* FindCXXVirtualBaseByResolvedType(
    Syntax* syntax, Struct* owner, TypeRecord* type) {
  if (owner == NULL || type == NULL) {
    return NULL;
  }
  TypeRecord* want =
      TypeMaterializeClassTemplateSpecialization(syntax, type);
  CXXVirtualBaseInfo* found = NULL;
  for (size_t i = 0; i < owner->virtual_bases.length; i++) {
    CXXVirtualBaseInfo* base = owner->virtual_bases.value.p[i];
    if (base == NULL || base->type == NULL) {
      continue;
    }
    TypeRecord* have =
        TypeMaterializeClassTemplateSpecialization(syntax, base->type);
    if (CXXMemInitializerTypesMatch(want, have)) {
      found = base;
    }
    if (have != base->type) {
      TypeRecordDelete(have);
    }
    if (found != NULL) {
      break;
    }
  }
  if (want != type) {
    TypeRecordDelete(want);
  }
  return found;
}

/* Bind `name(actuals)` to unused direct bases of that name.  A pack-expansion
 * mem-initializer (`Storage<Ts, I>(args)...`) produces one call per matching
 * base.  Returns true if the initializer was consumed: one or more bases were
 * bound, or an empty pack expansion matched no remaining base. */
static bool BindCXXBaseConstructorInitializers(
    Syntax* syntax, TypeRecord* func, CXXConstructorInitList* init_list,
    Struct* owner, const char* init_name, Vector* actuals,
    SourceLocation location, bool is_pack_expansion) {
  size_t pack_element_index = 0;
  CXXBaseSpecifier* base = FindCXXDirectBaseByNameSkipping(
      owner, init_name, &init_list->base_specs, pack_element_index);
  if (base != NULL && base->is_virtual) {
    return false;
  }
  if (base == NULL) {
    if (is_pack_expansion) {
      if (actuals != NULL) {
        VectorDelete(actuals);
      }
      return true;
    }
    return false;
  }

  Vector* pattern = actuals;
  while (base != NULL && !base->is_virtual) {
    CheckCXXConstructorInitializerOrder(
        syntax, init_list, init_name, CXXDirectBaseOrder(owner, base));
    Vector* call_actuals = CloneCXXConstructorInitializerActuals(pattern);
    ASTNode* call = NewCXXBaseSpecialMemberCall(syntax, func, base, false,
                                                /*complete_object=*/false,
                                                call_actuals, location);
    if (call != NULL) {
      if (is_pack_expansion) {
        MarkCXXConstructorInitPackExpansion(call);
      }
      VectorAppend(&init_list->base_statements, call);
    }
    // Record the base even when the call cannot be built.  A pack expansion
    // searches again for the same name, and an unrecorded base would be
    // returned forever.
    VectorAppend(&init_list->base_specs, base);
    if (!is_pack_expansion) {
      break;
    }
    CXXBaseSpecifier* previous = base;
    pack_element_index++;
    base = FindCXXDirectBaseByNameSkipping(owner, init_name,
                                           &init_list->base_specs,
                                           pack_element_index);
    if (base == previous) {
      break;
    }
  }
  if (pattern != NULL) {
    VectorDelete(pattern);
  }
  return true;
}

// Parses the arguments of the mem-initializer for `init_name` of class
// `owner`: a parenthesized list, or a braced list that is kept as one braced
// initializer for an aggregate or initializer-list constructor target and
// otherwise treated as a constructor argument list.  Returns the argument
// vector (empty after a diagnosed missing list).
static Vector* ParseCXXMemInitializerArguments(Syntax* syntax, Struct* owner,
                                               const char* init_name) {
  if (LexMatch(syntax->lex, TOK(lparen))) {
    return ParseCXXInitializerArgumentList(syntax, TOK(rparen));
  }
  if (!LexLookingAt(syntax->lex, TOK(lbrace))) {
    SyntaxError(syntax, "Expected constructor initializer argument list");
    return NewVector();
  }
  TypeRecord* target_type =
      CXXConstructorInitializerTargetType(owner, init_name);
  StructMember* constructor =
      TypeIsStructOrUnion(target_type)
          ? FindCXXConstructor(target_type)
          : NULL;
  LexNextToken(syntax->lex);
  if (target_type == NULL ||
      (TypeIsStructOrUnion(target_type) &&
       target_type->info.struct_info != NULL &&
       target_type->info.struct_info->is_aggregate) ||
      CXXConstructorSetHasInitializerList(constructor)) {
    Vector* actuals = NewVector();
    VectorAppend(actuals, SyntaxParseBracedInitializer(syntax));
    return actuals;
  }
  return ParseCXXInitializerArgumentList(syntax, TOK(rbrace));
}

// Binds a mem-initializer naming a base through a qualified name
// (`ns::Base(args)`) by resolving the name to its type and matching a direct
// or virtual base of `owner`.  Returns false, leaving `actuals` untouched,
// when the name resolves to no base.
static bool BindCXXQualifiedBaseInitializer(Syntax* syntax, TypeRecord* func,
                                            Struct* owner,
                                            CXXConstructorInitList* init_list,
                                            FullyQualifiedIdentifier* name,
                                            const char* init_name,
                                            Vector* actuals,
                                            SourceLocation location) {
  Symbol* named = SyntaxFindQualifiedSymbol(syntax, name);
  if (named == NULL || named->type == NULL) {
    named = SyntaxFindQualifiedTag(syntax, name);
  }
  if (named == NULL || named->type == NULL) {
    return false;
  }
  CXXBaseSpecifier* typed_base = FindCXXDirectBaseByResolvedType(
      syntax, owner, named->type, &init_list->base_specs);
  if (typed_base != NULL) {
    CheckCXXConstructorInitializerOrder(
        syntax, init_list, init_name,
        CXXDirectBaseOrder(owner, typed_base));
    ASTNode* call = NewCXXBaseSpecialMemberCall(
        syntax, func, typed_base, false, /*complete_object=*/false,
        actuals, location);
    if (call != NULL) {
      VectorAppend(&init_list->base_statements, call);
    }
    VectorAppend(&init_list->base_specs, typed_base);
    return true;
  }
  CXXVirtualBaseInfo* typed_virtual = FindCXXVirtualBaseByResolvedType(
      syntax, owner, named->type);
  if (typed_virtual == NULL ||
      VectorContainsPointer(&init_list->virtual_base_specs, typed_virtual)) {
    return false;
  }
  CheckCXXConstructorInitializerOrder(
      syntax, init_list, init_name,
      CXXVirtualBaseOrder(owner, typed_virtual));
  ASTNode* call = NewCXXVirtualBaseSpecialMemberCall(
      syntax, func, typed_virtual, false, actuals, location);
  if (call != NULL) {
    VectorAppend(&init_list->virtual_base_specs, typed_virtual);
    VectorAppend(&init_list->virtual_base_statements, call);
  }
  return true;
}

// Binds the mem-initializer of virtual base `virtual_base` of `owner`,
// diagnosing a duplicate.  Consumes `actuals`.
static void BindCXXVirtualBaseInitializer(Syntax* syntax, TypeRecord* func,
                                          Struct* owner,
                                          CXXConstructorInitList* init_list,
                                          CXXVirtualBaseInfo* virtual_base,
                                          const char* init_name,
                                          Vector* actuals,
                                          SourceLocation location) {
  if (VectorContainsPointer(&init_list->virtual_base_specs, virtual_base)) {
    SyntaxError(syntax, "Duplicate initializer for virtual base %s",
                init_name);
    VectorDelete(actuals);
    return;
  }
  CheckCXXConstructorInitializerOrder(
      syntax, init_list, init_name,
      CXXVirtualBaseOrder(owner, virtual_base));
  ASTNode* call = NewCXXVirtualBaseSpecialMemberCall(
      syntax, func, virtual_base, false, actuals, location);
  if (call != NULL) {
    VectorAppend(&init_list->virtual_base_specs, virtual_base);
    VectorAppend(&init_list->virtual_base_statements, call);
  }
}

// Binds the mem-initializer for `init_name` to a direct data member of
// `owner`, diagnosing a duplicate.  A name that is not a direct data member
// is deferred to SyntaxResolveCXXConstructorInitializerList.  Consumes
// `actuals`.
static void BindCXXDataMemberInitializer(Syntax* syntax, TypeRecord* func,
                                         Struct* owner,
                                         CXXConstructorInitList* init_list,
                                         const char* init_name,
                                         Vector* actuals,
                                         SourceLocation location,
                                         bool init_is_pack_expansion) {
  StructMember* member = FindCXXDirectDataMemberByName(owner, init_name);
  if (member == NULL) {
    VectorAppend(&init_list->deferred_initializers,
                 NewCXXDeferredConstructorInitializer(
                     init_name, actuals, location,
                     init_is_pack_expansion));
  } else if (VectorContainsPointer(&init_list->member_specs, member)) {
    SyntaxError(syntax, "Duplicate initializer for member %s", init_name);
    VectorDelete(actuals);
  } else {
    CheckCXXConstructorInitializerOrder(
        syntax, init_list, init_name,
        CXXDirectMemberOrder(owner, member));
    ASTNode* stmt = NewCXXMemberInitializerStatement(
        syntax, func, member, actuals, true, location);
    if (stmt != NULL) {
      VectorAppend(&init_list->member_specs, member);
      VectorAppend(&init_list->member_statements, stmt);
    }
  }
}

// Binds one parsed mem-initializer `name(actuals)` of constructor `func` to
// what it initializes: the delegated-to constructor (which must appear
// alone), a direct or virtual base, or a data member.  Consumes `actuals`.
static void BindCXXMemInitializer(Syntax* syntax, TypeRecord* func,
                                  Struct* owner,
                                  CXXConstructorInitList* init_list,
                                  FullyQualifiedIdentifier* name,
                                  Vector* actuals, SourceLocation location,
                                  bool init_is_pack_expansion) {
  const char* init_name = FullyQualifiedIdentifierLast(name);
  if (CXXConstructorInitializerNamesOwner(owner, init_name)) {
    if (init_list->delegating_statement != NULL ||
        init_list->base_specs.length != 0 ||
        init_list->virtual_base_specs.length != 0 ||
        init_list->member_specs.length != 0) {
      SyntaxError(syntax,
                  "delegating constructor initializer must appear alone");
      VectorDelete(actuals);
    } else {
      init_list->delegating_statement =
          NewCXXDelegatingConstructorCall(syntax, func, actuals, location);
    }
    return;
  }
  if (init_list->delegating_statement != NULL) {
    SyntaxError(syntax,
                "delegating constructor initializer must appear alone");
    VectorDelete(actuals);
    return;
  }
  if (name->is_qualified && !init_is_pack_expansion &&
      BindCXXQualifiedBaseInitializer(syntax, func, owner, init_list, name,
                                      init_name, actuals, location)) {
    return;
  }
  if (BindCXXBaseConstructorInitializers(
          syntax, func, init_list, owner, init_name, actuals, location,
          init_is_pack_expansion)) {
    return;
  }
  CXXVirtualBaseInfo* virtual_base =
      FindCXXVirtualBaseByName(owner, init_name);
  if (virtual_base != NULL) {
    BindCXXVirtualBaseInitializer(syntax, func, owner, init_list,
                                  virtual_base, init_name, actuals, location);
  } else {
    BindCXXDataMemberInitializer(syntax, func, owner, init_list, init_name,
                                 actuals, location, init_is_pack_expansion);
  }
}

// Parses the mem-initializer-list (`: base(x), member{y}`) of constructor
// `func` if one follows, binding each initializer into `init_list`.  Every
// initializer is also kept unbound in `init_list->raw_initializers` so a
// class template specialization can rebind it.  Does nothing for a
// non-constructor or when no `:` follows.
void SyntaxParseCXXConstructorInitializerList(
    Syntax* syntax, TypeRecord* func, CXXConstructorInitList* init_list) {
  if (!CompilerIsCXX() || func == NULL || !func->info.function.is_constructor ||
      func->info.function.cxx_member_owner == NULL ||
      !LexMatch(syntax->lex, TOK(colon))) {
    return;
  }

  Struct* owner = func->info.function.cxx_member_owner;
  while (!LexEof(syntax->lex) && !LexLookingAt(syntax->lex, TOK(lbrace))) {
    SourceLocation location = syntax->lex->current_token_location;
    FullyQualifiedIdentifier name;
    FullyQualifiedIdentifierInit(&name);
    // A base-class mem-initializer-id may be a template-id (`Base<T>(args)`), so
    // consume any template argument list here; the base is still matched by its
    // bare name (FullyQualifiedIdentifierLast strips the template arguments).
    // The following token is the initializer's opening `(` / `{`.
    if (!SyntaxParseFullyQualifiedIdentifierWithTemplateIds(syntax, &name,
                                                            TC(openbra))) {
      SyntaxError(syntax, "Expected constructor initializer name");
      FullyQualifiedIdentifierDestruct(&name);
      break;
    }

    const char* init_name = FullyQualifiedIdentifierLast(&name);
    Vector* actuals = ParseCXXMemInitializerArguments(syntax, owner, init_name);
    bool init_is_pack_expansion = false;
    if (CompilerIsCXX() && LexMatch(syntax->lex, TOK(ellipsis))) {
      init_is_pack_expansion = true;
    }

    VectorAppend(&init_list->raw_initializers,
                 NewCXXDeferredConstructorInitializer(
                     init_name, CloneCXXConstructorInitializerActuals(actuals),
                     location, init_is_pack_expansion));
    BindCXXMemInitializer(syntax, func, owner, init_list, &name, actuals,
                          location, init_is_pack_expansion);
    FullyQualifiedIdentifierDestruct(&name);

    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
  }
}

// Binds mem-initializers deferred at parse time (e.g. injected anonymous-
// aggregate members) into `init_list`, expanding braced lists and diagnosing
// duplicates and unknown names.
void SyntaxResolveCXXConstructorInitializerList(
    Syntax* syntax, TypeRecord* func, CXXConstructorInitList* init_list) {
  if (!CompilerIsCXX() || func == NULL || !func->info.function.is_constructor ||
      func->info.function.cxx_member_owner == NULL || init_list == NULL) {
    return;
  }
  Struct* owner = func->info.function.cxx_member_owner;
  for (size_t i = 0; i < init_list->deferred_initializers.length; i++) {
    CXXDeferredConstructorInitializer* deferred =
        init_list->deferred_initializers.value.p[i];
    if (deferred == NULL || deferred->actuals == NULL) {
      continue;
    }
    const char* init_name = deferred->name.value;
    Vector* actuals = deferred->actuals;
    deferred->actuals = NULL;
    SourceLocation location = deferred->location;
    actuals = ResolveCXXBracedConstructorInitializerActuals(
        CXXConstructorInitializerTargetType(owner, init_name), actuals);
    if (CXXConstructorInitializerNamesOwner(owner, init_name)) {
      if (init_list->delegating_statement != NULL ||
          init_list->base_specs.length != 0 ||
          init_list->virtual_base_specs.length != 0 ||
          init_list->member_specs.length != 0) {
        SyntaxError(syntax,
                    "delegating constructor initializer must appear alone");
        VectorDelete(actuals);
      } else {
        init_list->delegating_statement =
            NewCXXDelegatingConstructorCall(syntax, func, actuals, location);
      }
      continue;
    }
    if (init_list->delegating_statement != NULL) {
      SyntaxError(syntax,
                  "delegating constructor initializer must appear alone");
      VectorDelete(actuals);
      continue;
    }
    if (BindCXXBaseConstructorInitializers(
            syntax, func, init_list, owner, init_name, actuals, location,
            deferred->is_pack_expansion)) {
      continue;
    }
    CXXVirtualBaseInfo* virtual_base =
        FindCXXVirtualBaseByName(owner, init_name);
    if (virtual_base != NULL) {
      if (VectorContainsPointer(&init_list->virtual_base_specs, virtual_base)) {
        SyntaxError(syntax, "Duplicate initializer for virtual base %s",
                    init_name);
        VectorDelete(actuals);
        continue;
      }
      CheckCXXConstructorInitializerOrder(
          syntax, init_list, init_name,
          CXXVirtualBaseOrder(owner, virtual_base));
      ASTNode* call = NewCXXVirtualBaseSpecialMemberCall(
          syntax, func, virtual_base, false, actuals, location);
      if (call != NULL) {
        VectorAppend(&init_list->virtual_base_specs, virtual_base);
        VectorAppend(&init_list->virtual_base_statements, call);
      }
      continue;
    }
    StructMember* member = FindCXXDirectDataMemberByName(owner, init_name);
    if (member == NULL) {
      SyntaxError(syntax, "%s is not a direct base or member of %s",
                  init_name,
                  owner->tag_name != NULL ? owner->tag_name->value
                                          : "<anonymous>");
      VectorDelete(actuals);
      continue;
    }
    if (VectorContainsPointer(&init_list->member_specs, member)) {
      SyntaxError(syntax, "Duplicate initializer for member %s", init_name);
      VectorDelete(actuals);
      continue;
    }
    CheckCXXConstructorInitializerOrder(
        syntax, init_list, init_name, CXXDirectMemberOrder(owner, member));
    ASTNode* stmt = NewCXXMemberInitializerStatement(
        syntax, func, member, actuals, true, location);
    if (stmt != NULL) {
      VectorAppend(&init_list->member_specs, member);
      VectorAppend(&init_list->member_statements, stmt);
    }
  }
}
