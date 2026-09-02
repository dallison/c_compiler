//
//  type_compare.c
//  c_compiler
//

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
#include "rtti.h"
#include "set.h"
#include "type_traits_semantics.h"

static bool DependentExpressionNodeContainsParameter(ASTNode* node,
                                                     void* data) {
  (void)data;
  if (node == NULL || node->op != AST_OP(identifier)) {
    return false;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  if (id->symbol == NULL) {
    return false;
  }
  return (id->symbol->flags.is_template_parameter &&
          id->symbol->template_parameter_index >= 0) ||
         id->symbol->dependent_value_template_parameter_index >= 0 ||
         TypeContainsTemplateParameter(id->symbol->type) ||
         TemplateArgumentVectorContainsTemplateParameter(
             id->template_arguments);
}

bool TemplateArgumentPatternVectorEqual(Vector* left, Vector* right);
bool TemplateArgumentVectorEqual(Vector* left, Vector* right);
bool TemplateArgumentVectorContainsTemplateParameter(Vector* args);
static uint64_t HashTypeValue(uint64_t hash, uint64_t value);
static uint64_t HashTemplateArgument(uint64_t hash, TemplateArgument* arg);
static uint64_t HashTypeRecord(uint64_t hash, TypeRecord* type);

bool DependentExpressionContainsTemplateParameter(ASTNode* expr) {
  return ASTNodeAny(expr, DependentExpressionNodeContainsParameter, NULL);
}

/* True if a template argument is still dependent: it references a template
 * parameter directly, contains a dependent pack element, or names a dependent
 * type. */
bool TemplateArgumentContainsTemplateParameter(TemplateArgument* arg) {
  if (arg == NULL) {
    return false;
  }
  if ((arg->kind == kTemplateParameterNonType ||
       arg->kind == kTemplateParameterTemplate) &&
      arg->template_parameter_index >= 0) {
    return true;
  }
  if (arg->pack_arguments != NULL &&
      TemplateArgumentVectorContainsTemplateParameter(arg->pack_arguments)) {
    return true;
  }
  if (DependentExpressionContainsTemplateParameter(arg->dependent_expr)) {
    return true;
  }
  return TypeContainsTemplateParameter(arg->type);
}

/* True if any argument in the vector is still dependent (see above). */
bool TemplateArgumentVectorContainsTemplateParameter(Vector* args) {
  for (size_t i = 0; args != NULL && i < args->length; i++) {
    if (TemplateArgumentContainsTemplateParameter(args->value.p[i])) {
      return true;
    }
  }
  return false;
}

static bool TypeRecordNodeContainsTemplateParameter(TypeRecord* type) {
  if (TypeIsUnknown(type) && type->template_parameter_index >= 0) {
    return true;
  }
  // A deferred `decltype` whose operand is still type-dependent (e.g.
  // `iterator_t<R> = decltype(ranges::begin(declval<R&>()))` substituted while
  // `R` survives) carries the unevaluated expression here.  Such a type is
  // still dependent even though its placeholder spine (an `int` fallback)
  // carries no parameter index, so it must be re-substituted once concrete
  // arguments arrive.  The operand is only retained while dependent (it is
  // dropped once the decltype resolves), so its mere presence is sufficient.
  if (type->dependent_decltype_expr != NULL ||
      type->dependent_splice_expr != NULL || type->is_pack_index) {
    return true;
  }
  if (type->dependent_member_name != NULL &&
      (type->template_parameter_index >= 0 ||
       type->template_origin != NULL)) {
    return true;
  }
  if (type->template_origin != NULL &&
      type->template_origin->flags.is_template_template_parameter) {
    return true;
  }
  if (type->declarator == kDeclArray &&
      type->info.array.template_parameter_index >= 0) {
    return true;
  }
  if (TemplateArgumentVectorContainsTemplateParameter(
          type->template_arguments)) {
    return true;
  }
  if (TypeIsFunction(type)) {
    for (size_t i = 0; i < type->info.function.prototype.length; i++) {
      Symbol* formal = type->info.function.prototype.value.p[i];
      if (formal != NULL && TypeContainsTemplateParameter(formal->type)) {
        return true;
      }
    }
  }
  return false;
}

/* True if `type` still mentions an unresolved template parameter anywhere: as a
 * bare parameter, an array bound, a template argument, or a function parameter
 * type. Cache the answer on every queried suffix so shared declarator tails are
 * only examined once. */
static bool TypeContainsTemplateParameterImpl(TypeRecord* type,
                                              bool cache_result) {
  if (type == NULL) {
    return false;
  }
  if (type->template_parameter_summary !=
      kTypeTemplateParameterSummaryUnknown) {
    return type->template_parameter_summary ==
           kTypeTemplateParameterSummaryPresent;
  }
  bool contains = TypeRecordNodeContainsTemplateParameter(type) ||
                  TypeContainsTemplateParameterImpl(type->next, cache_result);
  if (cache_result) {
    type->template_parameter_summary =
        contains ? kTypeTemplateParameterSummaryPresent
                 : kTypeTemplateParameterSummaryAbsent;
  }
  return contains;
}

bool TypeContainsTemplateParameterSlow(TypeRecord* type) {
  return TypeContainsTemplateParameterImpl(type, false);
}

void TypeCacheTemplateParameterSummary(TypeRecord* type) {
  if (type == NULL) {
    return;
  }
  TypeCacheTemplateParameterSummary(type->next);
  (void)TypeContainsTemplateParameterImpl(type, true);
}

static bool StructStackContains(Vector* stack, Struct* str) {
  for (size_t i = 0; i < stack->length; i++) {
    if (stack->value.p[i] == str) {
      return true;
    }
  }
  return false;
}

static bool TypeIsConstevalOnlyRecursive(TypeRecord* type, Vector* active,
                                         Vector* checked) {
  for (TypeRecord* current = type; current != NULL; current = current->next) {
    if (TypeIsReflection(current)) {
      return true;
    }
    if (TypeIsFunction(current)) {
      for (size_t i = 0; i < current->info.function.prototype.length; i++) {
        Symbol* parameter = current->info.function.prototype.value.p[i];
        if (parameter != NULL &&
            TypeIsConstevalOnlyRecursive(parameter->type, active, checked)) {
          return true;
        }
      }
    }
    if (!TypeIsStructOrUnion(current) ||
        current->info.struct_info == NULL ||
        StructStackContains(active, current->info.struct_info) ||
        StructStackContains(checked, current->info.struct_info)) {
      continue;
    }
    Struct* str = current->info.struct_info;
    VectorAppend(active, str);
    for (size_t i = 0; i < str->bases.length; i++) {
      CXXBaseSpecifier* base = str->bases.value.p[i];
      if (base != NULL &&
          TypeIsConstevalOnlyRecursive(base->type, active, checked)) {
        VectorPop(active);
        return true;
      }
    }
    for (size_t i = 0; i < str->members.length; i++) {
      StructMember* member = str->members.value.p[i];
      if (member != NULL && !member->is_static && member->symbol != NULL &&
          TypeIsConstevalOnlyRecursive(member->symbol->type, active, checked)) {
        VectorPop(active);
        return true;
      }
    }
    VectorPop(active);
    VectorAppend(checked, str);
  }
  return false;
}

bool TypeIsConstevalOnly(TypeRecord* type) {
  Vector active;
  Vector checked;
  VectorInit(&active);
  VectorInit(&checked);
  bool result = TypeIsConstevalOnlyRecursive(type, &active, &checked);
  VectorDestruct(&checked);
  VectorDestruct(&active);
  return result;
}

static bool DependentTemplateArgExprEqual(ASTNode* a, ASTNode* b);

bool TemplateArgumentEqual(TemplateArgument* left,
                                  TemplateArgument* right) {
  if (left == NULL || right == NULL || left->kind != right->kind) {
    return left == right;
  }
  if (left->pack_arguments != NULL || right->pack_arguments != NULL) {
    if (left->pack_arguments == NULL || right->pack_arguments == NULL ||
        left->pack_arguments->length != right->pack_arguments->length) {
      return false;
    }
    for (size_t i = 0; i < left->pack_arguments->length; i++) {
      if (!TemplateArgumentEqual(left->pack_arguments->value.p[i],
                                 right->pack_arguments->value.p[i])) {
        return false;
      }
    }
    return true;
  }
  if (left->kind == kTemplateParameterType) {
    return TypeEqual(left->type, right->type);
  }
  if (left->kind == kTemplateParameterTemplate) {
    return left->template_symbol == right->template_symbol &&
           left->template_parameter_index ==
               right->template_parameter_index &&
           DependentTemplateArgExprEqual(left->pack_index_expr,
                                         right->pack_index_expr);
  }
  // Value-dependent non-type arguments (e.g. two `enable_if` SFINAE conditions)
  // are distinguished by comparing their stored expressions structurally, so
  // distinct overloads are not mistaken for redefinitions.
  if (left->dependent_expr != NULL || right->dependent_expr != NULL) {
    return DependentTemplateArgExprEqual(left->dependent_expr,
                                         right->dependent_expr);
  }
  return TemplateArgumentValuesEqual(left, right);
}

/* Element-wise equality of two concrete template argument vectors. */
bool TemplateArgumentVectorEqual(Vector* left, Vector* right) {
  if (left == NULL || right == NULL || left->length != right->length) {
    return left == right;
  }
  for (size_t i = 0; i < left->length; i++) {
    if (!TemplateArgumentEqual(left->value.p[i], right->value.p[i])) {
      return false;
    }
  }
  return true;
}

// Compare two `dependent_member_template_arguments` lists: a list of per-path-
// component argument vectors (the `<...>` of a dependent member-template access
// such as `Alloc::rebind_alloc<T>`).  Each entry is itself a nullable argument
// vector.  Two dependent member-template types are the same only if these
// match; otherwise `Traits<A>::rebind_alloc<node>` and
// `Traits<A>::rebind_alloc<node*>` would be treated as one type.
static bool TemplateArgumentVectorListEqual(Vector* left, Vector* right) {
  if (left == NULL || right == NULL) {
    return left == right;
  }
  if (left->length != right->length) {
    return false;
  }
  for (size_t i = 0; i < left->length; i++) {
    Vector* le = left->value.p[i];
    Vector* re = right->value.p[i];
    if (le == NULL || re == NULL) {
      if (le != re) {
        return false;
      }
      continue;
    }
    if (!TemplateArgumentVectorEqual(le, re)) {
      return false;
    }
  }
  return true;
}

static bool DependentTemplateArgExprEqual(ASTNode* a, ASTNode* b) {
  if (a == b) {
    return true;
  }
  if (a == NULL || b == NULL || a->op != b->op) {
    return false;
  }
  switch (a->op) {
    case AST_OP(number):
    case AST_OP(charconst):
    case AST_OP(charwide):
      return ((ConstantASTNode*)a)->value.ivalue ==
             ((ConstantASTNode*)b)->value.ivalue;
    case AST_OP(identifier): {
      IdentifierASTNode* ia = (IdentifierASTNode*)a;
      IdentifierASTNode* ib = (IdentifierASTNode*)b;
      Symbol* sa = ia->symbol;
      Symbol* sb = ib->symbol;
      if (sa == sb) {
        return true;
      }
      if (sa == NULL || sb == NULL) {
        return false;
      }
      TypeRecord* ta = sa->type;
      TypeRecord* tb = sb->type;
      // Dependent qualified names (`Trait<Args>::member`): equal iff the same
      // scope template, the same member, and structurally-equal scope arguments.
      if (ta != NULL && tb != NULL && ta->dependent_member_name != NULL &&
          tb->dependent_member_name != NULL) {
        return ta->template_origin == tb->template_origin &&
               ta->template_parameter_index == tb->template_parameter_index &&
               StringEqual(ta->dependent_member_name,
                           tb->dependent_member_name->value) &&
               TemplateArgumentVectorEqual(ta->template_arguments,
                                           tb->template_arguments);
      }
      // Plain non-type template parameter references compare by index.
      if (sa->flags.is_template_parameter && sb->flags.is_template_parameter) {
        return sa->template_parameter_index == sb->template_parameter_index;
      }
      return false;
    }
    case AST_OP(not):
    case AST_OP(noexcept_expr):
    case AST_OP(onescomp):
    case AST_OP(uminus):
    case AST_OP(uplus):
      return DependentTemplateArgExprEqual(((UnaryASTNode*)a)->sub,
                                           ((UnaryASTNode*)b)->sub);
    case AST_OP(plus):
    case AST_OP(minus):
    case AST_OP(mult):
    case AST_OP(div):
    case AST_OP(mod):
    case AST_OP(lshift):
    case AST_OP(rshifta):
    case AST_OP(rshiftl):
    case AST_OP(less):
    case AST_OP(lesseq):
    case AST_OP(greater):
    case AST_OP(greatereq):
    case AST_OP(equal):
    case AST_OP(noteq):
    case AST_OP(and):
    case AST_OP(bitor):
    case AST_OP(exor):
    case AST_OP(logand):
    case AST_OP(logor):
      return DependentTemplateArgExprEqual(((BinaryASTNode*)a)->left,
                                           ((BinaryASTNode*)b)->left) &&
             DependentTemplateArgExprEqual(((BinaryASTNode*)a)->right,
                                           ((BinaryASTNode*)b)->right);
    case AST_OP(cast): {
      CastASTNode* ca = (CastASTNode*)a;
      CastASTNode* cb = (CastASTNode*)b;
      return TypeEqual(ca->cast_type, cb->cast_type) &&
             DependentTemplateArgExprEqual(ca->expr, cb->expr);
    }
    case AST_OP(sizeof):
    case AST_OP(alignof): {
      if (ASTNodeGetShape(a) != kASTShapeSizeof ||
          ASTNodeGetShape(b) != kASTShapeSizeof) {
        return false;
      }
      SizeofASTNode* sa = (SizeofASTNode*)a;
      SizeofASTNode* sb = (SizeofASTNode*)b;
      return sa->is_pack_size == sb->is_pack_size &&
             DependentTemplateArgExprEqual(sa->expr, sb->expr) &&
             TypeEqual(sa->type_operand, sb->type_operand);
    }
    default: {
      ASTNodeShape shape = ASTNodeGetShape(a);
      if (shape != ASTNodeGetShape(b)) {
        return false;
      }
      if (shape == kASTShapeUnary) {
        return DependentTemplateArgExprEqual(((UnaryASTNode*)a)->sub,
                                             ((UnaryASTNode*)b)->sub);
      }
      if (shape == kASTShapeBinary) {
        return DependentTemplateArgExprEqual(((BinaryASTNode*)a)->left,
                                             ((BinaryASTNode*)b)->left) &&
               DependentTemplateArgExprEqual(((BinaryASTNode*)a)->right,
                                             ((BinaryASTNode*)b)->right);
      }
      if (shape == kASTShapeVector) {
        VectorASTNode* va = (VectorASTNode*)a;
        VectorASTNode* vb = (VectorASTNode*)b;
        if (!DependentTemplateArgExprEqual(va->left, vb->left)) {
          return false;
        }
        if (va->children == NULL || vb->children == NULL) {
          return va->children == vb->children;
        }
        if (va->children->length != vb->children->length) {
          return false;
        }
        for (size_t i = 0; i < va->children->length; i++) {
          if (!DependentTemplateArgExprEqual(va->children->value.p[i],
                                             vb->children->value.p[i])) {
            return false;
          }
        }
        return true;
      }
      return false;
    }
  }
}

bool TypeArrayBoundsEqual(ArrayInfo* a, ArrayInfo* b) {
  if (a->is_dependent_bound || b->is_dependent_bound) {
    return a->is_dependent_bound == b->is_dependent_bound &&
           a->size.vla.size == b->size.vla.size;
  }
  if (a->is_vla || b->is_vla) {
    return true;
  }
  return a->size.fixed == b->size.fixed;
}

/* Equality of two type *patterns* (types that may still mention template
 * parameters), comparing parameter indices structurally rather than resolving
 * them. Used to compare partial-specialization / template signatures. */
static bool TemplateTypePatternEqual(TypeRecord* left, TypeRecord* right) {
  if (left == NULL || right == NULL || left->declarator != right->declarator ||
      left->qualifiers != right->qualifiers) {
    return left == right;
  }
  // A primitive placeholder with an index denotes the entire template type.
  // Other declarators also use this field for one dependent component (for
  // example, the class in `R (C::*)(Args...)` or an array bound); those must
  // still be compared structurally below.
  bool left_is_type_parameter =
      left->declarator == kDeclPrimitive &&
      left->template_parameter_index >= 0;
  bool right_is_type_parameter =
      right->declarator == kDeclPrimitive &&
      right->template_parameter_index >= 0;
  if (left_is_type_parameter || right_is_type_parameter) {
    return left_is_type_parameter == right_is_type_parameter &&
           left->template_parameter_index ==
               right->template_parameter_index &&
           left->is_pack_index == right->is_pack_index &&
           (!left->is_pack_index ||
            (DependentTemplateArgExprEqual(left->pack_index_expr,
                                           right->pack_index_expr) &&
             TemplateArgumentEqual(left->pack_index_pack,
                                   right->pack_index_pack)));
  }
  if (left->type != right->type) {
    return false;
  }
  if (TypeIsStructOrUnion(left)) {
    if (left->info.struct_info != right->info.struct_info) {
      return false;
    }
  } else if (TypeIsEnum(left)) {
    if (left->info.enum_info != right->info.enum_info) {
      return false;
    }
  }
  if (!TemplateArgumentPatternVectorEqual(left->template_arguments,
                                          right->template_arguments)) {
    return false;
  }
  switch (left->declarator) {
    case kDeclArray:
      if (left->info.array.is_dependent_bound !=
          right->info.array.is_dependent_bound) {
        return false;
      }
      if (left->info.array.template_parameter_index !=
          right->info.array.template_parameter_index) {
        return false;
      }
      if (left->info.array.template_parameter_index < 0 &&
          !left->info.array.is_dependent_bound &&
          !TypeArrayBoundsEqual(&left->info.array, &right->info.array)) {
        return false;
      }
      return TemplateTypePatternEqual(left->next, right->next);
    case kDeclPointer:
    case kDeclReference:
    case kDeclRValueReference:
      return TemplateTypePatternEqual(left->next, right->next);
    case kDeclMemberPointer:
      if (left->template_parameter_index >= 0 ||
          right->template_parameter_index >= 0) {
        if (left->template_parameter_index != right->template_parameter_index) {
          return false;
        }
      } else if (left->info.struct_info != right->info.struct_info) {
        return false;
      }
      return TemplateTypePatternEqual(left->next, right->next);
    case kDeclFunction:
      if (!TemplateTypePatternEqual(left->next, right->next) ||
          left->info.function.prototype.length !=
              right->info.function.prototype.length ||
          left->info.function.is_const_member !=
              right->info.function.is_const_member ||
          left->info.function.is_volatile_member !=
              right->info.function.is_volatile_member ||
          left->info.function.ref_qualifier !=
              right->info.function.ref_qualifier ||
          left->info.function.is_noexcept !=
              right->info.function.is_noexcept) {
        return false;
      }
      for (size_t i = 0; i < left->info.function.prototype.length; i++) {
        Symbol* left_formal = left->info.function.prototype.value.p[i];
        Symbol* right_formal = right->info.function.prototype.value.p[i];
        if (left_formal == NULL || right_formal == NULL ||
            !TemplateTypePatternEqual(left_formal->type, right_formal->type)) {
          return left_formal == right_formal;
        }
      }
      return true;
    case kDeclPrimitive:
      return true;
  }
}

/* Equality of two template argument *patterns* (arguments that may still be
 * parameter-dependent), comparing parameter indices structurally. */
static bool TemplateArgumentPatternEqual(TemplateArgument* left,
                                         TemplateArgument* right) {
  if (left == NULL || right == NULL || left->kind != right->kind ||
      left->is_pack_expansion != right->is_pack_expansion) {
    return left == right;
  }
  if (left->pack_arguments != NULL || right->pack_arguments != NULL) {
    if (left->pack_arguments == NULL || right->pack_arguments == NULL ||
        left->pack_arguments->length != right->pack_arguments->length) {
      return false;
    }
    return TemplateArgumentPatternVectorEqual(left->pack_arguments,
                                             right->pack_arguments);
  }
  if (left->kind == kTemplateParameterType) {
    return TemplateTypePatternEqual(left->type, right->type);
  }
  return TemplateArgumentValuesEqual(left, right);
}

/* Element-wise equality of two template argument pattern vectors. */
bool TemplateArgumentPatternVectorEqual(Vector* left, Vector* right) {
  if (left == NULL || right == NULL || left->length != right->length) {
    return left == right;
  }
  for (size_t i = 0; i < left->length; i++) {
    if (!TemplateArgumentPatternEqual(left->value.p[i], right->value.p[i])) {
      return false;
    }
  }
  return true;
}

typedef struct FunctionTemplateInstantiationCacheEntry {
  uint64_t argument_hash;
  uint64_t asm_name_hash;
  Symbol* symbol;
  struct FunctionTemplateInstantiationCacheEntry* argument_next;
  struct FunctionTemplateInstantiationCacheEntry* asm_name_next;
} FunctionTemplateInstantiationCacheEntry;

struct FunctionTemplateInstantiationCache {
  size_t bucket_count;
  size_t entry_count;
  FunctionTemplateInstantiationCacheEntry** argument_buckets;
  FunctionTemplateInstantiationCacheEntry** asm_name_buckets;
};

#define FUNCTION_TEMPLATE_CACHE_INITIAL_BUCKETS 16

static uint64_t HashFunctionTemplateCacheString(const char* value) {
  uint64_t hash = UINT64_C(1469598103934665603);
  const unsigned char* current = (const unsigned char*)value;
  while (current != NULL && *current != '\0') {
    hash ^= *current++;
    hash *= UINT64_C(1099511628211);
  }
  return hash;
}

static uint64_t HashTemplateArgumentVector(uint64_t hash, Vector* args) {
  hash ^= args != NULL ? args->length + 1 : 0;
  hash *= UINT64_C(1099511628211);
  for (size_t i = 0; args != NULL && i < args->length; i++) {
    hash = HashTemplateArgument(hash, args->value.p[i]);
  }
  return hash;
}

static uint64_t HashFunctionTemplateArguments(Symbol* templ, Vector* args) {
  (void)templ;
  return HashTemplateArgumentVector(UINT64_C(1469598103934665603), args);
}

static struct FunctionTemplateInstantiationCache*
NewFunctionTemplateInstantiationCache(void) {
  struct FunctionTemplateInstantiationCache* cache = malloc(sizeof(*cache));
  cache->bucket_count = FUNCTION_TEMPLATE_CACHE_INITIAL_BUCKETS;
  cache->entry_count = 0;
  cache->argument_buckets =
      calloc(cache->bucket_count, sizeof(*cache->argument_buckets));
  cache->asm_name_buckets =
      calloc(cache->bucket_count, sizeof(*cache->asm_name_buckets));
  return cache;
}

void FunctionTemplateInstantiationCacheDelete(
    struct FunctionTemplateInstantiationCache* cache) {
  if (cache == NULL) {
    return;
  }
  for (size_t i = 0; i < cache->bucket_count; i++) {
    FunctionTemplateInstantiationCacheEntry* entry =
        cache->argument_buckets[i];
    while (entry != NULL) {
      FunctionTemplateInstantiationCacheEntry* next = entry->argument_next;
      free(entry);
      entry = next;
    }
  }
  free(cache->argument_buckets);
  free(cache->asm_name_buckets);
  free(cache);
}

static void ResizeFunctionTemplateInstantiationCache(
    struct FunctionTemplateInstantiationCache* cache) {
  size_t new_bucket_count = cache->bucket_count * 2;
  FunctionTemplateInstantiationCacheEntry** argument_buckets =
      calloc(new_bucket_count, sizeof(*argument_buckets));
  FunctionTemplateInstantiationCacheEntry** asm_name_buckets =
      calloc(new_bucket_count, sizeof(*asm_name_buckets));
  for (size_t i = 0; i < cache->bucket_count; i++) {
    FunctionTemplateInstantiationCacheEntry* entry =
        cache->argument_buckets[i];
    while (entry != NULL) {
      FunctionTemplateInstantiationCacheEntry* next = entry->argument_next;
      size_t argument_bucket = entry->argument_hash % new_bucket_count;
      entry->argument_next = argument_buckets[argument_bucket];
      argument_buckets[argument_bucket] = entry;
      if (entry->symbol->asm_name.value != NULL &&
          *entry->symbol->asm_name.value != '\0') {
        size_t asm_name_bucket = entry->asm_name_hash % new_bucket_count;
        entry->asm_name_next = asm_name_buckets[asm_name_bucket];
        asm_name_buckets[asm_name_bucket] = entry;
      } else {
        entry->asm_name_next = NULL;
      }
      entry = next;
    }
  }
  free(cache->argument_buckets);
  free(cache->asm_name_buckets);
  cache->bucket_count = new_bucket_count;
  cache->argument_buckets = argument_buckets;
  cache->asm_name_buckets = asm_name_buckets;
}

static void IndexFunctionTemplateInstantiation(
    struct FunctionTemplateInstantiationCache* cache, Symbol* templ,
    Symbol* instantiated) {
  if (instantiated == NULL || instantiated->type == NULL ||
      !TypeIsFunction(instantiated->type) ||
      instantiated->type->template_arguments == NULL) {
    return;
  }
  if ((cache->entry_count + 1) * 4 > cache->bucket_count * 3) {
    ResizeFunctionTemplateInstantiationCache(cache);
  }
  FunctionTemplateInstantiationCacheEntry* entry = malloc(sizeof(*entry));
  entry->argument_hash = HashFunctionTemplateArguments(
      templ, instantiated->type->template_arguments);
  entry->symbol = instantiated;
  size_t argument_bucket = entry->argument_hash % cache->bucket_count;
  entry->argument_next = cache->argument_buckets[argument_bucket];
  cache->argument_buckets[argument_bucket] = entry;

  const char* asm_name = instantiated->asm_name.value;
  if (asm_name != NULL && *asm_name != '\0') {
    entry->asm_name_hash = HashFunctionTemplateCacheString(asm_name);
    size_t asm_name_bucket = entry->asm_name_hash % cache->bucket_count;
    entry->asm_name_next = cache->asm_name_buckets[asm_name_bucket];
    cache->asm_name_buckets[asm_name_bucket] = entry;
  } else {
    entry->asm_name_hash = 0;
    entry->asm_name_next = NULL;
  }
  cache->entry_count++;
}

static struct FunctionTemplateInstantiationCache*
EnsureFunctionTemplateInstantiationCache(Symbol* templ) {
  FunctionInfo* info = &templ->type->info.function;
  if (info->template_instantiation_cache != NULL) {
    return info->template_instantiation_cache;
  }
  info->template_instantiation_cache =
      NewFunctionTemplateInstantiationCache();
  for (size_t i = 0; i < info->template_instantiations.length; i++) {
    IndexFunctionTemplateInstantiation(
        info->template_instantiation_cache, templ,
        info->template_instantiations.value.p[i]);
  }
  return info->template_instantiation_cache;
}

/* A function template and its completed arguments uniquely identify an
 * instantiation. Comparing the complete function type is both redundant and
 * unreliable for a deduced (`auto`) return, which is filled in after the
 * instantiation enters the cache. */
static bool FunctionTemplateInstantiationMatches(Symbol* candidate,
                                                 Vector* args) {
  if (candidate == NULL || candidate->flags.is_template ||
      candidate->type == NULL || !TypeIsFunction(candidate->type) ||
      !TemplateArgumentVectorEqual(candidate->type->template_arguments, args)) {
    return false;
  }
  return true;
}

/* Search a function template's instantiation overload chain for one whose type
 * and template arguments match (cache lookup), or NULL. */
Symbol* FindFunctionTemplateInstantiation(Symbol* templ,
                                                 TypeRecord* type,
                                                 Vector* args) {
  if (templ == NULL || templ->type == NULL || !TypeIsFunction(templ->type) ||
      !TypeIsFunction(type)) {
    return NULL;
  }
  for (Symbol* candidate = templ->overload_next; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->type != NULL &&
        candidate->type->info.function.template_origin == templ &&
        FunctionTemplateInstantiationMatches(candidate, args)) {
      return candidate;
    }
  }
  struct FunctionTemplateInstantiationCache* cache =
      EnsureFunctionTemplateInstantiationCache(templ);
  uint64_t argument_hash = HashFunctionTemplateArguments(templ, args);
  size_t bucket = argument_hash % cache->bucket_count;
  for (FunctionTemplateInstantiationCacheEntry* entry =
           cache->argument_buckets[bucket];
       entry != NULL; entry = entry->argument_next) {
    Symbol* candidate = entry->symbol;
    if (entry->argument_hash != argument_hash) {
      continue;
    }
    if (FunctionTemplateInstantiationMatches(candidate, args)) {
      return candidate;
    }
  }
  return NULL;
}

Symbol* FindFunctionTemplateInstantiationByAsmName(Symbol* templ,
                                                          const char* asm_name) {
  if (asm_name == NULL || *asm_name == '\0') {
    return NULL;
  }
  for (Symbol* candidate = templ; candidate != NULL;
       candidate = candidate->overload_next) {
    if (!candidate->flags.is_template && candidate->asm_name.value != NULL &&
        strcmp(candidate->asm_name.value, asm_name) == 0) {
      return candidate;
    }
  }
  // Instantiations are recorded in the template's `template_instantiations`
  // cache (not the overload chain), so they must be searched here too.  The
  // mangled name is the definitive ABI identity of an instantiation: two
  // requests that mangle identically denote the same function even when their
  // deduced signatures fail a structural `TypeEqual` (e.g. a `*this` self-type
  // that lost its template_origin metadata), so reuse the cached one to avoid
  // emitting a duplicate symbol.
  if (templ != NULL && templ->type != NULL && TypeIsFunction(templ->type)) {
    struct FunctionTemplateInstantiationCache* cache =
        EnsureFunctionTemplateInstantiationCache(templ);
    uint64_t asm_name_hash = HashFunctionTemplateCacheString(asm_name);
    size_t bucket = asm_name_hash % cache->bucket_count;
    for (FunctionTemplateInstantiationCacheEntry* entry =
             cache->asm_name_buckets[bucket];
         entry != NULL; entry = entry->asm_name_next) {
      Symbol* candidate = entry->symbol;
      if (entry->asm_name_hash == asm_name_hash &&
          strcmp(candidate->asm_name.value, asm_name) == 0) {
        return candidate;
      }
    }
  }
  return NULL;
}

/* Append a newly created instantiation to the template's cache. */
void AppendFunctionTemplateInstantiation(Symbol* templ,
                                                Symbol* instantiated) {
  if (templ == NULL || templ->type == NULL || !TypeIsFunction(templ->type) ||
      instantiated == NULL) {
    return;
  }
  instantiated->overload_next = NULL;
  instantiated->flags.is_overloaded = false;
  VectorAppend(&templ->type->info.function.template_instantiations,
               instantiated);
  if (templ->type->info.function.template_instantiation_cache != NULL) {
    IndexFunctionTemplateInstantiation(
        templ->type->info.function.template_instantiation_cache, templ,
        instantiated);
  }
}

bool TypeIsUninitializedFriendly(TypeRecord* type) {
  if (TypeIsUnsignedChar(type)) {
    return true;
  }
  Enum* info = TypeIsEnum(type) ? type->info.enum_info : NULL;
  return info != NULL && info->tag_name != NULL &&
         strcmp(info->tag_name->value, "byte") == 0 &&
         SymbolIsInStdNamespace(info->tag_symbol);
}

bool TypeIsInt(TypeRecord* type);
bool TypeIsChar(TypeRecord* type);
bool TypeIsChar8(TypeRecord* type);
bool TypeIsChar16(TypeRecord* type);
bool TypeIsChar32(TypeRecord* type);
bool TypeIsCharFamily(TypeRecord* type);
bool TypeChar8IdentityDiffers(TypeRecord* left, TypeRecord* right);
bool TypeIsShort(TypeRecord* type);
bool TypeIsLong(TypeRecord* type);
bool TypeIsLongLong(TypeRecord* type);
bool TypeIsUnsignedInt(TypeRecord* type);
bool TypeIsUnsignedChar(TypeRecord* type);
bool TypeIsUnsignedShort(TypeRecord* type);
bool TypeIsUnsignedLong(TypeRecord* type);
bool TypeIsUnsignedLongLong(TypeRecord* type);
bool TypeIsFloat(TypeRecord* type);
bool TypeIsFloat32(TypeRecord* type);
bool TypeIsDouble(TypeRecord* type);
bool TypeIsFloat64(TypeRecord* type);
bool TypeIsLongDouble(TypeRecord* type);
bool TypeUsesFloat32Representation(TypeRecord* type);
bool TypeUsesFloat64Representation(TypeRecord* type);
bool TypeIsBool(TypeRecord* type);
bool TypeIsVoid(TypeRecord* type);
bool TypeIsNullPointer(TypeRecord* type);
bool TypeIsReflection(TypeRecord* type);
bool TypeIsBitInt(TypeRecord* type);
bool TypeContainsReflection(TypeRecord* type);

bool TypeIsPointer(TypeRecord* type);
bool TypeIsPrimitive(TypeRecord* type);
bool TypeIsPointerOrArray(TypeRecord* type);
bool TypeIsIntegral(TypeRecord* type);
bool TypeIsFloatingPoint(TypeRecord* type);
bool TypeIsFunction(TypeRecord* type);
bool TypeIsFunctionDefinition(TypeRecord* type);
bool TypeIsFunctionPointer(TypeRecord* type);
bool TypeIsStructOrUnionPointer(TypeRecord* type);
bool TypeIsFunctionReturningStructOrUnion(TypeRecord* type);
bool TypeIsVoidFunction(TypeRecord* type);

bool TypeIsPointerToSameType(TypeRecord* ptr1, TypeRecord* ptr2);
bool TypeIsStructOrUnion(TypeRecord* type);
bool TypeIsScalar(TypeRecord* type);
bool TypeIsVoidPointer(TypeRecord* type);
bool TypeIsArray(TypeRecord* type);
bool TypeIsConst(TypeRecord* type);
bool TypeIsVolatile(TypeRecord* type);
bool TypeIsAtomic(TypeRecord* type);
bool TypeIsEnum(TypeRecord* type);

bool TypeIsUnsigned(TypeRecord* type) {
  if (TypeIsChar8(type) || TypeIsChar16(type) || TypeIsChar32(type)) {
    return true;
  }
  if (!compiler->plain_char_is_signed && type->type == kTypeChar) {
    return true;
  }
  return TypeIsPrimitive(type) && (type->type & (kTypeUnsigned | kTypeBool)) != 0;
}

bool TypeIsSigned(TypeRecord* type) {
  if (compiler->plain_char_is_signed && type->type == kTypeChar) {
    return true;
  }
  return TypeIsPrimitive(type) && (type->type & kTypeSigned) != 0;
}

bool TypeIsReference(TypeRecord* type) {
  return type->declarator == kDeclReference ||
         type->declarator == kDeclRValueReference;
}

bool TypeIsMemberPointer(TypeRecord* type) {
  return type != NULL && type->declarator == kDeclMemberPointer;
}

bool TypeIsScopedEnum(TypeRecord* type) {
  return TypeIsEnum(type) && type->info.enum_info != NULL &&
         type->info.enum_info->is_scoped;
}


bool TypeIsIntConstant(TypeRecord* type);
bool TypeIsFloatingPointConstant(TypeRecord* type);
bool TypeIsUnknown(TypeRecord* type);
bool TypeIsFixedArray(TypeRecord* type);
bool TypeIsVLA(TypeRecord* type);

static bool FunctionPrototypesEqual(FunctionInfo* a, FunctionInfo* b) {
  if (a->prototype.length != b->prototype.length) {
    return false;
  }
  // The trailing const on a C++ member function is part of its signature: a
  // non-const and a const member function with otherwise identical parameters
  // are distinct overloads.  This matters for dependent return types (e.g.
  // `T&` vs `const T&`) where the return type comparison cannot tell them
  // apart, leaving the const qualifier as the only distinguishing feature.
  if (a->is_const_member != b->is_const_member) {
    return false;
  }
  if (a->is_volatile_member != b->is_volatile_member) {
    return false;
  }
  if (a->ref_qualifier != b->ref_qualifier) {
    return false;
  }
  if (a->is_noexcept != b->is_noexcept) {
    return false;
  }
  for (size_t i = 0; i < a->prototype.length; i++) {
    Symbol* s1 = a->prototype.value.p[i];
    Symbol* s2 = b->prototype.value.p[i];
    if (!TypeEqual(s1->type, s2->type)) {
      return false;
    }
  }
  return true;
}

static bool CXXStructTagNameEqual(Struct* left, Struct* right) {
  if (left == NULL || right == NULL || left->tag_name == NULL ||
      right->tag_name == NULL) {
    return false;
  }
  // Closure types produced from the same lambda expression in different
  // enclosing template instantiations can have different capture layouts.
  // Their `$S...` suffix is therefore semantic type identity, not merely a
  // duplicate materialization suffix.
  if (strncmp(left->tag_name->value, "__invented__", 12) == 0 ||
      strncmp(right->tag_name->value, "__invented__", 12) == 0) {
    return StringEqualString(left->tag_name, right->tag_name);
  }
  // Materialized template specializations can acquire different internal
  // `$S...` suffixes even though they denote the same C++ type.  Ignore only
  // that implementation suffix; template arguments before it remain part of
  // the type identity.
  size_t left_len = strcspn(left->tag_name->value, "$");
  size_t right_len = strcspn(right->tag_name->value, "$");
  return left_len == right_len &&
         strncmp(left->tag_name->value, right->tag_name->value, left_len) == 0;
}

static bool CXXStructSameTemplateFamilyForTypeEquality(Struct* left,
                                                       Struct* right);

static bool CXXTemplateOriginsSameFamily(Symbol* left, Symbol* right) {
  if (left == right) {
    return true;
  }
  if (left == NULL || right == NULL ||
      !StringEqualString(&left->name, &right->name)) {
    return false;
  }
  Struct* left_struct =
      left->type != NULL && TypeIsStructOrUnion(left->type)
          ? left->type->info.struct_info
          : NULL;
  Struct* right_struct =
      right->type != NULL && TypeIsStructOrUnion(right->type)
          ? right->type->info.struct_info
          : NULL;
  Struct* left_parent =
      left_struct != NULL ? left_struct->lexical_parent : NULL;
  Struct* right_parent =
      right_struct != NULL ? right_struct->lexical_parent : NULL;
  if (left_parent != NULL || right_parent != NULL) {
    return left_parent != NULL && right_parent != NULL &&
           CXXStructSameTemplateFamilyForTypeEquality(left_parent,
                                                      right_parent);
  }
  return left->namespace_ == right->namespace_;
}

static bool CXXStructSameTemplateFamilyForTypeEquality(Struct* left,
                                                       Struct* right) {
  if (left == right) {
    return true;
  }
  Symbol* left_origin =
      left != NULL && left->tag_symbol != NULL && left->tag_symbol->type != NULL
          ? left->tag_symbol->type->template_origin
          : NULL;
  Symbol* right_origin =
      right != NULL && right->tag_symbol != NULL && right->tag_symbol->type != NULL
          ? right->tag_symbol->type->template_origin
          : NULL;
  if (left_origin != NULL || right_origin != NULL) {
    if (left_origin != NULL && right_origin != NULL) {
      if (!CXXTemplateOriginsSameFamily(left_origin, right_origin)) {
        return false;
      }
      TypeRecord* left_tag_type =
          left->tag_symbol != NULL ? left->tag_symbol->type : NULL;
      TypeRecord* right_tag_type =
          right->tag_symbol != NULL ? right->tag_symbol->type : NULL;
      if (left_tag_type != NULL && right_tag_type != NULL &&
          left_tag_type->template_arguments != NULL &&
          right_tag_type->template_arguments != NULL) {
        return TemplateArgumentVectorEqual(left_tag_type->template_arguments,
                                           right_tag_type->template_arguments);
      }
      if (left->tag_name != NULL && right->tag_name != NULL &&
          strchr(left->tag_name->value, '<') != NULL &&
          strchr(right->tag_name->value, '<') != NULL) {
        return strcmp(left->tag_name->value, right->tag_name->value) == 0;
      }
      return true;
    }
    Symbol* origin = left_origin != NULL ? left_origin : right_origin;
    Struct* other = left_origin != NULL ? right : left;
    if (other == NULL || other->tag_symbol == NULL) {
      return false;
    }
    size_t other_name_len = strcspn(other->tag_symbol->name.value, "<$");
    return origin->name.length == other_name_len &&
           strncmp(origin->name.value, other->tag_symbol->name.value,
                   other_name_len) == 0;
  }
  if (!CXXStructTagNameEqual(left, right)) {
    return false;
  }
  if (left->lexical_parent != NULL || right->lexical_parent != NULL) {
    return CXXStructSameTemplateFamilyForTypeEquality(left->lexical_parent,
                                                     right->lexical_parent);
  }
  return true;
}

// C and C++ allow the `int` keyword to be omitted from signed and unsigned
// integer type specifiers.  The parser preserves the spelling in Type bits, so
// canonicalize those equivalent spellings before comparing types.
static Type CanonicalPrimitiveType(Type type) {
  const Type integer_specifiers =
      kTypeInt | kTypeShort | kTypeLong | kTypeLongLong |
      kTypeSigned | kTypeUnsigned;
  if ((type & integer_specifiers) == 0 ||
      (type & ~integer_specifiers) != 0) {
    return type;
  }
  return (type | kTypeInt) & ~kTypeSigned;
}

static uint64_t HashTypeValue(uint64_t hash, uint64_t value) {
  hash ^= value;
  hash *= UINT64_C(1099511628211);
  return hash;
}

static uint64_t HashTypeBytes(uint64_t hash, const char* value,
                              size_t length) {
  for (size_t i = 0; value != NULL && i < length; i++) {
    hash = HashTypeValue(hash, (unsigned char)value[i]);
  }
  return HashTypeValue(hash, length);
}

static uint64_t HashDependentTemplateArgExpr(uint64_t hash, ASTNode* expr) {
  if (expr == NULL) {
    return HashTypeValue(hash, 0);
  }
  hash = HashTypeValue(hash, (uint64_t)expr->op + 1);
  switch (expr->op) {
    case AST_OP(number):
    case AST_OP(charconst):
    case AST_OP(charwide):
      return HashTypeValue(hash, (uint64_t)((ConstantASTNode*)expr)->value.ivalue);
    case AST_OP(identifier):
      // Distinct symbols can compare equal by template-parameter index or by
      // dependent qualified-name structure. Keep this deliberately coarse.
      return hash;
    case AST_OP(not):
    case AST_OP(noexcept_expr):
    case AST_OP(onescomp):
    case AST_OP(uminus):
    case AST_OP(uplus):
      return HashDependentTemplateArgExpr(hash, ((UnaryASTNode*)expr)->sub);
    case AST_OP(plus):
    case AST_OP(minus):
    case AST_OP(mult):
    case AST_OP(div):
    case AST_OP(mod):
    case AST_OP(lshift):
    case AST_OP(rshifta):
    case AST_OP(rshiftl):
    case AST_OP(less):
    case AST_OP(lesseq):
    case AST_OP(greater):
    case AST_OP(greatereq):
    case AST_OP(equal):
    case AST_OP(noteq):
    case AST_OP(and):
    case AST_OP(bitor):
    case AST_OP(exor):
    case AST_OP(logand):
    case AST_OP(logor): {
      BinaryASTNode* binary = (BinaryASTNode*)expr;
      hash = HashDependentTemplateArgExpr(hash, binary->left);
      return HashDependentTemplateArgExpr(hash, binary->right);
    }
    case AST_OP(cast): {
      CastASTNode* cast = (CastASTNode*)expr;
      hash = HashTypeRecord(hash, cast->cast_type);
      return HashDependentTemplateArgExpr(hash, cast->expr);
    }
    case AST_OP(sizeof):
    case AST_OP(alignof): {
      if (ASTNodeGetShape(expr) != kASTShapeSizeof) {
        return hash;
      }
      SizeofASTNode* size = (SizeofASTNode*)expr;
      hash = HashTypeValue(hash, size->is_pack_size);
      hash = HashDependentTemplateArgExpr(hash, size->expr);
      return HashTypeRecord(hash, size->type_operand);
    }
    default:
      break;
  }
  switch (ASTNodeGetShape(expr)) {
    case kASTShapeUnary:
      return HashDependentTemplateArgExpr(hash, ((UnaryASTNode*)expr)->sub);
    case kASTShapeBinary: {
      BinaryASTNode* binary = (BinaryASTNode*)expr;
      hash = HashDependentTemplateArgExpr(hash, binary->left);
      return HashDependentTemplateArgExpr(hash, binary->right);
    }
    case kASTShapeVector: {
      VectorASTNode* vector = (VectorASTNode*)expr;
      hash = HashDependentTemplateArgExpr(hash, vector->left);
      hash = HashTypeValue(
          hash, vector->children != NULL ? vector->children->length + 1 : 0);
      for (size_t i = 0;
           vector->children != NULL && i < vector->children->length; i++) {
        hash = HashDependentTemplateArgExpr(
            hash, vector->children->value.p[i]);
      }
      return hash;
    }
    default:
      return hash;
  }
}

static uint64_t HashStructForTypeEquality(uint64_t hash, Struct* str) {
  if (str == NULL) {
    return HashTypeValue(hash, 0);
  }
  if (str->tag_name == NULL) {
    Symbol* origin =
        str->tag_symbol != NULL && str->tag_symbol->type != NULL
            ? str->tag_symbol->type->template_origin
            : NULL;
    if (origin != NULL) {
      return HashTypeBytes(hash, origin->name.value, origin->name.length);
    }
    return HashTypeValue(hash, 1);
  }
  const char* name = str->tag_name->value;
  size_t length = strncmp(name, "__invented__", 12) == 0
                      ? str->tag_name->length
                      : strcspn(name, "$");
  return HashTypeBytes(hash, name, length);
}

static uint64_t HashTemplateArgument(uint64_t hash, TemplateArgument* arg) {
  if (arg == NULL) {
    return HashTypeValue(hash, 0);
  }
  hash = HashTypeValue(hash, (uint64_t)arg->kind + 1);
  if (arg->pack_arguments != NULL) {
    hash = HashTypeValue(hash, UINT64_C(0x7061636b));
    return HashTemplateArgumentVector(hash, arg->pack_arguments);
  }
  if (arg->kind == kTemplateParameterType) {
    return HashTypeRecord(hash, arg->type);
  }
  if (arg->kind == kTemplateParameterTemplate) {
    hash = HashTypeValue(hash, (uintptr_t)arg->template_symbol);
    hash = HashTypeValue(hash,
                         (uint64_t)(uint32_t)arg->template_parameter_index);
    return HashDependentTemplateArgExpr(hash, arg->pack_index_expr);
  }
  if (arg->dependent_expr != NULL) {
    return HashDependentTemplateArgExpr(hash, arg->dependent_expr);
  }
  hash = HashTypeValue(hash,
                       (uint64_t)(uint32_t)arg->template_parameter_index);
  TemplateValueKind kind = TemplateArgumentConcreteValueKind(arg);
  hash = HashTypeValue(hash, (uint64_t)kind);
  switch (kind) {
    case kTemplateValueIntegral:
      return HashTypeValue(hash, (uint64_t)arg->int_value);
    case kTemplateValuePointer:
      hash = HashTypeValue(hash, (uintptr_t)arg->value_symbol);
      return HashTypeValue(hash, (uint64_t)arg->value_offset);
    case kTemplateValueMemberPointer:
      hash = HashTypeValue(hash, (uintptr_t)arg->value_symbol);
      hash = HashTypeValue(hash, (uint64_t)arg->value_offset);
      hash = HashTypeValue(hash, (uint64_t)arg->value_adjustment);
      return HashTypeValue(hash, (uintptr_t)arg->member_function);
    case kTemplateValueNull:
    case kTemplateValueReflection:
    case kTemplateValueObject:
    case kTemplateValueNone:
      // Equality for these values is resolved by the bucket collision check.
      return hash;
  }
  return hash;
}

static uint64_t HashTypeRecord(uint64_t hash, TypeRecord* type) {
  if (type == NULL) {
    return HashTypeValue(hash, 0);
  }
  hash = HashTypeValue(hash, (uint64_t)type->declarator + 1);
  hash = HashTypeValue(hash, type->qualifiers & ~kQualRestrict);
  hash = HashTypeValue(hash, type->is_pack_index);
  if (type->is_pack_index) {
    hash = HashDependentTemplateArgExpr(hash, type->pack_index_expr);
    hash = HashTemplateArgument(hash, type->pack_index_pack);
  }
  if (type->declarator == kDeclPrimitive &&
      (type->type & kTypeUnknown) != 0) {
    if (type->template_parameter_index >= 0) {
      hash = HashTypeValue(
          hash, (uint64_t)(uint32_t)type->template_parameter_index);
      if (type->dependent_member_name != NULL) {
        hash = HashTypeBytes(hash, type->dependent_member_name->value,
                             type->dependent_member_name->length);
      }
    } else if (type->template_origin != NULL) {
      hash = HashTypeValue(hash, (uintptr_t)type->template_origin);
      if (type->dependent_member_name != NULL) {
        hash = HashTypeBytes(hash, type->dependent_member_name->value,
                             type->dependent_member_name->length);
      }
      hash = HashTemplateArgumentVector(hash, type->template_arguments);
    }
    return hash;
  }
  switch (type->declarator) {
    case kDeclArray:
      // TypeArrayBoundsEqual intentionally lets a VLA match any non-dependent
      // bound. Hash only the element type so that lenient comparison can never
      // produce a cache false negative; exact bounds are checked in the bucket.
      return HashTypeRecord(hash, type->next);
    case kDeclPointer:
    case kDeclReference:
    case kDeclRValueReference:
      return HashTypeRecord(hash, type->next);
    case kDeclMemberPointer:
      if (type->template_parameter_index >= 0) {
        hash = HashTypeValue(
            hash, (uint64_t)(uint32_t)type->template_parameter_index);
      } else {
        hash = HashTypeValue(hash, (uintptr_t)type->info.struct_info);
      }
      return HashTypeRecord(hash, type->next);
    case kDeclFunction:
      hash = HashTypeRecord(hash, type->next);
      hash = HashTypeValue(hash, type->info.function.prototype.length);
      hash = HashTypeValue(hash, type->info.function.is_const_member);
      hash = HashTypeValue(hash, type->info.function.is_volatile_member);
      hash = HashTypeValue(hash, type->info.function.ref_qualifier);
      hash = HashTypeValue(hash, type->info.function.is_noexcept);
      for (size_t i = 0; i < type->info.function.prototype.length; i++) {
        Symbol* formal = type->info.function.prototype.value.p[i];
        hash = HashTypeRecord(hash, formal != NULL ? formal->type : NULL);
      }
      return hash;
    case kDeclPrimitive:
      break;
  }
  if (TypeIsBitInt(type) && !TypeIsEnum(type)) {
    hash = HashTypeValue(hash, kTypeBitInt);
    hash = HashTypeValue(hash, (uint64_t)(uint32_t)type->bit_width);
    return HashTypeValue(hash, TypeIsUnsigned(type));
  }
  if (TypeIsStructOrUnion(type)) {
    hash = HashTypeValue(hash, type->type);
    Symbol* origin = type->template_origin;
    if (origin == NULL && type->info.struct_info != NULL &&
        type->info.struct_info->tag_symbol != NULL &&
        type->info.struct_info->tag_symbol->type != NULL) {
      origin =
          type->info.struct_info->tag_symbol->type->template_origin;
    }
    Vector* specialization_args =
        TypeSpecializationTemplateArguments(type);
    if (origin != NULL && specialization_args != NULL) {
      hash = HashTypeBytes(hash, origin->name.value, origin->name.length);
      return HashTemplateArgumentVector(hash, specialization_args);
    }
    return HashStructForTypeEquality(hash, type->info.struct_info);
  }
  if (TypeIsEnum(type)) {
    if (TypeIsScopedEnum(type)) {
      return HashTypeValue(hash, (uintptr_t)type->info.enum_info);
    }
    Type enum_type =
        type->type & ~(kTypeInt | kTypeChar | kTypeSigned | kTypeUnsigned);
    hash = HashTypeValue(hash, enum_type);
    if ((type->type & kTypeBitInt) != 0) {
      hash = HashTypeValue(hash, (uint64_t)(uint32_t)type->bit_width);
    }
    return hash;
  }
  return HashTypeValue(hash, CanonicalPrimitiveType(type->type));
}

bool TypeEqual(TypeRecord* t1, TypeRecord* t2) {
  if (t1 == t2) {
    return true;
  }
  if (t1 == NULL || t2 == NULL) {
    return false;
  }
  if (compiler != NULL) {
    TypeRecord* resolved1 =
        TypeRecordTryResolveTraitPlaceholder(&compiler->syntax, t1);
    TypeRecord* resolved2 =
        TypeRecordTryResolveTraitPlaceholder(&compiler->syntax, t2);
    if (resolved1 != NULL || resolved2 != NULL) {
      bool equal = TypeEqual(resolved1 != NULL ? resolved1 : t1,
                             resolved2 != NULL ? resolved2 : t2);
      if (resolved1 != NULL) {
        TypeRecordDelete(resolved1);
      }
      if (resolved2 != NULL) {
        TypeRecordDelete(resolved2);
      }
      return equal;
    }
  }
  if (t1->declarator != t2->declarator) {
    return false;
  }
  // `restrict` is an optimizer hint, not part of a type's identity for
  // redeclaration or overload purposes (e.g. `const char *restrict` and
  // `const char *` name the same parameter type), so ignore it when comparing
  // qualifiers.  The remaining cv-qualifiers stay significant so that template
  // argument identities such as `const T` versus `T` remain distinct.
  if ((t1->qualifiers & ~kQualRestrict) != (t2->qualifiers & ~kQualRestrict)) {
    return false;
  }
  if (t1->is_pack_index != t2->is_pack_index) {
    return false;
  }
  if (t1->is_pack_index &&
      (!DependentTemplateArgExprEqual(t1->pack_index_expr,
                                      t2->pack_index_expr) ||
       !TemplateArgumentEqual(t1->pack_index_pack, t2->pack_index_pack))) {
      return false;
  }
  // Dependent (unknown) leaves need care.  Only a leaf primitive carries a
  // template parameter's positional identity, so pointer/reference/array
  // wrappers fall through to the structural comparison below and recurse into
  // `next` (their declarator shapes already matched); function types likewise
  // compare their full signature via FunctionPrototypesEqual.  At a dependent
  // leaf, distinct template parameters (`T` vs `U`), a parameter versus a
  // concrete type, and a parameter versus a dependent member type (`T` vs
  // `T::type`) are all different signatures and must stay distinct so that
  // overloads like `f(const T&)` and `f(const U&)` do not collide.  Only when
  // neither leaf is a positionally-identified parameter do we keep the lenient
  // "unknown matches anything" behavior, so that an earlier error involving a
  // genuinely unresolved symbol does not cascade into a spurious overload
  // clash.
  if (t1->declarator == kDeclPrimitive &&
      ((t1->type & kTypeUnknown) != 0 || (t2->type & kTypeUnknown) != 0)) {
    bool t1_param = t1->template_parameter_index >= 0;
    bool t2_param = t2->template_parameter_index >= 0;
    if (t1_param || t2_param) {
      if (t1_param != t2_param ||
          t1->template_parameter_index != t2->template_parameter_index) {
        return false;
      }
      String* m1 = t1->dependent_member_name;
      String* m2 = t2->dependent_member_name;
      if ((m1 == NULL) != (m2 == NULL)) {
        return false;
      }
      return m1 == NULL || StringEqualString(m1, m2);
    }
    // A dependent member of a template-id scope (`enable_if<Cond, T>::type`):
    // the scope template, member name, and the scope's arguments -- which may
    // include value-dependent SFINAE conditions -- distinguish otherwise
    // identically-spelled unknown leaves (so two enable_if-guarded overloads are
    // not treated as one).
    if (t1->template_origin != NULL && t2->template_origin != NULL) {
      if (t1->template_origin != t2->template_origin) {
        return false;
      }
      String* m1 = t1->dependent_member_name;
      String* m2 = t2->dependent_member_name;
      if ((m1 == NULL) != (m2 == NULL)) {
        return false;
      }
      if (m1 != NULL && !StringEqualString(m1, m2)) {
        return false;
      }
      return TemplateArgumentVectorEqual(t1->template_arguments,
                                         t2->template_arguments);
    }
    return true;
  }
  switch (t1->declarator) {
    case kDeclArray:
      if (!TypeEqual(t1->next, t2->next)) {
        return false;
      }
      return TypeArrayBoundsEqual(&t1->info.array, &t2->info.array);
    case kDeclPointer:
    case kDeclReference:
    case kDeclRValueReference:
      return TypeEqual(t1->next, t2->next);
    case kDeclMemberPointer:
      if (t1->template_parameter_index >= 0 ||
          t2->template_parameter_index >= 0) {
        if (t1->template_parameter_index != t2->template_parameter_index) {
          return false;
        }
      } else if (t1->info.struct_info != t2->info.struct_info) {
        return false;
      }
      return TypeEqual(t1->next, t2->next);

    case kDeclFunction:
      if (!TypeEqual(t1->next, t2->next)) {
        return false;
      }
      return FunctionPrototypesEqual(&t1->info.function, &t2->info.function);
    case kDeclPrimitive:
      if ((TypeIsBitInt(t1) || TypeIsBitInt(t2)) &&
          !TypeIsEnum(t1) && !TypeIsEnum(t2)) {
        return TypeIsBitInt(t1) && TypeIsBitInt(t2) &&
               t1->bit_width == t2->bit_width &&
               TypeIsUnsigned(t1) == TypeIsUnsigned(t2);
      }
      if (TypeIsStructOrUnion(t1) || TypeIsStructOrUnion(t2)) {
        if (!TypeIsStructOrUnion(t1) || !TypeIsStructOrUnion(t2) ||
            t1->type != t2->type || t1->qualifiers != t2->qualifiers) {
          return false;
        }
        if (t1->template_origin != NULL || t2->template_origin != NULL) {
          // Deferred dependent-member template-ids (`X<Args>::iterator` vs
          // `X<Args>::const_iterator`) share origin and arguments but name
          // different members, so the member name is part of their identity.
          bool same_dependent_member =
              ((t1->dependent_member_name == NULL &&
                t2->dependent_member_name == NULL) ||
               (t1->dependent_member_name != NULL &&
                t2->dependent_member_name != NULL &&
                StringEqualString(t1->dependent_member_name,
                                  t2->dependent_member_name))) &&
              TemplateArgumentVectorListEqual(
                  t1->dependent_member_template_arguments,
                  t2->dependent_member_template_arguments);
          if (t1->template_origin == t2->template_origin &&
              same_dependent_member &&
              TemplateArgumentVectorEqual(t1->template_arguments,
                                          t2->template_arguments)) {
            return true;
          }
          // When both operands are template-ids that carry explicit arguments,
          // their identity is exactly the (origin, arguments) pair, so an
          // argument mismatch is decisive.  Must NOT fall through to the
          // struct-family check below: two still-dependent template-ids such as
          // `Iter<T, 0>` and `Iter<T, 1>` share the primary template's Struct on
          // both sides, and that check short-circuits on the shared Struct
          // pointer -- wrongly treating differing type or non-type arguments as
          // equal (and collapsing overloads keyed on them).
          if (t1->template_origin != NULL && t2->template_origin != NULL &&
              t1->template_arguments != NULL &&
              t2->template_arguments != NULL) {
            bool same_family = CXXTemplateOriginsSameFamily(
                t1->template_origin, t2->template_origin);
            return same_family && same_dependent_member &&
                   TemplateArgumentVectorEqual(t1->template_arguments,
                                               t2->template_arguments);
          }
          // Some substitution paths preserve the specialization's concrete
          // Struct but not the outer TypeRecord's template metadata.  Compare
          // the full specialization spelling (minus its internal unique
          // suffix), never merely its data layout or primary-template name.
          return CXXStructSameTemplateFamilyForTypeEquality(
              t1->info.struct_info, t2->info.struct_info);
        }
        return CXXStructSameTemplateFamilyForTypeEquality(t1->info.struct_info,
                                                          t2->info.struct_info);
      }
      if (TypeIsEnum(t1) && TypeIsEnum(t2)) {
        if (TypeIsScopedEnum(t1) || TypeIsScopedEnum(t2)) {
          return t1->info.enum_info == t2->info.enum_info &&
                 t1->qualifiers == t2->qualifiers;
        }
        // Enums can be char, signed int or unsigned int.
        int e1 = t1->type & ~(kTypeInt | kTypeChar | kTypeSigned | kTypeUnsigned);
        int e2 = t2->type & ~(kTypeInt | kTypeChar | kTypeSigned | kTypeUnsigned);
        return e1 == e2 && t1->qualifiers == t2->qualifiers &&
               (((t1->type | t2->type) & kTypeBitInt) == 0 ||
                t1->bit_width == t2->bit_width);

      }
      return CanonicalPrimitiveType(t1->type) ==
                 CanonicalPrimitiveType(t2->type) &&
             t1->qualifiers == t2->qualifiers;
  }
}

bool TypeEqualIgnoringTopLevelQualifierMask(TypeRecord* t1, TypeRecord* t2,
                                             Qualifiers ignored) {
  if (t1 == NULL || t2 == NULL) {
    return t1 == t2;
  }
  TypeRecord left = *t1;
  TypeRecord right = *t2;
  left.qualifiers &= ~ignored;
  right.qualifiers &= ~ignored;
  return TypeEqual(&left, &right);
}

bool TypeEqualIgnoringFunctionNoexcept(TypeRecord* t1, TypeRecord* t2) {
  if (!TypeIsFunction(t1) || !TypeIsFunction(t2)) {
    return TypeEqual(t1, t2);
  }
  TypeRecord left = *t1;
  TypeRecord right = *t2;
  left.info.function.is_noexcept = false;
  right.info.function.is_noexcept = false;
  return TypeEqual(&left, &right);
}

bool TypeEqualForCXXOverride(struct Syntax* syntax, TypeRecord* t1,
                             TypeRecord* t2) {
  if (t1 == t2) {
    return true;
  }
  TypeRecord* left = t1;
  TypeRecord* right = t2;
  if (syntax != NULL && CompilerIsCXX()) {
    left = TypeMaterializeClassTemplateSpecialization(syntax, t1);
    right = TypeMaterializeClassTemplateSpecialization(syntax, t2);
  }
  bool equal = TypeEqual(left, right);
  if (left != t1) {
    TypeRecordDelete(left);
  }
  if (right != t2) {
    TypeRecordDelete(right);
  }
  return equal;
}

bool StructIsDerivedFrom(Struct* from, Struct* to, bool public_only) {
  if (from == NULL || to == NULL) {
    return false;
  }
  for (size_t i = 0; i < from->bases.length; i++) {
    CXXBaseSpecifier* base = from->bases.value.p[i];
    if ((public_only && base->access != kAccessPublic) ||
        base->type == NULL || !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    if (base->type->info.struct_info == to ||
        StructIsDerivedFrom(base->type->info.struct_info, to, public_only)) {
      return true;
    }
  }
  return false;
}

int StructCountPublicDerivationPaths(Struct* from, Struct* to) {
  if (from == NULL || to == NULL) {
    return 0;
  }
  if (from == to) {
    return 1;
  }
  int count = 0;
  for (size_t i = 0; i < from->bases.length; i++) {
    CXXBaseSpecifier* base = from->bases.value.p[i];
    if (base->access != kAccessPublic || base->type == NULL ||
        !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    count += StructCountPublicDerivationPaths(base->type->info.struct_info, to);
  }
  return count;
}

bool TypeIsDerivedFrom(TypeRecord* from, TypeRecord* to) {
  if (from == NULL || to == NULL || !TypeIsStructOrUnion(from) ||
      !TypeIsStructOrUnion(to) || from->info.struct_info == NULL ||
      to->info.struct_info == NULL) {
    return false;
  }
  return StructIsDerivedFrom(from->info.struct_info, to->info.struct_info,
                             /*public_only=*/true);
}

static bool StructBaseOffset(Struct* from, Struct* to, bool public_only,
                             int inherited_offset, int* offset) {
  if (from == NULL || to == NULL) {
    return false;
  }
  for (size_t i = 0; i < from->bases.length; i++) {
    CXXBaseSpecifier* base = from->bases.value.p[i];
    if ((public_only && base->access != kAccessPublic) ||
        base->type == NULL || !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    int base_offset = inherited_offset + base->byte_offset;
    if (base->type->info.struct_info == to) {
      if (offset != NULL) {
        *offset = base_offset;
      }
      return true;
    }
    if (StructBaseOffset(base->type->info.struct_info, to, public_only,
                         base_offset, offset)) {
      return true;
    }
  }
  return false;
}

bool TypeBaseOffset(TypeRecord* from, TypeRecord* to, bool public_only,
                    int* offset) {
  if (from == NULL || to == NULL || !TypeIsStructOrUnion(from) ||
      !TypeIsStructOrUnion(to) || from->info.struct_info == NULL ||
      to->info.struct_info == NULL) {
    return false;
  }
  if (from->info.struct_info == to->info.struct_info) {
    if (offset != NULL) {
      *offset = 0;
    }
    return true;
  }
  return StructBaseOffset(from->info.struct_info, to->info.struct_info,
                          public_only, 0, offset);
}

static void CXXBaseAdjustmentSet(CXXBaseAdjustment* adjustment,
                                 CXXBaseAdjustmentKind kind,
                                 int byte_offset,
                                 int vbtable_index) {
  if (adjustment == NULL) {
    return;
  }
  adjustment->kind = kind;
  adjustment->byte_offset = byte_offset;
  adjustment->vbtable_index = vbtable_index;
}

static bool StructNonVirtualBaseOffset(Struct* from, Struct* to,
                                       bool public_only,
                                       int inherited_offset,
                                       int* offset) {
  if (from == NULL || to == NULL) {
    return false;
  }
  for (size_t i = 0; i < from->bases.length; i++) {
    CXXBaseSpecifier* base = from->bases.value.p[i];
    if (base->is_virtual ||
        (public_only && base->access != kAccessPublic) ||
        base->type == NULL || !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    int base_offset = inherited_offset + base->byte_offset;
    if (base->type->info.struct_info == to) {
      if (offset != NULL) {
        *offset = base_offset;
      }
      return true;
    }
    if (StructNonVirtualBaseOffset(base->type->info.struct_info, to,
                                   public_only, base_offset, offset)) {
      return true;
    }
  }
  return false;
}

static bool StructBaseAdjustment(Struct* from, Struct* to, bool public_only,
                                 int inherited_offset,
                                 CXXBaseAdjustment* adjustment) {
  if (from == NULL || to == NULL) {
    return false;
  }
  for (size_t i = 0; i < from->virtual_bases.length; i++) {
    CXXVirtualBaseInfo* base = from->virtual_bases.value.p[i];
    if ((public_only && base->access != kAccessPublic) ||
        base->type == NULL || !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    Struct* base_struct = base->type->info.struct_info;
    int virtual_base_offset = 0;
    if (base_struct == to ||
        StructNonVirtualBaseOffset(base_struct, to, public_only, 0,
                                   &virtual_base_offset)) {
      CXXBaseAdjustmentSet(adjustment, kCXXBaseAdjustmentVirtual,
                           virtual_base_offset, base->vbtable_index);
      return true;
    }
  }
  for (size_t i = 0; i < from->bases.length; i++) {
    CXXBaseSpecifier* base = from->bases.value.p[i];
    if ((public_only && base->access != kAccessPublic) ||
        base->type == NULL || !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    if (base->is_virtual) {
      continue;
    }
    int base_offset = inherited_offset + base->byte_offset;
    if (base->type->info.struct_info == to) {
      CXXBaseAdjustmentSet(adjustment, kCXXBaseAdjustmentStatic,
                           base_offset, -1);
      return true;
    }
    if (StructBaseAdjustment(base->type->info.struct_info, to, public_only,
                             base_offset, adjustment)) {
      return true;
    }
  }
  return false;
}

bool TypeBaseAdjustment(TypeRecord* from, TypeRecord* to, bool public_only,
                        CXXBaseAdjustment* adjustment) {
  if (from == NULL || to == NULL || !TypeIsStructOrUnion(from) ||
      !TypeIsStructOrUnion(to) || from->info.struct_info == NULL ||
      to->info.struct_info == NULL) {
    return false;
  }
  if (from->info.struct_info == to->info.struct_info) {
    CXXBaseAdjustmentSet(adjustment, kCXXBaseAdjustmentNone, 0, -1);
    return true;
  }
  return StructBaseAdjustment(from->info.struct_info, to->info.struct_info,
                              public_only, 0, adjustment);
}

bool TypeIsAbstractClass(TypeRecord* type) {
  return CompilerIsCXX() && type != NULL && TypeIsStructOrUnion(type) &&
         type->info.struct_info != NULL && type->info.struct_info->is_abstract;
}

bool TypeContainsAuto(TypeRecord* type) {
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if ((t->type & kTypeAuto) != 0) {
      return true;
    }
  }
  return false;
}

bool TypeFunctionReturnContainsAuto(TypeRecord* type) {
  return TypeIsFunction(type) && TypeContainsAuto(type->next);
}

static TypeRecord* NewDeclaratorLike(TypeRecord* pattern) {
  TypeRecord* result = NULL;
  switch (pattern->declarator) {
    case kDeclPointer:
      result = NewPointerTypeRecord(pattern->qualifiers);
      break;
    case kDeclReference:
      result = NewReferenceTypeRecord(pattern->qualifiers, false);
      break;
    case kDeclRValueReference:
      result = NewReferenceTypeRecord(pattern->qualifiers, true);
      break;
    case kDeclArray:
      result = TypeRecordCopy(pattern);
      TypeRecordDecRef(result->next);
      result->next = NULL;
      break;
    default:
      result = TypeRecordCopy(pattern);
      break;
  }
  return result;
}

// Apply the [temp.deduct.call] adjustments used when deducing a bare
// by-value template parameter.  C++23 `auto(expr)` uses exactly these rules:
// arrays and functions decay to pointers, references are removed, and
// top-level cv-qualification does not participate in deduction.
TypeRecord* TypeDecayForByValueDeduction(TypeRecord* type) {
  if (type == NULL) {
    return NULL;
  }
  while (TypeIsReference(type) && type->next != NULL) {
    type = type->next;
  }
  TypeRecord* result = NULL;
  if (TypeIsArray(type) && type->next != NULL) {
    result = NewPointerTo(kQualPlain, TypeRecordCopy(type->next));
  } else if (TypeIsFunction(type)) {
    result = NewPointerTo(kQualPlain, TypeRecordCopy(type));
  } else {
    result = TypeRecordCopy(type);
    result->qualifiers = kQualPlain;
  }
  TypeRecordCalculateSize(result);
  return result;
}

TypeRecord* TypeDeduceAuto(TypeRecord* pattern, TypeRecord* initializer_type) {
  if (pattern == NULL || initializer_type == NULL) {
    return NULL;
  }
  if ((pattern->type & kTypeAuto) != 0 &&
      pattern->declarator == kDeclPrimitive) {
    TypeRecord* deduced = TypeRecordCopy(initializer_type);
    deduced->qualifiers |= pattern->qualifiers;
    TypeRecordCalculateSize(deduced);
    return deduced;
  }

  switch (pattern->declarator) {
    case kDeclPointer:
      if (!TypeIsPointerOrArray(initializer_type)) {
        return NULL;
      }
      break;
    case kDeclArray:
      if (!TypeIsArray(initializer_type)) {
        return NULL;
      }
      break;
    case kDeclReference:
    case kDeclRValueReference:
      break;
    default:
      if ((pattern->type & kTypeAuto) != 0) {
        return NULL;
      }
      return TypeRecordCopy(pattern);
  }

  TypeRecord* next_initializer =
      TypeIsPointerOrArray(initializer_type) &&
              pattern->declarator != kDeclReference &&
              pattern->declarator != kDeclRValueReference
          ? initializer_type->next
          : initializer_type;
  TypeRecord* next = TypeDeduceAuto(pattern->next, next_initializer);
  if (next == NULL) {
    return NULL;
  }
  TypeRecord* result = NewDeclaratorLike(pattern);
  TypeRecordChain(result, next);
  result->type = next->type;
  TypeRecordCalculateSize(result);
  return result;
}

bool TypeAssignmentCompatible(TypeRecord* from, TypeRecord* to) {
  if (TypeEqual(to, from)) {
    return true;
  }
  // A pointer can be assigned to a const pointer of the same type.
  if (TypeIsPointerOrArray(to) && TypeIsPointerOrArray(from) &&
      to->next != NULL && from->next != NULL) {
    if (TypeBaseOffset(from->next, to->next, /*public_only=*/true, NULL)) {
      return true;
    }
    int to_quals = to->next->qualifiers & ~kQualConst;
    int from_quals = from->next->qualifiers & ~kQualConst;
    if (to_quals == from_quals) {
      return true;
    }
  }
  return false;
}

bool TypeEqualIgnoringSign(TypeRecord* t1, TypeRecord* t2) {
  if (t1 == NULL || t2 == NULL) {
    return t1 == t2;
  }
  if (t1->declarator != t2->declarator) {
    return false;
  }
  switch (t1->declarator) {
    case kDeclArray:
      if (!TypeEqual(t1->next, t2->next)) {
        return false;
      }
      return TypeArrayBoundsEqual(&t1->info.array, &t2->info.array);
    case kDeclPointer:
    case kDeclReference:
    case kDeclRValueReference:
      return TypeEqual(t1->next, t2->next);
    case kDeclMemberPointer:
      if (t1->template_parameter_index >= 0 ||
          t2->template_parameter_index >= 0) {
        if (t1->template_parameter_index != t2->template_parameter_index) {
          return false;
        }
      } else if (t1->info.struct_info != t2->info.struct_info) {
        return false;
      }
      return TypeEqual(t1->next, t2->next);

    case kDeclFunction:
      if (!TypeEqual(t1->next, t2->next)) {
        return false;
      }
      return FunctionPrototypesEqual(&t1->info.function, &t2->info.function);
    case kDeclPrimitive: {
      if ((TypeIsBitInt(t1) || TypeIsBitInt(t2)) &&
          !TypeIsEnum(t1) && !TypeIsEnum(t2)) {
        return TypeIsBitInt(t1) && TypeIsBitInt(t2) &&
               t1->bit_width == t2->bit_width;
      }
      if (TypeIsEnum(t1) && TypeIsEnum(t2) &&
          ((t1->type | t2->type) & kTypeBitInt) != 0 &&
          t1->bit_width != t2->bit_width) {
        return false;
      }
      Type a =
          CanonicalPrimitiveType(t1->type) & ~(kTypeUnsigned | kTypeSigned);
      Type b =
          CanonicalPrimitiveType(t2->type) & ~(kTypeUnsigned | kTypeSigned);
      return a == b;
    }
  }
}

static void FunctionPrototypesDetails(SourceLocation location, FunctionInfo* a, FunctionInfo* b) {
  const char* filename;
  int lineno;
  int start, end;
  DecodeSourceLocation(location, &filename, &lineno, &start, &end);
  if (a->prototype.length != b->prototype.length) {
    ReportNote(filename, lineno, "Different number of arguments: %zd vs %zd",
               a->prototype.length, b->prototype.length);
    return;
  }
  for (size_t i = 0; i < a->prototype.length; i++) {
    Symbol* s1 = a->prototype.value.p[i];
    Symbol* s2 = b->prototype.value.p[i];
    if (!TypeEqual(s1->type, s2->type)) {
      TypeErrorDetails(location, s1->type, s2->type);
      ReportNote(filename, lineno, "  for argument #%zd", i+1);
    }
  }
}

void TypeErrorDetails(SourceLocation location, TypeRecord* t1, TypeRecord* t2) {
  String error1 = {0};
  String error2 = {0};
  TypeRecordToString(t1, &error1);
  TypeRecordToString(t2, &error2);

  const char* filename;
  int lineno;
  int start, end;
  DecodeSourceLocation(location, &filename, &lineno, &start, &end);

  // Either side can be absent: a symbol that names an entity rather than an
  // object carries no type of its own (a using-alias for a class, for example),
  // and recovery paths reach here with an incomplete declarator chain.
  if (t1 == NULL || t2 == NULL) {
    ReportNote(filename, lineno, "Types '%s' and '%s' are different",
               error1.value, error2.value);
  } else if (t1->declarator != t2->declarator) {
    ReportNote(filename, lineno, "Declarators '%s' and '%s' are different",
               error1.value, error2.value);
  } else {
    switch (t1->declarator) {
      case kDeclArray:
      case kDeclPointer:
      case kDeclReference:
      case kDeclRValueReference:
      case kDeclMemberPointer:
        ReportNote(filename, lineno,
                   "Declaration of '%s' and '%s' are different",
                   error1.value, error2.value);
        TypeErrorDetails(location, t1->next, t2->next);
        break;

      case kDeclFunction:
        ReportNote(filename, lineno,
                   "Declaration of '%s' and '%s' are different",
                   error1.value, error2.value);
        TypeErrorDetails(location, t1->next, t2->next);
        FunctionPrototypesDetails(location, &t1->info.function,
                                  &t2->info.function);
        break;

      case kDeclPrimitive:
        if (CanonicalPrimitiveType(t1->type) !=
                CanonicalPrimitiveType(t2->type) ||
            t1->qualifiers != t2->qualifiers) {
          ReportNote(filename, lineno, "Types '%s' and '%s' are different",
                     error1.value, error2.value);
        }
    }
  }
  StringDestruct(&error1);
  StringDestruct(&error2);
}

