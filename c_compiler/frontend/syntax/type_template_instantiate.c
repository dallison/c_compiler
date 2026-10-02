//
//  type_template_instantiate.c
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
#include "init_semantics.h"
#include "member_pointer.h"
#include "reflection.h"
#include "statement_semantics.h"
#include "statement_parser.h"
#include "symbol_table.h"
#include "syntax.h"
#include "semantics.h"
#include "errors.h"
#include "debug.h"
#include "rtti.h"
#include "set.h"

/* Types whose dependent_decltype_expr is temporarily detached. See
 * TypeNoteDetachedDependentDecltype. */
static Vector g_detached_dependent_decltypes;
static bool g_detached_dependent_decltypes_ready = false;

void TypeNoteDetachedDependentDecltype(TypeRecord* type) {
  if (type == NULL) {
    return;
  }
  if (!g_detached_dependent_decltypes_ready) {
    VectorInit(&g_detached_dependent_decltypes);
    g_detached_dependent_decltypes_ready = true;
  }
  VectorAppend(&g_detached_dependent_decltypes, type);
}

void TypeForgetDetachedDependentDecltype(TypeRecord* type) {
  if (!g_detached_dependent_decltypes_ready ||
      g_detached_dependent_decltypes.length == 0) {
    return;
  }
  size_t last = g_detached_dependent_decltypes.length - 1;
  if (g_detached_dependent_decltypes.value.p[last] == type) {
    VectorPop(&g_detached_dependent_decltypes);
    return;
  }
  for (size_t i = 0; i < g_detached_dependent_decltypes.length; i++) {
    if (g_detached_dependent_decltypes.value.p[i] == type) {
      g_detached_dependent_decltypes.value.p[i] =
          g_detached_dependent_decltypes.value.p[last];
      VectorPop(&g_detached_dependent_decltypes);
      return;
    }
  }
}

bool TypeIsDetachedDependentDecltype(TypeRecord* type) {
  if (type == NULL || !g_detached_dependent_decltypes_ready) {
    return false;
  }
  for (size_t i = 0; i < g_detached_dependent_decltypes.length; i++) {
    if (g_detached_dependent_decltypes.value.p[i] == type) {
      return true;
    }
  }
  return false;
}

static void InjectInstantiatedAnonymousMembers(TypeParser* parser,
                                               Struct* str);
static bool StructHasBitFieldMembers(Struct* str);

static void CopySymbolAliasTemplate(Symbol* dest, Symbol* source) {
  if (dest == NULL || source == NULL || source->alias_template == NULL) {
    return;
  }
  dest->alias_template = malloc(sizeof(AliasTemplate));
  VectorInit(&dest->alias_template->parameters);
  dest->alias_template->ctad_names_template_template_parameter =
      source->alias_template->ctad_names_template_template_parameter;
  for (size_t p = 0; p < source->alias_template->parameters.length; p++) {
    VectorAppend(&dest->alias_template->parameters,
                 TemplateParameterCopy(
                     source->alias_template->parameters.value.p[p]));
  }
}

// When set, a parameter appearing only in a bare `T::member` non-deduced
// context is left unbound during argument deduction (so a default template
// argument can supply it) rather than deduced from the argument via this
// compiler's non-conforming member-access extension.  The extension is retried
// (flag cleared) only if a parameter is left unbound with no usable default.
static bool g_deduce_defer_bare_member = false;
// Primary member templates are numbered after the enclosing class parameters
// (`template_parameter_base`).  A call bound to that primary, rather than the
// rebased instantiation, still has those absolute indices.  Deduction's
// argument vector is one slot per parameter of the member template itself, so
// subtract the base while deducing.
static int g_deduce_template_parameter_base = 0;

static int DeductionArgumentIndex(int index) {
  if (g_deduce_template_parameter_base > 0 &&
      index >= g_deduce_template_parameter_base) {
    return index - g_deduce_template_parameter_base;
  }
  return index;
}

static bool DeduceFunctionTemplateTypeArgument(Vector* args,
                                               size_t explicit_arg_count,
                                               TypeRecord* formal,
                                               TypeRecord* actual);
static bool TemplateArgumentHasDependentMemberName(TemplateArgument* arg);
static bool DeduceFunctionTemplateStructMembers(Vector* args,
                                                size_t explicit_arg_count,
                                                TypeRecord* formal,
                                                TypeRecord* actual);
void MaxTemplateParameterIndexInArgument(TemplateArgument* arg,
                                         int* max_index);
static bool ClassTemplateArgumentPatternMatches(Vector* bindings,
                                                  TemplateArgument* pattern,
                                                  TemplateArgument* actual);
static bool ClassTemplateTypePatternMatches(Vector* bindings,
                                            TypeRecord* pattern,
                                            TypeRecord* actual);
static Vector* TemplateParameterListForSymbol(Symbol* symbol);
static bool TemplateTemplateParameterListsCompatible(Vector* formal,
                                                       Vector* actual);
static int TemplateArgumentVectorPatternSpecificity(Vector* args);
typedef enum {
  kTemplateArgumentsBorrow,
  kTemplateArgumentsConsume,
} TemplateArgumentOwnership;
static Vector* CompleteFunctionTemplateArguments(TypeParser* parser,
                                                   TypeRecord* func,
                                                   Vector* args,
                                                   bool emit_error,
                                                   TemplateArgumentOwnership ownership);
static Vector* CompleteTemplateArguments(TypeParser* parser, Vector* parameters,
                                         Vector* args, const char* error_message,
                                         bool emit_error,
                                         TemplateArgumentOwnership ownership);
static Vector* CompleteVariableTemplateArguments(TypeParser* parser,
                                                 VariableTemplate* vt,
                                                 Vector* args, bool emit_error);
static TypeRecord* InstantiateSimpleClassTemplateImpl(TypeParser* parser,
                                                      Symbol* templ,
                                                      Vector* args,
                                                      bool emit_constraint_error);
static TypeRecord* InstantiateAliasClassTemplateImpl(TypeParser* parser,
                                                   Symbol* alias, Vector* args,
                                                   bool emit_constraint_error);
static TypeRecord* InstantiateGenericAliasTemplate(TypeParser* parser,
                                                   Symbol* alias, Vector* args,
                                                   bool emit_constraint_error);
static TypeRecord* MaterializeClassBaseType(TypeParser* parser,
                                           TypeRecord* base_type);
static bool TypeIsStillDependentClassBase(TypeRecord* type);
static void ExpandConcreteAliasTemplateArguments(TypeParser* parser,
                                                 Vector* args);
static bool ClassTemplateArgumentsAreStillDependent(Vector* args);
static void ApplyFriendTypeDeclarations(TypeParser* parser, Struct* target,
                                        Struct* source, Vector* args);

static void ResolveInstantiatedFriendType(TypeParser* parser, Struct* target,
                                          TypeRecord* type,
                                          bool is_pack_expansion,
                                          SourceLocation location) {
  if (type == NULL) {
    return;
  }
  if (TypeContainsTemplateParameter(type) || TypeIsUnknown(type)) {
    VectorAppend(&target->friend_type_declarations,
                 NewCXXFriendTypeDeclaration(type, is_pack_expansion,
                                             location));
    TypeRecordDelete(type);
    return;
  }
  type = TypeMaterializeClassTemplateSpecialization(parser->syntax, type);
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL) {
    // [class.friend]: non-class friend type specifiers are ignored.
    TypeRecordDelete(type);
    return;
  }
  StructAddFriendClass(target, type->info.struct_info);
  TypeRecordDelete(type);
}

static void ApplyFriendTypeDeclarations(TypeParser* parser, Struct* target,
                                        Struct* source, Vector* args) {
  for (size_t i = 0; i < source->friend_type_declarations.length; i++) {
    CXXFriendTypeDeclaration* declaration =
        source->friend_type_declarations.value.p[i];
    int pack_index = -1;
    size_t pack_length = 0;
    bool can_expand =
        declaration->is_pack_expansion &&
        FindPackExpansionInType(declaration->type, args, &pack_index,
                                &pack_length);
    if (can_expand) {
      for (size_t j = 0; j < pack_length; j++) {
        TemplateArgument* pack = args->value.p[pack_index];
        TemplateArgument* element = pack->pack_arguments->value.p[j];
        Vector* element_args =
            TemplateArgumentVectorCopyWithPackElement(args, pack_index,
                                                      element);
        TypeRecord* friend_type = SubstituteTemplateParameters(
            parser, declaration->type, element_args);
        ResolveInstantiatedFriendType(parser, target, friend_type,
                                      /*is_pack_expansion=*/false,
                                      declaration->location);
        VectorDeleteWithContents(
            element_args,
            (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
      }
      continue;
    }

    TypeRecord* friend_type =
        SubstituteTemplateParameters(parser, declaration->type, args);
    ResolveInstantiatedFriendType(
        parser, target, friend_type, declaration->is_pack_expansion,
        declaration->location);
  }
}
TypeRecord* TypeInstantiateClassTemplateQuiet(Syntax* syntax, Symbol* templ,
                                              Vector* args);
static bool TypeInstantiateVariableTemplateConstantImpl(
    Syntax* syntax, Symbol* var_template, Vector* args, int64_t* out,
    bool emit_constraint_error);
static ClassTemplatePartialSpecialization*
SelectVariableTemplatePartialSpecialization(TypeParser* parser, Symbol* primary,
                                            Vector* actual_args,
                                            Vector** bindings_out);
static void ReportVariableTemplateConstraintFailure(Syntax* syntax,
                                                    Symbol* var_template,
                                                    Vector* arguments);
static void ReportAliasTemplateConstraintFailure(TypeParser* parser,
                                                 Symbol* alias,
                                                 Vector* arguments);
static void ReportClassTemplateConstraintFailure(TypeParser* parser,
                                                 Symbol* templ,
                                                 ConstraintExpr* constraint,
                                                 Vector* arguments);
static TypeRecord* ClassTemplateConstraintFailureType(
    TypeParser* parser, Symbol* templ, ConstraintExpr* constraint,
    Vector* arguments, bool emit_constraint_error);

void AppendTemplateInstantiationName(String* name, Symbol* templ, Vector* args);
Vector* CompleteClassTemplateArguments(TypeParser* parser, Struct* template_struct,
                                       Vector* args);
void AddClassTemplatePartialSpecialization(TypeParser* parser, Symbol* primary,
                                           Symbol* partial_tag,
                                           Vector* pattern_args);

static Symbol* InstantiatedMemberForSymbol(Struct* source, Struct* target,
                                           Symbol* symbol) {
  if (source == NULL || target == NULL || symbol == NULL ||
      symbol->name.value == NULL) {
    return NULL;
  }
  StructMember* found = NULL;
  for (size_t i = 0; i < source->members.length && found == NULL; i++) {
    for (StructMember* candidate = source->members.value.p[i];
         candidate != NULL; candidate = candidate->overload_next) {
      if (candidate->symbol == NULL) {
        continue;
      }
      bool same_symbol = candidate->symbol == symbol;
      bool same_entity = candidate->symbol->type == symbol->type &&
                         strcmp(candidate->symbol->name.value,
                                symbol->name.value) == 0;
      if (same_symbol || same_entity) {
        found = candidate;
        break;
      }
    }
  }
  if (found == NULL || found->symbol == NULL) {
    return NULL;
  }
  StructMember* head = FindStructMember(target, &found->symbol->name);
  for (StructMember* candidate = head; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->symbol == NULL ||
        candidate->is_member_function != found->is_member_function ||
        candidate->is_static != found->is_static) {
      continue;
    }
    if (!candidate->is_member_function) {
      return candidate->symbol;
    }
    TypeRecord* found_type = found->symbol->type;
    TypeRecord* candidate_type = candidate->symbol->type;
    if (found_type != NULL && candidate_type != NULL &&
        TypeIsFunction(found_type) && TypeIsFunction(candidate_type) &&
        found_type->info.function.prototype.length ==
            candidate_type->info.function.prototype.length) {
      return candidate->symbol;
    }
  }
  return NULL;
}

static void RebindInstantiatedMemberIdentifier(ASTNode* node, void* data,
                                               int child_id,
                                               VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(identifier) ||
      ASTNodeGetShape(node) != kASTShapeIdentifier) {
    return;
  }
  Struct** owners = data;
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  Symbol* replacement =
      InstantiatedMemberForSymbol(owners[0], owners[1], id->symbol);
  if (replacement != NULL) {
    id->symbol = replacement;
  }
}

static void FoldInstantiatedConstexprStaticMembers(Struct* source,
                                                   Struct* target) {
  if (target == NULL) {
    return;
  }
  for (size_t i = 0; i < target->members.length; i++) {
    StructMember* member = target->members.value.p[i];
    if (member == NULL || member->symbol == NULL || !member->is_static ||
        member->is_member_function || member->default_initializer == NULL) {
      continue;
    }
    Symbol* symbol = member->symbol;
    if (!symbol->flags.is_constexpr || symbol->flags.value_set ||
        symbol->type == NULL || TypeContainsTemplateParameter(symbol->type)) {
      continue;
    }
    if (!TypeIsStructOrUnion(symbol->type) && !TypeIsFixedArray(symbol->type)) {
      continue;
    }
    if (DependentExpressionContainsTemplateParameter(
            member->default_initializer)) {
      continue;
    }
    ASTNode* initializer = ASTNodeClone(member->default_initializer,
                                        IdentityCloneNode, NULL, NULL);
    if (initializer == NULL) {
      continue;
    }
    if (source != NULL) {
      Struct* owners[2] = {source, target};
      ASTNodeVisit(initializer, RebindInstantiatedMemberIdentifier, 0, owners);
    }
    DiagnosticSuppressBegin();
    compiler->constant_evaluation_required_depth++;
    initializer = AnalyzeExpression(initializer);
    compiler->constant_evaluation_required_depth--;
    if (initializer != NULL) {
      ConstexprEvaluateObjectConstantForSymbol(symbol, initializer);
    }
    DiagnosticSuppressEnd();
    ASTNodeDelete(initializer);
  }
}

static void EvaluateInstantiatedStaticAsserts(TypeParser* parser,
                                              Struct* source, Struct* target,
                                              Vector* args) {
  if (parser == NULL || source == NULL || target == NULL ||
      source->static_asserts.length == 0) {
    return;
  }
  FoldInstantiatedConstexprStaticMembers(source, target);
  for (size_t i = 0; i < source->static_asserts.length; i++) {
    ASTNode* node = source->static_asserts.value.p[i];
    if (node == NULL || ASTNodeGetShape(node) != kASTShapeStaticAssert) {
      continue;
    }
    StaticAssertASTNode* assert_node = (StaticAssertASTNode*)node;
    ASTNode* substituted = CloneDependentExpressionWithArgs(
        parser, assert_node->expr, args);
    if (substituted == NULL) {
      substituted =
          ASTNodeClone(assert_node->expr, IdentityCloneNode, NULL, NULL);
    }
    if (substituted == NULL) {
      continue;
    }
    Struct* owners[2] = {source, target};
    ASTNodeVisit(substituted, RebindInstantiatedMemberIdentifier, 0, owners);
    ASTNode* saved = assert_node->expr;
    assert_node->expr = substituted;
    bool dependent = false;
    int result = SyntaxEvaluateDeferredStaticAssert(node, &dependent);
    assert_node->expr = saved;
    if (result < 0 && dependent) {
      ASTNode* kept_expr =
          ASTNodeClone(substituted, IdentityCloneNode, NULL, NULL);
      ASTNodeDelete(substituted);
      if (kept_expr != NULL) {
        String message;
        StringInit(&message, assert_node->message.value);
        ASTNode* kept = NewStaticAssertASTNode(
            kept_expr, &message,
            ASTNodeClone(assert_node->message_expr, IdentityCloneNode, NULL,
                         NULL),
            node->location);
        StringDestruct(&message);
        VectorAppend(&target->static_asserts, kept);
      }
      continue;
    }
    ASTNodeDelete(substituted);
    if (result < 0) {
      SyntaxErrorAtLocation(
          parser->syntax, node->location,
          "static_assert expression is not an integer constant expression");
    }
  }
}

static void ClearBitWidthAnalysis(ASTNode* node, void* data, int child_id,
                                   VisitorMode mode) {
  (void)data;
  (void)child_id;
  if (mode == kVisitPreChildren && node != NULL) {
    node->flags &= ~kASTAnalyzed;
    ASTNodeClearType(node);
  }
}

/* Pair source bit-fields with the instantiation's bit-fields in declaration
 * order and fold widths that were dependent at parse time. */
static void ResolveInstantiatedBitFieldWidths(TypeParser* parser,
                                              Struct* source, Struct* target,
                                              Vector* args) {
  if (parser == NULL || source == NULL || target == NULL) {
    return;
  }
  size_t target_index = 0;
  for (size_t source_index = 0; source_index < source->members.length;
       source_index++) {
    StructMember* from = source->members.value.p[source_index];
    if (from == NULL || !from->is_bit_field) {
      continue;
    }
    StructMember* to = NULL;
    for (; target_index < target->members.length; target_index++) {
      StructMember* candidate = target->members.value.p[target_index];
      if (candidate != NULL && candidate->is_bit_field) {
        to = candidate;
        target_index++;
        break;
      }
    }
    if (to == NULL || from->bit_width_expr == NULL || to->symbol == NULL ||
        to->symbol->type == NULL) {
      continue;
    }
    ASTNode* substituted =
        CloneDependentExpressionWithArgs(parser, from->bit_width_expr, args);
    if (substituted == NULL) {
      substituted = ASTNodeClone(from->bit_width_expr, IdentityCloneNode, NULL,
                                 NULL);
    }
    if (substituted == NULL) {
      continue;
    }
    Struct* owners[2] = {source, target};
    ASTNodeVisit(substituted, RebindInstantiatedMemberIdentifier, 0, owners);
    // The cloned width still carries the definition-time type, which names
    // the class template's parameter.  Rebinding the identifier does not
    // clear that type, so dependence has to be judged after reanalysis.
    ASTNodeVisit(substituted, ClearBitWidthAnalysis, 0, NULL);
    int64_t width = 0;
    DiagnosticSuppressBegin();
    compiler->constant_evaluation_required_depth++;
    substituted = AnalyzeExpression(substituted);
    compiler->constant_evaluation_required_depth--;
    bool ok = substituted != NULL &&
              EvaluateIntegerExpression(substituted, &width);
    DiagnosticSuppressEnd();
    if (!ok && substituted != NULL &&
        ExpressionIsTemplateDependent(substituted)) {
      ASTNodeDelete(to->bit_width_expr);
      to->bit_width_expr = substituted;
      continue;
    }
    ASTNodeDelete(substituted);
    SourceLocation location =
        to->symbol->location != 0 ? to->symbol->location : from->symbol != NULL
                                                               ? from->symbol->location
                                                               : 0;
    if (!ok) {
      SyntaxErrorAtLocation(parser->syntax, location,
                            "Invalid bitfield; constant expression needed");
      continue;
    }
    TypeRecordCalculateSize(to->symbol->type);
    if (!TypeIsIntegral(to->symbol->type)) {
      SyntaxErrorAtLocation(
          parser->syntax, location,
          "Invalid bitfield; only integer types can be used for bitfields");
      continue;
    }
    int word_width = TypeIsBitInt(to->symbol->type)
                         ? to->symbol->type->bit_width
                         : to->symbol->type->size * 8;
    if (width == 0) {
      if (!to->symbol->flags.invented) {
        SyntaxErrorAtLocation(parser->syntax, location,
                              "Invalid bitfield; named bit-field has zero width");
      }
      to->bit_size = 0;
      continue;
    }
    if (width < 0 || (word_width > 0 && width > word_width)) {
      SyntaxErrorAtLocation(
          parser->syntax, location,
          "Invalid bitfield; width of %" PRId64
          " is out of bounds for type of size %d bits",
          width, word_width);
      continue;
    }
    to->bit_size = (int)width;
    to->is_bit_field = true;
  }
}

typedef struct {
  Struct* source;
  Vector* args;
  TypeRecord* type;
} NestedSubstitutionInProgress;

static Vector nested_substitution_stack;
static bool nested_substitution_stack_ready = false;

typedef struct {
  Struct* owner;
  Symbol* symbol;
  Symbol* template_definition;
  Struct* substitution_source;
  Vector* args;
} DeferredNestedMemberBody;

static Vector g_deferred_nested_member_bodies;
static bool g_deferred_nested_member_bodies_inited;

static void EnsureDeferredNestedMemberBodies(void) {
  if (!g_deferred_nested_member_bodies_inited) {
    VectorInit(&g_deferred_nested_member_bodies);
    g_deferred_nested_member_bodies_inited = true;
  }
}

/* Clone member bodies of nested classes that were instantiated while an
 * enclosing class template was still incomplete.  `s.emplace_at(...)` inside
 * `raw_hash_set::EmplaceDecomposable` is declared later in the enclosing
 * class; checking the nested body before that member exists reports a false
 * "not a member" error. */
static void FlushDeferredNestedMemberBodies(TypeParser* parser,
                                           size_t watermark) {
  EnsureDeferredNestedMemberBodies();
  size_t index = watermark;
  while (index < g_deferred_nested_member_bodies.length) {
    DeferredNestedMemberBody* body =
        g_deferred_nested_member_bodies.value.p[index];
    Struct* saved_source = parser->template_substitution_source;
    Struct* saved_target = parser->template_substitution_target;
    Struct* saved_enclosing_source =
        parser->enclosing_template_substitution_source;
    Struct* saved_enclosing_target =
        parser->enclosing_template_substitution_target;
    if (saved_source != NULL && saved_target != NULL) {
      parser->enclosing_template_substitution_source = saved_source;
      parser->enclosing_template_substitution_target = saved_target;
    }
    parser->template_substitution_source = body->substitution_source;
    parser->template_substitution_target = body->owner;
    CloneInstantiatedMemberFunctionBody(parser, body->owner, body->symbol,
                                        body->template_definition,
                                        body->substitution_source, body->args);
    parser->template_substitution_source = saved_source;
    parser->template_substitution_target = saved_target;
    parser->enclosing_template_substitution_source = saved_enclosing_source;
    parser->enclosing_template_substitution_target = saved_enclosing_target;
    if (body->args != NULL) {
      VectorDeleteWithContents(body->args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    free(body);
    g_deferred_nested_member_bodies.value.p[index] = NULL;
    index++;
  }
  g_deferred_nested_member_bodies.length = watermark;
}

static void DeferNestedMemberBody(Struct* owner, PendingMemberBody* pending,
                                  Vector* args) {
  EnsureDeferredNestedMemberBodies();
  DeferredNestedMemberBody* body = calloc(1, sizeof(*body));
  body->owner = owner;
  body->symbol = pending->symbol;
  body->template_definition = pending->template_definition;
  body->substitution_source = pending->substitution_source;
  body->args = TemplateArgumentVectorCopy(args);
  VectorAppend(&g_deferred_nested_member_bodies, body);
  free(pending);
}

static Vector g_late_member_bodies;
static bool g_late_member_bodies_inited;

static void LateMemberBodyDelete(LateMemberBody* late) {
  if (late->args != NULL) {
    VectorDeleteWithContents(late->args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  free(late);
}

static Symbol* LateMemberBodyDefinition(LateMemberBody* late) {
  Symbol* definition = late->pattern->value.func_defn;
  if (definition == NULL) {
    definition = late->pattern;
  }
  return definition->type != NULL &&
                 TypeIsFunction(definition->type) &&
                 definition->type->info.function.body != NULL
             ? definition
             : NULL;
}

static void RecordLateMemberBody(Struct* owner, PendingMemberBody* pending,
                                 Vector* args) {
  Symbol* symbol = pending->symbol;
  if (pending->pattern == NULL || symbol == NULL || symbol->type == NULL ||
      !TypeIsFunction(symbol->type) || symbol->flags.is_template ||
      symbol->type->info.function.is_deleted ||
      symbol->type->info.function.is_defaulted ||
      symbol->type->info.function.is_pure_virtual ||
      StructContainsTemplateParameter(owner)) {
    return;
  }
  if (!g_late_member_bodies_inited) {
    VectorInit(&g_late_member_bodies);
    g_late_member_bodies_inited = true;
  }
  LateMemberBody* late = calloc(1, sizeof(*late));
  late->symbol = symbol;
  late->pattern = pending->pattern;
  late->owner = owner;
  late->substitution_source = pending->substitution_source;
  late->args = args != NULL ? TemplateArgumentVectorCopy(args) : NULL;
  VectorAppend(&g_late_member_bodies, late);
}

void TypeInstantiateLateMemberBodies(Syntax* syntax, Vector* defined) {
  if (!g_late_member_bodies_inited) {
    return;
  }
  for (size_t i = 0; i < g_late_member_bodies.length;) {
    LateMemberBody* late = g_late_member_bodies.value.p[i];
    Symbol* definition = LateMemberBodyDefinition(late);
    if (definition == NULL) {
      i++;
      continue;
    }
    VectorDeleteElement(&g_late_member_bodies, i);
    if (late->symbol->type->info.function.body == NULL &&
        late->symbol->value.func_defn == NULL) {
      TypeParser parser;
      TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                     syntax->context);
      parser.template_substitution_source = late->substitution_source;
      parser.template_substitution_target = late->owner;
      CloneInstantiatedMemberFunctionBody(&parser, late->owner, late->symbol,
                                          definition, late->substitution_source,
                                          late->args);
      TypeParserDestruct(&parser);
      VectorAppend(defined, late->symbol);
    }
    LateMemberBodyDelete(late);
  }
}

void TypeReleaseLateMemberBodies(void) {
  if (!g_late_member_bodies_inited) {
    return;
  }
  VectorDestructWithContents(&g_late_member_bodies,
                             (VectorElementDestructor)LateMemberBodyDelete,
                             /*free_element=*/false);
  g_late_member_bodies_inited = false;
}

/* A lambda closure inside a function template is substituted once per mention
 * (the `auto` variable, the compound literal, each capture).  Each call built
 * a new struct, so `const auto f = [&]{ ... }` tried to initialize one closure
 * from a different copy of the same lambda.  Reuse the first result.  A named
 * local class is mentioned the same way (`new FactoryImpl{...}`), and each copy
 * would register its own vtable. */
typedef struct {
  Struct* source;
  Vector* args;
  TypeRecord* type;
} CachedLambdaSubstitution;

static Vector cached_lambda_substitutions;
static bool cached_lambda_substitutions_ready;

static bool CachesLocalClassSubstitution(Struct* source) {
  return source != NULL && source->tag_symbol != NULL &&
         (source->tag_symbol->flags.invented ||
          (source->tag_symbol->flags.is_block_scope && !source->is_template));
}

static TypeRecord* FindCachedLambdaSubstitution(Struct* source, Vector* args) {
  if (!cached_lambda_substitutions_ready ||
      !CachesLocalClassSubstitution(source)) {
    return NULL;
  }
  for (size_t i = 0; i < cached_lambda_substitutions.length; i++) {
    CachedLambdaSubstitution* entry = cached_lambda_substitutions.value.p[i];
    if (entry->source == source &&
        TemplateArgumentVectorEqual(entry->args, args)) {
      return entry->type;
    }
  }
  return NULL;
}

static void RememberLambdaSubstitution(Struct* source, Vector* args,
                                      TypeRecord* type) {
  if (!CachesLocalClassSubstitution(source) || type == NULL) {
    return;
  }
  if (FindCachedLambdaSubstitution(source, args) != NULL) {
    return;
  }
  if (!cached_lambda_substitutions_ready) {
    VectorInit(&cached_lambda_substitutions);
    cached_lambda_substitutions_ready = true;
  }
  CachedLambdaSubstitution* entry = malloc(sizeof(*entry));
  entry->source = source;
  entry->args = TemplateArgumentVectorCopy(args);
  TypeRecordIncRef(type);
  entry->type = type;
  VectorAppend(&cached_lambda_substitutions, entry);
}

static TypeRecord* NestedSubstitutionAlreadyInProgress(Struct* source,
                                                      Vector* args) {
  if (!nested_substitution_stack_ready || source == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < nested_substitution_stack.length; i++) {
    NestedSubstitutionInProgress* entry = nested_substitution_stack.value.p[i];
    // Argument vectors are copied at each call, so pointer identity misses a
    // re-entrant substitution of the same lambda (`[&]{ ... }` mentioned from
    // its own initializer).  Value equality is the instantiation key.
    if (entry->source == source &&
        TemplateArgumentVectorEqual(entry->args, args)) {
      return entry->type;
    }
    // Substituting the in-progress copy itself would invent another closure
    // (`__invented__$S1$S2$...`) and recurse until the stack overflows.
    if (entry->type != NULL && TypeIsStructOrUnion(entry->type) &&
        entry->type->info.struct_info == source) {
      return entry->type;
    }
  }
  return NULL;
}

static void PushNestedSubstitution(Struct* source, Vector* args,
                                   TypeRecord* type) {
  if (!nested_substitution_stack_ready) {
    VectorInit(&nested_substitution_stack);
    nested_substitution_stack_ready = true;
  }
  NestedSubstitutionInProgress* entry =
      malloc(sizeof(NestedSubstitutionInProgress));
  entry->source = source;
  entry->args = args;
  entry->type = type;
  VectorAppend(&nested_substitution_stack, entry);
}

static Vector* NestedSubstitutionArgumentsForOwner(Struct* owner,
                                                int member_base) {
  if (!nested_substitution_stack_ready || owner == NULL || member_base <= 0) {
    return NULL;
  }
  for (size_t i = nested_substitution_stack.length; i-- > 0;) {
    NestedSubstitutionInProgress* entry = nested_substitution_stack.value.p[i];
    if (entry == NULL || entry->args == NULL) {
      continue;
    }
    bool matches = entry->source == owner;
    if (!matches && entry->type != NULL && TypeIsStructOrUnion(entry->type) &&
        entry->type->info.struct_info == owner) {
      matches = true;
    }
    if (!matches || (int)entry->args->length < member_base ||
        TemplateArgumentVectorContainsTemplateParameter(entry->args)) {
      continue;
    }
    return entry->args;
  }
  return NULL;
}

static void PopNestedSubstitution(Struct* source) {
  if (!nested_substitution_stack_ready) {
    return;
  }
  for (size_t i = nested_substitution_stack.length; i-- > 0;) {
    NestedSubstitutionInProgress* entry = nested_substitution_stack.value.p[i];
    if (entry->source == source) {
      VectorDeleteElement(&nested_substitution_stack, i);
      free(entry);
      return;
    }
  }
}

static bool StructHasDependentDataMember(Struct* str) {
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member != NULL && member->symbol != NULL && !member->is_static &&
        !member->is_member_function &&
        TypeContainsTemplateParameter(member->symbol->type)) {
      return true;
    }
  }
  return false;
}

/* Copy a parameter of a template nested in the class being instantiated:
 * enclosing class parameters are bound to `args` and the nested template's own
 * parameters are renumbered from 0.  `from_owner` is the enclosing class
 * template and `to_owner` its instantiation. */
static TemplateParameter* CloneNestedTemplateParameter(TypeParser* parser,
                                                       TemplateParameter* from,
                                                       Vector* args,
                                                       Struct* from_owner,
                                                       Struct* to_owner) {
  int base = args != NULL ? (int)args->length : 0;
  TemplateParameter* param = TemplateParameterCopy(from);
  if (param->type != NULL) {
    TypeRecord* substituted =
        SubstituteTemplateParameters(parser, param->type, args);
    TypeRecordDelete(param->type);
    param->type = substituted;
    RebaseTemplateParameterIndices(param->type, base);
  }
  if (param->default_type != NULL) {
    TypeRecord* substituted =
        SubstituteTemplateParameters(parser, param->default_type, args);
    TypeRecordDelete(param->default_type);
    param->default_type = substituted;
    RebaseTemplateParameterIndices(param->default_type, base);
  }
  if (param->default_template_parameter_index >= base) {
    param->default_template_parameter_index -= base;
  }
  if (param->index >= base) {
    param->index -= base;
  }
  // `bool = (sizeof(M) <= 8)` still numbers `M` after the enclosing class's
  // parameters, but the nested template is completed with its own arguments.
  if (param->default_argument != NULL && base > 0) {
    TemplateArgument* default_argument = param->default_argument;
    if (default_argument->template_parameter_index >= base) {
      default_argument->template_parameter_index -= base;
    }
    if (default_argument->dependent_expr != NULL && parser != NULL &&
        parser->syntax != NULL) {
      // Bind the enclosing class's arguments too: after rebasing alone,
      // `MatcherBase::IsInlined<M>()` would name the class parameter and `M`
      // by the same index.  The owners send `IsInlined` to the instantiation's
      // member, whose own parameters are the ones renumbered here.
      bool remap_owner =
          from_owner != NULL && to_owner != NULL && from_owner != to_owner;
      ASTNode* rebased = TypeSubstituteMemberTemplateExpressionAndRebase(
          parser->syntax, default_argument->dependent_expr, args, base,
          default_argument->dependent_expr->location,
          remap_owner ? from_owner : NULL, remap_owner ? to_owner : NULL);
      if (rebased != NULL) {
        default_argument->dependent_expr = rebased;
      }
    }
  }
  return param;
}

/* Insert the hidden vptr/vbptr, lay out virtual bases and register the
 * vtables of a concrete instantiation, as a class definition does
 * (type_class.c).  The instantiation does not copy the template's own hidden
 * fields: its concrete bases decide which class owns them. */
static void CompleteInstantiatedPolymorphicLayout(TypeParser* parser,
                                                  Struct* str) {
  UpdateCXXAbstractStatus(str);
  AddCXXVPtrMember(parser, str);
  AddCXXVBPtrMember(parser, str);
  str->non_virtual_size = str->next_offset;
  LayoutCXXVirtualBaseSpecifiers(str);
  RegisterCXXVTable(parser, str);
  RegisterCXXVBTables(parser, str);
  CXXFixupSpecialMemberTrivialityAfterLayout(str);
  str->vtables_registered = true;
  SyntaxFlushPendingVPtrInitializers(str);
  // The vptr/vbptr insertion shifts member offsets and grows the object.
  FinalizeStructAlignment(str);
}

/* The partial specializations of a nested class template (`ValuePolicy<M,
 * false>` in `MatcherBase<T>`) belong to each instantiation of the enclosing
 * class, renumbered like the primary `to` cloned from `from`. */
static void CloneNestedTemplatePartialSpecializations(TypeParser* parser,
                                                      Struct* from, Struct* to,
                                                      Vector* args) {
  int base = args != NULL ? (int)args->length : 0;
  if (!from->is_template || base == 0) {
    return;
  }
  for (size_t i = 0; i < from->partial_specializations.length; i++) {
    ClassTemplatePartialSpecialization* partial =
        from->partial_specializations.value.p[i];
    // A constraint would need the same renumbering; such a specialization is
    // left out rather than checked against the wrong parameters.
    if (partial == NULL || partial->tag_symbol == NULL ||
        partial->tag_symbol->type == NULL ||
        !TypeIsStructOrUnion(partial->tag_symbol->type) ||
        partial->associated_constraint != NULL) {
      continue;
    }
    TypeRecord* body = SubstituteNestedStructTemplateParameters(
        parser, partial->tag_symbol->type, args);
    if (body == NULL || !TypeIsStructOrUnion(body) ||
        body->info.struct_info == NULL) {
      TypeRecordDelete(body);
      continue;
    }
    Symbol* tag = NewSymbol(partial->tag_symbol->name.value, body,
                            STO(implicit));
    tag->flags.is_defined = partial->tag_symbol->flags.is_defined;
    tag->flags.is_template = true;
    Vector* pattern = SubstituteTemplateArgumentVector(
        parser, &partial->pattern_arguments, args, base);
    ClassTemplatePartialSpecialization* clone =
        NewClassTemplatePartialSpecialization(tag, NULL, pattern);
    VectorDeleteWithContents(pattern,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    // The partial was registered with the enclosing class's parameters in
    // front of its own (`[T, U]` for `VP<U*, false>` in `MB<T>`); those are
    // bound by `args` now.
    for (size_t j = 0; j < partial->template_parameters.length; j++) {
      TemplateParameter* param = partial->template_parameters.value.p[j];
      if (param == NULL || param->index < base) {
        continue;
      }
      VectorAppend(&clone->template_parameters,
                   CloneNestedTemplateParameter(parser, param, args,
                                                from->lexical_parent,
                                                to->lexical_parent));
    }
    VectorAppend(&to->partial_specializations, clone);
  }
}

/* Remember which template member a specialization's static data member was
 * instantiated from, so TypeEnsureStaticDataMemberDefinition can instantiate
 * its definition when the member is first odr-used. */
static void RecordStaticDataMemberPattern(StructMember* member,
                                          Symbol* instantiated, Vector* args) {
  if (member == NULL || member->symbol == NULL || instantiated == NULL ||
      !member->is_static || member->is_member_function ||
      member->is_using_declaration || StructMemberIsNestedType(member) ||
      StorageIs(member->symbol->storage, STO(typedef)) ||
      TypeIsFunction(instantiated->type) || args == NULL) {
    return;
  }
  instantiated->static_data_member_pattern = member->symbol;
  instantiated->static_data_member_template_arguments =
      TemplateArgumentVectorCopy(args);
}

void TypeEnsureStaticDataMemberDefinition(Syntax* syntax, Symbol* symbol) {
  if (syntax == NULL || symbol == NULL || symbol->type == NULL ||
      symbol->static_data_member_pattern == NULL ||
      symbol->static_data_member_class == NULL ||
      TypeContainsTemplateParameter(symbol->type) ||
      TemplateArgumentVectorContainsTemplateParameter(
          symbol->static_data_member_template_arguments)) {
    return;
  }
  Symbol* pattern = symbol->static_data_member_pattern;
  Struct* owner = symbol->static_data_member_class;
  StructMember* member = MapFindPointerKey(&owner->symbol_table, &symbol->name);
  if (member == NULL || member->symbol != symbol) {
    return;
  }
  VariableDeclarationASTNode* definition =
      (VariableDeclarationASTNode*)pattern->static_data_member_template_definition;
  // A member variable template (`template <class U> static constexpr bool
  // v = ...;`) is instantiated per use, not here.
  if (definition == NULL && pattern->variable_template != NULL) {
    return;
  }
  // [temp.inst]: the definition is the out-of-class one
  // (`template <class T> const T C<T>::k[] = ...;`), or else the in-class
  // initializer of an inline / constexpr member.  A member that has neither
  // yet may still be defined later in this translation unit.  Once the
  // out-of-class definition is marked as a template, its initializer has
  // moved to the member's variable_template.
  ASTNode* pattern_initializer =
      definition != NULL ? definition->initializer : NULL;
  if (definition != NULL && pattern_initializer == NULL &&
      pattern->variable_template != NULL) {
    pattern_initializer = pattern->variable_template->initializer;
  }
  ASTNode* initializer = NULL;
  if (pattern_initializer != NULL) {
    TypeParser parser;
    TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                   syntax->context);
    parser.template_substitution_source = pattern->static_data_member_class;
    parser.template_substitution_target = owner;
    initializer = CloneDependentExpressionWithArgs(
        &parser, pattern_initializer,
        symbol->static_data_member_template_arguments);
    TypeParserDestruct(&parser);
    if (initializer == NULL) {
      initializer = CloneCXXDefaultMemberInitializer(pattern_initializer);
    }
  } else if (member->default_initializer != NULL) {
    initializer = CloneCXXDefaultMemberInitializer(member->default_initializer);
  } else if (definition == NULL) {
    Vector* undefined = &compiler->cxx_undefined_template_static_members;
    for (size_t i = 0; i < undefined->length; i++) {
      if (undefined->value.p[i] == symbol) {
        return;
      }
    }
    VectorAppend(undefined, symbol);
    return;
  }
  symbol->static_data_member_pattern = NULL;
  if (initializer != NULL && TypeIsArray(symbol->type) &&
      !symbol->type->info.array.is_flexible &&
      !symbol->type->info.array.is_vla &&
      !symbol->type->info.array.is_dependent_bound &&
      symbol->type->info.array.size.fixed <= 0) {
    symbol->type->info.array.is_flexible = true;
    TypeRecordCalculateSize(symbol->type);
  }
  // The specialization's member copied the pattern's flags, including the
  // template mark that its out-of-class definition received.
  symbol->flags.is_template = false;
  symbol->flags.is_defined = true;
  if (!StorageIs(symbol->storage, STO(static))) {
    symbol->flags.is_weak = true;
  }
  ASTNode* decl = NewVariableDeclarationASTNode(symbol, initializer,
                                                symbol->location);
  VectorAppend(&compiler->cxx_deferred_static_member_definitions, decl);
}

void TypeRecordStaticDataMemberDefinition(Syntax* syntax, Symbol* member,
                                          ASTNode* definition,
                                          bool is_template_definition) {
  if (member == NULL) {
    return;
  }
  if (!is_template_definition) {
    // An explicit specialization (`template <> const T C<X>::k[] = ...;`) is
    // this specialization's own definition.
    member->static_data_member_pattern = NULL;
    return;
  }
  member->static_data_member_template_definition = definition;
  Vector* undefined = &compiler->cxx_undefined_template_static_members;
  for (size_t i = 0; i < undefined->length;) {
    Symbol* symbol = undefined->value.p[i];
    if (symbol->static_data_member_pattern != member) {
      i++;
      continue;
    }
    VectorDeleteElement(undefined, i);
    TypeEnsureStaticDataMemberDefinition(syntax, symbol);
  }
}

TypeRecord* SubstituteNestedStructTemplateParameters(TypeParser* parser,
                                                            TypeRecord* type,
                                                            Vector* args) {
  Struct* from = type->info.struct_info;
  // A closure copied for one instantiation is named `__invented__N$S<serial>`.
  // Substituting that copy again (its lexical parent is now the instantiation,
  // which is the source of a nested substitution) invents another closure and
  // never reaches a fixed point.  The original, without `$S`, is what each
  // instantiation rebuilds.  A copy made for a class template still has the
  // capture fields of its member function template's pack (`Args&&... args`)
  // and is rebuilt once more when that member is instantiated.
  if (from->tag_symbol != NULL && from->tag_symbol->flags.invented &&
      from->tag_name != NULL && strstr(from->tag_name->value, "$S") != NULL &&
      !StructHasDependentDataMember(from)) {
    return TypeRecordCopy(type);
  }
  bool member_closure_resubstitution =
      from->tag_symbol != NULL && from->tag_symbol->flags.invented &&
      from->tag_name != NULL && strstr(from->tag_name->value, "$S") != NULL;
  // A concrete lambda used as a template argument (`is_copy_constructible<Lam>`)
  // is not part of the class being substituted.  Cloning it per mention makes
  // `T` and `const T&` name different closures, so the copy constructor does
  // not match.  The body scan treats leftover parameter indexes as dependence,
  // so a closure that merely calls other templates looks dependent.  Closures
  // nested in the class being instantiated, and closures in a function
  // template, still have to be rebuilt.
  if (from->tag_symbol != NULL && from->tag_symbol->flags.invented) {
    bool nested_in_substitution =
        parser != NULL &&
        ((parser->template_substitution_source != NULL &&
          from->lexical_parent == parser->template_substitution_source) ||
         (parser->enclosing_template_substitution_source != NULL &&
          from->lexical_parent ==
              parser->enclosing_template_substitution_source));
    Symbol* enclosing_fn = from->access_enclosing_function;
    bool inside_function_template =
        enclosing_fn != NULL &&
        (enclosing_fn->flags.is_template ||
         (enclosing_fn->type != NULL && TypeIsFunction(enclosing_fn->type) &&
          enclosing_fn->type->info.function.template_parameters.length > 0));
    if (!nested_in_substitution && !inside_function_template) {
      return TypeRecordCopy(type);
    }
  }
  // Cord's ChunkIterator member signatures name CharIterator, whose signatures
  // name Cord again.  Re-entering the substitution that is already building
  // that class would recurse until the stack overflows.  Share the in-progress
  // type; its struct is completed by the outer call.
  TypeRecord* in_progress = NestedSubstitutionAlreadyInProgress(from, args);
  if (in_progress != NULL) {
    return TypeRecordCopy(in_progress);
  }
  TypeRecord* cached_lambda = FindCachedLambdaSubstitution(from, args);
  if (cached_lambda != NULL) {
    return TypeRecordCopy(cached_lambda);
  }
  // An enclosing class template is still collecting members.  Keep this
  // nested class's function bodies until that class is complete so they can
  // call members declared later (raw_hash_set::emplace_at).
  bool defer_nested_bodies =
      parser != NULL && parser->template_substitution_target != NULL;
  TypeRecord* copy = TypeRecordCopy(type);
  // The copy's operator() has no body of its own.  Ensuring it clones the
  // lambda's pattern, which needs these member arguments after the class's.
  if (member_closure_resubstitution && copy->template_arguments == NULL &&
      args != NULL) {
    copy->template_arguments = TemplateArgumentVectorCopy(args);
  }
  Struct* str = NewStruct(from->is_union);
  if (parser != NULL && parser->template_substitution_target != NULL &&
      from->lexical_parent == parser->template_substitution_source) {
    str->lexical_parent = parser->template_substitution_target;
  } else if (parser != NULL &&
             parser->enclosing_template_substitution_target != NULL &&
             from->lexical_parent ==
                 parser->enclosing_template_substitution_source) {
    str->lexical_parent = parser->enclosing_template_substitution_target;
  } else {
    str->lexical_parent = from->lexical_parent;
  }
  str->access_enclosing_function = from->access_enclosing_function;
  if (from->tag_symbol != NULL && from->tag_symbol->flags.is_block_scope &&
      !from->tag_symbol->flags.invented && args != NULL) {
    str->local_class_arguments = TemplateArgumentVectorCopy(args);
  }
  if (StructHasMemberFunction(from)) {
    bool invented = from->tag_symbol != NULL && from->tag_symbol->flags.invented;
    String synthetic_name;
    StringInit(&synthetic_name, NULL);
    const char* tag_name = from->tag_name != NULL ? from->tag_name->value : NULL;
    if (invented || tag_name == NULL) {
      StringPrintf(&synthetic_name, "%s$S%d",
                   tag_name != NULL ? tag_name : "<anon>", str->serial);
      tag_name = synthetic_name.value;
    }
    Symbol* tag = NewSymbol(tag_name, copy, STO(implicit));
    tag->flags.is_defined = true;
    // Preserve invented-ness so nested lambda closures rebuilt during
    // template substitution are still recognized as lambda closures (their
    // operator() bodies must be cloned, not left lazy against the template).
    if (from->tag_symbol != NULL) {
      tag->flags.invented = from->tag_symbol->flags.invented;
    }
    str->tag_name = &tag->name;
    str->tag_symbol = tag;
    StringDestruct(&synthetic_name);
  } else {
    str->tag_name = from->tag_name;
    str->tag_symbol = from->tag_symbol;
  }
  str->is_class = from->is_class;
  str->is_template = from->is_template;
  if (str->is_template && str->tag_symbol != NULL) {
    str->tag_symbol->flags.is_template = true;
  }
  VectorDestructWithContents(&str->template_parameters,
                             (VectorElementDestructor)TemplateParameterDelete,
                             /*free_element=*/false);
  VectorInit(&str->template_parameters);
  int enclosing_template_parameter_count =
      args != NULL ? (int)args->length : 0;
  for (size_t i = 0; i < from->template_parameters.length; i++) {
    VectorAppend(&str->template_parameters,
                 CloneNestedTemplateParameter(
                     parser, from->template_parameters.value.p[i], args,
                     from->lexical_parent, str->lexical_parent));
  }
  str->is_aggregate = from->is_aggregate;
  str->cxx_special_members_complete = from->cxx_special_members_complete;
  str->packed = from->packed;
  str->explicit_alignment = from->explicit_alignment;
  str->pack = from->pack;
  for (size_t i = 0; i < from->friend_classes.length; i++) {
    StructAddFriendClass(str, from->friend_classes.value.p[i]);
  }
  for (size_t i = 0; i < from->friend_functions.length; i++) {
    StructAddFriendFunction(str, from->friend_functions.value.p[i]);
  }
  TypeRecordSetStructInfo(copy, str);
  copy->size = 0;
  PushNestedSubstitution(from, args, copy);

  Struct* saved_substitution_source = parser->template_substitution_source;
  Struct* saved_substitution_target = parser->template_substitution_target;
  Struct* saved_enclosing_substitution_source =
      parser->enclosing_template_substitution_source;
  Struct* saved_enclosing_substitution_target =
      parser->enclosing_template_substitution_target;
  if (saved_substitution_source != NULL && saved_substitution_target != NULL) {
    parser->enclosing_template_substitution_source =
        saved_substitution_source;
    parser->enclosing_template_substitution_target =
        saved_substitution_target;
  }
  parser->template_substitution_source = from;
  parser->template_substitution_target = str;
  ApplyFriendTypeDeclarations(parser, str, from, args);

  for (size_t i = 0; i < from->bases.length; i++) {
    CXXBaseSpecifier* template_base = from->bases.value.p[i];
    TypeRecord* base_type =
        SubstituteTemplateParameters(parser, template_base->type, args);
    // The nested class's own parameters were renumbered above to drop the
    // enclosing prefix.  A base such as `Base<T>` still names T at the old
    // index, so `Mono<const int&>` later instantiates `Base` instead of
    // `Base<const int&>` and virtual overrides do not match.
    RebaseTemplateParameterIndices(base_type,
                                   enclosing_template_parameter_count);
    base_type = MaterializeClassBaseType(parser, base_type);
    if (!TypeIsStructOrUnion(base_type)) {
      if (TypeIsStillDependentClassBase(base_type)) {
        // A nested class template's base can depend on its own parameters
        // (`ConstantIteratorsImpl<P, ...> : P::constant_iterators`).  Its
        // instantiations substitute the bases of this copy.
        if (str->is_template) {
          CXXBaseSpecifier* dependent_base = NewCXXBaseSpecifier(
              base_type, template_base->access, template_base->is_virtual);
          dependent_base->is_pack_expansion = template_base->is_pack_expansion;
          VectorAppend(&str->bases, dependent_base);
          continue;
        }
        TypeRecordDelete(base_type);
        continue;
      }
      SyntaxError(parser->syntax, "base class must be a class or struct type");
      TypeRecordDelete(base_type);
      continue;
    }
    TypeRecordCalculateSize(base_type);
    // The specifier takes the only reference of a fresh substitution.  Releasing
    // it would clear `Base<T>`'s arguments while the specifier still uses them.
    VectorAppend(&str->bases,
                 NewCXXBaseSpecifier(base_type, template_base->access,
                                     template_base->is_virtual));
  }
  CollectCXXVirtualBases(str);
  CopyCXXBaseVirtualMembers(str);
  LayoutCXXBaseSpecifiers(str);

  Vector deferred_member_functions;
  VectorInit(&deferred_member_functions);
  for (size_t i = 0; i < from->members.length; i++) {
    StructMember* member = from->members.value.p[i];
    if (member == NULL || member->symbol == NULL) {
      continue;
    }
    // Hidden vptr/vbptr fields describe the template definition's provisional
    // layout.  Concrete bases can change which class owns those fields, so they
    // must be recomputed after substitution instead of cloned as ordinary data
    // members.
    if (member == from->vptr_member || member == from->vbptr_member) {
      continue;
    }
    if (member->symbol->flags.is_parameter_pack && !member->is_static &&
        !member->is_member_function && !member->is_using_declaration &&
        !StructMemberIsNestedType(member)) {
      int pack_index = FirstTemplateParameterIndexInType(member->symbol->type);
      TemplateArgument* pack =
          pack_index >= 0 && args != NULL && (size_t)pack_index < args->length
              ? args->value.p[pack_index]
              : NULL;
      if (pack != NULL && pack->pack_arguments != NULL) {
        for (size_t j = 0; j < pack->pack_arguments->length; j++) {
          TypeRecord* member_type = SubstituteTemplateParametersForPackElement(
              parser, member->symbol->type, args, pack_index, j);
          // A by-reference capture of `Args&&... args` is stored as `Args*`.
          // With `Args = T&` the element is a pointer to `T`, not to `T&`.
          if (member_type != NULL && TypeIsPointer(member_type) &&
              member_type->next != NULL &&
              TypeIsReference(member_type->next) &&
              member_type->next->next != NULL) {
            TypeRecord* pointer = NewPointerTypeRecord(member_type->qualifiers);
            TypeRecordChain(pointer, member_type->next->next);
            TypeRecordDelete(member_type);
            member_type = pointer;
          }
          TypeRecordCalculateSize(member_type);
          String field_name;
          StringInit(&field_name, NULL);
          LambdaCapturePackElementName(&field_name,
                                       member->symbol->name.value, j);
          Symbol* member_symbol =
              NewSymbol(field_name.value, member_type, member->symbol->storage);
          StringDestruct(&field_name);
          member_symbol->location = member->symbol->location;
          member_symbol->flags = member->symbol->flags;
          member_symbol->flags.is_parameter_pack = false;
          member_symbol->value = member->symbol->value;
          member_symbol->dependent_value_template_parameter_index =
              member->symbol->dependent_value_template_parameter_index;
          VectorDestruct(&member_symbol->attributes);
          AttributeListClone(&member_symbol->attributes,
                             &member->symbol->attributes);
          SubstituteDependentSymbolValue(member_symbol, args);
          SubstituteDependentSymbolAlignment(parser, member_symbol, args);
          SubstituteStaticMemberInitializerValue(
              parser, member_symbol, member->default_initializer, args);
          if (!member_symbol->flags.value_set &&
              member->symbol->constexpr_initializer != NULL) {
            SubstituteStaticMemberInitializerValue(
                parser, member_symbol, member->symbol->constexpr_initializer,
                args);
          }

          StructMember* instantiated = NewStructMember(member_symbol);
          instantiated->access = member->access;
          instantiated->is_anon = member->is_anon;
          instantiated->is_static = member->is_static;
          instantiated->is_mutable = member->is_mutable;
          instantiated->is_member_function = member->is_member_function;
          instantiated->is_using_declaration = member->is_using_declaration;
          instantiated->bit_size = member->bit_size;
          instantiated->is_bit_field = member->is_bit_field;
          instantiated->bit_offset = member->bit_offset;
          instantiated->cxx_vcall_offset = member->cxx_vcall_offset;
          AlignNextOffsetForSymbol(str, member_symbol);
          instantiated->byte_offset = str->next_offset;
          instantiated->index = str->members.length;
          AddStructMember(parser, str, instantiated);
          UpdateStructSize(str, member_type, str->is_union);
        }
        continue;
      }
    }
    if (member->is_member_function) {
      VectorAppend(&deferred_member_functions, member);
      continue;
    }
    /* A by-reference lambda capture is stored as a pointer field `T*` whose
     * pointee `T` is the captured entity's type.  When the captured variable is
     * itself a forwarding reference (`V&& var`), `T` is the enclosing parameter
     * `V`; substituting `V = U&` would form an (invalid) pointer-to-reference
     * and flag substitution failure.  For a capture field this is not a
     * SFINAE error -- capturing a reference variable by reference captures the
     * underlying object -- so collapse `(U&)*` to `U*` and keep the prior
     * substitution-failed state. */
    bool saved_capture_subst_failed = parser->template_substitution_failed;
    TypeRecord* member_type =
        SubstituteTemplateParameters(parser, member->symbol->type, args);
    if (str->is_template && str->tag_symbol != NULL &&
        !str->tag_symbol->flags.invented) {
      RebaseTemplateParameterIndices(
          member_type, enclosing_template_parameter_count);
    }
    if (member->symbol->flags.invented && from->tag_symbol != NULL &&
        from->tag_symbol->flags.invented && member_type != NULL &&
        TypeIsPointer(member_type) && member_type->next != NULL &&
        TypeIsReference(member_type->next)) {
      TypeRecord* reference = member_type->next;
      TypeRecord* referent = reference->next;
      TypeRecordIncRef(referent);
      member_type->next = referent;
      member_type->type = referent != NULL ? referent->type : member_type->type;
      TypeRecordDelete(reference);
      parser->template_substitution_failed = saved_capture_subst_failed;
    }
    TypeRecordCalculateSize(member_type);
    Symbol* member_symbol =
        NewSymbol(member->symbol->name.value, member_type, member->symbol->storage);
    member_symbol->location = member->symbol->location;
    member_symbol->flags = member->symbol->flags;
    CopySymbolAliasTemplate(member_symbol, member->symbol);
    member_symbol->value = member->symbol->value;
    member_symbol->dependent_value_template_parameter_index =
        member->symbol->dependent_value_template_parameter_index;
    VectorDestruct(&member_symbol->attributes);
    AttributeListClone(&member_symbol->attributes,
                       &member->symbol->attributes);
    SubstituteDependentSymbolValue(member_symbol, args);
    SubstituteDependentSymbolAlignment(parser, member_symbol, args);
    SubstituteStaticMemberInitializerValue(parser, member_symbol,
                                           member->default_initializer, args);
    if (!member_symbol->flags.value_set &&
        member->symbol->constexpr_initializer != NULL) {
      SubstituteStaticMemberInitializerValue(
          parser, member_symbol, member->symbol->constexpr_initializer, args);
    }

    StructMember* instantiated = NewStructMember(member_symbol);
    // Substitute template parameters in a non-static default member initializer
    // (e.g. `W value = W();`).  A plain clone would leave the parameter-typed
    // value-initialization `W()` referencing the template parameter, which then
    // lowers to an undefined symbol; substitution rewrites it to e.g. `int()`.
    if (member->default_initializer != NULL) {
      ASTNode* substituted = NULL;
      // A nested class template's initializer (`static constexpr auto d =
      // &Payload<M>::Destroy;`) names its own parameters, renumbered from 0
      // like its member types.
      if (str->is_template && enclosing_template_parameter_count > 0 &&
          str->tag_symbol != NULL && !str->tag_symbol->flags.invented) {
        substituted = TypeSubstituteMemberTemplateExpressionAndRebase(
            parser->syntax, member->default_initializer, args,
            enclosing_template_parameter_count,
            member->default_initializer->location, NULL, NULL);
      }
      if (substituted == NULL) {
        substituted = CloneDependentExpressionWithArgs(
            parser, member->default_initializer, args);
      }
      instantiated->default_initializer =
          substituted != NULL
              ? substituted
              : CloneCXXDefaultMemberInitializer(member->default_initializer);
    } else {
      instantiated->default_initializer =
          CloneCXXDefaultMemberInitializer(member->default_initializer);
    }
    // A static constexpr aggregate (`static constexpr array<size_t, N> k =
    // {Ts...}`) is not a scalar, so the value fold above leaves it unset.
    // Constant evaluation reads symbol->constexpr_initializer, not the
    // member's default initializer.  Keep the substituted initializer there
    // once every template parameter in it has been replaced.
    if (member->is_static && !member_symbol->flags.value_set &&
        member_symbol->constexpr_initializer == NULL &&
        instantiated->default_initializer != NULL &&
        !DependentExpressionContainsTemplateParameter(
            instantiated->default_initializer)) {
      member_symbol->constexpr_initializer = ASTNodeClone(
          instantiated->default_initializer, IdentityCloneNode, NULL, NULL);
    }
    RecordStaticDataMemberPattern(member, member_symbol, args);
    instantiated->access = member->access;
    instantiated->is_anon = member->is_anon;
    instantiated->is_static = member->is_static;
    instantiated->is_mutable = member->is_mutable;
    instantiated->is_member_function = member->is_member_function;
    instantiated->is_using_declaration = member->is_using_declaration;
    instantiated->bit_size = member->bit_size;
    instantiated->is_bit_field = member->is_bit_field;
    instantiated->bit_offset = member->bit_offset;
    instantiated->cxx_vcall_offset = member->cxx_vcall_offset;

    if (!instantiated->is_static && !instantiated->is_member_function &&
        !instantiated->is_using_declaration &&
        !StructMemberIsNestedType(instantiated)) {
      AlignNextOffsetForSymbol(str, member_symbol);
      instantiated->byte_offset = str->next_offset;
      instantiated->index = str->members.length;
      if (instantiated->is_anon) {
        VectorAppend(&str->members, instantiated);
      } else {
        AddStructMember(parser, str, instantiated);
      }
      UpdateStructSize(str, member_type, str->is_union);
    } else {
      instantiated->byte_offset = member->byte_offset;
      instantiated->index = str->members.length;
      AddStructMember(parser, str, instantiated);
    }
  }
  Vector pending_member_bodies;
  VectorInit(&pending_member_bodies);
  for (size_t i = 0; i < deferred_member_functions.length; i++) {
    StructMember* member = deferred_member_functions.value.p[i];
    StructMember* instantiated = InstantiateTemplateMemberFunction(
        parser, str, member, args, &pending_member_bodies);
    StructMember* existing =
        MapFindPointerKey(&str->symbol_table, &instantiated->symbol->name);
    if (existing != NULL) {
      AppendStructMemberOverload(parser, str, existing, instantiated);
    } else {
      AddStructMember(parser, str, instantiated);
    }
  }
  VectorDestruct(&deferred_member_functions);
  for (size_t i = 0; i < pending_member_bodies.length; i++) {
    PendingMemberBody* pmb = pending_member_bodies.value.p[i];
    if (defer_nested_bodies) {
      DeferNestedMemberBody(str, pmb, args);
      continue;
    }
    CloneInstantiatedMemberFunctionBody(parser, str, pmb->symbol,
                                        pmb->template_definition,
                                        pmb->substitution_source, args);
    free(pmb);
  }
  VectorDestruct(&pending_member_bodies);
  EvaluateInstantiatedStaticAsserts(parser, from, str, args);
  parser->template_substitution_source = saved_substitution_source;
  parser->template_substitution_target = saved_substitution_target;
  parser->enclosing_template_substitution_source =
      saved_enclosing_substitution_source;
  parser->enclosing_template_substitution_target =
      saved_enclosing_substitution_target;
  if (StructHasBitFieldMembers(from)) {
    ResolveInstantiatedBitFieldWidths(parser, from, str, args);
    RelayoutStruct(str);
  }
  InjectInstantiatedAnonymousMembers(parser, str);
  FinalizeStructAlignment(str);
  ComputeCXXAggregateStatus(str);
  str->cxx_special_members_complete = false;
  // Nested classes of a class template are instantiated while the enclosing
  // template is still being parsed.  Special-member synthesis bails out in
  // that mode, which drops the implicit copy constructor (optional<set<T>::
  // const_iterator>).  The struct here is already a concrete instantiation.
  bool saved_synthesize = parser->synthesize_instantiated_special_members;
  parser->synthesize_instantiated_special_members = true;
  AddImplicitCXXSpecialMembers(parser, str, str->tag_symbol);
  AddImplicitCXXDestructorIfNeeded(parser, str, str->tag_symbol);
  parser->synthesize_instantiated_special_members = saved_synthesize;
  // A nested class template is completed when it is instantiated.
  if (!str->is_template) {
    CompleteInstantiatedPolymorphicLayout(parser, str);
  }
  RememberLambdaSubstitution(from, args, copy);
  PopNestedSubstitution(from);
  CloneNestedTemplatePartialSpecializations(parser, from, str, args);
  return TypeRecordCalculateSize(copy);
}

static bool TypeReferencesParameterAtOrAbove(TypeRecord* type, int rebase_base);

static bool ExprReferencesParameterAtOrAbove(ASTNode* node, void* data) {
  int rebase_base = *(int*)data;
  if (node == NULL || node->op != AST_OP(identifier) || rebase_base <= 0) {
    return false;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  if (id->symbol == NULL) {
    return false;
  }
  if (id->symbol->flags.is_template_parameter &&
      id->symbol->template_parameter_index >= rebase_base) {
    return true;
  }
  if (id->symbol->dependent_value_template_parameter_index >= rebase_base) {
    return true;
  }
  if (id->symbol->type != NULL &&
      TypeReferencesParameterAtOrAbove(id->symbol->type, rebase_base)) {
    return true;
  }
  // `Trait<const T&>::value` stores `T` on the identifier's template
  // arguments, not as a child node.  Missing it let enclosing-argument
  // substitution replace the member template's own parameter.
  if (id->template_arguments == NULL) {
    return false;
  }
  for (size_t i = 0; i < id->template_arguments->length; i++) {
    TemplateArgument* arg = id->template_arguments->value.p[i];
    if (arg == NULL) {
      continue;
    }
    if (arg->template_parameter_index >= rebase_base ||
        TypeReferencesParameterAtOrAbove(arg->type, rebase_base)) {
      return true;
    }
    if (arg->dependent_expr != NULL &&
        ASTNodeAny(arg->dependent_expr, ExprReferencesParameterAtOrAbove,
                   data)) {
      return true;
    }
  }
  return false;
}

/* True when `type` still names one of a member template's own parameters
 * (indices at or above the enclosing class's parameter count). */
static bool TypeReferencesParameterAtOrAbove(TypeRecord* type,
                                             int rebase_base) {
  if (type == NULL || rebase_base <= 0) {
    return false;
  }
  for (TypeRecord* current = type; current != NULL; current = current->next) {
    if (current->template_parameter_index >= rebase_base) {
      return true;
    }
    if (current->declarator == kDeclArray &&
        current->info.array.template_parameter_index >= rebase_base) {
      return true;
    }
    if (current->dependent_decltype_expr != NULL &&
        ASTNodeAny(current->dependent_decltype_expr,
                   ExprReferencesParameterAtOrAbove, &rebase_base)) {
      return true;
    }
    if (current->dependent_member_template_arguments != NULL) {
      for (size_t i = 0;
           i < current->dependent_member_template_arguments->length; i++) {
        Vector* args = current->dependent_member_template_arguments->value.p[i];
        if (args == NULL) {
          continue;
        }
        for (size_t j = 0; j < args->length; j++) {
          TemplateArgument* arg = args->value.p[j];
          if (arg == NULL) {
            continue;
          }
          if (arg->template_parameter_index >= rebase_base ||
              TypeReferencesParameterAtOrAbove(arg->type, rebase_base)) {
            return true;
          }
          if (arg->dependent_expr != NULL &&
              ASTNodeAny(arg->dependent_expr, ExprReferencesParameterAtOrAbove,
                         &rebase_base)) {
            return true;
          }
        }
      }
    }
    if (current->template_arguments == NULL) {
      continue;
    }
    for (size_t i = 0; i < current->template_arguments->length; i++) {
      TemplateArgument* arg = current->template_arguments->value.p[i];
      if (arg == NULL) {
        continue;
      }
      if (arg->template_parameter_index >= rebase_base) {
        return true;
      }
      if (TypeReferencesParameterAtOrAbove(arg->type, rebase_base)) {
        return true;
      }
      if (arg->dependent_expr != NULL &&
          ASTNodeAny(arg->dependent_expr, ExprReferencesParameterAtOrAbove,
                     &rebase_base)) {
        return true;
      }
    }
  }
  return false;
}

static void CopyFunctionTemplateParameters(TypeParser* parser, TypeRecord* to,
                                           TypeRecord* from, int rebase_base,
                                           Vector* subst_args) {
  if (to == NULL || from == NULL || !TypeIsFunction(to) ||
      !TypeIsFunction(from)) {
    return;
  }
  VectorDestructWithContents(&to->info.function.template_parameters,
                             (VectorElementDestructor)TemplateParameterDelete,
                             /*free_element=*/false);
  VectorInit(&to->info.function.template_parameters);
  for (size_t i = 0; i < from->info.function.template_parameters.length; i++) {
    TemplateParameter* original =
        from->info.function.template_parameters.value.p[i];
    if (original == NULL) {
      continue;
    }
    TemplateParameter* param =
        TemplateParameterCopy(original);
    // A non-type parameter's own type may name an enclosing-template parameter
    // too, which is where the pre-C++20 constraint idiom puts its condition:
    // `template <class T, enable_if_t<C<T, Types...>::value, int> = 0>` for a
    // member of `template <class... Types> struct box`.  Substitute the
    // enclosing arguments the same way, before rebasing, or `Types` keeps an
    // index that addresses one of the member's own parameters instead.
    if (param->type != NULL && subst_args != NULL && parser != NULL &&
        TypeContainsTemplateParameter(param->type)) {
      bool saved_failed = parser->template_substitution_failed;
      bool saved_enclosing_only =
          parser->substituting_enclosing_template_arguments_only;
      parser->template_substitution_failed = false;
      // `enable_if_t<Trait<T>::value, int>` still names this member template's
      // own parameter.  Resolving the trait against only the enclosing class
      // arguments binds `::value` on the primary, after which both SFINAE
      // overloads accept every call.
      parser->substituting_enclosing_template_arguments_only = true;
      TypeRecord* substituted =
          SubstituteTemplateParameters(parser, param->type, subst_args);
      parser->substituting_enclosing_template_arguments_only =
          saved_enclosing_only;
      bool failed = parser->template_substitution_failed;
      parser->template_substitution_failed = saved_failed;
      // A nested lookup can set the failure flag while still returning the
      // partially substituted type (class arguments filled in, the member
      // template's own parameters left in place).  That type has to be kept.
      // A failure that collapses to a concrete placeholder must not replace
      // the original dependent parameter type.
      // Substituting only the enclosing arguments can still bind a member
      // parameter whose index collides with an expanded class pack
      // (`enable_if_t<IsNotBitField<T>::value, int>` becomes `int`).  Keep
      // the original type so the call can SFINAE on `T`.
      // Substitution may replace the member parameter with another dependent
      // type (an expanded class pack) rather than with a concrete `int`.
      // Either way the member's own parameter is gone and the original type
      // has to stay so the call can still SFINAE on it.
      bool original_has_own =
          TypeReferencesParameterAtOrAbove(param->type, rebase_base);
      bool substituted_has_own =
          substituted != NULL &&
          TypeReferencesParameterAtOrAbove(substituted, rebase_base);
      bool own_parameter_erased =
          substituted != NULL && original_has_own && !substituted_has_own;
      if (substituted != NULL && !own_parameter_erased &&
          (!failed || TypeContainsTemplateParameter(substituted))) {
        TypeRecordDelete(param->type);
        param->type = substituted;
      } else {
        TypeRecordDelete(substituted);
      }
    }
    RebaseTemplateParameterIndices(param->type, rebase_base);
    // A parameter default may name an enclosing-template parameter (indices
    // below `rebase_base`), e.g. `template <class R = D>` for a member of a
    // class template with parameter `D`.  Substitute the enclosing arguments so
    // the default becomes concrete, then rebase the member's own placeholders.
    // A default that fails (`class = enable_if_t<sizeof...(Rest) == 1>` once
    // `Rest` is empty) only matters to a call that uses it.  It must not fail
    // whatever is instantiating the class, such as an overload candidate whose
    // return type names the class.
    if (param->default_type != NULL && subst_args != NULL && parser != NULL) {
      bool saved_failed = parser->template_substitution_failed;
      TypeRecord* substituted =
          SubstituteTemplateParameters(parser, param->default_type, subst_args);
      parser->template_substitution_failed = saved_failed;
      TypeRecordDelete(param->default_type);
      param->default_type = substituted;
    }
    RebaseTemplateParameterIndices(param->default_type, rebase_base);
    if (param->default_template_parameter_index >= rebase_base) {
      param->default_template_parameter_index -= rebase_base;
    }
    if (param->default_argument != NULL &&
        param->default_argument->template_parameter_index >= 0) {
      int default_index =
          param->default_argument->template_parameter_index;
      if (default_index < rebase_base && subst_args != NULL &&
          (size_t)default_index < subst_args->length) {
        TemplateArgument* actual = subst_args->value.p[default_index];
        if (actual != NULL) {
          TemplateArgumentDelete(param->default_argument);
          param->default_argument = TemplateArgumentCopy(actual);
        }
      } else if (default_index >= rebase_base) {
        param->default_argument->template_parameter_index -= rebase_base;
      }
    }
    if (param->index >= rebase_base) {
      param->index -= rebase_base;
    }
    // `int = enable_if_t<Trait<U>::value>()` is stored as a dependent
    // expression whose parameter indices still include the enclosing class
    // (`U` at 1 after class parameter `T`).  The later call-site fold only
    // has the member template's own arguments, numbered from 0.  Substitute
    // the enclosing arguments and rebase, or both SFINAE overloads fold the
    // primary trait and stay viable.
    if (param->default_argument != NULL &&
        param->default_argument->dependent_expr != NULL && parser != NULL &&
        parser->syntax != NULL && rebase_base > 0) {
      // Rebase only.  Substituting the enclosing class arguments here resolves
      // `enable_if_t<Trait<T>::value>()` before `T` exists and drops the
      // condition, so both SFINAE overloads stay viable.
      ASTNode* substituted = TypeSubstituteMemberTemplateExpressionAndRebase(
          parser->syntax, param->default_argument->dependent_expr,
          /*args=*/NULL, rebase_base,
          param->default_argument->dependent_expr->location,
          parser->template_substitution_source,
          parser->template_substitution_target);
      if (substituted != NULL) {
        param->default_argument->dependent_expr = substituted;
      }
    }
    VectorAppend(&to->info.function.template_parameters, param);
  }
}

static void AppendTemplateNonTypeInstantiationKey(
    String* name, TemplateArgument* arg);

static void AppendReflectionInstantiationKey(String* name,
                                             ReflectionValue* reflection) {
  if (reflection == NULL) {
    StringAppend(name, "R-");
    return;
  }
  StringAppendChar(name, 'R');
  StringAppendInt64(name, reflection->kind);
  StringAppend(name, ":T");
  StringAppendInt64(
      name,
      reflection->reflected_type != NULL ? reflection->reflected_type->id : -1);
  StringAppend(name, ":S");
  StringAppendInt64(name,
                    reflection->symbol != NULL ? reflection->symbol->id : -1);
  StringAppend(name, ":M");
  StringAppendInt64(
      name, reflection->member != NULL && reflection->member->symbol != NULL
                ? reflection->member->symbol->id
                : -1);
  StringAppend(name, ":B");
  StringAppendUInt64(name, reflection->base_index);
  StringAppend(name, ":P");
  StringAppendUInt64(name, reflection->parameter_index);
  StringAppend(name, ":Q");
  if (reflection->namespace_ != NULL) {
    StringAppendString(name, &reflection->namespace_->qualified_name);
  } else if (reflection->namespace_alias_target != NULL) {
    StringAppendString(name,
                       &reflection->namespace_alias_target->qualified_name);
  }
  StringAppend(name, ":A[");
  for (size_t i = 0; i < reflection->substituted_arguments.length; i++) {
    if (i != 0) {
      StringAppendChar(name, ',');
    }
    TemplateArgument* argument =
        reflection->substituted_arguments.value.p[i];
    if (argument->kind == kTemplateParameterType) {
      TypeRecordToTemplateKeyString(argument->type, name);
    } else if (argument->kind == kTemplateParameterTemplate) {
      StringAppend(name, "TT");
      StringAppendInt64(name, argument->template_symbol != NULL
                                  ? argument->template_symbol->id
                                  : argument->template_parameter_index);
    } else {
      AppendTemplateNonTypeInstantiationKey(name, argument);
    }
  }
  StringAppendChar(name, ']');
  if (reflection->kind == kReflectionValue) {
    if (reflection->constexpr_initializer != NULL) {
      StringAppend(name, ":O");
      if (!ConstexprObjectInitializerTemplateKey(
              reflection->reflected_type, reflection->constexpr_initializer,
              name)) {
        StringAppend(name, "?");
      }
    } else if (reflection->scalar_is_float) {
      StringPrintf(name, ":F%a", reflection->scalar_fvalue);
    } else {
      StringAppend(name, ":I");
      StringAppendInt64(name, reflection->scalar_ivalue);
    }
  }
}

static void AppendTemplateNonTypeInstantiationKey(
    String* name, TemplateArgument* arg) {
  switch (TemplateArgumentConcreteValueKind(arg)) {
    case kTemplateValueIntegral:
      StringAppendChar(name, 'I');
      StringAppendInt64(name, arg->int_value);
      break;
    case kTemplateValueNull:
      StringAppend(name, "N");
      break;
    case kTemplateValuePointer:
      StringAppendChar(name, 'P');
      StringAppendInt64(
          name, arg->value_symbol != NULL ? arg->value_symbol->id : -1);
      StringAppendChar(name, ':');
      StringAppendInt64(name, arg->value_offset);
      break;
    case kTemplateValueMemberPointer:
      StringAppendChar(name, 'M');
      StringAppendInt64(
          name, arg->value_symbol != NULL ? arg->value_symbol->id : -1);
      StringAppendChar(name, ':');
      StringAppendInt64(name, arg->value_offset);
      StringAppendChar(name, ':');
      StringAppendInt64(name, arg->value_adjustment);
      StringAppendChar(name, ':');
      StringAppendInt64(
          name, arg->member_function != NULL ? arg->member_function->id : -1);
      break;
    case kTemplateValueReflection:
      AppendReflectionInstantiationKey(name, arg->reflection_value);
      break;
    case kTemplateValueObject:
      StringAppend(name, "O");
      if (!ConstexprObjectInitializerTemplateKey(
              arg->type, arg->object_initializer, name)) {
        StringAppend(name, "?");
      }
      break;
    case kTemplateValueNone:
      StringPrintf(name, "D%p", (void*)arg->dependent_expr);
      break;
  }
}

static void AppendTemplateArgumentInstantiationKey(String* name,
                                                   TemplateArgument* arg) {
  if (arg->pack_arguments != NULL) {
    StringAppendChar(name, '[');
    for (size_t i = 0; i < arg->pack_arguments->length; i++) {
      if (i != 0) {
        StringAppendChar(name, ',');
      }
      AppendTemplateArgumentInstantiationKey(
          name, arg->pack_arguments->value.p[i]);
    }
    StringAppendChar(name, ']');
  } else if (arg->kind == kTemplateParameterType) {
    TypeRecordToTemplateKeyString(arg->type, name);
  } else if (arg->kind == kTemplateParameterTemplate) {
    if (arg->template_parameter_index >= 0) {
      StringAppend(name, "$TT");
      StringAppendInt64(name, arg->template_parameter_index);
    } else {
      StringAppend(name, "TT");
      StringAppendInt64(
          name, arg->template_symbol != NULL ? arg->template_symbol->id : -1);
    }
  } else if (arg->template_parameter_index >= 0) {
    StringAppend(name, "$N");
    StringAppendInt64(name, arg->template_parameter_index);
  } else {
    AppendTemplateNonTypeInstantiationKey(name, arg);
  }
}

void AppendTemplateInstantiationName(String* name, Symbol* templ,
                                     Vector* args) {
  Struct* template_struct =
      templ->type != NULL && TypeIsStructOrUnion(templ->type)
          ? templ->type->info.struct_info
          : NULL;
  if (template_struct != NULL && template_struct->lexical_parent != NULL) {
    StringSet(name, "$nested");
    StringAppendInt64(name, templ->id);
    StringAppendChar(name, '$');
    StringAppendString(name, &templ->name);
  } else {
    StringSet(name, templ->name.value);
  }
  StringAppendChar(name, '<');
  for (size_t i = 0; i < args->length; i++) {
    if (i != 0) {
      StringAppendChar(name, ',');
    }
    AppendTemplateArgumentInstantiationKey(name, args->value.p[i]);
  }
  StringAppendChar(name, '>');
}

/* Find an already-created instantiation tag named `name` for template `templ`,
 * or NULL. */
static Symbol* FindTemplateInstantiationTag(TypeParser* parser, Symbol* templ,
                                            String* name) {
  (void)parser;
  /* Look up the instantiation tag in exactly the template's own namespace.
   * Using a scope-walking lookup (SyntaxFindTag) here is wrong: two templates
   * with the same name in different namespaces (e.g. ::Foo<int> and
   * ns::Foo<int>) produce identical instantiation-name strings, so falling back
   * to enclosing/global scopes would alias the nested instantiation to a
   * previously created global one, corrupting its template_origin. This mirrors
   * how AddTemplateInstantiationTag inserts the tag. */
  if (templ->namespace_ != NULL &&
      templ->namespace_ != compiler->global_namespace) {
    return NamespaceFindTag(templ->namespace_, name);
  }
  return FindGlobalTag(name);
}

/* Register a freshly created instantiation tag in the template's own namespace
 * (temporarily switching the parser's tag scope so the tag is not added to some
 * unrelated local/current scope). Mirrors FindTemplateInstantiationTag. */
static bool AddTemplateInstantiationTag(TypeParser* parser, Symbol* templ,
                                        Symbol* tag) {
  LocalSymbolTable* saved_tag_stack = parser->syntax->local_tag_stack;
  Namespace* saved_namespace = parser->syntax->current_namespace;
  parser->syntax->local_tag_stack = NULL;
  parser->syntax->current_namespace =
      templ->namespace_ != NULL ? templ->namespace_ : compiler->global_namespace;
  bool added = SyntaxAddTag(parser->syntax, tag);
  parser->syntax->local_tag_stack = saved_tag_stack;
  parser->syntax->current_namespace = saved_namespace;
  return added;
}

/* Reject malformed member-function records. Virtual (and pure-virtual) member
 * functions are supported: the instantiation path completes the polymorphic
 * layout and emits the vtable(s) the same way a normal class definition does. */
static bool ClassTemplateInstantiationMembersSupported(TypeParser* parser,
                                                       Struct* str) {
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (StructMemberIsNestedType(member)) {
      continue;
    }
    if (member->is_member_function &&
        (member->symbol == NULL || member->symbol->type == NULL ||
         !TypeIsFunction(member->symbol->type))) {
      SyntaxError(parser->syntax,
                  "Cannot instantiate class template; member '%s' has no "
                  "function type",
                  member->symbol != NULL && member->symbol->name.value != NULL
                      ? member->symbol->name.value
                      : "<unknown>");
      return false;
    }
  }
  return true;
}

static void InjectInstantiatedAnonymousMembers(TypeParser* parser,
                                               Struct* str) {
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member == NULL || !member->is_anon || member->symbol == NULL ||
        member->symbol->type == NULL ||
        !TypeIsStructOrUnion(member->symbol->type) ||
        member->symbol->type->info.struct_info == NULL) {
      continue;
    }
    InjectCXXAnonymousMembers(parser, str,
                              member->symbol->type->info.struct_info,
                              member->byte_offset);
  }
}

static bool StructHasBitFieldMembers(Struct* str) {
  if (str == NULL) {
    return false;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member != NULL && StructMemberIsBitField(member)) {
      return true;
    }
  }
  return false;
}

/* Build the concrete function type for a member function of an instantiated
 * class template: copy all of `from`'s function attributes (cv/ref/noexcept,
 * special-member kind, coroutine info, etc.), substitute the return type and
 * each parameter type against `args` (expanding parameter packs), re-add the
 * implicit `this` parameter against the instantiated `owner`, and renumber
 * argument slots. */
static TypeRecord* InstantiateMemberFunctionType(TypeParser* parser,
                                                 Struct* owner,
                                                 bool is_static_member,
                                                 TypeRecord* from,
                                                 Vector* args,
                                                 SourceLocation location) {
  TypeRecord* func = NewFunctionTypeRecord();
  func->info.function.varargs = from->info.function.varargs;
  func->info.function.unknown_args = from->info.function.unknown_args;
  func->info.function.definition = false;
  func->info.function.is_inline = from->info.function.is_inline;
  func->info.function.is_constexpr = from->info.function.is_constexpr;
  func->info.function.is_consteval = from->info.function.is_consteval;
  func->info.function.is_constructor = from->info.function.is_constructor;
  func->info.function.is_destructor = from->info.function.is_destructor;
  func->info.function.is_const_member = from->info.function.is_const_member;
  func->info.function.is_volatile_member =
      from->info.function.is_volatile_member;
  func->info.function.has_explicit_object_parameter =
      from->info.function.has_explicit_object_parameter;
  func->info.function.ref_qualifier = from->info.function.ref_qualifier;
  func->info.function.is_explicit = from->info.function.is_explicit;
  func->info.function.is_explicit_conversion =
      from->info.function.is_explicit_conversion;
  func->info.function.explicit_condition = NULL;
  func->info.function.is_final = from->info.function.is_final;
  // Virtualness must survive instantiation so the instantiated member is
  // registered into a vtable slot (RegisterCXXVirtualMember, run via
  // AddStructMember, keys off is_virtual).  The concrete slot index is *not*
  // copied: it is reassigned during registration by override resolution against
  // the instantiated bases, matching how a normal class assigns slots.
  func->info.function.is_virtual = from->info.function.is_virtual;
  func->info.function.is_override = from->info.function.is_override;
  func->info.function.is_pure_virtual = from->info.function.is_pure_virtual;
  func->info.function.is_defaulted = from->info.function.is_defaulted;
  func->info.function.is_deleted = from->info.function.is_deleted;
  func->info.function.deleted_reason =
      from->info.function.deleted_reason != NULL
          ? NewStringWithLength(from->info.function.deleted_reason->value,
                                from->info.function.deleted_reason->length)
          : NULL;
  func->info.function.cxx_special_member_kind =
      from->info.function.cxx_special_member_kind;
  func->info.function.is_user_declared = from->info.function.is_user_declared;
  func->info.function.is_user_provided = from->info.function.is_user_provided;
  func->info.function.is_explicitly_defaulted =
      from->info.function.is_explicitly_defaulted;
  func->info.function.is_explicitly_deleted =
      from->info.function.is_explicitly_deleted;
  func->info.function.is_implicitly_declared =
      from->info.function.is_implicitly_declared;
  func->info.function.is_implicitly_deleted =
      from->info.function.is_implicitly_deleted;
  func->info.function.is_trivial_special_member =
      from->info.function.is_trivial_special_member;
  func->info.function.is_constexpr_eligible =
      from->info.function.is_constexpr_eligible;
  func->info.function.is_noexcept_eligible =
      from->info.function.is_noexcept_eligible;
  func->info.function.is_noexcept = from->info.function.is_noexcept;
  func->info.function.is_auto_return_deduced =
      from->info.function.is_auto_return_deduced;
  func->info.function.is_decltype_auto_return_deduced =
      from->info.function.is_decltype_auto_return_deduced;
  func->info.function.is_coroutine = from->info.function.is_coroutine;
  func->info.function.has_coroutine_syntax =
      from->info.function.has_coroutine_syntax;
  func->info.function.has_constexpr_if =
      from->info.function.has_constexpr_if;
  func->info.function.coroutine_promise_type =
      from->info.function.coroutine_promise_type != NULL
          ? TypeRecordCopy(from->info.function.coroutine_promise_type)
          : NULL;
  func->info.function.coroutine_frame_type =
      from->info.function.coroutine_frame_type != NULL
          ? TypeRecordCopy(from->info.function.coroutine_frame_type)
          : NULL;
  func->info.function.coroutine_suspend_count =
      from->info.function.coroutine_suspend_count;
  func->info.function.template_parameter_count =
      from->info.function.template_parameter_count;
  int member_template_base = from->info.function.template_parameter_base;
  int original_member_template_base = member_template_base;
  Struct* member_owner = from->info.function.cxx_member_owner;
  bool nested_member_owner =
      member_owner != NULL && member_owner->lexical_parent != NULL;
  if (nested_member_owner &&
      from->info.function.template_parameter_count == 0 && args != NULL) {
    // Ordinary members still need unresolved parameters rebased when a nested
    // class template is copied one enclosing-template layer at a time.
    member_template_base = (int)args->length;
  }
  if (nested_member_owner && args != NULL &&
      member_template_base > (int)args->length) {
    // A nested class template can be copied while only its enclosing class is
    // being instantiated. Remove only that supplied enclosing prefix so the
    // nested class parameters survive with the member template parameters.
    member_template_base = (int)args->length;
  }
  func->info.function.template_parameter_base =
      nested_member_owner && from->info.function.template_parameter_count > 0
          ? original_member_template_base - member_template_base
          : 0;
  // A member function template's own parameters are numbered at/after
  // `member_template_base`.  Only enclosing-template arguments (indices
  // below that base) may be substituted here; the member's own placeholders
  // must survive for later deduction.  When the member has already been
  // rebased to a standalone template (base == 0) and is rebuilt again —
  // e.g. a nested generic lambda whose closure is substituted while cloning
  // an outer generic-lambda body — the enclosing args would otherwise
  // consume the member's own `auto` placeholders at index 0.
  Vector enclosing_only_args;
  Vector* subst_args = args;
  bool use_enclosing_only = false;
  if (from->info.function.template_parameter_count > 0) {
    VectorInit(&enclosing_only_args);
    use_enclosing_only = true;
    Vector* enclosing_arguments = args;
    Symbol* member_origin = from->info.function.template_origin;
    if (nested_member_owner && member_origin != NULL &&
        member_origin->type != NULL &&
        member_origin->type->template_arguments != NULL &&
        !TemplateArgumentVectorContainsTemplateParameter(
            member_origin->type->template_arguments) &&
        member_origin->type->template_arguments->length >=
            (size_t)member_template_base) {
      enclosing_arguments = member_origin->type->template_arguments;
    } else if (nested_member_owner &&
               parser->template_substitution_target != NULL &&
               parser->template_substitution_target->tag_symbol != NULL &&
               parser->template_substitution_target->tag_symbol->type != NULL &&
               parser->template_substitution_target->tag_symbol->type
                       ->template_arguments != NULL &&
               !TemplateArgumentVectorContainsTemplateParameter(
                   parser->template_substitution_target->tag_symbol->type
                       ->template_arguments) &&
               parser->template_substitution_target->tag_symbol->type
                       ->template_arguments->length >=
                   (size_t)member_template_base) {
      enclosing_arguments =
          parser->template_substitution_target->tag_symbol->type
              ->template_arguments;
    } else if (nested_member_owner && owner != NULL &&
               owner->tag_symbol != NULL && owner->tag_symbol->type != NULL &&
               owner->tag_symbol->type->template_arguments != NULL &&
               !TemplateArgumentVectorContainsTemplateParameter(
                   owner->tag_symbol->type->template_arguments) &&
               owner->tag_symbol->type->template_arguments->length >=
                   (size_t)member_template_base) {
      enclosing_arguments = owner->tag_symbol->type->template_arguments;
    }
    for (size_t i = 0;
         enclosing_arguments != NULL && (int)i < member_template_base &&
         i < enclosing_arguments->length;
         i++) {
      VectorAppend(&enclosing_only_args, enclosing_arguments->value.p[i]);
    }
    subst_args = &enclosing_only_args;
  }
  // A member function template's explicit condition may depend on both its
  // enclosing class and its own template parameters.  Substitute only the
  // enclosing arguments here, then rebase the surviving member parameters to
  // the standalone function template's zero-based numbering.  Using `args`
  // directly would consume member parameters that happen to share an index
  // with an enclosing parameter and incorrectly make explicit constructors
  // implicit.
  if (from->info.function.explicit_condition != NULL) {
    ASTNode* condition = TypeSubstituteMemberTemplateExpressionAndRebase(
        parser->syntax, from->info.function.explicit_condition, subst_args,
        member_template_base, from->info.function.explicit_condition->location,
        parser->template_substitution_source,
        parser->template_substitution_target);
    if (ExpressionIsTemplateDependent(condition)) {
      func->info.function.explicit_condition = condition;
    } else {
      int64_t explicit_value = 0;
      DiagnosticSuppressBegin();
      condition = AnalyzeExpression(condition);
      bool folded = EvaluateIntegerExpression(condition, &explicit_value);
      DiagnosticSuppressEnd();
      if (folded) {
        func->info.function.is_explicit = explicit_value != 0;
        func->info.function.is_explicit_conversion =
            from->info.function.is_explicit_conversion && explicit_value != 0;
      }
      ASTNodeDelete(condition);
    }
  }
  // Copy the member's own template parameters, substituting enclosing arguments
  // into each parameter's default (e.g. a `= D` default that names the enclosing
  // class parameter) before rebasing.  Without this substitution the default is
  // left dangling and a parameter that can only be supplied by its default
  // (appearing solely in a non-deduced context) fails deduction.
  CopyFunctionTemplateParameters(parser, func, from, member_template_base,
                                 subst_args);
  TypeRecord* return_type =
      SubstituteTemplateParameters(parser, from->next, subst_args);
  RebaseTemplateParameterIndices(return_type, member_template_base);
  TypeRecordChain(func, return_type);

  if (is_static_member ||
      from->info.function.has_explicit_object_parameter) {
    func->info.function.cxx_member_owner = owner;
  } else {
    TypeRecordAddCXXThisParameter(func, owner, location);
  }
  size_t first_formal =
      from->info.function.cxx_member_owner != NULL && !is_static_member &&
              !from->info.function.has_explicit_object_parameter
          ? 1
          : 0;
  for (size_t i = first_formal; i < from->info.function.prototype.length; i++) {
    Symbol* formal = from->info.function.prototype.value.p[i];
    // The implicit `__complete_object` flag (present on constructors/destructors
    // of a class with virtual bases) is recreated by TypeRecordAddCXXThisParameter
    // for the instantiated function above; copying the source's copy as well would
    // duplicate it, giving the instantiated constructor a spurious extra int
    // parameter and breaking every call to it.
    if (formal != NULL && StringEqual(&formal->name, "__complete_object")) {
      continue;
    }
    if (formal != NULL && formal->flags.is_parameter_pack) {
      AppendSubstitutedFormalParameter(
          parser, &func->info.function.prototype, formal, subst_args,
          member_template_base);
      continue;
    }
    TypeRecord* formal_type =
        SubstituteTemplateParameters(parser, formal->type, subst_args);
    RebaseTemplateParameterIndices(formal_type, member_template_base);
    Symbol* clone = NewSymbol(formal->name.value, formal_type, formal->storage);
    clone->flags = formal->flags;
    clone->flags.is_argument = true;
    clone->location = formal->location;
    clone->default_argument =
        ASTNodeClone(formal->default_argument, IdentityCloneNode, NULL, NULL);
    VectorAppend(&func->info.function.prototype, clone);
  }
  for (size_t i = 0; i < func->info.function.prototype.length; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    formal->value.arg_number = (int32_t)i;
  }
  if (from->info.function.associated_constraint != NULL) {
    // Substituting an associated constraint is a purely type-level operation:
    // it resolves alias/decltype types such as `iterator_t<D>` (which itself is
    // `decltype(ranges::begin(declval<D&>()))`).  For a CRTP base whose Derived
    // is still incomplete at this point, such resolution can transiently name
    // helper function templates (e.g. `std::forward`/`std::move`) with degenerate
    // arguments.  Those must never be cloned, queued, or codegen'd here, so
    // perform the substitution in speculative (signature-only) mode.
    //
    // Resolving those alias/decltype types can also transiently *fail* while
    // Derived is incomplete (e.g. `ranges::begin(D&)` where `D::begin` is not
    // yet visible), emitting diagnostics.  This is only a best-effort signature
    // substitution: the constraint is re-checked properly when the member is
    // actually named/called.  Trap any such errors so they cannot leak into --
    // and poison -- an enclosing speculative context (e.g. a requires-expression
    // that merely instantiates this class), which would otherwise report the
    // requirement as unsatisfied.
    bool saved_trap = DiagnosticErrorTrapBegin();
    compiler->speculative_template_instantiation_depth++;
    func->info.function.associated_constraint =
        ConceptsSubstituteMemberConstraint(
            parser->syntax, from->info.function.associated_constraint,
            subst_args, member_template_base,
            from->info.function.cxx_member_owner, owner);
    compiler->speculative_template_instantiation_depth--;
    DiagnosticErrorTrapEnd(saved_trap);
  }
  if (use_enclosing_only) {
    // Shallow: elements are borrowed from `args`, not owned here.
    VectorDestruct(&enclosing_only_args);
  }
  (void)parser;
  return func;
}

bool TypeInstantiateVariableTemplateConstant(Syntax* syntax,
                                             Symbol* var_template, Vector* args,
                                             int64_t* out) {
  return TypeInstantiateVariableTemplateConstantImpl(syntax, var_template, args,
                                                       out,
                                                       /*emit_constraint_error=*/true);
}

bool TypeInstantiateVariableTemplateConstantQuiet(Syntax* syntax,
                                                  Symbol* var_template,
                                                  Vector* args, int64_t* out) {
  return TypeInstantiateVariableTemplateConstantImpl(syntax, var_template, args,
                                                       out,
                                                       /*emit_constraint_error=*/false);
}

static bool TypeInstantiateVariableTemplateConstantImpl(
    Syntax* syntax, Symbol* var_template, Vector* args, int64_t* out,
    bool emit_constraint_error) {
  if (var_template == NULL || var_template->variable_template == NULL ||
      var_template->variable_template->initializer == NULL) {
    return false;
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  Vector* completed_args = CompleteVariableTemplateArguments(
      &parser, var_template->variable_template, args, emit_constraint_error);
  if (completed_args == NULL) {
    TypeParserDestruct(&parser);
    return false;
  }
  // A matching partial (or explicit) specialization supplies its own
  // initializer and constraint, folded with the arguments deduced from the
  // specialization's pattern (its "bindings").
  Vector* partial_args = NULL;
  ClassTemplatePartialSpecialization* partial =
      SelectVariableTemplatePartialSpecialization(&parser, var_template,
                                                  completed_args, &partial_args);
  ASTNode* initializer = var_template->variable_template->initializer;
  ConstraintExpr* constraint =
      var_template->variable_template->associated_constraint;
  Vector* fold_args = completed_args;
  if (partial != NULL) {
    initializer = partial->variable_initializer;
    constraint = partial->associated_constraint;
    fold_args = partial_args;
  }
  bool ok = false;
  if (!ConceptsConstraintSatisfied(constraint, fold_args)) {
    if (emit_constraint_error && partial == NULL) {
      ReportVariableTemplateConstraintFailure(syntax, var_template,
                                              completed_args);
    }
  } else if (initializer != NULL) {
    ok = TryFoldDependentTemplateArgument(&parser, initializer, fold_args, out);
  }
  if (partial_args != NULL) {
    VectorDeleteWithContents(partial_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  VectorDeleteWithContents(completed_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  TypeParserDestruct(&parser);
  return ok;
}

static bool TypeInstantiateVariableTemplateFloatingConstantImpl(
    Syntax* syntax, Symbol* var_template, Vector* args, double* out,
    bool emit_constraint_error) {
  if (var_template == NULL || var_template->variable_template == NULL ||
      var_template->variable_template->initializer == NULL || out == NULL) {
    return false;
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  Vector* completed_args = CompleteVariableTemplateArguments(
      &parser, var_template->variable_template, args, emit_constraint_error);
  if (completed_args == NULL) {
    TypeParserDestruct(&parser);
    return false;
  }

  Vector* partial_args = NULL;
  ClassTemplatePartialSpecialization* partial =
      SelectVariableTemplatePartialSpecialization(
          &parser, var_template, completed_args, &partial_args);
  ASTNode* initializer = var_template->variable_template->initializer;
  ConstraintExpr* constraint =
      var_template->variable_template->associated_constraint;
  Vector* fold_args = completed_args;
  if (partial != NULL) {
    initializer = partial->variable_initializer;
    constraint = partial->associated_constraint;
    fold_args = partial_args;
  }

  bool ok = false;
  if (ConceptsConstraintSatisfied(constraint, fold_args) &&
      initializer != NULL) {
    ASTNode* concrete =
        CloneDependentExpressionWithArgs(&parser, initializer, fold_args);
    if (concrete != NULL) {
      concrete = AnalyzeExpression(concrete);
      ok = EvaluateFloatingPointExpression(concrete, out);
      ASTNodeDelete(concrete);
    }
  } else if (emit_constraint_error && partial == NULL) {
    ReportVariableTemplateConstraintFailure(syntax, var_template,
                                            completed_args);
  }
  if (partial_args != NULL) {
    VectorDeleteWithContents(partial_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             false);
  }
  VectorDeleteWithContents(completed_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           false);
  TypeParserDestruct(&parser);
  return ok;
}

bool TypeInstantiateVariableTemplateFloatingConstant(
    Syntax* syntax, Symbol* var_template, Vector* args, double* out) {
  return TypeInstantiateVariableTemplateFloatingConstantImpl(
      syntax, var_template, args, out, /*emit_constraint_error=*/true);
}

bool TypeInstantiateVariableTemplateFloatingConstantQuiet(
    Syntax* syntax, Symbol* var_template, Vector* args, double* out) {
  return TypeInstantiateVariableTemplateFloatingConstantImpl(
      syntax, var_template, args, out, /*emit_constraint_error=*/false);
}

static TypeRecord* TypeInstantiateVariableTemplateTypeImpl(
    Syntax* syntax, Symbol* var_template, Vector* args,
    bool emit_constraint_error) {
  if (var_template == NULL || var_template->variable_template == NULL ||
      var_template->type == NULL) {
    return NULL;
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  Vector* completed_args = CompleteVariableTemplateArguments(
      &parser, var_template->variable_template, args, emit_constraint_error);
  if (completed_args == NULL) {
    TypeParserDestruct(&parser);
    return NULL;
  }
  if (!ConceptsConstraintSatisfied(
          var_template->variable_template->associated_constraint,
          completed_args)) {
    if (emit_constraint_error) {
      ReportVariableTemplateConstraintFailure(syntax, var_template,
                                              completed_args);
    }
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    TypeParserDestruct(&parser);
    return NULL;
  }
  TypeRecord* concrete =
      SubstituteTemplateParameters(&parser, var_template->type, completed_args);
  VectorDeleteWithContents(completed_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  TypeParserDestruct(&parser);
  return concrete;
}

/* Instantiate the *type* of a C++ variable template against concrete template
 * arguments `args`, e.g. `in_place_index<1>` -> `in_place_index_t<1>`.  Used for
 * variable templates whose value is a class-type object (a tag such as
 * `std::in_place_index`) rather than a folded constant.  Returns a freshly
 * allocated concrete type, or NULL if the template has no type. */
TypeRecord* TypeInstantiateVariableTemplateType(Syntax* syntax,
                                                Symbol* var_template,
                                                Vector* args) {
  return TypeInstantiateVariableTemplateTypeImpl(syntax, var_template, args,
                                                 /*emit_constraint_error=*/true);
}

TypeRecord* TypeInstantiateVariableTemplateTypeQuiet(Syntax* syntax,
                                                     Symbol* var_template,
                                                     Vector* args) {
  return TypeInstantiateVariableTemplateTypeImpl(syntax, var_template, args,
                                                 /*emit_constraint_error=*/false);
}

ASTNode* TypeInstantiateVariableTemplateInitializer(Syntax* syntax,
                                                    Symbol* var_template,
                                                    Vector* args) {
  if (var_template == NULL || var_template->variable_template == NULL ||
      var_template->variable_template->initializer == NULL) {
    return NULL;
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  Vector* completed_args = CompleteVariableTemplateArguments(
      &parser, var_template->variable_template, args,
      /*emit_constraint_error=*/true);
  if (completed_args == NULL) {
    TypeParserDestruct(&parser);
    return NULL;
  }

  Vector* partial_args = NULL;
  ClassTemplatePartialSpecialization* partial =
      SelectVariableTemplatePartialSpecialization(
          &parser, var_template, completed_args, &partial_args);
  ASTNode* initializer = var_template->variable_template->initializer;
  ConstraintExpr* constraint =
      var_template->variable_template->associated_constraint;
  Vector* substitution_args = completed_args;
  if (partial != NULL) {
    initializer = partial->variable_initializer;
    constraint = partial->associated_constraint;
    substitution_args = partial_args;
  }

  ASTNode* concrete = NULL;
  if (ConceptsConstraintSatisfied(constraint, substitution_args) &&
      initializer != NULL) {
    concrete =
        CloneDependentExpressionWithArgs(&parser, initializer,
                                         substitution_args);
  } else if (partial == NULL) {
    ReportVariableTemplateConstraintFailure(syntax, var_template,
                                            completed_args);
  }
  if (partial_args != NULL) {
    VectorDeleteWithContents(partial_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             false);
  }
  VectorDeleteWithContents(completed_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           false);
  TypeParserDestruct(&parser);
  return concrete;
}

static void ParserApplyMemberOwnerSubstitution(TypeParser* parser,
                                               TypeRecord* func,
                                               Struct** saved_source,
                                               Struct** saved_target) {
  if (parser == NULL || saved_source == NULL || saved_target == NULL) {
    return;
  }
  *saved_source = parser->template_substitution_source;
  *saved_target = parser->template_substitution_target;
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.cxx_member_owner == NULL) {
    return;
  }
  Struct* owner = func->info.function.cxx_member_owner;
  parser->template_substitution_target = owner;
  if (owner->tag_symbol != NULL && owner->tag_symbol->type != NULL &&
      owner->tag_symbol->type->template_origin != NULL &&
      owner->tag_symbol->type->template_origin->type != NULL &&
      TypeIsStructOrUnion(owner->tag_symbol->type->template_origin->type)) {
    parser->template_substitution_source =
        owner->tag_symbol->type->template_origin->type->info.struct_info;
  } else {
    parser->template_substitution_source = owner;
  }
}

static TypeRecord* InstantiateFunctionTemplateType(TypeParser* parser,
                                                   TypeRecord* from,
                                                   Vector* args) {
  TypeRecord* func = NewFunctionTypeRecord();
  func->info.function.varargs = from->info.function.varargs;
  func->info.function.unknown_args = from->info.function.unknown_args;
  func->info.function.definition = false;
  func->info.function.old_style = from->info.function.old_style;
  func->info.function.is_inline = from->info.function.is_inline;
  func->info.function.is_constexpr = from->info.function.is_constexpr;
  func->info.function.is_consteval = from->info.function.is_consteval;
  func->info.function.is_const_member = from->info.function.is_const_member;
  func->info.function.is_volatile_member =
      from->info.function.is_volatile_member;
  func->info.function.has_explicit_object_parameter =
      from->info.function.has_explicit_object_parameter;
  func->info.function.ref_qualifier = from->info.function.ref_qualifier;
  // Preserve constructor/destructor-ness so an instantiated constructor
  // template is still recognized as a constructor (its call is void-typed and
  // must not be treated as a copy-initialization of the object).
  func->info.function.is_constructor = from->info.function.is_constructor;
  func->info.function.is_destructor = from->info.function.is_destructor;
  func->info.function.is_explicit = from->info.function.is_explicit;
  func->info.function.is_explicit_conversion =
      from->info.function.is_explicit_conversion;
  func->info.function.explicit_condition = NULL;
  if (from->info.function.explicit_condition != NULL) {
    int64_t explicit_value = 0;
    if (TryFoldDependentTemplateArgument(
            parser, from->info.function.explicit_condition, args,
            &explicit_value)) {
      func->info.function.is_explicit = explicit_value != 0;
      func->info.function.is_explicit_conversion =
          from->info.function.is_explicit_conversion && explicit_value != 0;
    } else {
      ASTNode* condition = CloneDependentExpressionWithArgs(
          parser, from->info.function.explicit_condition, args);
      if (DependentExpressionContainsTemplateParameter(condition)) {
        func->info.function.explicit_condition = condition;
      } else {
        ASTNodeDelete(condition);
      }
    }
  }
  func->info.function.is_virtual = from->info.function.is_virtual;
  func->info.function.is_override = from->info.function.is_override;
  func->info.function.is_final = from->info.function.is_final;
  func->info.function.is_pure_virtual = from->info.function.is_pure_virtual;
  func->info.function.is_defaulted = from->info.function.is_defaulted;
  func->info.function.is_deleted = from->info.function.is_deleted;
  func->info.function.cxx_special_member_kind =
      from->info.function.cxx_special_member_kind;
  func->info.function.is_user_declared = from->info.function.is_user_declared;
  func->info.function.is_user_provided = from->info.function.is_user_provided;
  func->info.function.is_explicitly_defaulted =
      from->info.function.is_explicitly_defaulted;
  func->info.function.is_explicitly_deleted =
      from->info.function.is_explicitly_deleted;
  func->info.function.is_implicitly_declared =
      from->info.function.is_implicitly_declared;
  func->info.function.is_implicitly_deleted =
      from->info.function.is_implicitly_deleted;
  func->info.function.is_trivial_special_member =
      from->info.function.is_trivial_special_member;
  func->info.function.is_constexpr_eligible =
      from->info.function.is_constexpr_eligible;
  func->info.function.is_noexcept_eligible =
      from->info.function.is_noexcept_eligible;
  func->info.function.is_noexcept = from->info.function.is_noexcept;
  func->info.function.is_auto_return_deduced =
      from->info.function.is_auto_return_deduced;
  func->info.function.is_decltype_auto_return_deduced =
      from->info.function.is_decltype_auto_return_deduced;
  func->info.function.is_deduction_guide =
      from->info.function.is_deduction_guide;
  func->info.function.is_coroutine = from->info.function.is_coroutine;
  func->info.function.has_coroutine_syntax =
      from->info.function.has_coroutine_syntax;
  func->info.function.has_constexpr_if =
      from->info.function.has_constexpr_if;
  func->info.function.coroutine_promise_type =
      from->info.function.coroutine_promise_type != NULL
          ? TypeRecordCopy(from->info.function.coroutine_promise_type)
          : NULL;
  func->info.function.coroutine_frame_type =
      from->info.function.coroutine_frame_type != NULL
          ? TypeRecordCopy(from->info.function.coroutine_frame_type)
          : NULL;
  func->info.function.coroutine_suspend_count =
      from->info.function.coroutine_suspend_count;
  func->info.function.virtual_index = from->info.function.virtual_index;
  func->info.function.cxx_member_owner = from->info.function.cxx_member_owner;
  Struct* saved_subst_source = NULL;
  Struct* saved_subst_target = NULL;
  ParserApplyMemberOwnerSubstitution(parser, from, &saved_subst_source,
                                     &saved_subst_target);
  // A member function template parsed inside a class template numbers its own
  // parameters after the enclosing ones (`template_parameter_base`).  Call
  // deduction records those parameters in a 0-based vector.  Rebase a copy of
  // the pattern before substituting so `hash_of(const K&)` binds `K` instead
  // of leaving the enclosing-relative placeholder in the specialization.
  int rebase_base = from->info.function.template_parameter_base;
  TypeRecord* return_pattern = from->next;
  TypeRecord* rebased_return = NULL;
  if (rebase_base > 0 && return_pattern != NULL) {
    rebased_return = TypeRecordCopy(return_pattern);
    RebaseTemplateParameterIndices(rebased_return, rebase_base);
    return_pattern = rebased_return;
  }
  TypeRecord* return_type =
      SubstituteTemplateParameters(parser, return_pattern, args);
  if (rebased_return != NULL) {
    TypeRecordDelete(rebased_return);
  }
  TypeRecordChain(func, return_type);

  for (size_t i = 0; i < from->info.function.prototype.length; i++) {
    Symbol* formal = from->info.function.prototype.value.p[i];
    if (formal == NULL) {
      continue;
    }
    if (formal->flags.is_parameter_pack) {
      AppendSubstitutedFormalParameter(parser, &func->info.function.prototype,
                                       formal, args, rebase_base);
      continue;
    }
    TypeRecord* formal_pattern = formal != NULL ? formal->type : NULL;
    TypeRecord* rebased_formal = NULL;
    if (rebase_base > 0 && formal_pattern != NULL) {
      rebased_formal = TypeRecordCopy(formal_pattern);
      RebaseTemplateParameterIndices(rebased_formal, rebase_base);
      formal_pattern = rebased_formal;
    }
    TypeRecord* formal_type =
        SubstituteTemplateParameters(parser, formal_pattern, args);
    if (rebased_formal != NULL) {
      TypeRecordDelete(rebased_formal);
    }
    Symbol* clone = NewSymbol(formal->name.value, formal_type, formal->storage);
    clone->flags = formal->flags;
    clone->flags.is_argument = true;
    clone->location = formal->location;
    clone->default_argument =
        ASTNodeClone(formal->default_argument, IdentityCloneNode, NULL, NULL);
    VectorAppend(&func->info.function.prototype, clone);
  }
  for (size_t i = 0; i < func->info.function.prototype.length; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    formal->value.arg_number = (int32_t)i;
  }
  if (parser != NULL) {
    parser->template_substitution_source = saved_subst_source;
    parser->template_substitution_target = saved_subst_target;
  }
  TypeCacheTemplateParameterSummary(func);
  return func;
}

/* Structural equality of two concrete template arguments (kind, pack contents,
 * type, or non-type value). Used to find an existing matching instantiation. */
bool TypeTemplateArgumentVectorEqual(Vector* left, Vector* right) {
  if (TemplateArgumentVectorEqual(left, right)) {
    return true;
  }
  if (left == NULL || right == NULL || left->length != right->length) {
    return false;
  }
  for (size_t i = 0; i < left->length; i++) {
    TemplateArgument* left_arg = left->value.p[i];
    TemplateArgument* right_arg = right->value.p[i];
    if (TemplateArgumentEqual(left_arg, right_arg)) {
      continue;
    }
    if (left_arg != NULL && left_arg->pack_arguments != NULL &&
        left_arg->pack_arguments->length == 1 &&
        TemplateArgumentEqual(left_arg->pack_arguments->value.p[0],
                              right_arg)) {
      continue;
    }
    if (right_arg != NULL && right_arg->pack_arguments != NULL &&
        right_arg->pack_arguments->length == 1 &&
        TemplateArgumentEqual(left_arg,
                              right_arg->pack_arguments->value.p[0])) {
      continue;
    }
    return false;
  }
  return true;
}

/* Structural equality of two value-dependent non-type template-argument
 * expressions (e.g. the conditions of two `enable_if` SFINAE overloads).  Only
 * the node shapes that appear in constant/SFINAE conditions are compared; any
 * other shape is conservatively treated as unequal so that distinct overloads
 * are never merged into one. */
/* Stack of function-template specializations whose bodies are currently being
 * cloned.  A self-recursive function template (e.g. an introsort or merge-sort
 * helper that calls itself on subranges) contains a call to the very
 * specialization being instantiated.  Cloning its body triggers that call,
 * which re-enters instantiation and finds the in-progress specialization.  The
 * body has not been assigned yet at that point, so without this marker the
 * re-entry would clone the body again, recursing until the stack overflows.
 * Instantiation runs single-threaded, so a plain intrusive stack suffices. */
static FunctionInstantiationInProgress* g_function_instantiations_in_progress =
    NULL;

bool FunctionTemplateInstantiationInProgress(Symbol* symbol) {
  for (FunctionInstantiationInProgress* node =
           g_function_instantiations_in_progress;
       node != NULL; node = node->next) {
    if (node->symbol == symbol) {
      return true;
    }
  }
  return false;
}

void PushFunctionInstantiationInProgress(
    FunctionInstantiationInProgress* node, Symbol* symbol) {
  node->symbol = symbol;
  node->next = g_function_instantiations_in_progress;
  g_function_instantiations_in_progress = node;
}

void PopFunctionInstantiationInProgress(
    FunctionInstantiationInProgress* node) {
  g_function_instantiations_in_progress = node->next;
}

static Vector* StructConcreteTemplateArguments(Struct* owner) {
  if (owner == NULL || owner->tag_symbol == NULL ||
      owner->tag_symbol->type == NULL) {
    return NULL;
  }
  Vector* args = owner->tag_symbol->type->template_arguments;
  if (args == NULL || args->length == 0 ||
      TemplateArgumentVectorContainsTemplateParameter(args)) {
    return NULL;
  }
  return args;
}

static bool StructDeclaresMemberNamed(Struct* str, String* name) {
  if (str == NULL || name == NULL) {
    return false;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member != NULL && member->symbol != NULL &&
        StringEqualString(&member->symbol->name, name)) {
      return true;
    }
  }
  return false;
}

static Vector* BaseDeclaringConcreteTemplateArguments(Struct* str, String* name,
                                                     int depth) {
  if (str == NULL || name == NULL || depth > 64) {
    return NULL;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base == NULL || base->type == NULL || !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    Struct* base_struct = base->type->info.struct_info;
    if (base_struct == str) {
      continue;
    }
    Vector* args = StructConcreteTemplateArguments(base_struct);
    if (args != NULL && StructDeclaresMemberNamed(base_struct, name)) {
      return args;
    }
    Vector* nested =
        BaseDeclaringConcreteTemplateArguments(base_struct, name, depth + 1);
    if (nested != NULL) {
      return nested;
    }
  }
  return NULL;
}

Vector* MemberFunctionEnclosingClassArguments(Symbol* symbol) {
  if (symbol == NULL || symbol->type == NULL ||
      !TypeIsFunction(symbol->type)) {
    return NULL;
  }
  Struct* owner = symbol->type->info.function.cxx_member_owner;
  Vector* direct = StructConcreteTemplateArguments(owner);
  if (direct != NULL) {
    return direct;
  }
  return BaseDeclaringConcreteTemplateArguments(owner, &symbol->name, 0);
}

bool CXXRetargetDropsDeclaringTemplateArguments(Struct* existing,
                                                Struct* named) {
  return existing != NULL && existing != named &&
         StructConcreteTemplateArguments(existing) != NULL &&
         StructConcreteTemplateArguments(named) == NULL;
}

static Vector* FunctionTemplateBodyArguments(Symbol* template_definition,
                                             Symbol* symbol,
                                             Vector* member_args) {
  if (template_definition == NULL || template_definition->type == NULL ||
      symbol == NULL || symbol->type == NULL ||
      !TypeIsFunction(template_definition->type) ||
      !TypeIsFunction(symbol->type)) {
    return member_args;
  }
  int enclosing_count =
      template_definition->type->info.function.template_parameter_base;
  size_t own_count =
      template_definition->type->info.function.template_parameters.length;
  // The body lives on the primary member template, whose parameters are still
  // numbered after the enclosing class.  Instantiating `get<0>` supplies only
  // `[0]`; without the class arguments (`[long, 0]`) `StorageT<I>` keeps `I`
  // unbound (`Storage<long, I0, StorageTag, I0>`) and `this` cannot convert
  // to that incomplete Storage base.
  if (enclosing_count > 0 && member_args != NULL &&
      member_args->length == own_count && symbol->type != NULL &&
      TypeIsFunction(symbol->type) &&
      symbol->type->info.function.cxx_member_owner != NULL) {
    Vector* class_args = MemberFunctionEnclosingClassArguments(symbol);
    if (class_args != NULL && class_args->length > 0) {
      Vector* combined = NewVector();
      for (size_t i = 0; i < class_args->length; i++) {
        VectorAppend(combined, TemplateArgumentCopy(class_args->value.p[i]));
      }
      for (size_t i = 0; i < member_args->length; i++) {
        VectorAppend(combined, TemplateArgumentCopy(member_args->value.p[i]));
      }
      return combined;
    }
  }
  for (size_t i = 0;
       enclosing_count > 0 &&
       i < template_definition->type->info.function.prototype.length;
       i++) {
    Symbol* formal =
        template_definition->type->info.function.prototype.value.p[i];
    int parameter_index =
        formal != NULL ? FirstTemplateParameterIndexInType(formal->type) : -1;
    if (parameter_index >= 0 && parameter_index < enclosing_count) {
      // The formal still names an enclosing parameter (`H state`), so this
      // body has not been rebased.  A pack can make `member_args` longer or
      // shorter than the template's own parameter list; prepend the class
      // arguments unless they are already in front.  Leaving them off binds
      // `H` to the first function argument (`H::combine` looks up `combine`
      // on `Cord` or `const Coroutine*`).
      Vector* class_args = MemberFunctionEnclosingClassArguments(symbol);
      if (class_args == NULL || class_args->length == 0 ||
          member_args == NULL) {
        return member_args;
      }
      bool already_prefixed = member_args->length >= class_args->length;
      for (size_t j = 0; already_prefixed && j < class_args->length; j++) {
        if (!TemplateArgumentEqual(member_args->value.p[j],
                                  class_args->value.p[j])) {
          already_prefixed = false;
        }
      }
      if (already_prefixed) {
        return member_args;
      }
      Vector* combined = NewVector();
      for (size_t j = 0; j < class_args->length; j++) {
        VectorAppend(combined, TemplateArgumentCopy(class_args->value.p[j]));
      }
      for (size_t j = 0; j < member_args->length; j++) {
        VectorAppend(combined, TemplateArgumentCopy(member_args->value.p[j]));
      }
      return combined;
    }
  }
  Symbol* member_template = symbol->type->info.function.template_origin;
  Vector* enclosing_args =
      member_template != NULL && member_template->type != NULL
          ? member_template->type->template_arguments
          : NULL;
  if (enclosing_count <= 0 || enclosing_args == NULL ||
      enclosing_args->length < (size_t)enclosing_count) {
    return member_args;
  }
  Vector* combined = NewVector();
  for (int i = 0; i < enclosing_count; i++) {
    VectorAppend(combined,
                 TemplateArgumentCopy(enclosing_args->value.p[i]));
  }
  for (size_t i = 0; member_args != NULL && i < member_args->length; i++) {
    VectorAppend(combined, TemplateArgumentCopy(member_args->value.p[i]));
  }
  return combined;
}

static void EnsureFunctionTemplateInstantiationQueued(TypeParser* parser,
                                                      Symbol* template_definition,
                                                      Symbol* symbol,
                                                      Vector* args) {
  if (parser == NULL || template_definition == NULL ||
      template_definition->type == NULL || symbol == NULL ||
      symbol->type == NULL ||
      template_definition->type->info.function.body == NULL ||
      (symbol->type->info.function.body != NULL &&
       symbol->type->info.function.definition) ||
      FunctionTemplateInstantiationInProgress(symbol) ||
      PendingTemplateInstantiationHasAsmName(symbol->asm_name.value)) {
    return;
  }
  // A body cloned only to deduce the return type was analyzed speculatively,
  // so the calls in it never queued their callees.  Clone it again.  The old
  // body is left alone: deduction may still refer to its nodes.
  symbol->type->info.function.body = NULL;
  FunctionInstantiationInProgress in_progress;
  PushFunctionInstantiationInProgress(&in_progress, symbol);
  Vector* body_args =
      FunctionTemplateBodyArguments(template_definition, symbol, args);
  symbol->type->info.function.body =
      CloneTemplateFunctionBody(parser, template_definition->type,
                                symbol->type, body_args);
  PopFunctionInstantiationInProgress(&in_progress);
  if (body_args != args) {
    VectorDeleteWithContents(body_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  SyntaxInsertClonedTemplateConstructorPreamble(parser, template_definition,
                                                symbol, args);
  symbol->type->info.function.definition = true;
  symbol->flags.is_defined = true;
  if (symbol->type->info.function.is_inline) {
    symbol->flags.is_inline_defn = true;
  }
  if (!StorageIs(symbol->storage, STO(static)) &&
      !symbol->flags.is_explicit_specialization) {
    symbol->flags.is_weak = true;
  }
  CompilerQueuePendingFunctionDefinition(symbol);
  VectorAppend(&compiler->declaration_asts, symbol->type->info.function.body);
}

static bool TemplateArgumentContainsTemplateParameterForInstantiation(
    TemplateArgument* arg) {
  if (arg == NULL) {
    return false;
  }
  if (arg->template_parameter_index >= 0 ||
      TypeContainsTemplateParameter(arg->type) ||
      DependentExpressionContainsTemplateParameter(arg->dependent_expr)) {
    return true;
  }
  for (size_t i = 0; arg->pack_arguments != NULL &&
                     i < arg->pack_arguments->length; i++) {
    if (TemplateArgumentContainsTemplateParameterForInstantiation(
            arg->pack_arguments->value.p[i])) {
      return true;
    }
  }
  return false;
}

/* True if any argument in the vector is still dependent (see above). */
static bool TemplateArgumentVectorContainsTemplateParameterForInstantiation(
    Vector* args) {
  for (size_t i = 0; args != NULL && i < args->length; i++) {
    if (TemplateArgumentContainsTemplateParameterForInstantiation(
            args->value.p[i])) {
      return true;
    }
  }
  return false;
}

/* An out-of-line member template (`HashStateBase<H>::combine`) may be defined
 * after a specialization of the class has already been built.  That
 * specialization copied the member while `func_defn` was still null.  The
 * primary's `func_defn` is filled in when the definition is parsed; use it. */
static Symbol* OutOfLineMemberTemplateDefinition(Symbol* templ) {
  if (templ == NULL || templ->type == NULL || !TypeIsFunction(templ->type)) {
    return NULL;
  }
  Symbol* direct = templ->value.func_defn;
  if (direct != NULL && direct->type != NULL && TypeIsFunction(direct->type) &&
      direct->type->info.function.body != NULL) {
    return direct;
  }
  if (templ->type->info.function.body != NULL) {
    return templ;
  }
  Struct* owner = templ->type->info.function.cxx_member_owner;
  if (owner == NULL || owner->tag_symbol == NULL ||
      owner->tag_symbol->type == NULL ||
      owner->tag_symbol->type->template_origin == NULL ||
      owner->tag_symbol->type->template_origin->type == NULL ||
      !TypeIsStructOrUnion(owner->tag_symbol->type->template_origin->type) ||
      owner->tag_symbol->type->template_origin->type->info.struct_info ==
          NULL) {
    return NULL;
  }
  Struct* primary =
      owner->tag_symbol->type->template_origin->type->info.struct_info;
  for (size_t i = 0; i < primary->members.length; i++) {
    StructMember* member = primary->members.value.p[i];
    if (member == NULL || member->symbol == NULL ||
        member->symbol->type == NULL ||
        !TypeIsFunction(member->symbol->type) ||
        !StringEqualString(&member->symbol->name, &templ->name) ||
        member->symbol->flags.is_template != templ->flags.is_template ||
        member->symbol->type->info.function.template_parameter_count !=
            templ->type->info.function.template_parameter_count ||
        member->symbol->type->info.function.prototype.length !=
            templ->type->info.function.prototype.length) {
      continue;
    }
    Symbol* defn = member->symbol->value.func_defn;
    if (defn != NULL && defn->type != NULL && TypeIsFunction(defn->type) &&
        defn->type->info.function.body != NULL) {
      return defn;
    }
    if (member->symbol->type->info.function.body != NULL) {
      return member->symbol;
    }
  }
  return NULL;
}

/* Re-analyze one assignment whose operand conversion was deferred because an
 * operand was still type-dependent when the enclosing member function template
 * was first (class-level) instantiated.  Now that the body has been cloned with
 * concrete arguments the correct conversion can be selected. */
static Symbol* InstantiateSimpleFunctionTemplate(TypeParser* parser,
                                                 Symbol* templ,
                                                 Vector* args,
                                                 bool args_are_completed) {
  if (templ == NULL || templ->type == NULL || !templ->flags.is_template ||
      !TypeIsFunction(templ->type)) {
    return templ;
  }
  if (templ->type->info.function.template_origin != NULL &&
      templ->type->template_arguments != NULL &&
      templ->type->info.function.body != NULL &&
      !TypeContainsTemplateParameter(templ->type)) {
    // This is already a concrete specialization.  Re-instantiating it with its
    // retained arguments substitutes pointer/reference declarators a second
    // time (for example allocator<T>::destroy<U>(U*) becoming U**).
    return templ;
  }
  if (templ->type->info.function.template_origin != NULL &&
      templ->type->template_arguments == NULL &&
      (!templ->is_imported_module_symbol ||
       templ->type->info.function.template_parameters.length == 0)) {
    templ = templ->type->info.function.template_origin;
  }
  TypeRecord* completion_type = templ->type;
  if (templ->is_imported_module_symbol && completion_type != NULL &&
      TypeIsFunction(completion_type) &&
      completion_type->info.function.template_parameters.length == 0 &&
      templ->value.func_defn != NULL && templ->value.func_defn->type != NULL &&
      TypeIsFunction(templ->value.func_defn->type) &&
      templ->value.func_defn->type->info.function.template_parameters.length > 0) {
    completion_type = templ->value.func_defn->type;
  }
  bool owns_completed_args = !args_are_completed;
  Vector* completed_args =
      args_are_completed
          ? args
          : CompleteFunctionTemplateArguments(parser, completion_type, args,
                                              /*emit_error=*/true,
                                              kTemplateArgumentsBorrow);
  if (completed_args == NULL) {
    return templ;
  }
  if (TemplateArgumentVectorContainsTemplateParameterForInstantiation(
          completed_args)) {
    if (owns_completed_args) {
      TemplateArgumentVectorDelete(completed_args);
    }
    return templ;
  }
  Symbol* template_definition = templ;
  if ((completion_type != templ->type ||
       template_definition->type == NULL ||
       template_definition->type->info.function.body == NULL) &&
      templ->value.func_defn != NULL &&
      templ->value.func_defn->type != NULL &&
      templ->value.func_defn->type->info.function.body != NULL) {
    template_definition = templ->value.func_defn;
  }
  if (template_definition->type == NULL ||
      template_definition->type->info.function.body == NULL) {
    Symbol* out_of_line = OutOfLineMemberTemplateDefinition(templ);
    if (out_of_line != NULL) {
      template_definition = out_of_line;
      if (templ->value.func_defn == NULL ||
          templ->value.func_defn->type == NULL ||
          templ->value.func_defn->type->info.function.body == NULL) {
        templ->value.func_defn = out_of_line;
      }
    }
  }
  // A declaration-only function template (`extern template` specializations,
  // `std::declval`) still gets a concrete signature so a call can be type-checked.
  // There is no body to clone; the symbol stays undefined for the linker.
  bool saved_substitution_failed = parser->template_substitution_failed;
  parser->template_substitution_failed = false;
  TypeRecord* func =
      InstantiateFunctionTemplateType(parser, completion_type,
                                      completed_args);
  bool substitution_failed = parser->template_substitution_failed;
  parser->template_substitution_failed = saved_substitution_failed;
  bool pattern_trailing_decltype =
      completion_type != NULL && completion_type->next != NULL &&
      (completion_type->next->dependent_decltype_expr != NULL ||
       TypeIsDetachedDependentDecltype(completion_type->next));
  bool unresolved_trailing_decltype =
      func != NULL && func->next != NULL && TypeIsUnknown(func->next) &&
      pattern_trailing_decltype;
  if (substitution_failed || unresolved_trailing_decltype) {
    // A dependent type in the signature (e.g. an `enable_if` SFINAE guard) had
    // no valid substitution.  Abandon this instantiation quietly so overload
    // resolution can discard the candidate; do not create or queue a symbol.
    TypeRecordDelete(func);
    if (owns_completed_args) {
      TemplateArgumentVectorDelete(completed_args);
    }
    return templ;
  }
  Symbol* existing = FindFunctionTemplateInstantiation(templ, func,
                                                       completed_args);
  if (existing != NULL) {
    if (compiler->speculative_template_instantiation_depth == 0) {
      EnsureFunctionTemplateInstantiationQueued(
          parser, template_definition, existing, completed_args);
    }
    TypeRecordDelete(func);
    if (owns_completed_args) {
      TemplateArgumentVectorDelete(completed_args);
    }
    return existing;
  }

  Symbol* symbol = NewSymbol(templ->name.value, func, templ->storage);
  symbol->location = templ->location;
  symbol->namespace_ = templ->namespace_;
  func->info.function.symbol = symbol;
  func->info.function.template_origin = templ;
  // Freshly completed arguments can move directly into permanent storage.
  // Already-completed caller arguments remain borrowed and are copied only on
  // this cache-miss path; cache hits above require no argument copy.
  func->template_arguments =
      owns_completed_args ? completed_args
                          : TemplateArgumentVectorCopy(completed_args);
  Vector* instantiation_args = func->template_arguments;
  SymbolSetCXXMangledAsmName(symbol);
  existing = FindFunctionTemplateInstantiationByAsmName(templ,
                                                       symbol->asm_name.value);
  if (existing != NULL) {
    if (compiler->speculative_template_instantiation_depth == 0) {
      EnsureFunctionTemplateInstantiationQueued(
          parser, template_definition, existing, instantiation_args);
    }
    SymbolDelete(symbol);
    return existing;
  }
  if (PendingTemplateInstantiationHasAsmName(symbol->asm_name.value)) {
    symbol->type->info.function.definition = true;
    symbol->flags.is_defined = true;
    AppendFunctionTemplateInstantiation(templ, symbol);
    return symbol;
  }
  VectorDestruct(&symbol->attributes);
  AttributeListClone(&symbol->attributes, &templ->attributes);
  // Register this instantiation in the template's cache *before* cloning its
  // body.  A recursive function template (e.g. an introsort/merge-sort helper
  // that calls itself on subranges) contains a call to the very specialization
  // being instantiated; cloning the body triggers that call, which re-enters
  // here.  If the symbol were not yet recorded, the lookups above would miss it
  // and we would instantiate an endless chain of identical specializations
  // until the stack overflows.  Recording it first lets the recursive call
  // resolve to this in-progress symbol and terminate.
  AppendFunctionTemplateInstantiation(templ, symbol);
  // A signature probe (`decltype(IsTrivial<A>())`) of a function with a
  // placeholder return type still needs the body to deduce that type, and a
  // constexpr function still needs it for a constant inside the probe
  // (`decltype(Or({integral_constant<bool, Should<T>()>()}))`).  Such a body
  // is not a definition; a later real use clones it again.
  bool deduction_only_body =
      compiler->speculative_template_instantiation_depth > 0 &&
      (TypeFunctionReturnContainsAuto(symbol->type) ||
       symbol->type->info.function.is_constexpr ||
       symbol->type->info.function.is_consteval);
  if (template_definition->type->info.function.body != NULL &&
      (compiler->speculative_template_instantiation_depth == 0 ||
       deduction_only_body)) {
    FunctionInstantiationInProgress in_progress;
    PushFunctionInstantiationInProgress(&in_progress, symbol);
    Vector* body_args =
        FunctionTemplateBodyArguments(template_definition, symbol,
                                      instantiation_args);
    symbol->type->info.function.body =
        CloneTemplateFunctionBody(parser, template_definition->type,
                                  symbol->type, body_args);
    PopFunctionInstantiationInProgress(&in_progress);
    // A member function *template* constructor has its member-initializer
    // preamble intentionally deferred from class instantiation (see
    // QueueTemplateMemberFunctionDefinitionImpl); insert it here now that the
    // member's own template arguments are concrete, or its base/member
    // subobjects (and any constexpr evaluation of them) would be skipped.
    SyntaxInsertClonedTemplateConstructorPreamble(parser, template_definition,
                                                  symbol, body_args);
    if (body_args != instantiation_args) {
      VectorDeleteWithContents(body_args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    ReanalyzeDeferredDependentAssignments(parser, symbol->type);
    if (deduction_only_body) {
      return symbol;
    }
    symbol->type->info.function.definition = true;
    symbol->flags.is_defined = true;
    if (symbol->type->info.function.is_inline) {
      symbol->flags.is_inline_defn = true;
    }
    if (!StorageIs(symbol->storage, STO(static)) &&
        !symbol->flags.is_explicit_specialization) {
      symbol->flags.is_weak = true;
    }
    CompilerQueuePendingFunctionDefinition(symbol);
    VectorAppend(&compiler->declaration_asts, symbol->type->info.function.body);
  }
  return symbol;
}

/* Create a type template argument holding an independent copy of a deduced
 * type.  Deduced array/reference spines must not share element records with the
 * argument expression because deduction temporaries are destroyed earlier. */
static TemplateArgument* NewDeducedTypeTemplateArgument(TypeRecord* type) {
  TemplateArgument* arg = TemplateArgumentAlloc();
  arg->kind = kTemplateParameterType;
  arg->is_pack_expansion = false;
  arg->type = TypeRecordCalculateSize(TypeRecordCloneSpine(type));
  arg->int_value = 0;
  arg->template_parameter_index = -1;
  arg->pack_arguments = NULL;
  arg->dependent_expr = NULL;
  arg->location = SOURCE_LOCATION_MISSING;
  return arg;
}

/* Create a non-type template argument holding a deduced integer value. */
static TemplateArgument* NewDeducedNonTypeTemplateArgument(long long value) {
  TemplateArgument* arg = TemplateArgumentAlloc();
  arg->kind = kTemplateParameterNonType;
  arg->is_pack_expansion = false;
  arg->type = NULL;
  arg->int_value = value;
  arg->value_kind = kTemplateValueIntegral;
  arg->template_parameter_index = -1;
  arg->pack_arguments = NULL;
  arg->dependent_expr = NULL;
  arg->location = SOURCE_LOCATION_MISSING;
  return arg;
}

/* The deduced type for a parameter `T` from an argument: a top-level cv-stripped
 * copy of the actual type (cv-qualifiers are not deduced into a bare `T`). */
static TypeRecord* FunctionTemplateDeductionActualType(TypeRecord* actual) {
  if (actual == NULL) {
    return NULL;
  }
  TypeRecord* deduced =
      TypeRecordCalculateSize(TypeRecordCloneSpine(actual));
  deduced->qualifiers = kQualPlain;
  return deduced;
}

/* Record a deduced type for template parameter `index`. Explicitly supplied
 * arguments (index < explicit_arg_count) are kept as-is; otherwise set the
 * deduced type, or verify consistency if it was already deduced elsewhere. */
static bool SetDeducedFunctionTemplateTypeArgumentImpl(Vector* args,
                                                       size_t explicit_arg_count,
                                                       int index,
                                                       TypeRecord* actual,
                                                       bool strip_top_level_cv) {
  index = DeductionArgumentIndex(index);
  if (index < 0 || (size_t)index >= args->length) {
    return false;
  }
  if ((size_t)index < explicit_arg_count) {
    return true;
  }

  TypeRecord* deduced = strip_top_level_cv
                            ? FunctionTemplateDeductionActualType(actual)
                            : TypeRecordCalculateSize(
                                  TypeRecordCloneSpine(actual));
  TemplateArgument* existing = args->value.p[index];
  if (existing == NULL) {
    args->value.p[index] = NewDeducedTypeTemplateArgument(deduced);
    TypeRecordDelete(deduced);
    return true;
  }
  bool same = existing->kind == kTemplateParameterType &&
              existing->type != NULL &&
              TypeEqual(existing->type, deduced);
  TypeRecordDelete(deduced);
  return same;
}

static bool SetDeducedFunctionTemplateTypeArgument(Vector* args,
                                                   size_t explicit_arg_count,
                                                   int index,
                                                   TypeRecord* actual) {
  return SetDeducedFunctionTemplateTypeArgumentImpl(
      args, explicit_arg_count, index, actual, /*strip_top_level_cv=*/true);
}

static bool SetDeducedFunctionTemplateTypeArgumentPreserveQualifiers(
    Vector* args, size_t explicit_arg_count, int index, TypeRecord* actual) {
  return SetDeducedFunctionTemplateTypeArgumentImpl(
      args, explicit_arg_count, index, actual, /*strip_top_level_cv=*/false);
}

static bool AppendDeducedFunctionTemplatePackElement(
    Vector* args, size_t explicit_arg_count, int pack_index, TypeRecord* formal,
    TypeRecord* actual) {
  pack_index = DeductionArgumentIndex(pack_index);
  if (pack_index < 0 || args == NULL || (size_t)pack_index >= args->length) {
    return false;
  }
  TemplateArgument* pack = args->value.p[pack_index];
  if (pack == NULL) {
    pack = NewEmptyPackTemplateArgument(kTemplateParameterType);
    args->value.p[pack_index] = pack;
  }
  if (pack->kind != kTemplateParameterType || pack->pack_arguments == NULL) {
    return false;
  }

  Vector* element_args = TemplateArgumentVectorCopy(args);
  TemplateArgumentDelete(element_args->value.p[pack_index]);
  element_args->value.p[pack_index] = NULL;
  bool ok = DeduceFunctionTemplateTypeArgument(element_args, explicit_arg_count,
                                               formal, actual);
  TemplateArgument* element = ok ? element_args->value.p[pack_index] : NULL;
  ok = ok && element != NULL && element->kind == kTemplateParameterType &&
       element->pack_arguments == NULL;
  if (ok) {
    for (size_t i = 0; i < element_args->length; i++) {
      if (i == (size_t)pack_index) {
        continue;
      }
      TemplateArgument* deduced = element_args->value.p[i];
      TemplateArgument* existing = args->value.p[i];
      if (deduced == NULL) {
        continue;
      }
      if (existing == NULL) {
        args->value.p[i] = TemplateArgumentCopy(deduced);
      } else if (!TemplateArgumentEqual(existing, deduced)) {
        ok = false;
        break;
      }
    }
  }
  if (ok) {
    VectorAppend(pack->pack_arguments, TemplateArgumentCopy(element));
  }
  VectorDeleteWithContents(element_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return ok;
}

/* [temp.deduct.call]: when a function parameter is not a reference, an array
 * argument is replaced by the array-to-pointer result and a function argument
 * by the function-to-pointer result before deduction.  Returns a freshly
 * allocated decayed type with one reference held (release it with
 * TypeRecordDelete), or NULL when no decay applies and the caller should deduce
 * against the argument's own type.  Top-level cv-qualifiers on the decayed
 * pointer are dropped, as the standard requires. */
static TypeRecord* DecayCallArgumentTypeForDeduction(TypeRecord* formal,
                                                     TypeRecord* actual) {
  if (formal == NULL || actual == NULL) {
    return NULL;
  }
  // Decay applies to every by-value function parameter, including a bare
  // placeholder (`T`) and compound patterns such as `T*`. References retain
  // the array/function type. Keep an explicit array pattern structural because
  // it is used by aggregate deduction to recover its bound.
  if (TypeIsReference(formal) || formal->declarator == kDeclArray) {
    return NULL;
  }
  TypeRecord* decayed = NULL;
  if (actual->declarator == kDeclArray) {
    decayed = NewPointerTo(kQualPlain, TypeRecordCopy(actual->next));
  } else if (actual->declarator == kDeclFunction) {
    decayed = NewPointerTo(kQualPlain, TypeRecordCopy(actual));
  }
  if (decayed != NULL) {
    TypeRecordCalculateSize(decayed);
    TypeRecordIncRef(decayed);
  }
  return decayed;
}

/* An id-expression naming a function parameter is an lvalue.  Trailing
 * decltype substitution deduces calls such as `std::move(state)` before that
 * identifier is analyzed, while its value category is still the default
 * prvalue.  Treating the parameter as an rvalue deduces `T` rather than `T&`,
 * and the resulting `T&&` then rejects the same argument once it is
 * categorized as an lvalue.
 *
 * A braced temporary (`Find{key}`) is lowered to a compound literal, which
 * keeps C's lvalue category, but in C++ it is a prvalue and deduces `T`. */
static bool ForwardingArgumentIsLvalue(ASTNode* actual) {
  if (actual == NULL) {
    return false;
  }
  if (CompilerIsCXX() && actual->op == AST_OP(compound_literal)) {
    return false;
  }
  if (actual->value_category == kValueCategoryLvalue) {
    return true;
  }
  if (actual->value_category != kValueCategoryPrvalue ||
      (actual->flags & kASTAnalyzed) != 0 ||
      actual->op != AST_OP(identifier)) {
    return false;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)actual;
  return id->symbol != NULL && id->symbol->flags.is_argument;
}

/* Deduce a pack element from a call argument expression, applying forwarding-
 * reference rules: a `T&&` pack parameter binding an lvalue deduces `T&`
 * (reference collapsing); otherwise deduce from the argument's type. */
static bool DeduceFunctionTemplatePackCallArgument(
    Vector* args, size_t explicit_arg_count, int pack_index, TypeRecord* formal,
    ASTNode* actual) {
  if (formal == NULL || actual == NULL || actual->type == NULL) {
    return false;
  }
  TypeRecord* target = TypeIsReference(formal) ? formal->next : formal;
  if (TypeIsCXXInitializerList(target) && actual->op != AST_OP(braced_init) &&
      !TypeIsCXXInitializerList(TypeIsReference(actual->type)
                                    ? actual->type->next
                                    : actual->type)) {
    return false;
  }
  if (actual->op == AST_OP(braced_init) &&
      !TypeIsCXXInitializerList(target) &&
      (target == NULL || target->declarator != kDeclArray)) {
    return false;
  }
  int placeholder_index = -1;
  if (formal->declarator == kDeclRValueReference &&
      TypeIsTemplateParameterPlaceholder(formal->next, &placeholder_index) &&
      ForwardingArgumentIsLvalue(actual)) {
    TypeRecord* lvalue_ref = NewReferenceTypeRecord(kQualPlain, false);
    TypeRecordChain(lvalue_ref, actual->type);
    lvalue_ref->type = actual->type->type;
    TypeRecordCalculateSize(lvalue_ref);
    if (placeholder_index == pack_index) {
      TemplateArgument* pack = args->value.p[pack_index];
      if (pack == NULL) {
        pack = NewEmptyPackTemplateArgument(kTemplateParameterType);
        args->value.p[pack_index] = pack;
      }
      bool ok = pack->kind == kTemplateParameterType &&
                pack->pack_arguments != NULL;
      if (ok) {
        VectorAppend(pack->pack_arguments,
                     NewDeducedTypeTemplateArgument(lvalue_ref));
      }
      TypeRecordDelete(lvalue_ref);
      return ok;
    }
    bool ok = AppendDeducedFunctionTemplatePackElement(
        args, explicit_arg_count, pack_index, formal, lvalue_ref);
    TypeRecordDelete(lvalue_ref);
    return ok;
  }
  if (formal->declarator == kDeclRValueReference &&
      TypeIsTemplateParameterPlaceholder(formal->next, &placeholder_index) &&
      actual->value_category != kValueCategoryLvalue &&
      placeholder_index == pack_index) {
    TemplateArgument* pack = args->value.p[pack_index];
    if (pack == NULL) {
      pack = NewEmptyPackTemplateArgument(kTemplateParameterType);
      args->value.p[pack_index] = pack;
    }
    if (pack->kind != kTemplateParameterType ||
        pack->pack_arguments == NULL) {
      return false;
    }
    VectorAppend(pack->pack_arguments,
                 NewDeducedTypeTemplateArgument(actual->type));
    return true;
  }
  TypeRecord* decayed = DecayCallArgumentTypeForDeduction(formal, actual->type);
  if (decayed != NULL) {
    bool ok = AppendDeducedFunctionTemplatePackElement(
        args, explicit_arg_count, pack_index, formal, decayed);
    TypeRecordDelete(decayed);
    return ok;
  }
  return AppendDeducedFunctionTemplatePackElement(
      args, explicit_arg_count, pack_index, formal, actual->type);
}

/* Record a deduced non-type (integer) argument for parameter `index`, keeping
 * explicit args, or verifying consistency with a prior deduction. */
static bool SetDeducedFunctionTemplateNonTypeArgument(Vector* args,
                                                      size_t explicit_arg_count,
                                                      int index,
                                                      long long value) {
  index = DeductionArgumentIndex(index);
  if (index < 0 || (size_t)index >= args->length) {
    return false;
  }
  if ((size_t)index < explicit_arg_count) {
    return true;
  }
  TemplateArgument* existing = args->value.p[index];
  if (existing == NULL) {
    args->value.p[index] = NewDeducedNonTypeTemplateArgument(value);
    return true;
  }
  return existing->kind == kTemplateParameterNonType &&
         existing->int_value == value;
}

static bool SetDeducedTemplateNonTypeArgument(
    Vector* args, size_t explicit_arg_count, int index,
    TemplateArgument* value) {
  index = DeductionArgumentIndex(index);
  if (index < 0 || (size_t)index >= args->length || value == NULL ||
      value->kind != kTemplateParameterNonType) {
    return false;
  }
  if ((size_t)index < explicit_arg_count) {
    TemplateArgument* existing = args->value.p[index];
    return existing == NULL || TemplateArgumentEqual(existing, value);
  }
  TemplateArgument* existing = args->value.p[index];
  if (existing == NULL) {
    args->value.p[index] = TemplateArgumentCopy(value);
    return true;
  }
  return TemplateArgumentEqual(existing, value);
}

static bool SetDeducedTemplateTemplateArgument(
    Vector* args, size_t explicit_arg_count, int index,
    TemplateArgument* value) {
  if (index < 0 || args == NULL || (size_t)index >= args->length ||
      value == NULL || value->kind != kTemplateParameterTemplate) {
    return false;
  }
  if ((size_t)index < explicit_arg_count) {
    TemplateArgument* existing = args->value.p[index];
    return existing == NULL || TemplateArgumentEqual(existing, value);
  }
  TemplateArgument* existing = args->value.p[index];
  if (existing == NULL) {
    args->value.p[index] = TemplateArgumentCopy(value);
    return true;
  }
  return TemplateArgumentEqual(existing, value);
}

/* Unwrap a braced-initializer element to its underlying expression. */
static ASTNode* CXXBracedInitializerElementExpression(ASTNode* init) {
  if (init == NULL) {
    return NULL;
  }
  if (init->op == AST_OP(expr_init)) {
    ExpressionInitializerASTNode* expr_init =
        (ExpressionInitializerASTNode*)init;
    return expr_init->expr;
  }
  return init;
}

/* Deduce template arguments when a `T[N]`/nested-array parameter is matched
 * against a braced initializer: deduce the bound `N` from the element count and
 * the element type from each initializer (recursing for nested braces). */
static bool DeduceFunctionTemplateArrayInitializerArgument(
    Vector* args, size_t explicit_arg_count, TypeRecord* formal,
    ASTNode* actual) {
  if (actual == NULL || actual->op != AST_OP(braced_init) || formal == NULL ||
      formal->declarator != kDeclArray) {
    return false;
  }
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)actual;
  if (formal->info.array.template_parameter_index >= 0 &&
      !SetDeducedFunctionTemplateNonTypeArgument(
          args, explicit_arg_count, formal->info.array.template_parameter_index,
          (long long)braced->initializers->length)) {
    return false;
  }
  if (formal->info.array.template_parameter_index < 0 &&
      !formal->info.array.is_flexible && !formal->info.array.is_vla &&
      !formal->info.array.is_dependent_bound &&
      formal->info.array.size.fixed < (int)braced->initializers->length) {
    return false;
  }
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* init = braced->initializers->value.p[i];
    if (init == NULL) {
      return false;
    }
    if (init->op == AST_OP(braced_init)) {
      if (!DeduceFunctionTemplateArrayInitializerArgument(
              args, explicit_arg_count, formal->next, init)) {
        return false;
      }
      continue;
    }
    ASTNode* element = CXXBracedInitializerElementExpression(init);
    if (element == NULL) {
      return false;
    }
    element = AnalyzeExpression(element);
    if (!DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                            formal->next, element->type)) {
      return false;
    }
  }
  return true;
}

/* Deduce template arguments when a `std::initializer_list<T>` parameter is
 * matched against a braced initializer: deduce `T` from each element. */
static bool DeduceFunctionTemplateInitializerListArgument(
    Vector* args, size_t explicit_arg_count, TypeRecord* formal,
    ASTNode* actual) {
  TypeRecord* target = TypeIsReference(formal) ? formal->next : formal;
  if (actual == NULL || actual->op != AST_OP(braced_init) ||
      !TypeIsCXXInitializerList(target)) {
    return false;
  }
  TypeRecord* element_type = TypeCXXInitializerListElement(target);
  if (element_type == NULL) {
    return false;
  }
  /* [temp.deduct.call]: an element pattern that names no template parameter
   * deduces nothing.  Whether each element converts to that concrete type is
   * decided later, during overload resolution.  An exact match here rejects
   * `initializer_list<E>` for a `const E` element, as in
   * `Span<const E>({e})` where `value_type` is `remove_cv_t<T>`. */
  if (!TypeContainsTemplateParameter(element_type)) {
    return true;
  }
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)actual;
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* element =
        CXXBracedInitializerElementExpression(braced->initializers->value.p[i]);
    if (element == NULL) {
      return false;
    }
    element = AnalyzeExpression(element);
    if (!DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                            element_type, element->type)) {
      return false;
    }
  }
  return true;
}

/* Deduce template arguments when both formal and actual are instantiations of
 * the same class template (e.g. formal `Wrapper<T>` vs actual `Wrapper<int>`):
 * match them argument-by-argument, recursing into type arguments. */
/* Match one non-pack formal class-template argument against one actual argument,
 * deducing any function-template parameters it mentions into `args`. */
static bool DeduceFunctionTemplateOneTemplateArgument(Vector* args,
                                                      size_t explicit_arg_count,
                                                      TemplateArgument* formal_arg,
                                                      TemplateArgument* actual_arg) {
  if (formal_arg == NULL || actual_arg == NULL ||
      formal_arg->kind != actual_arg->kind) {
    return false;
  }
  if (formal_arg->kind == kTemplateParameterType) {
    int index = -1;
    if (TypeIsTemplateParameterPlaceholder(formal_arg->type, &index)) {
      return SetDeducedFunctionTemplateTypeArgumentPreserveQualifiers(
          args, explicit_arg_count, index, actual_arg->type);
    }
    // [temp.deduct.type]: a type named by a qualified-id whose nested-name-
    // specifier is dependent (`typename basic_string<CharT>::const_iterator`)
    // is a non-deduced context.  Defer it in phase 1 so sibling parameters
    // (`Allocator` in `match_results<...::const_iterator, Allocator>`) can still
    // be deduced.  Phase 2 retries this compiler's nested-type extension; a
    // failure must not reject the candidate, because the parameter may still
    // come from another argument or a default.
    if (TemplateArgumentHasDependentMemberName(formal_arg)) {
      if (!g_deduce_defer_bare_member) {
        DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                           formal_arg->type, actual_arg->type);
      }
      return true;
    }
    return DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                              formal_arg->type,
                                              actual_arg->type);
  }
  if (formal_arg->kind == kTemplateParameterTemplate) {
    if (formal_arg->template_parameter_index >= 0) {
      return SetDeducedTemplateTemplateArgument(
          args, explicit_arg_count, formal_arg->template_parameter_index,
          actual_arg);
    }
    return TemplateArgumentEqual(formal_arg, actual_arg);
  }
  if (formal_arg->template_parameter_index >= 0) {
    return SetDeducedTemplateNonTypeArgument(
        args, explicit_arg_count, formal_arg->template_parameter_index,
        actual_arg);
  }
  return TemplateArgumentEqual(formal_arg, actual_arg);
}

// The template-argument list describing a class-template specialization type.
// A type record obtained from an expression -- notably the type of a
// CTAD-deduced variable used as a further deduction source (`Pair p{...}; Pair
// q = p;`) -- may not carry `template_arguments` on the record itself, but the
// specialization's tag symbol always records them.  Recover from the tag so
// deduction against a same-template argument still sees the concrete arguments.
Vector* TypeSpecializationTemplateArguments(TypeRecord* type) {
  if (type == NULL) {
    return NULL;
  }
  if (type->template_arguments != NULL) {
    return type->template_arguments;
  }
  if (TypeIsStructOrUnion(type) && type->info.struct_info != NULL &&
      type->info.struct_info->tag_symbol != NULL &&
      type->info.struct_info->tag_symbol->type != NULL) {
    return type->info.struct_info->tag_symbol->type->template_arguments;
  }
  return NULL;
}

// The class template that `type` (a class specialization) was instantiated from,
// or NULL if `type` is not a class-template specialization.
static Symbol* ClassTemplateOriginOf(TypeRecord* type) {
  if (type == NULL) {
    return NULL;
  }
  Symbol* origin = type->template_origin;
  if (origin == NULL && TypeIsStructOrUnion(type) &&
      type->info.struct_info != NULL &&
      type->info.struct_info->tag_symbol != NULL &&
      type->info.struct_info->tag_symbol->type != NULL) {
    origin = type->info.struct_info->tag_symbol->type->template_origin;
  }
  return origin;
}

static bool ClassTemplateOriginMatches(Symbol* a, Symbol* b) {
  if (a == NULL || b == NULL) {
    return false;
  }
  return a == b || StringEqualString(&a->name, &b->name);
}

// An explicit class-template specialization is a distinct Struct whose tag is
// spelled `Primary<args>` and which does not carry template_origin (that
// metadata would re-mangle the specialization's own members).  Deduction of
// `Primary<T>` against that type still has to succeed.
static bool CXXExplicitSpecializationMatchesPrimary(TypeRecord* actual,
                                                Symbol* primary) {
  if (actual == NULL || primary == NULL || primary->name.value == NULL ||
      !TypeIsStructOrUnion(actual) || actual->info.struct_info == NULL ||
      actual->info.struct_info->tag_name == NULL ||
      TypeSpecializationTemplateArguments(actual) == NULL) {
    return false;
  }
  const char* tag = actual->info.struct_info->tag_name->value;
  const char* name = primary->name.value;
  size_t n = strlen(name);
  return strncmp(tag, name, n) == 0 && tag[n] == '<';
}

// [temp.deduct.call]/4.3: when the parameter is a class template specialization
// and the argument is derived from a specialization of that same template,
// deduction proceeds against the base class.  Searches `actual`'s base classes
// (transitively) for a base that is a specialization of `formal_origin`'s
// template and returns that base's concrete type.  Returns NULL when there is no
// such base, or more than one distinct matching base (an ambiguous case in which
// deduction fails).
static TypeRecord* FindTemplateBaseForDeduction(TypeRecord* actual,
                                                Symbol* formal_origin) {
  if (actual == NULL || !TypeIsStructOrUnion(actual) ||
      actual->info.struct_info == NULL || formal_origin == NULL) {
    return NULL;
  }
  Struct* str = actual->info.struct_info;
  TypeRecord* found = NULL;
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base == NULL || base->type == NULL) {
      continue;
    }
    TypeRecord* candidate = NULL;
    if (ClassTemplateOriginMatches(ClassTemplateOriginOf(base->type),
                                   formal_origin)) {
      candidate = base->type;
    } else {
      candidate = FindTemplateBaseForDeduction(base->type, formal_origin);
    }
    if (candidate != NULL) {
      if (found != NULL && found != candidate && !TypeEqual(found, candidate)) {
        return NULL;
      }
      found = candidate;
    }
  }
  return found;
}

static bool TemplateArgumentHasDependentMemberName(TemplateArgument* arg) {
  if (arg == NULL) {
    return false;
  }
  if (arg->pack_arguments != NULL) {
    for (size_t i = 0; i < arg->pack_arguments->length; i++) {
      if (TemplateArgumentHasDependentMemberName(
              arg->pack_arguments->value.p[i])) {
        return true;
      }
    }
    return false;
  }
  for (TypeRecord* type = arg->type; type != NULL; type = type->next) {
    if (type->dependent_member_name != NULL) {
      return true;
    }
  }
  return false;
}

static bool DeduceFunctionTemplateTemplateArguments(Vector* args,
                                                    size_t explicit_arg_count,
                                                    TypeRecord* formal,
                                                    TypeRecord* actual) {
  if (formal == NULL || actual == NULL) {
    return false;
  }
  Symbol* formal_origin = ClassTemplateOriginOf(formal);
  Symbol* actual_origin = ClassTemplateOriginOf(actual);
  if (formal_origin == NULL) {
    return false;
  }
  bool formal_is_template_parameter =
      formal_origin->flags.is_template_template_parameter;
  Vector* formal_args = TypeSpecializationTemplateArguments(formal);
  if (StorageIs(formal_origin->storage, STO(typedef)) &&
      formal_origin->type != NULL &&
      formal_origin->type->template_origin != NULL && formal_args != NULL) {
    Vector* alias_args =
        CompleteAliasTemplateArguments(formal_origin, formal_args);
    if (alias_args == NULL) {
      return false;
    }
    TypeParser parser;
    TypeParserInit(&parser, compiler->syntax.lex, &compiler->syntax,
                   STO(implicit), compiler->syntax.context);
    bool saved_trap = DiagnosticErrorTrapBegin();
    DiagnosticSuppressBegin();
    TypeRecord* expanded =
        SubstituteTemplateParameters(&parser, formal_origin->type, alias_args);
    bool failed =
        parser.template_substitution_failed || DiagnosticErrorTrapped();
    DiagnosticSuppressEnd();
    DiagnosticErrorTrapEnd(saved_trap);
    TypeParserDestruct(&parser);
    VectorDeleteWithContents(alias_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    bool ok =
        !failed && DeduceFunctionTemplateTypeArgument(
                       args, explicit_arg_count, expanded, actual);
    TypeRecordDelete(expanded);
    return ok;
  }
  if (formal_is_template_parameter) {
    if (actual_origin == NULL ||
        !TemplateTemplateParameterListsCompatible(
            formal_origin->template_template_parameters,
            TemplateParameterListForSymbol(actual_origin))) {
      return false;
    }
    TemplateArgument* deduced =
        NewTemplateTemplateArgument(actual_origin, -1);
    bool bound = SetDeducedTemplateTemplateArgument(
        args, explicit_arg_count,
        formal->template_parameter_index >= 0
            ? formal->template_parameter_index
            : formal_origin->template_parameter_index,
        deduced);
    TemplateArgumentDelete(deduced);
    if (!bound) {
      return false;
    }
  } else if (actual_origin == NULL ||
             !ClassTemplateOriginMatches(formal_origin, actual_origin)) {
    // The argument is not itself a specialization of the parameter's template.
    // If it derives from one, deduce against that (unique) base class.
    TypeRecord* base = FindTemplateBaseForDeduction(actual, formal_origin);
    if (base != NULL) {
      return DeduceFunctionTemplateTemplateArguments(args, explicit_arg_count,
                                                     formal, base);
    }
    if (!CXXExplicitSpecializationMatchesPrimary(actual, formal_origin)) {
      return false;
    }
  }
  Vector* completed_formal_args = NULL;
  if (formal_origin->type != NULL &&
      TypeIsStructOrUnion(formal_origin->type) &&
      formal_origin->type->info.struct_info != NULL && formal_args != NULL &&
      formal_origin->type->info.struct_info->template_parameters.length >
          formal_args->length) {
    // `P` written as `basic_string<CharT>` still has the class template's later
    // default arguments (`char_traits<CharT>`, `allocator<CharT>`).  Fill
    // them in so deduction can match a fully-specified argument type such as
    // `basic_string<char, char_traits<char>, allocator<char>>`.
    TypeParser parser;
    TypeParserInit(&parser, compiler->syntax.lex, &compiler->syntax,
                   STO(implicit), compiler->syntax.context);
    bool saved_trap = DiagnosticErrorTrapBegin();
    DiagnosticSuppressBegin();
    completed_formal_args = CompleteTemplateArguments(
        &parser, &formal_origin->type->info.struct_info->template_parameters,
        formal_args, "", /*emit_error=*/false, kTemplateArgumentsBorrow);
    DiagnosticSuppressEnd();
    DiagnosticErrorTrapEnd(saved_trap);
    TypeParserDestruct(&parser);
    if (completed_formal_args != NULL) {
      formal_args = completed_formal_args;
    }
  }
  Vector* actual_args = TypeSpecializationTemplateArguments(actual);
  if (formal_args == NULL || actual_args == NULL) {
    if (completed_formal_args != NULL) {
      VectorDeleteWithContents(completed_formal_args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    return false;
  }
  Vector flat_actual_args;
  bool flattened_actual_args = false;
  VectorInit(&flat_actual_args);
  for (size_t i = 0; i < actual_args->length; i++) {
    TemplateArgument* actual_arg = actual_args->value.p[i];
    if (actual_arg != NULL && actual_arg->pack_arguments != NULL) {
      flattened_actual_args = true;
      for (size_t j = 0; j < actual_arg->pack_arguments->length; j++) {
        VectorAppend(&flat_actual_args, actual_arg->pack_arguments->value.p[j]);
      }
    } else {
      VectorAppend(&flat_actual_args, actual_arg);
    }
  }
  if (flattened_actual_args) {
    actual_args = &flat_actual_args;
  }
  size_t actual_index = 0;
  bool ok = true;
  for (size_t i = 0; i < formal_args->length; i++) {
    TemplateArgument* formal_arg = formal_args->value.p[i];
    if (formal_arg == NULL) {
      ok = false;
      break;
    }
    // A formal parameter-pack argument (e.g. `Wrapper<Types...>` matched against
    // `Wrapper<int, char>`): absorb the actual arguments that line up with it,
    // leaving enough for any fixed formal arguments that follow the pack.  Each
    // absorbed actual is deduced as one element of the enclosing function
    // template's pack parameter.
    int pack_index = -1;
    TemplateArgument* pack_pattern = formal_arg;
    if (formal_arg->pack_arguments != NULL &&
        formal_arg->pack_arguments->length == 1) {
      TemplateArgument* bundled_pattern =
          formal_arg->pack_arguments->value.p[0];
      if (bundled_pattern != NULL && bundled_pattern->is_pack_expansion) {
        pack_pattern = bundled_pattern;
      }
    }
    if (pack_pattern->is_pack_expansion &&
        pack_pattern->kind == kTemplateParameterNonType &&
        pack_pattern->template_parameter_index >= 0) {
      pack_index = pack_pattern->template_parameter_index;
      size_t trailing_formals = formal_args->length - i - 1;
      if ((size_t)pack_index >= args->length ||
          actual_args->length < actual_index + trailing_formals) {
        ok = false;
        break;
      }
      size_t pack_end = actual_args->length - trailing_formals;
      TemplateArgument* deduced_pack =
          NewEmptyPackTemplateArgument(kTemplateParameterNonType);
      for (; actual_index < pack_end; actual_index++) {
        TemplateArgument* actual_arg = actual_args->value.p[actual_index];
        if (actual_arg == NULL ||
            actual_arg->kind != kTemplateParameterNonType) {
          ok = false;
          break;
        }
        VectorAppend(deduced_pack->pack_arguments,
                     TemplateArgumentCopy(actual_arg));
      }
      if (ok) {
        TemplateArgument* existing = args->value.p[pack_index];
        if (existing == NULL ||
            (existing->kind == kTemplateParameterNonType &&
             existing->pack_arguments != NULL &&
             existing->pack_arguments->length == 0)) {
          TemplateArgumentDelete(existing);
          args->value.p[pack_index] = deduced_pack;
          deduced_pack = NULL;
        } else if (!TemplateArgumentEqual(existing, deduced_pack)) {
          ok = false;
        }
      }
      TemplateArgumentDelete(deduced_pack);
      if (!ok) {
        break;
      }
      continue;
    }
    if (formal_arg->is_pack_expansion &&
        formal_arg->kind == kTemplateParameterType &&
        formal_arg->type != NULL &&
        TypeIsTemplateParameterPlaceholder(formal_arg->type, &pack_index) &&
        pack_index >= 0) {
      size_t trailing_formals = formal_args->length - i - 1;
      if (actual_args->length < actual_index + trailing_formals) {
        ok = false;
        break;
      }
      size_t pack_end = actual_args->length - trailing_formals;
      // [temp.deduct]: the same template parameter pack may be named in more
      // than one function parameter, e.g.
      //   operator==(const variant<Types...>&, const variant<Types...>&).
      // Each occurrence must deduce a *consistent* sequence for `Types...`, not
      // concatenate onto it.  `AppendDeducedFunctionTemplatePackElement` only
      // ever appends, so remember how many elements a previous parameter
      // already deduced; if the pack is non-empty here we compare the freshly
      // deduced tail against that prefix and collapse the duplicate instead of
      // doubling the pack.
      TemplateArgument* pack_prev = args->value.p[pack_index];
      size_t pack_prefix =
          (pack_prev != NULL && pack_prev->pack_arguments != NULL)
              ? pack_prev->pack_arguments->length
              : 0;
      for (; actual_index < pack_end; actual_index++) {
        TemplateArgument* actual_arg = actual_args->value.p[actual_index];
        if (actual_arg == NULL) {
          ok = false;
          break;
        }
        // The actual may be a still-dependent pack expansion (e.g. matching
        // visit's `variant<VisitTypes...>` parameter against a dependent
        // `variant<Types...>` actual inside another template).  Bind the formal
        // pack to that dependent pack as a unit; it will be re-expanded when the
        // surrounding template is instantiated.
        if (actual_arg->is_pack_expansion) {
          TemplateArgument* existing = args->value.p[pack_index];
          if (existing == NULL) {
            args->value.p[pack_index] = TemplateArgumentCopy(actual_arg);
          } else if (!TemplateArgumentEqual(existing, actual_arg)) {
            ok = false;
            break;
          }
          continue;
        }
        // The actual may itself be an already-bundled pack; splice its elements.
        if (actual_arg->pack_arguments != NULL) {
          for (size_t j = 0; j < actual_arg->pack_arguments->length; j++) {
            TemplateArgument* element = actual_arg->pack_arguments->value.p[j];
            if (element == NULL || element->kind != kTemplateParameterType ||
                element->type == NULL ||
                !AppendDeducedFunctionTemplatePackElement(
                    args, explicit_arg_count, pack_index, formal_arg->type,
                    element->type)) {
              ok = false;
              break;
            }
          }
          if (!ok) {
            break;
          }
          continue;
        }
        if (actual_arg->kind != kTemplateParameterType ||
            actual_arg->type == NULL ||
            !AppendDeducedFunctionTemplatePackElement(
                args, explicit_arg_count, pack_index, formal_arg->type,
                actual_arg->type)) {
          ok = false;
          break;
        }
      }
      if (!ok) {
        break;
      }
      if (pack_prefix > 0) {
        TemplateArgument* pack_now = args->value.p[pack_index];
        if (pack_now == NULL || pack_now->pack_arguments == NULL) {
          ok = false;
          break;
        }
        size_t total = pack_now->pack_arguments->length;
        size_t appended = total - pack_prefix;
        if (appended != pack_prefix) {
          ok = false;
          break;
        }
        for (size_t k = 0; k < appended; k++) {
          if (!TemplateArgumentEqual(
                  pack_now->pack_arguments->value.p[k],
                  pack_now->pack_arguments->value.p[pack_prefix + k])) {
            ok = false;
            break;
          }
        }
        if (!ok) {
          break;
        }
        for (size_t k = total; k > pack_prefix; k--) {
          TemplateArgumentDelete(pack_now->pack_arguments->value.p[k - 1]);
          VectorPop(pack_now->pack_arguments);
        }
      }
      continue;
    }
    if (actual_index >= actual_args->length ||
        !DeduceFunctionTemplateOneTemplateArgument(
            args, explicit_arg_count, formal_arg,
            actual_args->value.p[actual_index])) {
      ok = false;
      break;
    }
    actual_index++;
  }
  ok = ok && actual_index == actual_args->length;
  VectorDestruct(&flat_actual_args);
  if (completed_formal_args != NULL) {
    VectorDeleteWithContents(completed_formal_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  return ok;
}

/* An index belongs to the call operator itself when it falls in
 * [own_base, own_limit).  Anything else is an enclosing template parameter. */
static bool TemplateParameterIndexIsEnclosing(int index, int own_base,
                                              int own_limit) {
  return index >= 0 && (index < own_base || index >= own_limit);
}

static void NoteEnclosingTemplateParameterInType(TypeRecord* type,
                                                 int own_base, int own_limit,
                                                 bool* found, int depth);

static void NoteEnclosingTemplateParameterInArgument(TemplateArgument* argument,
                                                     int own_base,
                                                     int own_limit,
                                                     bool* found, int depth) {
  if (argument == NULL || *found || depth > 32) {
    return;
  }
  if (TemplateParameterIndexIsEnclosing(argument->template_parameter_index,
                                        own_base, own_limit)) {
    *found = true;
    return;
  }
  NoteEnclosingTemplateParameterInType(argument->type, own_base, own_limit,
                                       found, depth + 1);
  if (argument->pack_arguments == NULL) {
    return;
  }
  for (size_t i = 0; i < argument->pack_arguments->length && !*found; i++) {
    NoteEnclosingTemplateParameterInArgument(argument->pack_arguments->value.p[i],
                                             own_base, own_limit, found,
                                             depth + 1);
  }
}

static void NoteEnclosingTemplateParameterInType(TypeRecord* type,
                                                 int own_base, int own_limit,
                                                 bool* found, int depth) {
  if (*found || depth > 32) {
    return;
  }
  for (TypeRecord* current = type; current != NULL && !*found;
       current = current->next) {
    if (TemplateParameterIndexIsEnclosing(current->template_parameter_index,
                                          own_base, own_limit)) {
      *found = true;
      return;
    }
    if (current->declarator == kDeclArray &&
        TemplateParameterIndexIsEnclosing(
            current->info.array.template_parameter_index, own_base,
            own_limit)) {
      *found = true;
      return;
    }
    if (current->template_arguments == NULL) {
      continue;
    }
    for (size_t i = 0; i < current->template_arguments->length && !*found;
         i++) {
      NoteEnclosingTemplateParameterInArgument(
          current->template_arguments->value.p[i], own_base, own_limit, found,
          depth + 1);
    }
  }
}

typedef struct {
  int own_base;
  int own_limit;
  bool found;
  int depth;
} LambdaBodyEnclosingUse;

static bool ClosureBodyDependsOnEnclosingTemplate(Struct* closure, int depth);

static void NoteLambdaBodyEnclosingTemplate(ASTNode* node, void* data,
                                            int child_id, VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL) {
    return;
  }
  LambdaBodyEnclosingUse* use = data;
  if (use->found) {
    return;
  }
  // A use of another function template (`std::endl`, `operator<<`) carries
  // that template's own parameter indices.  Those indices overlap the
  // enclosing template's, so walking the callee's type looks like a
  // dependent capture and the closure's operator() is never queued.
  bool names_other_function_template = false;
  if (node->op == AST_OP(identifier)) {
    IdentifierASTNode* named = (IdentifierASTNode*)node;
    if (named->symbol != NULL && named->symbol->type != NULL &&
        TypeIsFunction(named->symbol->type) &&
        (named->symbol->flags.is_template ||
         named->symbol->type->info.function.template_origin != NULL ||
         named->symbol->type->info.function.template_parameter_count > 0)) {
      names_other_function_template = true;
    }
  }
  if (!names_other_function_template) {
    NoteEnclosingTemplateParameterInType(node->type, use->own_base,
                                         use->own_limit, &use->found, 0);
  }
  if (node->op == AST_OP(cast)) {
    NoteEnclosingTemplateParameterInType(((CastASTNode*)node)->cast_type,
                                         use->own_base, use->own_limit,
                                         &use->found, 0);
  } else if (node->op == AST_OP(sizeof) || node->op == AST_OP(alignof)) {
    NoteEnclosingTemplateParameterInType(((SizeofASTNode*)node)->type_operand,
                                         use->own_base, use->own_limit,
                                         &use->found, 0);
  } else if (node->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node;
    if (id->symbol != NULL) {
      if (TemplateParameterIndexIsEnclosing(id->symbol->template_parameter_index,
                                            use->own_base, use->own_limit)) {
        use->found = true;
        return;
      }
      if (!names_other_function_template) {
        NoteEnclosingTemplateParameterInType(id->symbol->type, use->own_base,
                                             use->own_limit, &use->found, 0);
      }
    }
    if (id->template_arguments != NULL) {
      for (size_t i = 0; i < id->template_arguments->length && !use->found;
           i++) {
        NoteEnclosingTemplateParameterInArgument(
            id->template_arguments->value.p[i], use->own_base, use->own_limit,
            &use->found, 0);
      }
    }
  } else if (node->op == AST_OP(vardecl)) {
    VariableDeclarationASTNode* decl = (VariableDeclarationASTNode*)node;
    if (decl->symbol != NULL) {
      NoteEnclosingTemplateParameterInType(decl->symbol->type, use->own_base,
                                           use->own_limit, &use->found, 0);
    }
  }
  if (use->found || use->depth >= 8 ||
      (node->flags & kASTLambdaExpression) == 0 || node->type == NULL ||
      !TypeIsStructOrUnion(node->type) || node->type->info.struct_info == NULL ||
      node->type->info.struct_info->tag_symbol == NULL ||
      !node->type->info.struct_info->tag_symbol->flags.invented) {
    return;
  }
  if (ClosureBodyDependsOnEnclosingTemplate(node->type->info.struct_info,
                                            use->depth + 1)) {
    use->found = true;
  }
}

static bool FunctionBodyDependsOnEnclosingTemplate(Symbol* function,
                                                   int depth) {
  if (function == NULL || function->type == NULL ||
      !TypeIsFunction(function->type)) {
    return false;
  }
  ASTNode* body = function->type->info.function.body;
  if (body == NULL && function->value.func_defn != NULL &&
      function->value.func_defn->type != NULL) {
    body = function->value.func_defn->type->info.function.body;
  }
  if (body == NULL) {
    return false;
  }
  TypeRecord* func = function->type;
  int own_base = func->info.function.template_parameter_base;
  int own_count = func->info.function.template_parameter_count;
  if (own_count <= 0) {
    own_count = (int)func->info.function.template_parameters.length;
  }
  LambdaBodyEnclosingUse use = {.own_base = own_base,
                                .own_limit = own_base + own_count,
                                .found = false,
                                .depth = depth};
  ASTNodeVisit(body, NoteLambdaBodyEnclosingTemplate, 0, &use);
  return use.found;
}

static bool ClosureBodyDependsOnEnclosingTemplate(Struct* closure, int depth) {
  if (closure == NULL || depth > 8) {
    return false;
  }
  for (size_t i = 0; i < closure->members.length; i++) {
    StructMember* member = closure->members.value.p[i];
    if (member == NULL || !member->is_member_function ||
        member->symbol == NULL) {
      continue;
    }
    if (FunctionBodyDependsOnEnclosingTemplate(member->symbol, depth)) {
      return true;
    }
  }
  return false;
}

bool LambdaCallOperatorBodyDependsOnEnclosingTemplate(Symbol* call_operator) {
  return FunctionBodyDependsOnEnclosingTemplate(call_operator, 0);
}

/* True if any non-static data member of `str` still has a template-dependent
 * type (so the struct itself is dependent). */
bool StructContainsTemplateParameter(Struct* str) {
  if (str == NULL) {
    return false;
  }
  // A lambda closure with no captures has no data members, so its dependence on
  // an enclosing template parameter may live only in its `operator()` body
  // (`auto p = static_cast<T*>(storage)`).  The signature check below misses
  // that.  Rebuild the closure so the body is substituted and `auto` is
  // deduced against the concrete parameter.  A generic lambda's own parameters
  // are not enclosing ones; only an index outside that operator's range counts.
  bool is_lambda_closure =
      str->tag_symbol != NULL && str->tag_symbol->flags.invented;
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_using_declaration) {
      continue;
    }
    if (member->is_member_function &&
        !(is_lambda_closure && !member->symbol->flags.is_template)) {
      continue;
    }
    if (TypeContainsTemplateParameter(member->symbol->type)) {
      return true;
    }
    // A by-value capture of an undeduced `auto` local (`auto old_size =
    // str.size()` inside a function template) is not a template parameter,
    // but the field type is only known once that local is deduced in each
    // instantiation.  Rebuild the closure so those instantiations do not
    // share one `auto` field.
    if (is_lambda_closure && !member->is_member_function &&
        TypeContainsAuto(member->symbol->type)) {
      return true;
    }
  }
  if (is_lambda_closure && ClosureBodyDependsOnEnclosingTemplate(str, 0)) {
    return true;
  }
  return false;
}

// A named local class of a function template.  Its member function bodies may
// use the function's parameters, so every instantiation of the function
// rebuilds it.  A rebuilt copy has its own tag symbol, which is not a
// block-scope tag.
bool StructIsFunctionTemplateLocalClass(Struct* str) {
  if (str == NULL || str->lexical_parent != NULL || str->is_template ||
      str->local_class_arguments != NULL || str->tag_symbol == NULL ||
      !str->tag_symbol->flags.is_block_scope ||
      str->tag_symbol->flags.invented || !StructHasMemberFunction(str)) {
    return false;
  }
  Symbol* function = str->access_enclosing_function;
  return function != NULL &&
         (function->flags.is_template ||
          (function->type != NULL && TypeIsFunction(function->type) &&
           function->type->info.function.template_parameters.length > 0));
}

/* Core type-against-type deduction: match a (possibly dependent) parameter type
 * `formal` against a concrete argument type `actual`, recording deduced
 * arguments into `args`. Handles dependent member typedefs, bare parameters
 * `T`, references, arrays `T[N]`, pointers, same-template instantiations, and
 * structural recursion through the type chain. Returns false on a mismatch. */
static bool DeduceFunctionTemplateTypeArgument(Vector* args,
                                               size_t explicit_arg_count,
                                               TypeRecord* formal,
                                               TypeRecord* actual) {
  if (formal == NULL || actual == NULL) {
    return false;
  }
  // [temp.deduct.type]: a C++26 pack-indexing specifier is a non-deduced
  // context.  The pack must be supplied explicitly or deduced elsewhere.
  if (formal->is_pack_index) {
    return true;
  }
  // A qualified dependent type is a non-deduced context. Defer both the
  // `Owner<T>::member` and bare `T::member` forms during the first deduction
  // pass so another parameter can supply T without this argument contaminating
  // that deduction. The extension below is retried only if T remains unbound.
  if (formal->dependent_member_name != NULL && g_deduce_defer_bare_member) {
    return true;
  }
  if (formal->template_origin != NULL &&
      formal->dependent_member_name != NULL) {
    bool ok = false;
    // `typename Owner<T, N>::Inner` against a nested class of a concrete
    // `Owner<A, B>` specialization: recover T and N from the enclosing
    // class's template arguments.  Structural matching of Inner's members
    // can bind type parameters (e.g. `T value`) while leaving a non-type
    // parameter that appears only as an array bound (`int data[N]`) unset.
    if (TypeIsStructOrUnion(actual) && actual->info.struct_info != NULL) {
      Struct* parent = actual->info.struct_info->lexical_parent;
      if (parent != NULL && parent->tag_symbol != NULL &&
          parent->tag_symbol->type != NULL &&
          ClassTemplateOriginMatches(
              ClassTemplateOriginOf(parent->tag_symbol->type),
              formal->template_origin)) {
        StructMember* nested =
            FindStructMember(parent, formal->dependent_member_name);
        if (nested != NULL &&
            actual->info.struct_info->tag_name != NULL &&
            StringEqualString(actual->info.struct_info->tag_name,
                              formal->dependent_member_name)) {
          ok = DeduceFunctionTemplateTemplateArguments(
              args, explicit_arg_count, formal, parent->tag_symbol->type);
        }
      }
    }
    if (!ok) {
      TypeRecord* owner =
          TypeInstantiateClassTemplate(&compiler->syntax, formal->template_origin,
                                       formal->template_arguments);
      if (owner != NULL && TypeIsStructOrUnion(owner) &&
          owner->info.struct_info != NULL) {
        StructMember* member =
            FindStructMember(owner->info.struct_info,
                             formal->dependent_member_name);
        if (member != NULL && member->symbol != NULL &&
            StorageIs(member->symbol->storage, STO(typedef))) {
          ok = DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                                  member->symbol->type, actual);
        }
      }
      TypeRecordDelete(owner);
    }
    return ok;
  }
  int bare_parameter_index = -1;
  if (TypeIsTemplateParameterPlaceholder(formal, &bare_parameter_index)) {
    return SetDeducedFunctionTemplateTypeArgument(
        args, explicit_arg_count, bare_parameter_index, actual);
  }

  if (TypeIsReference(formal)) {
    // In call deduction the argument type `actual` has already had any
    // top-level reference stripped, so matching against the formal's referent
    // is correct.  When deducing one *function type* against another
    // ([temp.deduct.funcaddr]), however, both the return type and the parameter
    // types keep their reference qualifiers, so a reference `actual` must be
    // reduced to its referent symmetrically for the referents to be matched.
    TypeRecord* actual_referent =
        TypeIsReference(actual) ? actual->next : actual;
    // [temp.deduct.call] A is used as-is when P is a reference.  Stripping
    // cv from `const int` against `T&` would deduce `T = int` and then reject
    // the call for binding `int&` to a const lvalue (`std::addressof`).
    // `const T&` is different: the cv belongs to the parameter pattern, not
    // to T.  Ignoring it on both sides lets `min(size_t, const size_t)`
    // deduce one T ([temp.deduct.type]).
    int ref_placeholder = -1;
    if (TypeIsTemplateParameterPlaceholder(formal->next, &ref_placeholder)) {
      Qualifiers pattern_cv =
          formal->next->qualifiers & (kQualConst | kQualVolatile);
      if (pattern_cv != 0) {
        return SetDeducedFunctionTemplateTypeArgument(
            args, explicit_arg_count, ref_placeholder, actual_referent);
      }
      return SetDeducedFunctionTemplateTypeArgumentPreserveQualifiers(
          args, explicit_arg_count, ref_placeholder, actual_referent);
    }
    return DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                              formal->next, actual_referent);
  }

  int array_nttp_index = -1;
  if (formal->declarator == kDeclArray) {
    array_nttp_index = formal->info.array.template_parameter_index;
    if (array_nttp_index < 0 && formal->info.array.is_dependent_bound &&
        formal->info.array.size.vla.size != NULL &&
        formal->info.array.size.vla.size->op == AST_OP(identifier)) {
      IdentifierASTNode* bound =
          (IdentifierASTNode*)formal->info.array.size.vla.size;
      if (bound->symbol != NULL && bound->symbol->flags.is_template_parameter &&
          !bound->symbol->flags.is_template_type_parameter) {
        array_nttp_index = bound->symbol->template_parameter_index;
      }
    }
  }
  if (array_nttp_index >= 0) {
    if (actual->declarator != kDeclArray || actual->info.array.is_vla ||
        actual->info.array.is_dependent_bound ||
        actual->info.array.template_parameter_index >= 0 ||
        !SetDeducedFunctionTemplateNonTypeArgument(
            args, explicit_arg_count, array_nttp_index,
            actual->info.array.size.fixed)) {
      return false;
    }
  }

  if (formal->declarator != actual->declarator) {
    if (formal->declarator == kDeclPointer &&
        actual->declarator == kDeclArray) {
      return DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                                formal->next, actual->next);
    }
    if (formal->declarator == kDeclPointer &&
        actual->declarator == kDeclFunction) {
      return DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                                formal->next, actual);
    }
    // `HidePtr<void>(nullptr)`: T is explicit, so T* is not deduced from the
    // argument.  nullptr converts to that pointer; deduction must not reject
    // it for not already being a pointer.
    if (formal->declarator == kDeclPointer && TypeIsNullPointer(actual) &&
        formal->next != NULL) {
      int pointee_index = -1;
      if (TypeIsTemplateParameterPlaceholder(formal->next, &pointee_index) &&
          pointee_index >= 0 &&
          (size_t)pointee_index < explicit_arg_count) {
        return true;
      }
    }
    return false;
  }
  switch (formal->declarator) {
    case kDeclPointer: {
      // [temp.deduct.call] strips top-level cv from a by-value parameter, not
      // from the pointee.  `template <class U> void f(U*)` called with
      // `const T*` deduces `U = const T`.  A cv-qualified pattern pointee
      // (`const U*`) still ignores that cv, which the stripping deduction does.
      int pointee_index = -1;
      if (formal->next != NULL && actual->next != NULL &&
          TypeIsTemplateParameterPlaceholder(formal->next, &pointee_index)) {
        Qualifiers pattern_cv =
            formal->next->qualifiers & (kQualConst | kQualVolatile);
        if (pattern_cv == 0) {
          return SetDeducedFunctionTemplateTypeArgumentPreserveQualifiers(
              args, explicit_arg_count, pointee_index, actual->next);
        }
      }
      return DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                                formal->next, actual->next);
    }
    case kDeclArray: {
      // This type model can store cv-qualification written on an array object
      // on the array node itself, while a parameter such as
      // `const T (&)[N]` stores it on the element node.  C++ treats those as
      // the same cv-qualified array type, so carry wrapper qualifiers into
      // private element copies before recursing.
      TypeRecord* formal_element = TypeRecordCopy(formal->next);
      TypeRecord* actual_element = TypeRecordCopy(actual->next);
      formal_element->qualifiers |= formal->qualifiers;
      actual_element->qualifiers |= actual->qualifiers;
      bool ok = DeduceFunctionTemplateTypeArgument(
          args, explicit_arg_count, formal_element, actual_element);
      TypeRecordDelete(formal_element);
      TypeRecordDelete(actual_element);
      return ok;
    }
    case kDeclVector:
      return formal->info.array.size.fixed == actual->info.array.size.fixed &&
             DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                                formal->next, actual->next);
    case kDeclMemberPointer: {
      // `Result (T::*)()` stores T as a template parameter index on the
      // member-pointer node, not as a class.  Deduce T from the argument's
      // class, then deduce Result from the function type.
      if (formal->template_parameter_index >= 0) {
        Struct* actual_class = TypeMemberPointerClass(actual);
        if (actual_class == NULL || actual_class->tag_symbol == NULL ||
            actual_class->tag_symbol->type == NULL ||
            !SetDeducedFunctionTemplateTypeArgument(
                args, explicit_arg_count, formal->template_parameter_index,
                actual_class->tag_symbol->type)) {
          return false;
        }
      } else if (TypeMemberPointerClass(formal) !=
                 TypeMemberPointerClass(actual)) {
        return false;
      }
      return DeduceFunctionTemplateTypeArgument(
          args, explicit_arg_count, TypeMemberPointerPointeeType(formal),
          TypeMemberPointerPointeeType(actual));
    }
    case kDeclReference:
    case kDeclRValueReference:
      return false;
    case kDeclPrimitive:
      if (DeduceFunctionTemplateTemplateArguments(args, explicit_arg_count,
                                                  formal, actual)) {
        return true;
      }
      if (TypeIsStructOrUnion(formal) && TypeIsStructOrUnion(actual) &&
          (TypeContainsTemplateParameter(formal) ||
           StructContainsTemplateParameter(formal->info.struct_info))) {
        return DeduceFunctionTemplateStructMembers(args, explicit_arg_count,
                                                   formal, actual);
      }
      return TypeEqual(formal, actual);
    case kDeclFunction:
      if (!DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                              formal->next, actual->next)) {
        return false;
      }
      size_t actual_index = 0;
      for (size_t i = 0; i < formal->info.function.prototype.length; i++) {
        Symbol* formal_arg = formal->info.function.prototype.value.p[i];
        if (formal_arg == NULL) {
          return false;
        }
        if (formal_arg->flags.is_parameter_pack) {
          int pack_index = -1;
          if (!TypeIsTemplateParameterPlaceholder(formal_arg->type,
                                                  &pack_index) ||
              pack_index < 0) {
            return false;
          }
          size_t trailing =
              formal->info.function.prototype.length - i - 1;
          if (actual->info.function.prototype.length <
              actual_index + trailing) {
            return false;
          }
          size_t pack_end =
              actual->info.function.prototype.length - trailing;
          for (; actual_index < pack_end; actual_index++) {
            Symbol* actual_arg =
                actual->info.function.prototype.value.p[actual_index];
            if (actual_arg == NULL ||
                !AppendDeducedFunctionTemplatePackElement(
                    args, explicit_arg_count, pack_index,
                    formal_arg->type, actual_arg->type)) {
              return false;
            }
          }
          continue;
        }
        if (actual_index >= actual->info.function.prototype.length) {
          return false;
        }
        Symbol* actual_arg =
            actual->info.function.prototype.value.p[actual_index++];
        if (actual_arg == NULL ||
            !DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                                formal_arg->type,
                                                actual_arg->type)) {
          return false;
        }
      }
      return actual_index == actual->info.function.prototype.length;
  }
  return false;
}

static TypeRecord* TypeSkipReferencesForDeduction(TypeRecord* type) {
  while (type != NULL && TypeIsReference(type)) {
    type = type->next;
  }
  return type;
}

// [temp.deduct.call]: when P is a class-template-id (possibly behind
// references) and A is not a specialization of that template — nor derived
// from one — parameters mentioned only in P are a non-deduced context.
// `Format(const Spec<Args...>&, const Args&...)` called as `Format("%08x", 1u)`
// must take Args from the later pack, then convert the literal via Spec's ctor.
// A failed match against the *same* template is a real deduction failure
// (conflicting bindings) and is not treated as non-deduced.  Only an alias
// template's expansion can hide its parameters in non-deduced arguments; a
// class template's own `const variant<Types...>&` is a deduced context, and a
// mismatch fails rather than deducing an empty pack.
static bool FormalClassTemplateIdIsNonDeducedAgainst(TypeRecord* formal,
                                                     TypeRecord* actual) {
  formal = TypeSkipReferencesForDeduction(formal);
  actual = TypeSkipReferencesForDeduction(actual);
  if (formal == NULL || actual == NULL) {
    return false;
  }
  Symbol* origin = ClassTemplateOriginOf(formal);
  if (origin == NULL && TypeSpecializationTemplateArguments(formal) == NULL) {
    return false;
  }
  bool class_template_origin =
      origin != NULL && origin->type != NULL &&
      TypeIsStructOrUnion(origin->type) &&
      origin->type->template_origin == NULL;
  if (class_template_origin) {
    return false;
  }
  if (origin != NULL) {
    if (ClassTemplateOriginMatches(origin, ClassTemplateOriginOf(actual))) {
      return false;
    }
    if (FindTemplateBaseForDeduction(actual, origin) != NULL) {
      return false;
    }
    if (CXXExplicitSpecializationMatchesPrimary(actual, origin)) {
      return false;
    }
  }
  return true;
}

/* `KeyArg<true>::type<K, key_type>` is the alias `using type = K`.  The
 * parameter type keeps that dependent member (`const key_arg<K>&`) so a later
 * deduction pass can recover K from the call argument.  The pattern's
 * parameter is numbered after the class template; the written `<K, key_type>`
 * arguments are the member-template argument list. */
static TypeRecord* AliasMemberPatternType(TypeRecord* type) {
  if (type == NULL || type->dependent_member_name == NULL ||
      !TypeIsStructOrUnion(type) || type->info.struct_info == NULL ||
      type->dependent_member_template_arguments == NULL ||
      type->dependent_member_template_arguments->length == 0) {
    return NULL;
  }
  Vector* member_args = type->dependent_member_template_arguments->value.p[0];
  if (member_args == NULL || member_args->length == 0) {
    return NULL;
  }
  StructMember* member =
      FindStructMember(type->info.struct_info, type->dependent_member_name);
  if (member == NULL || member->symbol == NULL ||
      member->symbol->type == NULL ||
      !StorageIs(member->symbol->storage, STO(typedef))) {
    return NULL;
  }
  int pattern_index = -1;
  if (!TypeIsTemplateParameterPlaceholder(member->symbol->type,
                                          &pattern_index) ||
      pattern_index < 0) {
    return NULL;
  }
  int class_count = (int)type->info.struct_info->template_parameters.length;
  int member_index = pattern_index - class_count;
  if (member_index < 0 || (size_t)member_index >= member_args->length) {
    if ((size_t)pattern_index < member_args->length) {
      member_index = pattern_index;
    } else {
      return NULL;
    }
  }
  TemplateArgument* arg = member_args->value.p[member_index];
  if (arg == NULL || arg->type == NULL ||
      !TypeContainsTemplateParameter(arg->type)) {
    return NULL;
  }
  TypeRecord* pattern = TypeRecordCopy(arg->type);
  pattern->qualifiers |= type->qualifiers;
  return pattern;
}

/* Replace a dependent member-alias parameter type with the alias pattern so
 * deduction sees `const K&` rather than the non-deduced `KeyArg::type`. */
static TypeRecord* FormalWithAliasPattern(TypeRecord* formal) {
  if (formal == NULL) {
    return NULL;
  }
  if (formal->declarator == kDeclReference ||
      formal->declarator == kDeclRValueReference) {
    TypeRecord* inner = FormalWithAliasPattern(formal->next);
    if (inner == NULL) {
      return NULL;
    }
    TypeRecord* ref = NewReferenceTypeRecord(
        formal->qualifiers, formal->declarator == kDeclRValueReference);
    TypeRecordChain(ref, inner);
    ref->type = inner->type;
    return TypeRecordCalculateSize(ref);
  }
  return AliasMemberPatternType(formal);
}

/* Deduce template arguments for one (non-pack) call argument expression against
 * formal parameter type `formal`, applying the forwarding-reference rule: a
 * `T&&` parameter binding an lvalue deduces `T&` (reference collapsing). */
static bool DeduceFunctionTemplateCallArgument(Vector* args,
                                               size_t explicit_arg_count,
                                               TypeRecord* formal,
                                               ASTNode* actual) {
  if (formal == NULL || actual == NULL || actual->type == NULL) {
    // A braced list may still be untyped while the surrounding call is
    // deduced.  [temp.deduct.call]: that argument is a non-deduced context
    // unless the parameter is std::initializer_list or an array, even when the
    // parameter type names a template parameter deduced from another argument
    // (`SetFlag(&FLAGS_fromenv, {})`).
    if (actual != NULL && actual->op == AST_OP(braced_init) && formal != NULL) {
      TypeRecord* untyped_target =
          TypeIsReference(formal) ? formal->next : formal;
      if (!TypeIsCXXInitializerList(untyped_target) &&
          (untyped_target == NULL ||
           untyped_target->declarator != kDeclArray)) {
        return true;
      }
    }
    return false;
  }
  TypeRecord* alias_pattern = FormalWithAliasPattern(formal);
  if (alias_pattern != NULL) {
    bool ok = DeduceFunctionTemplateCallArgument(args, explicit_arg_count,
                                                 alias_pattern, actual);
    TypeRecordDelete(alias_pattern);
    return ok;
  }
  TypeRecord* target = TypeIsReference(formal) ? formal->next : formal;
  /* An `initializer_list<U>` parameter deduces `U` either from a braced-init
   * argument (`{1, 2, 3}`) or from an argument that is already an
   * `initializer_list<V>` (e.g. forwarding a bound `init` parameter through a
   * second template); reject only arguments that are neither. */
  if (TypeIsCXXInitializerList(target) && actual->op != AST_OP(braced_init) &&
      !TypeIsCXXInitializerList(TypeIsReference(actual->type)
                                    ? actual->type->next
                                    : actual->type)) {
    return false;
  }
  if (actual->op == AST_OP(braced_init) &&
      !TypeIsCXXInitializerList(target) &&
      (target == NULL || target->declarator != kDeclArray)) {
    // [temp.deduct.call] A braced-init-list is a non-deduced context unless
    // the parameter is an initializer_list or an array.  Deduction of the
    // other parameters still proceeds (`FormatF(mantissa, exp, {sign, prec,
    // conv, sink})` deduces `Int` and later checks the `FormatState` conversion).
    return true;
  }
  /* A string literal may initialize an array of characters, so a parameter of
   * type `CharT[N]` deduces its bound `N` (and, in aggregate CTAD, its element
   * type) directly from the literal.  The literal itself has a `const`-qualified
   * element type (`const char[M]`), which need not match the (possibly
   * non-const) parameter element, so deduce against a copy of the literal's
   * array type whose element cv-qualifiers are adjusted to the parameter's. */
  if ((actual->op == AST_OP(string) || actual->op == AST_OP(string_wide)) &&
      target != NULL && target->declarator == kDeclArray &&
      target->next != NULL && actual->type != NULL &&
      actual->type->declarator == kDeclArray && actual->type->next != NULL) {
    // Copy the whole array->element chain so the adjustment below never mutates
    // the shared element type of the original string-literal expression.
    TypeRecord* adjusted = TypeRecordCopy(actual->type);
    TypeRecord* element = TypeRecordCopy(actual->type->next);
    element->qualifiers = target->next->qualifiers;
    TypeRecordIncRef(element);
    // `adjusted->next` still aliases the original (shared) element; drop that
    // borrowed reference before repointing at our private copy.
    TypeRecordDelete(adjusted->next);
    adjusted->next = element;
    TypeRecordCalculateSize(adjusted);
    TypeRecordIncRef(adjusted);
    bool ok = DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                                 target, adjusted);
    TypeRecordDelete(adjusted);
    return ok;
  }
  /* [temp.deduct.call]: if the parameter type P contains no template
   * parameters, no deduction is performed from this argument.  The pairing
   * trivially succeeds; whether the argument is convertible to P is decided
   * later during overload resolution.  Requiring an exact match here would
   * wrongly reject calls needing a standard conversion (e.g. int -> long) and
   * corrupt deduction of the other, dependent, parameters. */
  if (!TypeContainsTemplateParameter(formal)) {
    return true;
  }
  /* [temp.deduct.type]: a type nominated by a qualified-id whose
   * nested-name-specifier names a member of a dependent type -- e.g.
   * `typename iterator_traits<It>::difference_type` -- is a non-deduced
   * context.  We still *attempt* deduction from it (this compiler can often
   * recover the parameter from the argument's concrete nested type, an
   * extension relied upon by e.g. `T f(typename Owner<T>::Inner)`), but a
   * *failure* to deduce here must not fail the whole call: the parameter may
   * be deduced from another argument or supplied explicitly (e.g. std::next's
   * defaulted `difference_type` second parameter).  We look through leading
   * references and pointers so that `const X<T>::type&` and `X<T>::type*` are
   * recognized too, but not `T*`/`T&` (ordinary deduced contexts, which lack a
   * dependent member name). */
  bool non_deduced_member = false;
  for (TypeRecord* p = formal; p != NULL; p = p->next) {
    if (p->dependent_member_name != NULL) {
      non_deduced_member = true;
      break;
    }
    if (p->declarator != kDeclReference && p->declarator != kDeclRValueReference &&
        p->declarator != kDeclPointer) {
      break;
    }
  }
  Vector* specialization_args = TypeSpecializationTemplateArguments(formal);
  for (size_t i = 0;
       !non_deduced_member && specialization_args != NULL &&
       i < specialization_args->length;
       i++) {
    non_deduced_member = TemplateArgumentHasDependentMemberName(
        specialization_args->value.p[i]);
  }
  int placeholder_index = -1;
  if (formal->declarator == kDeclRValueReference &&
      TypeIsTemplateParameterPlaceholder(formal->next, &placeholder_index) &&
      ForwardingArgumentIsLvalue(actual)) {
    TypeRecord* lvalue_ref = NewReferenceTypeRecord(kQualPlain, false);
    TypeRecordChain(lvalue_ref, actual->type);
    lvalue_ref->type = actual->type->type;
    TypeRecordCalculateSize(lvalue_ref);
    bool ok = DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                                 formal->next, lvalue_ref);
    TypeRecordDelete(lvalue_ref);
    return ok || non_deduced_member;
  }
  if (formal->declarator == kDeclRValueReference &&
      TypeIsTemplateParameterPlaceholder(formal->next, &placeholder_index) &&
      actual->value_category != kValueCategoryLvalue) {
    // A forwarding reference binding an rvalue deduces the referred-to type
    // exactly, including top-level cv-qualification.  Stripping const here
    // turns `const T&&` actuals into `T&&` specializations and later rejects
    // the call for discarding qualifiers.
    return SetDeducedFunctionTemplateTypeArgumentPreserveQualifiers(
               args, explicit_arg_count, placeholder_index, actual->type) ||
           non_deduced_member;
  }
  TypeRecord* decayed = DecayCallArgumentTypeForDeduction(formal, actual->type);
  if (decayed != NULL) {
    bool ok = DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                                 formal, decayed);
    TypeRecordDelete(decayed);
    return ok || non_deduced_member ||
           FormalClassTemplateIdIsNonDeducedAgainst(formal, actual->type);
  }
  bool ok = DeduceFunctionTemplateTypeArgument(args, explicit_arg_count, formal,
                                               actual->type);
  return ok || non_deduced_member ||
         FormalClassTemplateIdIsNonDeducedAgainst(formal, actual->type);
}

/* True for the members that contribute to structural template-argument
 * deduction: non-static data members and nested types.  Member functions and
 * static members never yield a deduced argument, and their presence differs
 * between a fully-instantiated specialization (which has synthesized implicit
 * special members) and a still-dependent primary, so they are ignored. */
static bool StructMemberParticipatesInDeduction(StructMember* member) {
  return member != NULL && member->symbol != NULL &&
         !member->is_member_function && !member->is_static;
}

/* Deduce template arguments by matching two structurally-identical structs
 * member-by-member (used for aggregate deduction): the non-static data members
 * (and nested types) must agree in order, kind, and name, and each data
 * member's type is deduced recursively. */
static bool DeduceFunctionTemplateStructMembers(Vector* args,
                                                size_t explicit_arg_count,
                                                TypeRecord* formal,
                                                TypeRecord* actual) {
  if (!TypeIsStructOrUnion(formal) || !TypeIsStructOrUnion(actual) ||
      formal->info.struct_info == NULL || actual->info.struct_info == NULL) {
    return false;
  }
  Struct* formal_struct = formal->info.struct_info;
  Struct* actual_struct = actual->info.struct_info;

  // Only non-static data members (and nested types) participate in deduction;
  // member functions and static members never contribute a deduced argument.
  // The two sides may legitimately differ in their member-function/static-member
  // sets: a concrete specialization that has already been fully instantiated has
  // its implicit special members (default/copy/move constructor, destructor,
  // assignment operators) synthesized, whereas the still-dependent primary the
  // formal side comes from does not.  So walk only the deduction-relevant
  // members on each side, in order, and ignore the rest -- comparing raw member
  // counts (or positions) would spuriously fail after such synthesis.
  size_t fi = 0;
  size_t ai = 0;
  for (;;) {
    while (fi < formal_struct->members.length &&
           !StructMemberParticipatesInDeduction(
               formal_struct->members.value.p[fi])) {
      fi++;
    }
    while (ai < actual_struct->members.length &&
           !StructMemberParticipatesInDeduction(
               actual_struct->members.value.p[ai])) {
      ai++;
    }
    bool formal_done = fi >= formal_struct->members.length;
    bool actual_done = ai >= actual_struct->members.length;
    if (formal_done && actual_done) {
      break;
    }
    if (formal_done != actual_done) {
      return false;
    }
    StructMember* formal_member = formal_struct->members.value.p[fi];
    StructMember* actual_member = actual_struct->members.value.p[ai];
    if (StructMemberIsNestedType(formal_member) !=
            StructMemberIsNestedType(actual_member) ||
        !StringEqualString(&formal_member->symbol->name,
                           &actual_member->symbol->name)) {
      return false;
    }
    if (StructMemberIsNestedType(formal_member)) {
      if (!TypeEqual(formal_member->symbol->type, actual_member->symbol->type)) {
        return false;
      }
    } else if (!DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                                   formal_member->symbol->type,
                                                   actual_member->symbol->type)) {
      return false;
    }
    fi++;
    ai++;
  }
  return true;
}

/* True if every still-undeduced template parameter has a usable default
 * (type default or default int value), so deduction can be completed. */
static bool FunctionTemplateCanCompleteDeducedArguments(TypeRecord* func,
                                                        Vector* args) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.template_parameters.length != args->length) {
    return false;
  }
  for (size_t i = 0; i < args->length; i++) {
    if (args->value.p[i] != NULL) {
      continue;
    }
    TemplateParameter* param = func->info.function.template_parameters.value.p[i];
    if (param->kind == kTemplateParameterType) {
      if (param->default_type == NULL) {
        return false;
      }
    } else if (!param->has_default_int &&
               param->default_argument == NULL) {
      return false;
    }
  }
  return true;
}

static bool TypeContainsPackIndexSpecifier(TypeRecord* type, int* pack_index) {
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (t->is_pack_index) {
      if (pack_index != NULL) {
        *pack_index = t->template_parameter_index;
      }
      return true;
    }
  }
  return false;
}

/* Build the initial deduction-argument vector for a function template, one slot
 * per template parameter. Explicitly provided arguments are placed (a trailing
 * parameter pack absorbs all remaining explicit args), and parameters left to
 * be deduced are NULL. Reports how many leading slots are explicit. Returns
 * NULL on an explicit-argument/parameter mismatch. */
static Vector* NewFunctionTemplateDeductionArguments(TypeRecord* func,
                                                     Vector* explicit_args,
                                                     size_t* explicit_arg_count) {
  Vector* args = NewVector();
  VectorReserve(args, func->info.function.template_parameters.length);
  size_t explicit_index = 0;
  size_t fixed_explicit_count = 0;
  for (size_t i = 0; i < func->info.function.template_parameters.length; i++) {
    TemplateParameter* param =
        func->info.function.template_parameters.value.p[i];
    if (param != NULL && param->is_parameter_pack) {
      TemplateArgument* pack = NewEmptyPackTemplateArgument(param->kind);
      // A pack absorbs the tail of an explicit argument list.  `take<Ts...>()`
      // has already bound the pack; call deduction must not append again.
      // A list that stops before the pack (`f<T>(a, b)`) leaves it to
      // deduction.  An empty list whose first parameter is the pack is an
      // explicit empty pack (`take<>()`).  int_value is not otherwise a value
      // of the pack itself; deduction reads it and clears it before returning.
      bool explicitly_specified = false;
      if (explicit_args != NULL) {
        size_t remaining = explicit_args->length - explicit_index;
        if (remaining > 0 || i == 0) {
          explicitly_specified = true;
        }
      }
      pack->int_value = explicitly_specified ? 1 : 0;
      while (explicit_args != NULL && explicit_index < explicit_args->length) {
        TemplateArgument* explicit_arg = explicit_args->value.p[explicit_index++];
        if (explicit_arg == NULL || explicit_arg->kind != param->kind) {
          TemplateArgumentDelete(pack);
          VectorDeleteWithContents(args,
                                   (VectorElementDestructor)TemplateArgumentDelete,
                                   /*free_element=*/false);
          return NULL;
        }
        if (explicit_arg->pack_arguments != NULL) {
          for (size_t j = 0; j < explicit_arg->pack_arguments->length; j++) {
            VectorAppend(pack->pack_arguments,
                         TemplateArgumentCopy(
                             explicit_arg->pack_arguments->value.p[j]));
          }
        } else {
          VectorAppend(pack->pack_arguments, TemplateArgumentCopy(explicit_arg));
        }
      }
      VectorAppend(args, pack);
      continue;
    }

    TemplateArgument* explicit_arg = NULL;
    if (explicit_args != NULL && explicit_index < explicit_args->length) {
      TemplateArgument* source = explicit_args->value.p[explicit_index++];
      explicit_arg = source != NULL && source->pack_arguments != NULL &&
                             source->pack_arguments->length == 1
                         ? TemplateArgumentCopy(source->pack_arguments->value.p[0])
                         : TemplateArgumentCopy(source);
    }
    if (explicit_arg != NULL) {
      fixed_explicit_count = i + 1;
    }
    VectorAppend(args, explicit_arg);
  }
  if (explicit_args != NULL && explicit_index < explicit_args->length) {
    VectorDeleteWithContents(args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return NULL;
  }
  if (explicit_arg_count != NULL) {
    *explicit_arg_count = fixed_explicit_count;
  }
  return args;
}

/* Number of leading fixed formals that must be supplied by call arguments (i.e.
 * those without a default argument), used to validate the call's arity. */
static size_t RequiredFixedFunctionTemplateFormals(TypeRecord* func,
                                                   size_t first_formal_arg,
                                                   size_t fixed_formal_count) {
  size_t required = 0;
  for (size_t i = 0; i < fixed_formal_count; i++) {
    Symbol* formal =
        func->info.function.prototype.value.p[i + first_formal_arg];
    if (formal != NULL && formal->default_argument == NULL) {
      required = i + 1;
    }
  }
  return required;
}

/* Deduce the full template argument vector for a call to function template
 * `templ` from the explicit template arguments and the actual call arguments
 * `actuals` (skipping `first_formal_arg` leading formals, e.g. an implicit
 * `this`). Validates arity (including a trailing parameter pack), deduces each
 * argument, and returns the deduced argument vector or NULL if deduction fails.
 */
static Vector* DeduceSimpleFunctionTemplateArguments(Symbol* templ,
                                                     Vector* explicit_args,
                                                     Vector* actuals,
                                                     size_t first_formal_arg) {
  if (templ == NULL || templ->type == NULL || !templ->flags.is_template ||
      !TypeIsFunction(templ->type)) {
    return NULL;
  }
  // Deduce against the candidate's current signature.  In particular, a
  // member template of an instantiated class has already had the enclosing
  // class arguments substituted and its own parameter indices rebased to
  // zero.  Falling back to value.func_defn here selects the primary class's
  // stale signature (whose member parameters still start after the enclosing
  // parameters), so deduction writes past the argument vector and rejects
  // valid calls such as a range-adaptor closure's operator()(R&&).
  TypeRecord* func = templ->type;
  int saved_deduce_base = g_deduce_template_parameter_base;
  g_deduce_template_parameter_base =
      func->info.function.template_parameter_base;
  if (first_formal_arg > func->info.function.prototype.length) {
    g_deduce_template_parameter_base = saved_deduce_base;
    return NULL;
  }
  int formal_pack_index = -1;
  size_t formal_pack_count = 0;
  for (size_t i = first_formal_arg; i < func->info.function.prototype.length;
       i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    if (formal != NULL && formal->flags.is_parameter_pack) {
      formal_pack_index = (int)i;
      formal_pack_count++;
    }
  }
  size_t fixed_formal_count =
      func->info.function.prototype.length - first_formal_arg;
  if (formal_pack_index >= 0) {
    fixed_formal_count = (size_t)formal_pack_index - first_formal_arg;
  }
  size_t required_formal_count = RequiredFixedFunctionTemplateFormals(
      func, first_formal_arg, fixed_formal_count);
  if (formal_pack_count > 1) {
    required_formal_count = 0;
    for (size_t i = first_formal_arg;
         i < func->info.function.prototype.length; i++) {
      Symbol* formal = func->info.function.prototype.value.p[i];
      if (formal != NULL && !formal->flags.is_parameter_pack &&
          formal->default_argument == NULL) {
        required_formal_count++;
      }
    }
  }
  if (func->info.function.template_parameter_count <= 0 ||
      func->info.function.unknown_args || func->info.function.varargs ||
      (formal_pack_index < 0 &&
       (actuals->length < required_formal_count ||
        actuals->length > fixed_formal_count)) ||
      (formal_pack_index >= 0 && actuals->length < required_formal_count)) {
    g_deduce_template_parameter_base = saved_deduce_base;
    return NULL;
  }
  // Deduce in two phases so a default template argument takes precedence over
  // this compiler's non-conforming `T::member` member-access deduction
  // extension: phase 1 defers bare-member parameters (leaving them for their
  // defaults); if that leaves a parameter unbound with no usable default,
  // phase 2 re-runs deduction with the extension enabled.
  bool defer_bare_member = true;
  size_t explicit_arg_count = 0;
  Vector* args = NULL;
retry_deduction:
  explicit_arg_count = 0;
  args = NewFunctionTemplateDeductionArguments(func, explicit_args,
                                               &explicit_arg_count);
  if (args == NULL) {
    g_deduce_template_parameter_base = saved_deduce_base;
    return NULL;
  }
  g_deduce_defer_bare_member = defer_bare_member;
  if (formal_pack_count > 1) {
    size_t actual_index = 0;
    for (size_t formal_index = first_formal_arg;
         formal_index < func->info.function.prototype.length; formal_index++) {
      Symbol* formal = func->info.function.prototype.value.p[formal_index];
      if (formal == NULL) {
        continue;
      }
      if (formal->flags.is_parameter_pack) {
        int pack_type_index = -1;
        size_t pack_length = 0;
        if (!FindPackExpansionInType(formal->type, args, &pack_type_index,
                                     &pack_length) ||
            pack_type_index < 0 ||
            (size_t)pack_type_index >= args->length) {
          goto deduction_failed;
        }
        bool trailing =
            formal_index + 1 == func->info.function.prototype.length;
        TemplateArgument* pack = args->value.p[pack_type_index];
        size_t consume = 0;
        if (trailing) {
          consume = actuals->length - actual_index;
        } else if (pack != NULL && pack->pack_arguments != NULL) {
          // A non-trailing function parameter pack is a non-deduced context.
          // Explicitly supplied pack elements still determine how many call
          // arguments this formal consumes.
          consume = pack->pack_arguments->length;
        } else {
          args->value.p[pack_type_index] =
              NewEmptyPackTemplateArgument(kTemplateParameterType);
          pack = args->value.p[pack_type_index];
        }
        if (actual_index + consume > actuals->length) {
          goto deduction_failed;
        }
        if (!trailing) {
          actual_index += consume;
          continue;
        }
        if (pack != NULL && pack->int_value != 0) {
          size_t explicit_len = pack->pack_arguments != NULL
                                    ? pack->pack_arguments->length
                                    : 0;
          size_t provided = actuals->length - actual_index;
          bool expansion = false;
          for (size_t j = actual_index; j < actuals->length; j++) {
            ASTNode* pack_actual = actuals->value.p[j];
            if (pack_actual != NULL &&
                (pack_actual->flags & kASTPackExpansion) != 0) {
              expansion = true;
              break;
            }
          }
          if (!expansion && provided != explicit_len) {
            goto deduction_failed;
          }
          actual_index = actuals->length;
          continue;
        }
        for (size_t j = 0; j < consume; j++) {
          ASTNode* actual = actuals->value.p[actual_index++];
          if (actual == NULL || actual->type == NULL ||
              !DeduceFunctionTemplatePackCallArgument(
                  args, explicit_arg_count, pack_type_index, formal->type,
                  actual)) {
            goto deduction_failed;
          }
        }
        continue;
      }
      if (actual_index >= actuals->length) {
        if (formal->default_argument == NULL) {
          goto deduction_failed;
        }
        continue;
      }
      ASTNode* actual = actuals->value.p[actual_index++];
      if (actual == NULL ||
          !(DeduceFunctionTemplateArrayInitializerArgument(
                args, explicit_arg_count, formal->type, actual) ||
            DeduceFunctionTemplateInitializerListArgument(
                args, explicit_arg_count, formal->type, actual) ||
            DeduceFunctionTemplateCallArgument(args, explicit_arg_count,
                                               formal->type, actual))) {
        goto deduction_failed;
      }
    }
    if (actual_index != actuals->length) {
      goto deduction_failed;
    }
    goto deduction_finished;
  }
  for (size_t i = 0; i < actuals->length; i++) {
    Symbol* formal = NULL;
    if (formal_pack_index >= 0 && i >= fixed_formal_count) {
      formal = func->info.function.prototype.value.p[formal_pack_index];
      int pack_type_index = -1;
      ASTNode* actual = actuals->value.p[i];
      size_t pack_length = 0;
      if (formal != NULL &&
          FindPackExpansionInType(formal->type, args, &pack_type_index,
                                  &pack_length) &&
          pack_type_index >= 0 &&
          (size_t)pack_type_index < args->length) {
        TemplateArgument* explicit_pack = args->value.p[pack_type_index];
        if (explicit_pack != NULL && explicit_pack->int_value != 0) {
          if (i == fixed_formal_count) {
            size_t explicit_len = explicit_pack->pack_arguments != NULL
                                      ? explicit_pack->pack_arguments->length
                                      : 0;
            size_t provided = actuals->length - fixed_formal_count;
            bool expansion = false;
            for (size_t j = fixed_formal_count; j < actuals->length; j++) {
              ASTNode* pack_actual = actuals->value.p[j];
              if (pack_actual != NULL &&
                  (pack_actual->flags & kASTPackExpansion) != 0) {
                expansion = true;
                break;
              }
            }
            if (!expansion && provided != explicit_len) {
              g_deduce_defer_bare_member = false;
              g_deduce_template_parameter_base = saved_deduce_base;
              VectorDeleteWithContents(
                  args, (VectorElementDestructor)TemplateArgumentDelete,
                  /*free_element=*/false);
              return NULL;
            }
          }
          continue;
        }
      }
      if (formal == NULL || actual == NULL || actual->type == NULL ||
          !FindPackExpansionInType(formal->type, args, &pack_type_index,
                                   &pack_length) ||
          !DeduceFunctionTemplatePackCallArgument(
              args, explicit_arg_count, pack_type_index, formal->type,
              actual)) {
        g_deduce_defer_bare_member = false;
        g_deduce_template_parameter_base = saved_deduce_base;
        VectorDeleteWithContents(args,
                                 (VectorElementDestructor)TemplateArgumentDelete,
                                 /*free_element=*/false);
        return NULL;
      }
      continue;
    }
    formal = func->info.function.prototype.value.p[i + first_formal_arg];
    ASTNode* actual = actuals->value.p[i];
    if (formal == NULL || actual == NULL ||
        !(DeduceFunctionTemplateArrayInitializerArgument(
              args, explicit_arg_count, formal->type, actual) ||
          DeduceFunctionTemplateInitializerListArgument(
              args, explicit_arg_count, formal->type, actual) ||
          DeduceFunctionTemplateCallArgument(args, explicit_arg_count,
                                             formal->type, actual))) {
      g_deduce_defer_bare_member = false;
      g_deduce_template_parameter_base = saved_deduce_base;
      VectorDeleteWithContents(args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      return NULL;
    }
  }
deduction_finished:
  g_deduce_defer_bare_member = false;
  if (args != NULL) {
    for (size_t i = 0; i < args->length; i++) {
      TemplateArgument* arg = args->value.p[i];
      if (arg != NULL && arg->pack_arguments != NULL) {
        arg->int_value = 0;
      }
    }
  }
  if (formal_pack_index >= 0) {
    Symbol* formal = func->info.function.prototype.value.p[formal_pack_index];
    int pack_type_index = -1;
    if (formal != NULL &&
        TypeIsTemplateParameterPlaceholder(formal->type, &pack_type_index) &&
        pack_type_index >= 0 && (size_t)pack_type_index < args->length &&
        args->value.p[pack_type_index] == NULL) {
      args->value.p[pack_type_index] =
          NewEmptyPackTemplateArgument(kTemplateParameterType);
    }
  }
  for (size_t i = 0; i < args->length; i++) {
    if (args->value.p[i] == NULL) {
      if (!FunctionTemplateCanCompleteDeducedArguments(func, args)) {
        VectorDeleteWithContents(args,
                                 (VectorElementDestructor)TemplateArgumentDelete,
                                 /*free_element=*/false);
        // A parameter is unbound with no usable default: retry with the
        // bare-member extension enabled before giving up.
        if (defer_bare_member) {
          defer_bare_member = false;
          goto retry_deduction;
        }
        g_deduce_template_parameter_base = saved_deduce_base;
        return NULL;
      }
      break;
    }
  }
  // A pack-indexing parameter is non-deduced, but an empty placeholder pack
  // created for deduction is not a valid binding: every index is out of range.
  // Reject the candidate unless that pack was explicitly supplied or deduced
  // from another parameter.
  for (size_t i = first_formal_arg;
       i < func->info.function.prototype.length; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    int pack_index = -1;
    if (formal == NULL ||
        !TypeContainsPackIndexSpecifier(formal->type, &pack_index)) {
      continue;
    }
    TemplateArgument* pack =
        pack_index >= 0 && (size_t)pack_index < args->length
            ? args->value.p[pack_index]
            : NULL;
    if (pack == NULL || pack->pack_arguments == NULL ||
        pack->pack_arguments->length == 0) {
      VectorDeleteWithContents(
          args, (VectorElementDestructor)TemplateArgumentDelete,
          /*free_element=*/false);
      g_deduce_template_parameter_base = saved_deduce_base;
      return NULL;
    }
  }
  g_deduce_template_parameter_base = saved_deduce_base;
  return args;

deduction_failed:
  g_deduce_defer_bare_member = false;
  VectorDeleteWithContents(args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  if (defer_bare_member) {
    defer_bare_member = false;
    goto retry_deduction;
  }
  g_deduce_template_parameter_base = saved_deduce_base;
  return NULL;
}

/* Public: deduce and instantiate a function template from a call's actuals. */
Symbol* TypeDeduceFunctionTemplateFromCall(Syntax* syntax, Symbol* templ,
                                           Vector* actuals) {
  return TypeDeduceFunctionTemplateFromCallWithExplicitArgsAndOffset(
      syntax, templ, NULL, actuals, 0);
}

/* Public: as above, with caller-supplied explicit template arguments. */
Symbol* TypeDeduceFunctionTemplateFromCallWithExplicitArgs(
    Syntax* syntax, Symbol* templ, Vector* explicit_args, Vector* actuals) {
  return TypeDeduceFunctionTemplateFromCallWithExplicitArgsAndOffset(
      syntax, templ, explicit_args, actuals, 0);
}

/* Public: as above, skipping `first_formal_arg` leading formals (e.g. implicit
 * `this` for member functions). */
Symbol* TypeDeduceFunctionTemplateFromCallWithOffset(Syntax* syntax,
                                                     Symbol* templ,
                                                     Vector* actuals,
                                                     size_t first_formal_arg) {
  return TypeDeduceFunctionTemplateFromCallWithExplicitArgsAndOffset(
      syntax, templ, NULL, actuals, first_formal_arg);
}

static void SetMemberTemplateSubstitutionContext(TypeParser* parser,
                                                 TypeRecord* func_type);

/* Public: deduce template arguments from a call (with explicit args and formal
 * offset) and instantiate the template. Returns `templ` unchanged if deduction
 * fails. */
Symbol* TypeDeduceFunctionTemplateFromCallWithExplicitArgsAndOffset(
    Syntax* syntax, Symbol* templ, Vector* explicit_args, Vector* actuals,
    size_t first_formal_arg) {
  Vector* args =
      DeduceSimpleFunctionTemplateArguments(templ, explicit_args, actuals,
                                            first_formal_arg);
  if (args == NULL) {
    return templ;
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                 syntax->context);
  TypeRecord* func_type = templ->type;
  if (templ->is_imported_module_symbol && func_type != NULL &&
      TypeIsFunction(func_type) &&
      func_type->info.function.template_parameters.length == 0 &&
      templ->value.func_defn != NULL && templ->value.func_defn->type != NULL &&
      TypeIsFunction(templ->value.func_defn->type) &&
      templ->value.func_defn->type->info.function.template_parameters.length > 0) {
    func_type = templ->value.func_defn->type;
  }
  SetMemberTemplateSubstitutionContext(&parser, func_type);
  Vector* completed_args =
      CompleteFunctionTemplateArguments(&parser, func_type, args,
                                        /*emit_error=*/true,
                                        kTemplateArgumentsConsume);
  TypeParserDestruct(&parser);
  VectorDeleteWithContents(args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  if (completed_args == NULL) {
    return templ;
  }
  if (!ConceptsFunctionTemplateConstraintsSatisfied(templ, completed_args)) {
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return templ;
  }
  Symbol* symbol = TypeInstantiateFunctionTemplateWithCompletedArguments(
      syntax, templ, completed_args);
  VectorDeleteWithContents(completed_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return symbol;
}

/* Member-alias defaults (`EnableIfValueIsConst<LazyT>`,
 * `IsLifetimeBoundAssignmentFrom<U>`) mention the enclosing class parameters.
 * Overload resolution builds a parser with no active instantiation, so without
 * the member's class the alias binds its own argument into the class slot and
 * SFINAE rejects a valid constructor. */
static void SetMemberTemplateSubstitutionContext(TypeParser* parser,
                                                 TypeRecord* func_type) {
  if (parser == NULL || func_type == NULL || !TypeIsFunction(func_type) ||
      func_type->info.function.cxx_member_owner == NULL) {
    return;
  }
  parser->template_substitution_target =
      func_type->info.function.cxx_member_owner;
  Struct* owner = parser->template_substitution_target;
  if (owner->tag_symbol != NULL && owner->tag_symbol->type != NULL &&
      owner->tag_symbol->type->template_origin != NULL &&
      owner->tag_symbol->type->template_origin->type != NULL &&
      TypeIsStructOrUnion(owner->tag_symbol->type->template_origin->type)) {
    parser->template_substitution_source =
        owner->tag_symbol->type->template_origin->type->info.struct_info;
  }
}

/* Public: test whether a function template's arguments can be deduced from a
 * call (used for overload viability) without instantiating it. */
bool TypeCanDeduceFunctionTemplateFromCallWithExplicitArgsAndOffset(
    Symbol* templ, Vector* explicit_args, Vector* actuals,
    size_t first_formal_arg) {
  Vector* args =
      DeduceSimpleFunctionTemplateArguments(templ, explicit_args, actuals,
                                            first_formal_arg);
  if (args == NULL) {
    return false;
  }
  TypeParser parser;
  TypeParserInit(&parser, compiler->syntax.lex, &compiler->syntax,
                 STO(implicit), compiler->syntax.context);
  TypeRecord* func_type = templ->type;
  if (templ->is_imported_module_symbol && func_type != NULL &&
      TypeIsFunction(func_type) &&
      func_type->info.function.template_parameters.length == 0 &&
      templ->value.func_defn != NULL && templ->value.func_defn->type != NULL &&
      TypeIsFunction(templ->value.func_defn->type) &&
      templ->value.func_defn->type->info.function.template_parameters.length > 0) {
    func_type = templ->value.func_defn->type;
  }
  SetMemberTemplateSubstitutionContext(&parser, func_type);
  Vector* completed_args =
      CompleteFunctionTemplateArguments(&parser, func_type, args,
                                        /*emit_error=*/false,
                                        kTemplateArgumentsConsume);
  TypeParserDestruct(&parser);
  VectorDeleteWithContents(args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  if (completed_args == NULL) {
    return false;
  }
  bool ok = ConceptsFunctionTemplateConstraintsSatisfied(templ, completed_args);
  VectorDeleteWithContents(completed_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return ok;
}

FunctionTemplateCandidateStatus TypeClassifyFunctionTemplateCandidate(
    Syntax* syntax, Symbol* templ, Vector* explicit_args, Vector* actuals,
    size_t first_formal_arg) {
  if (templ == NULL || !templ->flags.is_template) {
    return kFunctionTemplateCandidateViable;
  }
  Vector* args =
      DeduceSimpleFunctionTemplateArguments(templ, explicit_args, actuals,
                                            first_formal_arg);
  if (args == NULL) {
    return kFunctionTemplateCandidateDeductionFailed;
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                 syntax->context);
  TypeRecord* func_type = templ->type;
  if (templ->is_imported_module_symbol && func_type != NULL &&
      TypeIsFunction(func_type) &&
      func_type->info.function.template_parameters.length == 0 &&
      templ->value.func_defn != NULL && templ->value.func_defn->type != NULL &&
      TypeIsFunction(templ->value.func_defn->type) &&
      templ->value.func_defn->type->info.function.template_parameters.length > 0) {
    func_type = templ->value.func_defn->type;
  }
  SetMemberTemplateSubstitutionContext(&parser, func_type);
  Vector* completed_args =
      CompleteFunctionTemplateArguments(&parser, func_type, args,
                                        /*emit_error=*/false,
                                        kTemplateArgumentsConsume);
  VectorDeleteWithContents(args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  if (completed_args == NULL ||
      TemplateArgumentVectorContainsTemplateParameterForInstantiation(
          completed_args)) {
    if (completed_args != NULL) {
      VectorDeleteWithContents(
          completed_args, (VectorElementDestructor)TemplateArgumentDelete,
          /*free_element=*/false);
    }
    TypeParserDestruct(&parser);
    return kFunctionTemplateCandidateDeductionFailed;
  }
  if (!ConceptsFunctionTemplateConstraintsSatisfied(templ, completed_args)) {
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    TypeParserDestruct(&parser);
    return kFunctionTemplateCandidateConstraintsNotSatisfied;
  }
  VectorDeleteWithContents(completed_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  TypeParserDestruct(&parser);
  return kFunctionTemplateCandidateViable;
}

/* Public: build a non-emitting concrete candidate for overload resolution.
 * This performs deduction, constraint checking, default completion, and
 * signature substitution, but it does not clone a body or enqueue codegen. */
Symbol* TypeCreateFunctionTemplateCandidate(Syntax* syntax, Symbol* templ,
                                            Vector* explicit_args,
                                            Vector* actuals,
                                            size_t first_formal_arg) {
  Vector* args =
      DeduceSimpleFunctionTemplateArguments(templ, explicit_args, actuals,
                                            first_formal_arg);
  if (args == NULL) {
    return NULL;
  }

  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                 syntax->context);
  TypeRecord* func_type = templ->type;
  if (templ->is_imported_module_symbol && func_type != NULL &&
      TypeIsFunction(func_type) &&
      func_type->info.function.template_parameters.length == 0 &&
      templ->value.func_defn != NULL && templ->value.func_defn->type != NULL &&
      TypeIsFunction(templ->value.func_defn->type) &&
      templ->value.func_defn->type->info.function.template_parameters.length > 0) {
    func_type = templ->value.func_defn->type;
  }
  SetMemberTemplateSubstitutionContext(&parser, func_type);
  Vector* completed_args =
      CompleteFunctionTemplateArguments(&parser, func_type, args,
                                        /*emit_error=*/false,
                                        kTemplateArgumentsConsume);
  VectorDeleteWithContents(args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  if (completed_args == NULL ||
      TemplateArgumentVectorContainsTemplateParameterForInstantiation(
          completed_args)) {
    if (completed_args != NULL) {
      VectorDeleteWithContents(
          completed_args, (VectorElementDestructor)TemplateArgumentDelete,
          /*free_element=*/false);
    }
    TypeParserDestruct(&parser);
    return NULL;
  }
  if (!ConceptsFunctionTemplateConstraintsSatisfied(templ, completed_args)) {
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    TypeParserDestruct(&parser);
    return NULL;
  }

  Symbol* template_definition = templ;
  if ((template_definition->type == NULL ||
       template_definition->type->info.function.body == NULL) &&
      templ->value.func_defn != NULL &&
      templ->value.func_defn->type != NULL) {
    template_definition = templ->value.func_defn;
  }
  bool saved_substitution_failed = parser.template_substitution_failed;
  parser.template_substitution_failed = false;
  // Member templates of an instantiated class carry a current signature with
  // the enclosing arguments substituted and their own parameters rebased.
  // Instantiate that signature; the primary definition remains the body source.
  TypeRecord* func =
      InstantiateFunctionTemplateType(&parser, func_type, completed_args);
  bool substitution_failed = parser.template_substitution_failed;
  parser.template_substitution_failed = saved_substitution_failed;
  // A trailing `decltype` that is still unknown after concrete arguments were
  // substituted did not yield a type (`decltype(P::soo_enabled())` when `P`
  // has no such member).  Keeping the candidate lets it beat the non-template
  // overload that the failed substitution was meant to select.
  bool unresolved_trailing_decltype =
      func != NULL && func->next != NULL && TypeIsUnknown(func->next) &&
      func_type != NULL && func_type->next != NULL &&
      (func_type->next->dependent_decltype_expr != NULL ||
       TypeIsDetachedDependentDecltype(func_type->next));
  if (substitution_failed || TypeContainsTemplateParameter(func) ||
      unresolved_trailing_decltype) {
    TypeRecordDelete(func);
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    TypeParserDestruct(&parser);
    return NULL;
  }

  Symbol* symbol = NewSymbol(templ->name.value, func, templ->storage);
  symbol->location = templ->location;
  symbol->namespace_ = templ->namespace_;
  func->info.function.symbol = symbol;
  func->info.function.template_origin = templ;
  // The candidate type owns this completed vector. Temporary candidate
  // teardown releases it if overload resolution does not select it.
  func->template_arguments = completed_args;
  TypeParserDestruct(&parser);
  return symbol;
}

/* Public: deduce (but do not instantiate) a function template's argument vector
 * from a call's actuals. */
Vector* TypeDeduceFunctionTemplateArgumentsFromCall(Symbol* templ,
                                                    Vector* actuals,
                                                    size_t first_formal_arg) {
  return DeduceSimpleFunctionTemplateArguments(templ, NULL, actuals,
                                               first_formal_arg);
}

/* Public: deduce the template arguments of a conversion function template for a
 * requested target type.  A conversion function template has no value
 * parameters, so its arguments are deduced by matching the declared (dependent)
 * target type (`func->next`) against the required type `target`
 * ([temp.deduct.conv]) rather than from call arguments.  Returns the completed
 * argument vector (caller owns) or NULL if deduction/default-completion/
 * constraints fail. */
Vector* TypeDeduceConversionOperatorTemplateArguments(Syntax* syntax,
                                                      Symbol* templ,
                                                      TypeRecord* target) {
  if (templ == NULL || templ->type == NULL || !templ->flags.is_template ||
      !TypeIsFunction(templ->type) || target == NULL) {
    return NULL;
  }
  TypeRecord* func =
      templ->value.func_defn != NULL && templ->value.func_defn->type != NULL
          ? templ->value.func_defn->type
          : templ->type;
  if (func->info.function.template_parameter_count <= 0 || func->next == NULL) {
    return NULL;
  }
  // [temp.deduct.conv]: deduce against the referred-to type, ignoring
  // top-level cv-qualifiers.  `const vector&` must deduce `Container` as
  // `vector`, not as a reference; the prvalue then binds to the reference.
  TypeRecord* pattern = func->next;
  TypeRecord* deduction_target = target;
  if (TypeIsReference(pattern)) {
    pattern = pattern->next;
  }
  if (TypeIsReference(deduction_target)) {
    deduction_target = deduction_target->next;
  }
  TypeRecord* unqualified_target = NULL;
  if (pattern == NULL || deduction_target == NULL) {
    return NULL;
  }
  if (deduction_target->qualifiers != kQualPlain) {
    unqualified_target = TypeRecordCopy(deduction_target);
    unqualified_target->qualifiers = kQualPlain;
    deduction_target = unqualified_target;
  }
  size_t explicit_arg_count = 0;
  Vector* args = NewFunctionTemplateDeductionArguments(func, /*explicit_args=*/
                                                       NULL, &explicit_arg_count);
  if (args == NULL) {
    if (unqualified_target != NULL) {
      TypeRecordDelete(unqualified_target);
    }
    return NULL;
  }
  if (!DeduceFunctionTemplateTypeArgument(args, explicit_arg_count, pattern,
                                          deduction_target)) {
    if (unqualified_target != NULL) {
      TypeRecordDelete(unqualified_target);
    }
    VectorDeleteWithContents(args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return NULL;
  }
  if (unqualified_target != NULL) {
    TypeRecordDelete(unqualified_target);
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  Vector* completed_args =
      CompleteFunctionTemplateArguments(&parser, func, args,
                                        /*emit_error=*/false,
                                        kTemplateArgumentsConsume);
  TypeParserDestruct(&parser);
  VectorDeleteWithContents(args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  if (completed_args == NULL ||
      TemplateArgumentVectorContainsTemplateParameterForInstantiation(
          completed_args)) {
    if (completed_args != NULL) {
      VectorDeleteWithContents(completed_args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    return NULL;
  }
  if (!ConceptsFunctionTemplateConstraintsSatisfied(templ, completed_args)) {
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return NULL;
  }
  return completed_args;
}

/* Public: deduce the template arguments of a function template whose *address*
 * is being taken against a required function type ([temp.deduct.funcaddr]).
 * Unlike call deduction, the template arguments are deduced by matching the
 * whole (dependent) function type of the template against the concrete target
 * function type `target_fn` (the pointee of the destination function pointer).
 * `explicit_args` supplies any explicitly-written template arguments (may be
 * NULL).  Returns the completed argument vector (caller owns) or NULL if
 * deduction / default completion / constraints fail. */
Vector* TypeDeduceFunctionTemplateArgumentsFromFunctionType(Syntax* syntax,
                                                            Symbol* templ,
                                                            Vector* explicit_args,
                                                            TypeRecord* target_fn) {
  if (templ == NULL || templ->type == NULL || !templ->flags.is_template ||
      !TypeIsFunction(templ->type) || target_fn == NULL ||
      !TypeIsFunction(target_fn)) {
    return NULL;
  }
  TypeRecord* func =
      templ->value.func_defn != NULL && templ->value.func_defn->type != NULL
          ? templ->value.func_defn->type
          : templ->type;
  if (func->info.function.template_parameter_count <= 0) {
    return NULL;
  }
  size_t explicit_arg_count = 0;
  Vector* args = NewFunctionTemplateDeductionArguments(func, explicit_args,
                                                       &explicit_arg_count);
  if (args == NULL) {
    return NULL;
  }
  if (!DeduceFunctionTemplateTypeArgument(args, explicit_arg_count, func,
                                          target_fn)) {
    VectorDeleteWithContents(args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return NULL;
  }
  // Default template arguments are written on the declaration, not on an
  // out-of-class definition (`template <class T, EnableIf<T>> C::C(T&&)`).
  TypeRecord* completion_func =
      templ->type->info.function.template_parameters.length > 0 ? templ->type
                                                                : func;
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  Vector* completed_args =
      CompleteFunctionTemplateArguments(&parser, completion_func, args,
                                        /*emit_error=*/false,
                                        kTemplateArgumentsConsume);
  TypeParserDestruct(&parser);
  VectorDeleteWithContents(args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  if (completed_args == NULL ||
      TemplateArgumentVectorContainsTemplateParameterForInstantiation(
          completed_args)) {
    if (completed_args != NULL) {
      VectorDeleteWithContents(completed_args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    return NULL;
  }
  if (!ConceptsFunctionTemplateConstraintsSatisfied(templ, completed_args)) {
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return NULL;
  }
  return completed_args;
}

/* True if conversion function template `specialized` is at least as specialized
 * as `general` for partial ordering ([temp.func.order], [temp.deduct.partial]):
 * treat `specialized`'s target pattern as the argument (its own parameters act
 * as unique opaque types on the argument side) and try to deduce `general`'s
 * parameters from it.  Success means `general` is at least as general, i.e.
 * `specialized` is at least as specialized. */
static bool ConversionTargetAtLeastAsSpecialized(Symbol* specialized,
                                                 Symbol* general) {
  TypeRecord* sfunc =
      specialized->value.func_defn != NULL &&
              specialized->value.func_defn->type != NULL
          ? specialized->value.func_defn->type
          : specialized->type;
  TypeRecord* gfunc =
      general->value.func_defn != NULL && general->value.func_defn->type != NULL
          ? general->value.func_defn->type
          : general->type;
  if (sfunc->next == NULL || gfunc->next == NULL) {
    return false;
  }
  size_t explicit_arg_count = 0;
  Vector* args =
      NewFunctionTemplateDeductionArguments(gfunc, NULL, &explicit_arg_count);
  if (args == NULL) {
    return false;
  }
  bool ok = DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                               gfunc->next, sfunc->next);
  VectorDeleteWithContents(args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return ok;
}

/* Public: partial ordering of two conversion function templates by their target
 * type ([temp.func.order]).  Returns 1 if `a` is more specialized than `b`, -1
 * if `b` is more specialized than `a`, and 0 if neither is (they are equivalent
 * or incomparable, i.e. ambiguous). */
int TypeConversionOperatorTemplateMoreSpecialized(Syntax* syntax, Symbol* a,
                                                  Symbol* b) {
  (void)syntax;
  if (a == NULL || b == NULL || a->type == NULL || b->type == NULL) {
    return 0;
  }
  bool a_at_least_as_specialized = ConversionTargetAtLeastAsSpecialized(a, b);
  bool b_at_least_as_specialized = ConversionTargetAtLeastAsSpecialized(b, a);
  if (a_at_least_as_specialized && !b_at_least_as_specialized) {
    return 1;
  }
  if (b_at_least_as_specialized && !a_at_least_as_specialized) {
    return -1;
  }
  return 0;
}

static TypeRecord* FunctionTemplatePatternFunctionType(Symbol* symbol) {
  if (symbol == NULL || symbol->type == NULL || !TypeIsFunction(symbol->type)) {
    return NULL;
  }
  Symbol* origin = symbol->type->info.function.template_origin;
  if (origin != NULL && origin->type != NULL && TypeIsFunction(origin->type)) {
    return origin->type;
  }
  if (symbol->flags.is_template) {
    return symbol->type;
  }
  return NULL;
}

static bool TypeChainHasDependentMemberName(TypeRecord* type) {
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (t->dependent_member_name != NULL) {
      return true;
    }
  }
  return false;
}

/* True if `specialized`'s parameter patterns can deduce `general`
 * ([temp.deduct.partial]).  Each of `specialized`'s template parameters acts
 * as a unique type on the argument side, so a repeated parameter (`It, It`)
 * is a tighter pattern than an independent one (`It, Pred`).  Parameter packs
 * are left unordered; a pack on either side is not at least as specialized.
 * A dependent member typedef (`type_identity<T>::type`) is not that parameter,
 * so it does not satisfy a plain `T` pattern; the reverse is a non-deduced
 * context and still succeeds.  That orders `f(T*, T*)` ahead of
 * `f(T*, type_identity<T>::type*)` when both are exact matches. */
static bool FunctionParametersAtLeastAsSpecialized(Symbol* specialized,
                                                   Symbol* general) {
  TypeRecord* sfunc = FunctionTemplatePatternFunctionType(specialized);
  TypeRecord* gfunc = FunctionTemplatePatternFunctionType(general);
  if (sfunc == NULL || gfunc == NULL) {
    return false;
  }
  if (sfunc->info.function.prototype.length !=
      gfunc->info.function.prototype.length) {
    return false;
  }
  for (size_t i = 0; i < sfunc->info.function.prototype.length; i++) {
    Symbol* sparam = sfunc->info.function.prototype.value.p[i];
    Symbol* gparam = gfunc->info.function.prototype.value.p[i];
    if (sparam == NULL || gparam == NULL || sparam->flags.is_parameter_pack ||
        gparam->flags.is_parameter_pack) {
      return false;
    }
  }
  int saved_base = g_deduce_template_parameter_base;
  bool saved_defer = g_deduce_defer_bare_member;
  g_deduce_template_parameter_base =
      gfunc->info.function.template_parameter_base;
  g_deduce_defer_bare_member = true;
  size_t explicit_arg_count = 0;
  Vector* args =
      NewFunctionTemplateDeductionArguments(gfunc, NULL, &explicit_arg_count);
  bool ok = args != NULL;
  for (size_t i = 0; ok && i < gfunc->info.function.prototype.length; i++) {
    Symbol* gparam = gfunc->info.function.prototype.value.p[i];
    Symbol* sparam = sfunc->info.function.prototype.value.p[i];
    if (TypeChainHasDependentMemberName(sparam->type) &&
        !TypeChainHasDependentMemberName(gparam->type)) {
      ok = false;
      break;
    }
    ok = DeduceFunctionTemplateTypeArgument(args, explicit_arg_count,
                                           gparam->type, sparam->type);
  }
  if (args != NULL) {
    VectorDeleteWithContents(args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  g_deduce_template_parameter_base = saved_base;
  g_deduce_defer_bare_member = saved_defer;
  return ok;
}

int TypeFunctionTemplateMoreSpecialized(Symbol* a, Symbol* b) {
  if (a == NULL || b == NULL) {
    return 0;
  }
  bool a_at_least = FunctionParametersAtLeastAsSpecialized(a, b);
  bool b_at_least = FunctionParametersAtLeastAsSpecialized(b, a);
  if (a_at_least && !b_at_least) {
    return 1;
  }
  if (b_at_least && !a_at_least) {
    return -1;
  }
  return 0;
}

/* Public: register a user-written CTAD deduction guide for a class template. */
void TypeAddCXXDeductionGuide(Symbol* class_template, Symbol* guide) {
  if (class_template == NULL || class_template->type == NULL ||
      !TypeIsStructOrUnion(class_template->type) ||
      class_template->type->info.struct_info == NULL || guide == NULL) {
    return;
  }
  VectorAppend(&class_template->type->info.struct_info->deduction_guides, guide);
}

/* True if `origin` is an alias template whose pattern names another template
 * (so a use of the alias acts as a class-template placeholder for CTAD). */
static bool CXXTemplateOriginIsAliasTemplatePlaceholderOrigin(Symbol* origin) {
  return origin != NULL && StorageIs(origin->storage, STO(typedef)) &&
         origin->type != NULL && origin->type->template_origin != NULL;
}

// Bare `Alias x = ...` copies the alias pattern onto the type.  An explicit
// `Alias<UserArgs> x` carries those arguments and is a specialization, not
// CTAD, even when the arguments are still dependent.
static bool TypeCarriesAliasTemplatePatternArguments(TypeRecord* type) {
  Symbol* origin = type != NULL ? type->template_origin : NULL;
  return CXXTemplateOriginIsAliasTemplatePlaceholderOrigin(origin) &&
         TemplateArgumentVectorEqual(type->template_arguments,
                                     origin->type->template_arguments);
}

/* True if `type` is a class-template name used without arguments (a CTAD
 * placeholder), i.e. `Foo x = ...;` where Foo is a class template, including
 * the alias-template form. */
bool TypeIsClassTemplatePlaceholder(TypeRecord* type) {
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (TypeIsStructOrUnion(t) && t->template_origin != NULL &&
        t->info.struct_info != NULL && t->info.struct_info->is_template &&
        (t->template_arguments == NULL ||
         TypeCarriesAliasTemplatePatternArguments(t))) {
      return true;
    }
  }
  return false;
}

/* Return the class template a CTAD placeholder type refers to (resolving alias
 * templates to the underlying class template), or NULL. */
Symbol* TypeClassTemplatePlaceholderOrigin(TypeRecord* type) {
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (TypeIsStructOrUnion(t) && t->template_origin != NULL &&
        t->info.struct_info != NULL && t->info.struct_info->is_template &&
        (t->template_arguments == NULL ||
         TypeCarriesAliasTemplatePatternArguments(t))) {
      if (CXXTemplateOriginIsAliasTemplatePlaceholderOrigin(t->template_origin)) {
        return t->template_origin->type->template_origin;
      }
      return t->template_origin;
    }
  }
  return NULL;
}

/* Return the type-chain node that is the CTAD placeholder within `type`. */
static TypeRecord* TypeClassTemplatePlaceholderBase(TypeRecord* type) {
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (TypeIsStructOrUnion(t) && t->template_origin != NULL &&
        t->info.struct_info != NULL && t->info.struct_info->is_template &&
        (t->template_arguments == NULL ||
         TypeCarriesAliasTemplatePatternArguments(t))) {
      return t;
    }
  }
  return NULL;
}

static void MaxTemplateParameterIndexInType(TypeRecord* type, int* max_index) {
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (t->template_parameter_index > *max_index) {
      *max_index = t->template_parameter_index;
    }
    if (t->declarator == kDeclArray &&
        t->info.array.template_parameter_index > *max_index) {
      *max_index = t->info.array.template_parameter_index;
    }
    if (t->template_arguments != NULL) {
      for (size_t i = 0; i < t->template_arguments->length; i++) {
        MaxTemplateParameterIndexInArgument(t->template_arguments->value.p[i],
                                            max_index);
      }
    }
    if (TypeIsFunction(t)) {
      for (size_t i = 0; i < t->info.function.prototype.length; i++) {
        Symbol* formal = t->info.function.prototype.value.p[i];
        if (formal != NULL) {
          MaxTemplateParameterIndexInType(formal->type, max_index);
        }
      }
    }
  }
}

/* Update `*max_index` with the largest template-parameter index referenced by a
 * value-dependent argument expression (e.g. the `R` in `Box<R::num>`).  Without
 * this, an alias template whose only parameter appears solely inside a
 * dependent non-type argument expression would look parameterless and fail to
 * instantiate. */
static void MaxTemplateParameterIndexInExprVisitor(ASTNode* node, void* data,
                                                   int child_id,
                                                   VisitorMode mode) {
  (void)child_id;
  (void)mode;
  if (node == NULL || node->op != AST_OP(identifier)) {
    return;
  }
  int* max_index = (int*)data;
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  if (id->symbol != NULL) {
    if (id->symbol->flags.is_template_parameter &&
        id->symbol->template_parameter_index > *max_index) {
      *max_index = id->symbol->template_parameter_index;
    }
    if (id->symbol->dependent_value_template_parameter_index > *max_index) {
      *max_index = id->symbol->dependent_value_template_parameter_index;
    }
    MaxTemplateParameterIndexInType(id->symbol->type, max_index);
  }
  if (id->template_arguments != NULL) {
    for (size_t i = 0; i < id->template_arguments->length; i++) {
      MaxTemplateParameterIndexInArgument(id->template_arguments->value.p[i],
                                          max_index);
    }
  }
}

void MaxTemplateParameterIndexInArgument(TemplateArgument* arg,
                                                int* max_index) {
  if (arg == NULL) {
    return;
  }
  if ((arg->kind == kTemplateParameterNonType ||
       arg->kind == kTemplateParameterTemplate) &&
      arg->template_parameter_index > *max_index) {
    *max_index = arg->template_parameter_index;
  }
  if (arg->dependent_expr != NULL) {
    ASTNodeVisit(arg->dependent_expr, MaxTemplateParameterIndexInExprVisitor, 0,
                 max_index);
  }
  if (arg->pack_arguments != NULL) {
    for (size_t i = 0; i < arg->pack_arguments->length; i++) {
      MaxTemplateParameterIndexInArgument(arg->pack_arguments->value.p[i],
                                          max_index);
    }
  }
  MaxTemplateParameterIndexInType(arg->type, max_index);
}

/* Match a partial-specialization argument `pattern` against an already-deduced
 * argument `deduced`, binding the specialization's own parameters into
 * `bindings`. Non-type params bind/compare values; type params either deduce
 * (if dependent) or require exact type equality. */
static bool TemplateArgumentPatternMatchesDeduced(Vector* bindings,
                                                  TemplateArgument* pattern,
                                                  TemplateArgument* deduced) {
  if (pattern == NULL || deduced == NULL || pattern->kind != deduced->kind) {
    return false;
  }
  if (pattern->kind == kTemplateParameterNonType) {
    if (pattern->template_parameter_index >= 0) {
      return SetDeducedTemplateNonTypeArgument(
          bindings, 0, pattern->template_parameter_index, deduced);
    }
    return TemplateArgumentEqual(pattern, deduced);
  }
  if (pattern->kind == kTemplateParameterTemplate) {
    if (pattern->template_parameter_index >= 0) {
      return SetDeducedTemplateTemplateArgument(
          bindings, 0, pattern->template_parameter_index, deduced);
    }
    return TemplateArgumentEqual(pattern, deduced);
  }
  if (pattern->type == NULL || deduced->type == NULL) {
    return false;
  }
  if (TemplateArgumentContainsTemplateParameter(pattern)) {
    return DeduceFunctionTemplateTypeArgument(bindings, 0, pattern->type,
                                              deduced->type);
  }
  return TypeEqual(pattern->type, deduced->type);
}

/* Bind class-template parameter `index` to a deduced type (or verify a prior
 * binding is consistent) when matching a partial specialization. */
static bool SetDeducedClassTemplateTypeArgument(Vector* bindings, int index,
                                                TypeRecord* actual) {
  if (index < 0 || bindings == NULL || (size_t)index >= bindings->length) {
    return false;
  }
  TemplateArgument* existing = bindings->value.p[index];
  if (existing == NULL) {
    bindings->value.p[index] = NewDeducedTypeTemplateArgument(actual);
    return true;
  }
  return existing->kind == kTemplateParameterType &&
         TypeEqual(existing->type, actual);
}

static TemplateArgument* NewDeducedTypePackTemplateArgument(void) {
  TemplateArgument* arg = TemplateArgumentAlloc();
  arg->kind = kTemplateParameterType;
  arg->is_pack_expansion = false;
  arg->type = NULL;
  arg->int_value = 0;
  arg->template_parameter_index = -1;
  arg->pack_arguments = NewVector();
  arg->dependent_expr = NULL;
  arg->location = SOURCE_LOCATION_MISSING;
  return arg;
}

static bool SetDeducedClassTemplateTypePackArgument(Vector* bindings, int index,
                                                    Vector* actuals,
                                                    size_t first_actual,
                                                    size_t last_actual) {
  if (index < 0 || bindings == NULL || (size_t)index >= bindings->length ||
      actuals == NULL || first_actual > actuals->length ||
      last_actual > actuals->length || first_actual > last_actual) {
    return false;
  }
  TemplateArgument* pack = NewDeducedTypePackTemplateArgument();
  for (size_t i = first_actual; i < last_actual; i++) {
    TemplateArgument* actual = actuals->value.p[i];
    if (actual == NULL) {
      TemplateArgumentDelete(pack);
      return false;
    }
    // The actual argument may itself be an already-expanded pack bundle (e.g.
    // `box<int, char, long>` stores its variadic arguments as a single pack
    // argument).  Splice that bundle's elements into the deduced pack rather
    // than nesting it, so `sizeof...` and later expansion see the individual
    // types.
    if (actual->pack_arguments != NULL) {
      for (size_t j = 0; j < actual->pack_arguments->length; j++) {
        VectorAppend(pack->pack_arguments,
                     TemplateArgumentCopy(actual->pack_arguments->value.p[j]));
      }
      continue;
    }
    if (actual->kind != kTemplateParameterType) {
      TemplateArgumentDelete(pack);
      return false;
    }
    VectorAppend(pack->pack_arguments, TemplateArgumentCopy(actual));
  }
    TemplateArgument* existing = bindings->value.p[index];
  if (existing == NULL) {
    bindings->value.p[index] = pack;
    return true;
  }
  bool equal = TemplateArgumentEqual(existing, pack);
  TemplateArgumentDelete(pack);
  return equal;
}

static bool SetDeducedClassTemplateValuePackArgument(
    Vector* bindings, int index, TemplateParameterKind kind, Vector* actuals,
    size_t first_actual, size_t last_actual) {
  if (index < 0 || bindings == NULL || (size_t)index >= bindings->length ||
      actuals == NULL || first_actual > last_actual ||
      last_actual > actuals->length) {
    return false;
  }
  TemplateArgument* pack = NewEmptyPackTemplateArgument(kind);
  for (size_t i = first_actual; i < last_actual; i++) {
    TemplateArgument* actual = actuals->value.p[i];
    if (actual == NULL || actual->kind != kind) {
      TemplateArgumentDelete(pack);
      return false;
    }
    VectorAppend(pack->pack_arguments, TemplateArgumentCopy(actual));
  }
  TemplateArgument* existing = bindings->value.p[index];
  if (existing == NULL) {
    bindings->value.p[index] = pack;
    return true;
  }
  bool equal = TemplateArgumentEqual(existing, pack);
  TemplateArgumentDelete(pack);
  return equal;
}

static bool SetDeducedClassTemplateFunctionTypePackArgument(
    Vector* bindings, int index, Vector* actual_formals, size_t first_actual,
    size_t last_actual) {
  if (index < 0 || bindings == NULL || (size_t)index >= bindings->length ||
      actual_formals == NULL || first_actual > last_actual ||
      last_actual > actual_formals->length) {
    return false;
  }
  TemplateArgument* pack = NewDeducedTypePackTemplateArgument();
  for (size_t i = first_actual; i < last_actual; i++) {
    Symbol* formal = actual_formals->value.p[i];
    if (formal == NULL || formal->type == NULL) {
      TemplateArgumentDelete(pack);
      return false;
    }
    VectorAppend(pack->pack_arguments,
                 NewDeducedTypeTemplateArgument(formal->type));
  }
  TemplateArgument* existing = bindings->value.p[index];
  if (existing == NULL) {
    bindings->value.p[index] = pack;
    return true;
  }
  bool equal = TemplateArgumentEqual(existing, pack);
  TemplateArgumentDelete(pack);
  return equal;
}

static bool TemplateArgumentIsTypeParameterPackPattern(TemplateArgument* arg,
                                                       int* index) {
  if (arg != NULL && arg->pack_arguments != NULL &&
      arg->pack_arguments->length == 1) {
    return TemplateArgumentIsTypeParameterPackPattern(
        arg->pack_arguments->value.p[0], index);
  }
  if (index != NULL) {
    *index = -1;
  }
  if (arg == NULL || arg->kind != kTemplateParameterType ||
      arg->type == NULL || arg->type->is_pack_index ||
      !TypeIsTemplateParameterPlaceholder(arg->type, index)) {
    return false;
  }
  return arg->is_pack_expansion;
}

static bool TemplateArgumentIsParameterPackPattern(TemplateArgument* arg,
                                                   int* index) {
  if (TemplateArgumentIsTypeParameterPackPattern(arg, index)) {
    return true;
  }
  if (index != NULL) {
    *index = -1;
  }
  if (arg == NULL || !arg->is_pack_expansion ||
      (arg->kind != kTemplateParameterNonType &&
       arg->kind != kTemplateParameterTemplate) ||
      arg->template_parameter_index < 0) {
    return false;
  }
  if (index != NULL) {
    *index = arg->template_parameter_index;
  }
  return true;
}

static bool ClassTemplateArgumentPackPatternMatches(Vector* bindings,
                                                    Vector* pattern_args,
                                                    Vector* actual_args) {
  if (pattern_args == NULL || actual_args == NULL) {
    return pattern_args == actual_args;
  }
  size_t actual_index = 0;
  for (size_t i = 0; i < pattern_args->length; i++) {
    TemplateArgument* pattern = pattern_args->value.p[i];
    int pack_index = -1;
    if (TemplateArgumentIsParameterPackPattern(pattern, &pack_index)) {
      if (pattern->kind == kTemplateParameterType) {
        return SetDeducedClassTemplateTypePackArgument(
            bindings, pack_index, actual_args, actual_index,
            actual_args->length);
      }
      return SetDeducedClassTemplateValuePackArgument(
          bindings, pack_index, pattern->kind, actual_args, actual_index,
          actual_args->length);
    }
    if (actual_index >= actual_args->length ||
        !ClassTemplateArgumentPatternMatches(
            bindings, pattern, actual_args->value.p[actual_index])) {
      return false;
    }
    actual_index++;
  }
  return actual_index == actual_args->length;
}

static bool PartialSpecializationBindingsContainNull(Vector* bindings) {
  if (bindings == NULL) {
    return false;
  }
  for (size_t i = 0; i < bindings->length; i++) {
    if (bindings->value.p[i] == NULL) {
      return true;
    }
  }
  return false;
}

/* Match one partial-specialization pattern argument against an actual class
 * template argument, binding the specialization's parameters into `bindings`. */
typedef struct {
  bool referenced[64];
} TemplateParameterReferenceSet;

static void NoteTemplateParameterReference(TemplateParameterReferenceSet* set,
                                          int index) {
  if (set != NULL && index >= 0 && index < 64) {
    set->referenced[index] = true;
  }
}

static void CollectTemplateParameterReferenceVisitor(ASTNode* node, void* data,
                                                    int child_id,
                                                    VisitorMode mode) {
  (void)child_id;
  (void)mode;
  if (node == NULL || node->op != AST_OP(identifier)) {
    return;
  }
  TemplateParameterReferenceSet* set = data;
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  if (id->symbol != NULL) {
    if (id->symbol->flags.is_template_parameter) {
      NoteTemplateParameterReference(set, id->symbol->template_parameter_index);
    }
    NoteTemplateParameterReference(
        set, id->symbol->dependent_value_template_parameter_index);
  }
  if (id->template_arguments != NULL) {
    for (size_t i = 0; i < id->template_arguments->length; i++) {
      TemplateArgument* arg = id->template_arguments->value.p[i];
      if (arg != NULL && arg->type != NULL) {
        int index = -1;
        if (TypeIsTemplateParameterPlaceholder(arg->type, &index)) {
          NoteTemplateParameterReference(set, index);
        }
      }
      if (arg != NULL && arg->dependent_expr != NULL) {
        ASTNodeVisit(arg->dependent_expr,
                     CollectTemplateParameterReferenceVisitor, 0, set);
      }
    }
  }
}

/* A decltype pattern can be checked once every parameter it names is bound.
 * A pack that the expression does not use may still be unbound; requiring
 * every binding to be filled first skips the substitution
 * (`FindFirstPrinter`'s decltype names Printer, not the trailing printer
 * pack). */
static bool BindingsReadyForPatternType(Vector* bindings, TypeRecord* type) {
  if (!PartialSpecializationBindingsContainNull(bindings)) {
    return true;
  }
  if (type == NULL || type->dependent_decltype_expr == NULL ||
      bindings == NULL) {
    return false;
  }
  TemplateParameterReferenceSet referenced;
  memset(&referenced, 0, sizeof(referenced));
  ASTNodeVisit(type->dependent_decltype_expr,
               CollectTemplateParameterReferenceVisitor, 0, &referenced);
  for (size_t i = 0; i < bindings->length && i < 64; i++) {
    if (referenced.referenced[i] && bindings->value.p[i] == NULL) {
      return false;
    }
  }
  return true;
}

static bool ClassTemplateArgumentPatternMatches(Vector* bindings,
                                                TemplateArgument* pattern,
                                                TemplateArgument* actual) {
  if (pattern == NULL || actual == NULL || pattern->kind != actual->kind) {
    return false;
  }
  if (pattern->pack_arguments != NULL && actual->pack_arguments == NULL &&
      pattern->pack_arguments->length == 1) {
    TemplateArgument* inner = pattern->pack_arguments->value.p[0];
    int pack_index = -1;
    if (TemplateArgumentIsParameterPackPattern(inner, &pack_index)) {
      Vector one;
      VectorInit(&one);
      VectorAppend(&one, actual);
      bool ok =
          inner->kind == kTemplateParameterType
              ? SetDeducedClassTemplateTypePackArgument(
                    bindings, pack_index, &one, 0, 1)
              : SetDeducedClassTemplateValuePackArgument(
                    bindings, pack_index, inner->kind, &one, 0, 1);
      VectorDestruct(&one);
      return ok;
    }
  }
  if (pattern->pack_arguments != NULL || actual->pack_arguments != NULL) {
    return pattern->pack_arguments != NULL && actual->pack_arguments != NULL &&
           ClassTemplateArgumentPackPatternMatches(bindings,
                                                   pattern->pack_arguments,
                                                   actual->pack_arguments);
  }
  int placeholder_index = -1;
  if (pattern->kind == kTemplateParameterType && pattern->type != NULL &&
      !TypeIsTemplateParameterPlaceholder(pattern->type, &placeholder_index) &&
      BindingsReadyForPatternType(bindings, pattern->type)) {
    TypeParser parser;
    TypeParserInit(&parser, compiler->syntax.lex, &compiler->syntax,
                   STO(implicit), compiler->syntax.context);
    bool saved_trap = DiagnosticErrorTrapBegin();
    DiagnosticSuppressBegin();
    TypeRecord* substituted =
        SubstituteTemplateParameters(&parser, pattern->type, bindings);
    bool substitution_failed =
        parser.template_substitution_failed || DiagnosticErrorTrapped();
    DiagnosticSuppressEnd();
    DiagnosticErrorTrapEnd(saved_trap);
    TypeParserDestruct(&parser);
    if (substitution_failed) {
      TypeRecordDelete(substituted);
      return false;
    }
    bool ok = ClassTemplateTypePatternMatches(bindings, substituted,
                                              actual->type);
    TypeRecordDelete(substituted);
    return ok;
  }
  if (pattern->kind == kTemplateParameterNonType) {
    if (pattern->template_parameter_index >= 0) {
      return SetDeducedTemplateNonTypeArgument(
          bindings, 0, pattern->template_parameter_index, actual);
    }
    // Both arguments have already been accepted for the primary template's
    // non-type parameter, so compare integral values after that parameter's
    // implicit conversion. The expression spelling the partial pattern may
    // retain its original literal type (for example, int for `0`) while the
    // actual argument has the parameter type (for example, size_t).
    if (pattern->dependent_expr == NULL && actual->dependent_expr == NULL &&
        pattern->type != NULL && actual->type != NULL &&
        TypeIsIntegral(pattern->type) && TypeIsIntegral(actual->type)) {
      return pattern->int_value == actual->int_value;
    }
    return TemplateArgumentEqual(pattern, actual);
  }
  if (pattern->kind == kTemplateParameterTemplate) {
    if (pattern->template_parameter_index >= 0) {
      return SetDeducedTemplateTemplateArgument(
          bindings, 0, pattern->template_parameter_index, actual);
    }
    return TemplateArgumentEqual(pattern, actual);
  }
  return ClassTemplateTypePatternMatches(bindings, pattern->type, actual->type);
}

static bool MatchPatternArgumentListTwoPass(Vector* bindings, Vector* patterns,
                                            Vector* actuals, size_t pattern_begin,
                                            size_t actual_begin, size_t count);

static bool ClassTemplateArgumentVectorPatternMatchesExpanded(
    Vector* bindings, Vector* pattern_args, Vector* actual_args) {
  // A template-parameter pack in the pattern (e.g. `box<T...>`) absorbs a
  // variable number of actual arguments, so the pattern and actual argument
  // counts need not match exactly. Locate a pack pattern (at most one) and let
  // it consume the middle, while leading/trailing fixed patterns match 1:1.
  int pack_pattern_pos = -1;
  for (size_t i = 0; i < pattern_args->length; i++) {
    int idx = -1;
    if (TemplateArgumentIsParameterPackPattern(pattern_args->value.p[i],
                                               &idx)) {
      pack_pattern_pos = (int)i;
      break;
    }
  }
  if (pack_pattern_pos < 0) {
    if (pattern_args->length != actual_args->length) {
      return false;
    }
    return MatchPatternArgumentListTwoPass(bindings, pattern_args, actual_args,
                                           0, 0, pattern_args->length);
  }
  size_t leading = (size_t)pack_pattern_pos;
  size_t trailing = pattern_args->length - leading - 1;
  if (actual_args->length < leading + trailing) {
    return false;
  }
  if (!MatchPatternArgumentListTwoPass(bindings, pattern_args, actual_args, 0, 0,
                                      leading)) {
    return false;
  }
  if (!MatchPatternArgumentListTwoPass(
          bindings, pattern_args, actual_args, leading + 1,
          actual_args->length - trailing, trailing)) {
    return false;
  }
  int pack_index = -1;
  TemplateArgument* pack_pattern =
      pattern_args->value.p[pack_pattern_pos];
  TemplateArgumentIsParameterPackPattern(pack_pattern, &pack_index);
  if (pack_pattern->kind == kTemplateParameterType) {
    return SetDeducedClassTemplateTypePackArgument(
        bindings, pack_index, actual_args, leading,
        actual_args->length - trailing);
  }
  return SetDeducedClassTemplateValuePackArgument(
      bindings, pack_index, pack_pattern->kind, actual_args, leading,
      actual_args->length - trailing);
}

/* Match a partial-specialization argument-pattern vector against the actual
 * argument vector, accumulating parameter bindings.  Concrete variadic
 * specializations store their expanded arguments in one pack bundle; expose
 * those elements as the logical argument sequence while matching patterns such
 * as `wrapper<Head, Tail...>`. */
static bool ClassTemplateArgumentVectorPatternMatches(Vector* bindings,
                                                      Vector* pattern_args,
                                                      Vector* actual_args) {
  if (pattern_args == NULL || actual_args == NULL) {
    return pattern_args == actual_args;
  }
  // Parser-produced pack expansions are represented as a bundle in both the
  // pattern and the specialization. Preserve those corresponding bundles so
  // the nested pack matcher can bind type, non-type, or template packs. The
  // flattened path below is for patterns whose expansion remains a direct
  // argument entry.
  bool corresponding_storage =
      pattern_args->length == actual_args->length;
  for (size_t i = 0; corresponding_storage && i < pattern_args->length; i++) {
    TemplateArgument* pattern = pattern_args->value.p[i];
    TemplateArgument* actual = actual_args->value.p[i];
    corresponding_storage =
        (pattern->pack_arguments != NULL) ==
        (actual->pack_arguments != NULL);
  }
  if (corresponding_storage) {
    return MatchPatternArgumentListTwoPass(bindings, pattern_args, actual_args,
                                           0, 0, pattern_args->length);
  }
  Vector expanded_actuals;
  VectorInit(&expanded_actuals);
  for (size_t i = 0; i < actual_args->length; i++) {
    TemplateArgument* actual = actual_args->value.p[i];
    if (actual != NULL && actual->pack_arguments != NULL) {
      for (size_t j = 0; j < actual->pack_arguments->length; j++) {
        VectorAppend(&expanded_actuals, actual->pack_arguments->value.p[j]);
      }
    } else {
      VectorAppend(&expanded_actuals, actual);
    }
  }
  bool result = ClassTemplateArgumentVectorPatternMatchesExpanded(
      bindings, pattern_args, &expanded_actuals);
  VectorDestruct(&expanded_actuals);
  return result;
}

/* Match a partial-specialization type pattern (e.g. `T*`, `vector<T>`) against
 * a concrete actual type, binding `T` etc. into `bindings`. A bare parameter
 * placeholder binds the actual; otherwise structure must match exactly while
 * recursing through pointers/references/arrays/template arguments. */
static bool ClassTemplateTypePatternMatches(Vector* bindings,
                                            TypeRecord* pattern,
                                            TypeRecord* actual) {
  if (pattern == NULL || actual == NULL) {
    return pattern == actual;
  }
  if (pattern->is_pack_index) {
    // Pack-indexing specifiers are non-deduced contexts; another part of the
    // partial-specialization pattern must bind the indexed pack.
    return true;
  }
  int index = -1;
  if (TypeIsTemplateParameterPlaceholder(pattern, &index) &&
      !pattern->is_pack_index) {
    if ((pattern->qualifiers & ~actual->qualifiers) != 0) {
      return false;
    }
    TypeRecord* deduced = TypeRecordCopy(actual);
    deduced->qualifiers &= ~pattern->qualifiers;
    bool ok = SetDeducedClassTemplateTypeArgument(bindings, index, deduced);
    TypeRecordDelete(deduced);
    return ok;
  }
  if (pattern->template_origin != NULL &&
      CXXTemplateOriginIsAliasTemplatePlaceholderOrigin(
          pattern->template_origin) &&
      pattern->template_arguments != NULL) {
    Vector* alias_args = CompleteAliasTemplateArguments(
        pattern->template_origin, pattern->template_arguments);
    if (alias_args == NULL) {
      return false;
    }
    TypeParser parser;
    TypeParserInit(&parser, compiler->syntax.lex, &compiler->syntax,
                   STO(implicit), compiler->syntax.context);
    TypeRecord* expanded = SubstituteTemplateParameters(
        &parser, pattern->template_origin->type, alias_args);
    bool substitution_failed = parser.template_substitution_failed;
    TypeParserDestruct(&parser);
    VectorDeleteWithContents(alias_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    bool ok =
        !substitution_failed &&
        ClassTemplateTypePatternMatches(bindings, expanded, actual);
    TypeRecordDelete(expanded);
    return ok;
  }
  if (pattern->template_origin != NULL &&
      pattern->template_origin->flags.is_template_template_parameter) {
    Symbol* actual_origin = ClassTemplateOriginOf(actual);
    Vector* pattern_args = pattern->template_arguments;
    Vector* actual_args = TypeSpecializationTemplateArguments(actual);
    if (actual_origin == NULL || pattern_args == NULL || actual_args == NULL ||
        !TemplateTemplateParameterListsCompatible(
            pattern->template_origin->template_template_parameters,
            TemplateParameterListForSymbol(actual_origin))) {
      return false;
    }
    TemplateArgument* template_argument =
        NewTemplateTemplateArgument(actual_origin, -1);
    bool ok = SetDeducedTemplateTemplateArgument(
        bindings, 0,
        pattern->template_parameter_index >= 0
            ? pattern->template_parameter_index
            : pattern->template_origin->template_parameter_index,
        template_argument);
    TemplateArgumentDelete(template_argument);
    return ok && ClassTemplateArgumentVectorPatternMatches(
                     bindings, pattern_args, actual_args);
  }
  if (pattern->declarator != actual->declarator ||
      pattern->qualifiers != actual->qualifiers) {
    return false;
  }
  switch (pattern->declarator) {
    case kDeclPointer:
    case kDeclReference:
    case kDeclRValueReference:
      return ClassTemplateTypePatternMatches(bindings, pattern->next,
                                             actual->next);
    case kDeclMemberPointer:
      if (pattern->template_parameter_index >= 0) {
        Struct* actual_class = TypeMemberPointerClass(actual);
        if (actual_class == NULL || actual_class->tag_symbol == NULL ||
            actual_class->tag_symbol->type == NULL ||
            !SetDeducedClassTemplateTypeArgument(
                bindings, pattern->template_parameter_index,
                actual_class->tag_symbol->type)) {
          return false;
        }
      } else if (TypeMemberPointerClass(pattern) !=
                 TypeMemberPointerClass(actual)) {
        return false;
      }
      return ClassTemplateTypePatternMatches(
          bindings, TypeMemberPointerPointeeType(pattern),
          TypeMemberPointerPointeeType(actual));
    case kDeclArray:
      if (pattern->info.array.template_parameter_index >= 0) {
        if (actual->info.array.is_flexible ||
            actual->info.array.is_vla ||
            actual->info.array.is_dependent_bound ||
            !SetDeducedFunctionTemplateNonTypeArgument(
                bindings, 0, pattern->info.array.template_parameter_index,
                actual->info.array.size.fixed)) {
          return false;
        }
      } else if (pattern->info.array.is_flexible !=
                     actual->info.array.is_flexible ||
                 pattern->info.array.size.fixed !=
                 actual->info.array.size.fixed) {
        return false;
      }
      return ClassTemplateTypePatternMatches(bindings, pattern->next,
                                             actual->next);
    case kDeclVector:
      return pattern->info.array.size.fixed ==
                 actual->info.array.size.fixed &&
             ClassTemplateTypePatternMatches(bindings, pattern->next,
                                             actual->next);
    case kDeclFunction:
      if (!ClassTemplateTypePatternMatches(bindings, pattern->next,
                                           actual->next) ||
          pattern->info.function.is_const_member !=
              actual->info.function.is_const_member ||
          pattern->info.function.is_volatile_member !=
              actual->info.function.is_volatile_member ||
          pattern->info.function.ref_qualifier !=
              actual->info.function.ref_qualifier ||
          pattern->info.function.is_noexcept !=
              actual->info.function.is_noexcept) {
        return false;
      }
      Vector* pattern_formals = &pattern->info.function.prototype;
      Vector* actual_formals = &actual->info.function.prototype;
      int pack_position = -1;
      int pack_index = -1;
      for (size_t i = 0; i < pattern_formals->length; i++) {
        Symbol* pattern_formal = pattern->info.function.prototype.value.p[i];
        int candidate_index = -1;
        if (pattern_formal != NULL &&
            pattern_formal->flags.is_parameter_pack &&
            TypeIsTemplateParameterPlaceholder(pattern_formal->type,
                                               &candidate_index)) {
          if (pack_position >= 0) {
            return false;
          }
          pack_position = (int)i;
          pack_index = candidate_index;
        }
      }
      if (pack_position < 0) {
        if (pattern_formals->length != actual_formals->length) {
          return false;
        }
        for (size_t i = 0; i < pattern_formals->length; i++) {
          Symbol* pattern_formal = pattern_formals->value.p[i];
          Symbol* actual_formal = actual_formals->value.p[i];
          if (pattern_formal == NULL || actual_formal == NULL ||
              !ClassTemplateTypePatternMatches(bindings, pattern_formal->type,
                                               actual_formal->type)) {
            return false;
          }
        }
        return true;
      }
      size_t leading = (size_t)pack_position;
      size_t trailing = pattern_formals->length - leading - 1;
      if (actual_formals->length < leading + trailing) {
        return false;
      }
      for (size_t i = 0; i < leading; i++) {
        Symbol* pattern_formal = pattern_formals->value.p[i];
        Symbol* actual_formal = actual->info.function.prototype.value.p[i];
        if (pattern_formal == NULL || actual_formal == NULL ||
            !ClassTemplateTypePatternMatches(bindings, pattern_formal->type,
                                             actual_formal->type)) {
          return false;
        }
      }
      for (size_t i = 0; i < trailing; i++) {
        Symbol* pattern_formal =
            pattern_formals->value.p[leading + 1 + i];
        Symbol* actual_formal =
            actual_formals->value.p[actual_formals->length - trailing + i];
        if (pattern_formal == NULL || actual_formal == NULL ||
            !ClassTemplateTypePatternMatches(bindings, pattern_formal->type,
                                             actual_formal->type)) {
          return false;
        }
      }
      return SetDeducedClassTemplateFunctionTypePackArgument(
          bindings, pack_index, actual_formals, leading,
          actual_formals->length - trailing);
    case kDeclPrimitive:
      if (pattern->type != actual->type) {
        return false;
      }
      if (TypeIsStructOrUnion(pattern)) {
        if (pattern->info.struct_info == actual->info.struct_info) {
          // Deferred template-ids copy the primary's struct_info and hang the
          // concrete arguments on the type record.  Those still have to bind
          // against a pattern such as `optional<T>` rather than matching by
          // identity and leaving T unbound.
          Vector* pattern_args = TypeSpecializationTemplateArguments(pattern);
          Vector* actual_args = TypeSpecializationTemplateArguments(actual);
          if (pattern_args != NULL || actual_args != NULL) {
            return ClassTemplateArgumentVectorPatternMatches(
                bindings, pattern_args, actual_args);
          }
          return true;
        }
        bool origin_matches = ClassTemplateOriginMatches(
            ClassTemplateOriginOf(pattern), ClassTemplateOriginOf(actual));
        if (!origin_matches) {
          return false;
        }
        bool args_match = ClassTemplateArgumentVectorPatternMatches(
            bindings, TypeSpecializationTemplateArguments(pattern),
            TypeSpecializationTemplateArguments(actual));
        if (!args_match) {
          return false;
        }
        return ClassTemplateOriginOf(pattern) != NULL;
      }
      if (TypeIsEnum(pattern)) {
        return pattern->info.enum_info == actual->info.enum_info;
      }
      return true;
  }
  return false;
}

/* Public: for partial CTAD (e.g. `Foo<int> x = ...`), check that a deduction
 * candidate's class template arguments are consistent with the partially
 * specified placeholder arguments by matching them as patterns. */
bool TypeClassTemplatePlaceholderAcceptsDeduced(TypeRecord* placeholder,
                                                TypeRecord* deduced) {
  TypeRecord* base = TypeClassTemplatePlaceholderBase(placeholder);
  if (base == NULL || base->template_arguments == NULL) {
    return true;
  }
  if (deduced == NULL || !TypeIsStructOrUnion(deduced)) {
    return false;
  }
  Vector* deduced_args = deduced->template_arguments;
  if (deduced_args == NULL && deduced->info.struct_info != NULL &&
      deduced->info.struct_info->tag_symbol != NULL &&
      deduced->info.struct_info->tag_symbol->type != NULL) {
    TypeRecord* tag_type = deduced->info.struct_info->tag_symbol->type;
    deduced_args = tag_type->template_arguments;
  }
  if (deduced_args == NULL ||
      deduced_args->length != base->template_arguments->length) {
    return false;
  }
  int max_index = -1;
  for (size_t i = 0; i < base->template_arguments->length; i++) {
    MaxTemplateParameterIndexInArgument(base->template_arguments->value.p[i],
                                        &max_index);
  }
  Vector bindings;
  VectorInit(&bindings);
  for (int i = 0; i <= max_index; i++) {
    VectorAppend(&bindings, NULL);
  }
  bool ok = true;
  for (size_t i = 0; ok && i < base->template_arguments->length; i++) {
    ok = TemplateArgumentPatternMatchesDeduced(
        &bindings,
        base->template_arguments->value.p[i],
        deduced_args->value.p[i]);
  }
  VectorDestructWithContents(&bindings,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  return ok;
}

static TemplateArgument* NewTemplateTemplateParameterCTADPatternArgument(
    TemplateParameter* param) {
  if (param == NULL) {
    return NULL;
  }
  TemplateArgument* arg = TemplateArgumentAlloc();
  arg->kind = param->kind;
  arg->is_pack_expansion = param->is_parameter_pack;
  arg->references_parameter_pack = param->is_parameter_pack;
  arg->template_parameter_index = -1;
  arg->location = SOURCE_LOCATION_MISSING;
  if (param->kind == kTemplateParameterType) {
    arg->type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
    arg->type->template_parameter_index = param->index;
    if (param->name.length != 0) {
      arg->type->template_parameter_name = NewString(param->name.value);
    }
  } else {
    arg->template_parameter_index = param->index;
  }
  return arg;
}

static void AppendFlattenedTemplateArguments(Vector* out, Vector* args) {
  for (size_t i = 0; args != NULL && i < args->length; i++) {
    TemplateArgument* arg = args->value.p[i];
    if (arg != NULL && arg->pack_arguments != NULL) {
      AppendFlattenedTemplateArguments(out, arg->pack_arguments);
    } else {
      VectorAppend(out, arg);
    }
  }
}

bool TypeTemplateTemplateParameterAcceptsDeduced(Symbol* parameter,
                                                 TypeRecord* deduced) {
  if (parameter == NULL ||
      !parameter->flags.is_template_template_parameter ||
      parameter->template_template_parameters == NULL || deduced == NULL ||
      !TypeIsStructOrUnion(deduced)) {
    return false;
  }
  Vector* deduced_args = TypeSpecializationTemplateArguments(deduced);
  if (deduced_args == NULL) {
    return false;
  }
  Vector flat_deduced_args;
  VectorInit(&flat_deduced_args);
  AppendFlattenedTemplateArguments(&flat_deduced_args, deduced_args);

  Vector patterns;
  VectorInit(&patterns);
  int max_index = -1;
  for (size_t i = 0; i < parameter->template_template_parameters->length; i++) {
    TemplateParameter* param =
        parameter->template_template_parameters->value.p[i];
    TemplateArgument* pattern =
        NewTemplateTemplateParameterCTADPatternArgument(param);
    if (pattern != NULL) {
      VectorAppend(&patterns, pattern);
      MaxTemplateParameterIndexInArgument(pattern, &max_index);
    }
  }
  Vector bindings;
  VectorInit(&bindings);
  for (int i = 0; i <= max_index; i++) {
    VectorAppend(&bindings, NULL);
  }
  bool accepted = ClassTemplateArgumentVectorPatternMatches(
      &bindings, &patterns, &flat_deduced_args);
  VectorDestruct(&flat_deduced_args);
  VectorDestructWithContents(&bindings,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  VectorDestructWithContents(&patterns,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  return accepted;
}

static TemplateArgument* TemplateTemplateParameterCTADDefaultArgument(
    TypeParser* parser, TemplateParameter* param, Vector* preceding) {
  if (param == NULL) {
    return NULL;
  }
  if (param->kind == kTemplateParameterType) {
    if (param->default_type == NULL) {
      return NULL;
    }
    TypeRecord* type =
        SubstituteTemplateParameters(parser, param->default_type, preceding);
    TemplateArgument* arg = NewTypeTemplateArgument(type);
    TypeRecordDelete(type);
    return arg;
  }
  if (param->default_argument != NULL) {
    return NewSubstitutedTemplateArgument(parser, param->default_argument,
                                          preceding);
  }
  if (param->kind == kTemplateParameterNonType && param->has_default_int) {
    return NewIntegralTemplateArgument(param->default_int_value);
  }
  return NULL;
}

TypeRecord* TypeTemplateTemplateParameterApplyDefaults(
    TypeParser* parser, Symbol* parameter, Symbol* argument_template,
    TypeRecord* deduced) {
  if (parser == NULL || parameter == NULL || argument_template == NULL ||
      deduced == NULL ||
      TypeTemplateTemplateParameterAcceptsDeduced(parameter, deduced)) {
    return deduced != NULL ? TypeRecordCopy(deduced) : NULL;
  }
  Vector* parameters = parameter->template_template_parameters;
  Vector* deduced_args = TypeSpecializationTemplateArguments(deduced);
  Vector flat_deduced_args;
  VectorInit(&flat_deduced_args);
  AppendFlattenedTemplateArguments(&flat_deduced_args, deduced_args);
  if (parameters == NULL || deduced_args == NULL ||
      flat_deduced_args.length >= parameters->length) {
    VectorDestruct(&flat_deduced_args);
    return NULL;
  }

  Vector* completed = TemplateArgumentVectorCopy(&flat_deduced_args);
  size_t deduced_count = flat_deduced_args.length;
  VectorDestruct(&flat_deduced_args);
  for (size_t i = deduced_count; i < parameters->length; i++) {
    TemplateParameter* param = parameters->value.p[i];
    TemplateArgument* default_arg =
        TemplateTemplateParameterCTADDefaultArgument(parser, param, completed);
    if (default_arg == NULL) {
      VectorDeleteWithContents(
          completed, (VectorElementDestructor)TemplateArgumentDelete,
          /*free_element=*/false);
      return NULL;
    }
    VectorAppend(completed, default_arg);
  }

  TypeRecord* adjusted =
      InstantiateSimpleClassTemplate(parser, argument_template, completed);
  VectorDeleteWithContents(completed,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  if (adjusted != NULL &&
      !TypeTemplateTemplateParameterAcceptsDeduced(parameter, adjusted)) {
    TypeRecordDelete(adjusted);
    return NULL;
  }
  return adjusted;
}

/* The largest declared index among a template parameter vector. */
static int TemplateParameterVectorMaxIndex(Vector* params) {
  int max_index = -1;
  for (size_t i = 0; params != NULL && i < params->length; i++) {
    TemplateParameter* param = params->value.p[i];
    if (param != NULL && param->index > max_index) {
      max_index = param->index;
    }
  }
  return max_index;
}

/* Allocate an empty (all-NULL) bindings vector sized to hold one slot per
 * parameter of a partial specialization. */
static Vector* NewPartialSpecializationBindings(
    ClassTemplatePartialSpecialization* partial) {
  int max_index = TemplateParameterVectorMaxIndex(&partial->template_parameters);
  Vector* bindings = NewVector();
  for (int i = 0; i <= max_index; i++) {
    VectorAppend(bindings, NULL);
  }
  return bindings;
}

/* True if every (non-pack) parameter of a partial specialization received a
 * binding during pattern matching, i.e. the specialization fully applies. */
static bool PartialSpecializationBindingsComplete(
    ClassTemplatePartialSpecialization* partial, Vector* bindings) {
  for (size_t i = 0; i < partial->template_parameters.length; i++) {
    TemplateParameter* param = partial->template_parameters.value.p[i];
    if (param == NULL || param->is_parameter_pack) {
      continue;
    }
    if (param->index < 0 || (size_t)param->index >= bindings->length ||
        bindings->value.p[param->index] == NULL) {
      return false;
    }
  }
  return true;
}

/* Heuristic "specificity" score for a type pattern: a bare parameter scores 0,
 * more concrete structure (qualifiers, pointers, arrays, template arguments)
 * scores higher. Used to pick the most specialized partial specialization. */
static int TemplateTypePatternSpecificity(TypeRecord* type) {
  if (type == NULL) {
    return 0;
  }
  int placeholder_index = -1;
  if (TypeIsTemplateParameterPlaceholder(type, &placeholder_index) &&
      !type->is_pack_index) {
    return 0;
  }
  int score = 1;
  if (type->qualifiers != kQualPlain) {
    score++;
  }
  switch (type->declarator) {
    case kDeclPointer:
    case kDeclReference:
    case kDeclRValueReference:
    case kDeclMemberPointer:
      return score + 2 + TemplateTypePatternSpecificity(type->next);
    case kDeclArray:
      return score + 2 + TemplateTypePatternSpecificity(type->next);
    case kDeclVector:
      return score + 3 + TemplateTypePatternSpecificity(type->next);
    case kDeclFunction:
      if (type->info.function.is_const_member) {
        score++;
      }
      if (type->info.function.is_volatile_member) {
        score++;
      }
      if (type->info.function.ref_qualifier != kCXXRefQualifierNone) {
        score++;
      }
      if (type->info.function.is_noexcept) {
        score++;
      }
      score += TemplateTypePatternSpecificity(type->next);
      for (size_t i = 0; i < type->info.function.prototype.length; i++) {
        Symbol* formal = type->info.function.prototype.value.p[i];
        if (formal != NULL) {
          score += TemplateTypePatternSpecificity(formal->type);
        }
      }
      return score;
    case kDeclPrimitive:
      if (type->template_arguments != NULL) {
        for (size_t i = 0; i < type->template_arguments->length; i++) {
          TemplateArgument* arg = type->template_arguments->value.p[i];
          if (arg != NULL && arg->kind == kTemplateParameterType) {
            score += TemplateTypePatternSpecificity(arg->type);
          } else if (arg != NULL) {
            score += 2;
          }
        }
      }
      return score;
  }
  return score;
}

static int TemplateArgumentPatternSpecificity(TemplateArgument* arg) {
  if (arg == NULL) {
    return 0;
  }
  if (arg->pack_arguments != NULL) {
    return TemplateArgumentVectorPatternSpecificity(arg->pack_arguments);
  }
  if (arg->kind == kTemplateParameterType) {
    return TemplateTypePatternSpecificity(arg->type);
  }
  return arg->template_parameter_index >= 0 ? 0 : 2;
}

static void NoteSpecificityTypeParameter(int index, int* score,
                                         int* seen_type_parameters,
                                         size_t* seen_type_parameter_count) {
  for (size_t j = 0; j < *seen_type_parameter_count; j++) {
    if (seen_type_parameters[j] == index) {
      *score += 3;
      return;
    }
  }
  if (*seen_type_parameter_count < 64) {
    seen_type_parameters[(*seen_type_parameter_count)++] = index;
  }
}

static void NoteSpecificityArgumentParameters(TemplateArgument* arg, int* score,
                                              int* seen_type_parameters,
                                              size_t* seen_type_parameter_count) {
  if (arg == NULL) {
    return;
  }
  if (arg->pack_arguments != NULL) {
    for (size_t i = 0; i < arg->pack_arguments->length; i++) {
      NoteSpecificityArgumentParameters(arg->pack_arguments->value.p[i], score,
                                        seen_type_parameters,
                                        seen_type_parameter_count);
    }
    return;
  }
  if (arg->kind != kTemplateParameterType || arg->type == NULL ||
      !TypeIsTemplateParameterPlaceholder(arg->type, NULL)) {
    return;
  }
  NoteSpecificityTypeParameter(arg->type->template_parameter_index, score,
                               seen_type_parameters,
                               seen_type_parameter_count);
}

/* Total specificity score across an argument-pattern vector. */
static int TemplateArgumentVectorPatternSpecificity(Vector* args) {
  int score = 0;
  int seen_type_parameters[64];
  size_t seen_type_parameter_count = 0;
  for (size_t i = 0; args != NULL && i < args->length; i++) {
    TemplateArgument* arg = args->value.p[i];
    score += TemplateArgumentPatternSpecificity(arg);
    NoteSpecificityArgumentParameters(arg, &score, seen_type_parameters,
                                      &seen_type_parameter_count);
  }
  return score;
}

/* A bare parameter (including a pack written as its own argument) can be
 * bound before later arguments are checked.  A decltype or other computed
 * pattern cannot: `FindFirstPrinter<T, decltype(Printer::PrintValue(...)),
 * Printer, Printers...>` must bind Printer before the decltype is substituted
 * and compared with the actual argument. */
static bool PatternArgumentIsImmediateDeduction(TemplateArgument* pattern) {
  if (pattern == NULL) {
    return false;
  }
  if (pattern->pack_arguments != NULL) {
    for (size_t i = 0; i < pattern->pack_arguments->length; i++) {
      if (!PatternArgumentIsImmediateDeduction(
              pattern->pack_arguments->value.p[i])) {
        return false;
      }
    }
    return true;
  }
  int index = -1;
  if (pattern->kind == kTemplateParameterType && pattern->type != NULL &&
      TypeIsTemplateParameterPlaceholder(pattern->type, &index) &&
      !pattern->type->is_pack_index) {
    return true;
  }
  if (pattern->kind == kTemplateParameterNonType &&
      pattern->template_parameter_index >= 0 &&
      pattern->dependent_expr == NULL) {
    return true;
  }
  if (pattern->kind == kTemplateParameterTemplate &&
      pattern->template_parameter_index >= 0) {
    return true;
  }
  return false;
}

static bool MatchPatternArgumentListTwoPass(Vector* bindings, Vector* patterns,
                                            Vector* actuals, size_t pattern_begin,
                                            size_t actual_begin, size_t count) {
  for (int pass = 0; pass < 2; pass++) {
    for (size_t i = 0; i < count; i++) {
      TemplateArgument* pattern = patterns->value.p[pattern_begin + i];
      bool immediate = PatternArgumentIsImmediateDeduction(pattern);
      if ((pass == 0) != immediate) {
        continue;
      }
      if (!ClassTemplateArgumentPatternMatches(
              bindings, pattern, actuals->value.p[actual_begin + i])) {
        return false;
      }
    }
  }
  return true;
}

/* Try to match a single partial specialization against the actual class
 * template arguments. On success, returns its parameter bindings and a
 * specificity score (for choosing the most specialized one). */
static bool MatchClassTemplatePartialSpecialization(
    ClassTemplatePartialSpecialization* partial, Vector* actual_args,
    Vector** bindings_out, int* score_out) {
  if (partial == NULL || actual_args == NULL) {
    return false;
  }
  Vector* bindings = NewPartialSpecializationBindings(partial);
  bool ok = true;
  if (partial->pattern_arguments.length == actual_args->length) {
    ok = MatchPatternArgumentListTwoPass(bindings, &partial->pattern_arguments,
                                        actual_args, 0, 0,
                                        partial->pattern_arguments.length);
  } else {
    ok = ClassTemplateArgumentVectorPatternMatches(
        bindings, &partial->pattern_arguments, actual_args);
  }
  ok = ok && PartialSpecializationBindingsComplete(partial, bindings);
  if (!ok) {
    VectorDeleteWithContents(bindings,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return false;
  }
  *bindings_out = bindings;
  *score_out =
      TemplateArgumentVectorPatternSpecificity(&partial->pattern_arguments);
  return true;
}

/* Count the parameter-pack arguments in a partial specialization's pattern.
 * Used as a partial-ordering tiebreaker: a specialization whose pattern has a
 * trailing pack (e.g. `S<I, T, Rest...>`) is less specialized than one with a
 * fixed argument list (e.g. `S<I, T>`), so when both match an argument list the
 * one with fewer packs is preferred rather than reported as ambiguous. */
static int PartialSpecializationPackCount(
    ClassTemplatePartialSpecialization* partial) {
  int count = 0;
  for (size_t i = 0; i < partial->pattern_arguments.length; i++) {
    TemplateArgument* arg = partial->pattern_arguments.value.p[i];
    if (arg != NULL && arg->is_pack_expansion) {
      count++;
    }
  }
  // A trailing template parameter pack (e.g. `class... Rest`) may match zero
  // actual arguments, in which case it leaves no pack-expansion entry in the
  // pattern.  Count such parameter packs too so that `S<I, T, Rest...>` is
  // still ranked as less specialized than `S<I, T>`.
  for (size_t i = 0; i < partial->template_parameters.length; i++) {
    TemplateParameter* param = partial->template_parameters.value.p[i];
    if (param != NULL && param->is_parameter_pack) {
      count++;
    }
  }
  return count;
}

static bool PartialSpecializationConstraintsSatisfied(
    ClassTemplatePartialSpecialization* partial, Vector* bindings) {
  return ConceptsConstraintSatisfied(partial->associated_constraint, bindings);
}

static int ComparePartialSpecializationConstraints(
    ClassTemplatePartialSpecialization* left,
    ClassTemplatePartialSpecialization* right, Vector* left_bindings,
    Vector* right_bindings) {
  return ConceptsCompareAssociatedConstraints(
      left->associated_constraint, right->associated_constraint, left_bindings,
      right_bindings);
}

static void ReportClassTemplateConstraintFailure(TypeParser* parser,
                                                 Symbol* templ,
                                                 ConstraintExpr* constraint,
                                                 Vector* arguments) {
  if (!ConceptsHasAssociatedConstraint(constraint)) {
    return;
  }
  SyntaxError(parser->syntax, "constraints not satisfied for class template %s",
              templ != NULL ? templ->name.value : "<unknown>");
  ConceptsReportAssociatedConstraintFailure(
      constraint, arguments, templ != NULL ? templ->location : SOURCE_LOCATION_MISSING,
      NULL);
}

static TypeRecord* ClassTemplateConstraintFailureType(TypeParser* parser,
                                                    Symbol* templ,
                                                    ConstraintExpr* constraint,
                                                    Vector* arguments,
                                                    bool emit_constraint_error) {
  if (emit_constraint_error) {
    ReportClassTemplateConstraintFailure(parser, templ, constraint, arguments);
  }
  return NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
}

static Vector* CompleteVariableTemplateArguments(TypeParser* parser,
                                                 VariableTemplate* vt,
                                                 Vector* args,
                                                 bool emit_error) {
  if (vt == NULL) {
    return NULL;
  }
  return CompleteTemplateArguments(parser, &vt->parameters, args,
                                   "Variable template instantiation failed",
                                   emit_error, kTemplateArgumentsBorrow);
}

static void ReportVariableTemplateConstraintFailure(Syntax* syntax,
                                                    Symbol* var_template,
                                                    Vector* arguments) {
  if (var_template == NULL || var_template->variable_template == NULL ||
      !ConceptsHasAssociatedConstraint(
          var_template->variable_template->associated_constraint)) {
    return;
  }
  SyntaxError(syntax, "constraints not satisfied for variable template %s",
              var_template->name.value);
  ConceptsReportAssociatedConstraintFailure(
      var_template->variable_template->associated_constraint, arguments,
      var_template->location, NULL);
}

static void ReportAliasTemplateConstraintFailure(TypeParser* parser,
                                                 Symbol* alias,
                                                 Vector* arguments) {
  if (!ConceptsHasAssociatedConstraint(alias->associated_constraint)) {
    return;
  }
  SyntaxError(parser->syntax, "constraints not satisfied for alias template %s",
              alias->name.value);
  ConceptsReportAssociatedConstraintFailure(alias->associated_constraint,
                                            arguments, alias->location, NULL);
}

/* Choose the best-matching partial specialization from a list (shared by class
 * and variable templates): the one with the highest specificity score. Reports
 * an ambiguity error if two equally-specific specializations match, and returns
 * NULL when none match (the primary template is then used). */
static ClassTemplatePartialSpecialization* SelectPartialSpecializationFromList(
    TypeParser* parser, Vector* specializations, const char* diagnostic_kind,
    const char* diagnostic_name, Vector* actual_args, Vector** bindings_out) {
  if (specializations == NULL) {
    return NULL;
  }
  ClassTemplatePartialSpecialization* best = NULL;
  Vector* best_bindings = NULL;
  int best_score = -1;
  int best_pack_count = 0;
  bool ambiguous = false;
  for (size_t i = 0; i < specializations->length; i++) {
    ClassTemplatePartialSpecialization* partial =
        specializations->value.p[i];
    Vector* bindings = NULL;
    int score = 0;
    if (!MatchClassTemplatePartialSpecialization(partial, actual_args,
                                                &bindings, &score)) {
      continue;
    }
    if (!PartialSpecializationConstraintsSatisfied(partial, bindings)) {
      VectorDeleteWithContents(bindings,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      continue;
    }
    int pack_count = PartialSpecializationPackCount(partial);
    // Higher specificity wins; on a tie the specialization with fewer trailing
    // parameter packs is more specialized (partial ordering) and is preferred.
    bool better = best == NULL || score > best_score ||
                  (score == best_score && pack_count < best_pack_count);
    bool tied = best != NULL && score == best_score &&
                pack_count == best_pack_count;
    if (tied) {
      int constraint_cmp = ComparePartialSpecializationConstraints(
          partial, best, bindings, best_bindings);
      if (constraint_cmp > 0) {
        better = true;
        tied = false;
      } else if (constraint_cmp < 0) {
        tied = false;
        VectorDeleteWithContents(bindings,
                                 (VectorElementDestructor)TemplateArgumentDelete,
                                 /*free_element=*/false);
        continue;
      }
    }
    if (better) {
      if (best_bindings != NULL) {
        VectorDeleteWithContents(
            best_bindings, (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
      }
      best = partial;
      best_bindings = bindings;
      best_score = score;
      best_pack_count = pack_count;
      ambiguous = false;
    } else if (tied) {
      ambiguous = true;
      VectorDeleteWithContents(bindings,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    } else {
      VectorDeleteWithContents(bindings,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
  }
  if (ambiguous) {
    SyntaxError(parser->syntax,
                "Ambiguous %s partial specialization for %s",
                diagnostic_kind != NULL ? diagnostic_kind : "template",
                diagnostic_name != NULL ? diagnostic_name : "<unknown>");
    if (best_bindings != NULL) {
      VectorDeleteWithContents(best_bindings,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    return NULL;
  }
  if (best != NULL) {
    *bindings_out = best_bindings;
  }
  return best;
}

/* Choose the best-matching partial specialization of class template `primary`
 * for the actual arguments; returns NULL when none match. */
static ClassTemplatePartialSpecialization* SelectClassTemplatePartialSpecialization(
    TypeParser* parser, Symbol* primary, Vector* actual_args,
    Vector** bindings_out) {
  if (primary == NULL || primary->type == NULL ||
      !TypeIsStructOrUnion(primary->type) ||
      primary->type->info.struct_info == NULL) {
    return NULL;
  }
  return SelectPartialSpecializationFromList(
      parser, &primary->type->info.struct_info->partial_specializations,
      "class template", primary->name.value, actual_args, bindings_out);
}

/* Choose the best-matching partial specialization of variable template
 * `primary` for the actual arguments; returns NULL when none match. */
static ClassTemplatePartialSpecialization*
SelectVariableTemplatePartialSpecialization(TypeParser* parser, Symbol* primary,
                                            Vector* actual_args,
                                            Vector** bindings_out) {
  if (primary == NULL || primary->variable_template == NULL) {
    return NULL;
  }
  return SelectPartialSpecializationFromList(
      parser, &primary->variable_template->partial_specializations,
      "variable template", primary->name.value, actual_args, bindings_out);
}

static bool TypeIsArithmeticType(TypeRecord* type) {
  return TypeIsIntegral(type) || TypeIsFloatingPoint(type);
}

static bool TypeEqualIgnoringTopLevelQualifiers(TypeRecord* left,
                                                TypeRecord* right) {
  if (left == NULL || right == NULL) {
    return false;
  }
  return TypeEqualIgnoringTopLevelQualifierMask(
      left, right, left->qualifiers | right->qualifiers);
}

static bool TypeCanAddTopLevelQualifiers(TypeRecord* from, TypeRecord* to) {
  if (!TypeEqualIgnoringTopLevelQualifiers(from, to)) {
    return false;
  }
  return (from->qualifiers & ~to->qualifiers) == 0;
}

/* Rough conversion rank for matching a CTAD deduction-guide parameter `formal`
 * against an argument type `actual` (lower is a better match, -1 = no match).
 * Used to pick the best deduction guide overload. */
static int DeductionGuideConversionRank(TypeRecord* formal,
                                        TypeRecord* actual) {
  if (formal == NULL || actual == NULL) {
    return -1;
  }
  if (TypeIsReference(formal)) {
    TypeRecord* target = formal->next;
    if (TypeEqual(target, actual)) {
      return 0;
    }
    return TypeCanAddTopLevelQualifiers(actual, target) ? 1 : -1;
  }
  if (TypeEqual(formal, actual)) {
    return 0;
  }
  if (TypeEqualIgnoringTopLevelQualifiers(formal, actual)) {
    return 1;
  }
  if (formal->declarator == kDeclPointer &&
      actual->declarator == kDeclArray &&
      TypeCanAddTopLevelQualifiers(actual->next, formal->next)) {
    return 1;
  }
  if (formal->declarator == kDeclPointer &&
      actual->declarator == kDeclFunction &&
      TypeEqual(formal->next, actual)) {
    return 1;
  }
  if (TypeEqualIgnoringSign(formal, actual)) {
    return 1;
  }
  if (TypeIsArithmeticType(formal) && TypeIsArithmeticType(actual)) {
    return 2;
  }
  if (TypeAssignmentCompatible(actual, formal)) {
    return 3;
  }
  return -1;
}

/* True if a braced initializer `{...}` can deduce a guide's
 * `std::initializer_list<T>` parameter: every element must be convertible to T. */
static bool CXXInitializerListBracedInitIsViableForDeduction(ASTNode* actual,
                                                             TypeRecord* formal) {
  TypeRecord* target = TypeIsReference(formal) ? formal->next : formal;
  if (actual == NULL || actual->op != AST_OP(braced_init) ||
      !TypeIsCXXInitializerList(target)) {
    return false;
  }
  TypeRecord* element_type = TypeCXXInitializerListElement(target);
  if (element_type == NULL) {
    return false;
  }
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)actual;
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* element =
        CXXBracedInitializerElementExpression(braced->initializers->value.p[i]);
    if (element == NULL) {
      return false;
    }
    element = AnalyzeExpression(element);
    if (!TypeAssignmentCompatible(element->type, element_type)) {
      return false;
    }
  }
  return true;
}

/* True if a braced initializer can deduce a guide's array parameter `T[N]`:
 * within bounds, every (possibly nested) element convertible to the element. */
static bool CXXArrayBracedInitIsViableForDeduction(ASTNode* actual,
                                                   TypeRecord* formal) {
  TypeRecord* target = TypeIsReference(formal) ? formal->next : formal;
  if (actual == NULL || actual->op != AST_OP(braced_init) || target == NULL ||
      target->declarator != kDeclArray) {
    return false;
  }
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)actual;
  if (!target->info.array.is_flexible && !target->info.array.is_vla &&
      !target->info.array.is_dependent_bound &&
      target->info.array.template_parameter_index < 0 &&
      target->info.array.size.fixed < (int)braced->initializers->length) {
    return false;
  }
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* init = braced->initializers->value.p[i];
    if (init == NULL) {
      return false;
    }
    if (init->op == AST_OP(braced_init)) {
      if (!CXXArrayBracedInitIsViableForDeduction(init, target->next)) {
        return false;
      }
      continue;
    }
    ASTNode* element = CXXBracedInitializerElementExpression(init);
    if (element == NULL) {
      return false;
    }
    element = AnalyzeExpression(element);
    if (!TypeAssignmentCompatible(element->type, target->next)) {
      return false;
    }
  }
  return true;
}

/* Score a CTAD deduction guide against the constructor call's actual arguments:
 * substitute the guide's parameters with `template_args`, rank each
 * parameter/argument conversion, and sum the ranks (lower total = better).
 * Returns false if the guide is not viable for these arguments. */
static bool ScoreDeductionGuideCall(TypeParser* parser,
                                    TypeRecord* guide_type,
                                    Vector* template_args,
                                    Vector* actuals,
                                    int* score) {
  if (parser == NULL || guide_type == NULL || !TypeIsFunction(guide_type) ||
      actuals == NULL || score == NULL) {
    return false;
  }
  TypeRecord* concrete_guide =
      template_args != NULL
          ? InstantiateFunctionTemplateType(parser, guide_type, template_args)
          : guide_type;
  if (concrete_guide == NULL || !TypeIsFunction(concrete_guide) ||
      concrete_guide->info.function.prototype.length < actuals->length) {
    if (template_args != NULL) {
      TypeRecordDelete(concrete_guide);
    }
    return false;
  }
  for (size_t i = actuals->length;
       i < concrete_guide->info.function.prototype.length; i++) {
    Symbol* formal = concrete_guide->info.function.prototype.value.p[i];
    if (formal == NULL || formal->default_argument == NULL) {
      if (template_args != NULL) {
        TypeRecordDelete(concrete_guide);
      }
      return false;
    }
  }
  int total = 0;
  for (size_t i = 0; i < actuals->length; i++) {
    Symbol* formal = concrete_guide->info.function.prototype.value.p[i];
    ASTNode* actual = actuals->value.p[i];
    if (formal == NULL || formal->type == NULL || actual == NULL) {
      if (template_args != NULL) {
        TypeRecordDelete(concrete_guide);
      }
      return false;
    }
    TypeRecord* formal_type = formal->type;
    int rank = actual->op == AST_OP(braced_init)
        ? (CXXArrayBracedInitIsViableForDeduction(actual, formal_type) ||
                   CXXInitializerListBracedInitIsViableForDeduction(actual,
                                                                    formal_type)
               ? 0
               : -1)
        : DeductionGuideConversionRank(formal_type, actual->type);
    if (rank < 0) {
      if (template_args != NULL) {
        TypeRecordDelete(concrete_guide);
      }
      return false;
    }
    total += rank;
  }
  if (template_args != NULL) {
    TypeRecordDelete(concrete_guide);
  }
  *score = total;
  return true;
}

static bool CXXDeductionCandidateEqual(TypeRecord* left, TypeRecord* right) {
  if (TypeIsStructOrUnion(left) || TypeIsStructOrUnion(right)) {
    return TypeIsStructOrUnion(left) && TypeIsStructOrUnion(right) &&
           left->info.struct_info == right->info.struct_info;
  }
  return TypeEqual(left, right);
}

static Symbol* CXXInheritedConstructorBaseTemplate(
    CXXMemberUsingDeclaration* decl) {
  if (decl == NULL || decl->base_type == NULL ||
      decl->member_name.value == NULL) {
    return NULL;
  }
  Symbol* base_template = ClassTemplateOriginOf(decl->base_type);
  if (base_template == NULL || !base_template->flags.is_template ||
      strcmp(decl->member_name.value, base_template->name.value) != 0) {
    return NULL;
  }
  return base_template;
}

static TypeRecord* CXXMapInheritedDeductionToDerived(
    Syntax* syntax, Symbol* class_template, TypeRecord* base_pattern,
    TypeRecord* base_deduced) {
  if (syntax == NULL || class_template == NULL ||
      class_template->type == NULL || base_pattern == NULL ||
      base_deduced == NULL ||
      !TypeIsStructOrUnion(class_template->type) ||
      class_template->type->info.struct_info == NULL) {
    return NULL;
  }
  Struct* str = class_template->type->info.struct_info;
  Vector bindings;
  VectorInit(&bindings);
  for (size_t i = 0; i < str->template_parameters.length; i++) {
    VectorAppend(&bindings, NULL);
  }
  bool matched =
      ClassTemplateTypePatternMatches(&bindings, base_pattern, base_deduced);
  for (size_t i = 0; matched && i < bindings.length; i++) {
    matched = bindings.value.p[i] != NULL;
  }
  TypeRecord* result = NULL;
  if (matched &&
      ConceptsConstraintSatisfied(str->associated_constraint, &bindings)) {
    result =
        TypeInstantiateClassTemplateQuiet(syntax, class_template, &bindings);
  }
  VectorDestructWithContents(&bindings,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  return result;
}

static TypeRecord* TypeDeduceClassTemplateFromGuideFiltered(
    Syntax* syntax, Symbol* class_template, TypeRecord* placeholder,
    Vector* actuals, bool allow_explicit, bool* alias_rejected,
    int* selected_score, bool* selected_explicit) {
  if (syntax == NULL || class_template == NULL || class_template->type == NULL ||
      !class_template->flags.is_template || actuals == NULL ||
      !TypeIsStructOrUnion(class_template->type) ||
      class_template->type->info.struct_info == NULL) {
    return NULL;
  }
  Struct* str = class_template->type->info.struct_info;
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  TypeRecord* result = NULL;
  int best_score = -1;
  bool ambiguous = false;
  bool best_is_explicit = false;
  bool best_is_inherited = false;
  Symbol* best_guide = NULL;
  for (size_t i = 0; i < str->deduction_guides.length; i++) {
    Symbol* guide = str->deduction_guides.value.p[i];
    if (guide == NULL || guide->type == NULL || !TypeIsFunction(guide->type)) {
      continue;
    }
    TypeRecord* guide_return = NULL;
    int guide_score = 0;
    if (guide->flags.is_template) {
      Vector* deduced_args =
          TypeDeduceFunctionTemplateArgumentsFromCall(guide, actuals, 0);
      if (deduced_args == NULL) {
        continue;
      }
      Vector* args = CompleteFunctionTemplateArguments(
          &parser, guide->type, deduced_args, /*emit_error=*/false,
          kTemplateArgumentsConsume);
      VectorDeleteWithContents(
          deduced_args, (VectorElementDestructor)TemplateArgumentDelete,
          /*free_element=*/false);
      if (args == NULL ||
          TemplateArgumentVectorContainsTemplateParameterForInstantiation(
              args)) {
        if (args != NULL) {
          VectorDeleteWithContents(
              args, (VectorElementDestructor)TemplateArgumentDelete,
              /*free_element=*/false);
        }
        continue;
      }
      if (!ConceptsFunctionTemplateConstraintsSatisfied(guide, args)) {
        VectorDeleteWithContents(args,
                                 (VectorElementDestructor)TemplateArgumentDelete,
                                 /*free_element=*/false);
        continue;
      }
      if (!ScoreDeductionGuideCall(&parser, guide->type, args, actuals,
                                   &guide_score)) {
        VectorDeleteWithContents(args,
                                 (VectorElementDestructor)TemplateArgumentDelete,
                                 /*free_element=*/false);
        continue;
      }
      guide_return = SubstituteTemplateParameters(&parser, guide->type->next,
                                                  args);
      VectorDeleteWithContents(args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    } else {
      if (!ScoreDeductionGuideCall(&parser, guide->type, NULL, actuals,
                                   &guide_score)) {
        continue;
      }
      guide_return = TypeRecordCopy(guide->type->next);
    }
    if (guide_return == NULL) {
      continue;
    }
    if (guide->type->info.function.is_implicitly_declared) {
      guide_score += 1000;
    }
    if (guide->flags.is_template) {
      guide_score += 10;
    }
    TypeRecord* candidate = NULL;
    if (guide_return->template_origin == class_template &&
        guide_return->template_arguments != NULL) {
      Struct* class_struct = class_template->type->info.struct_info;
      if (class_struct != NULL &&
          ConceptsConstraintSatisfied(class_struct->associated_constraint,
                                    guide_return->template_arguments)) {
        candidate = TypeInstantiateClassTemplateQuiet(
            syntax, class_template, guide_return->template_arguments);
      }
    } else if (guide_return->template_origin == class_template ||
               (TypeIsStructOrUnion(guide_return) &&
                guide_return->info.struct_info != NULL &&
                !guide_return->info.struct_info->is_template)) {
      candidate = TypeRecordCopy(guide_return);
    }
    TypeRecordDelete(guide_return);
    if (candidate == NULL) {
      continue;
    }
    if (placeholder != NULL &&
        !TypeClassTemplatePlaceholderAcceptsDeduced(placeholder, candidate)) {
      if (alias_rejected != NULL) {
        *alias_rejected = true;
      }
      TypeRecordDelete(candidate);
      continue;
    }
    if (result == NULL || guide_score < best_score) {
      if (result != NULL) {
        TypeRecordDelete(result);
      }
      result = candidate;
      best_score = guide_score;
      best_is_explicit = guide->type->info.function.is_explicit;
      best_is_inherited = false;
      best_guide = guide;
      ambiguous = false;
    } else if (guide_score == best_score &&
               !CXXDeductionCandidateEqual(result, candidate)) {
      int specificity =
          best_guide != NULL
              ? CompareFunctionTemplateSpecificity(guide, best_guide)
              : 0;
      if (specificity > 0) {
        TypeRecordDelete(result);
        result = candidate;
        best_is_explicit = guide->type->info.function.is_explicit;
        best_is_inherited = false;
        best_guide = guide;
        ambiguous = false;
      } else {
        ambiguous = specificity == 0;
        TypeRecordDelete(candidate);
      }
    } else {
      TypeRecordDelete(candidate);
    }
  }
  if (CompilerCXXAtLeast(kLanguageStandardCXX23)) {
    for (size_t i = 0; i < str->member_using_declarations.length; i++) {
      CXXMemberUsingDeclaration* decl =
          str->member_using_declarations.value.p[i];
      Symbol* base_template = CXXInheritedConstructorBaseTemplate(decl);
      if (base_template == NULL) {
        continue;
      }
      int inherited_score = -1;
      bool inherited_explicit = false;
      DiagnosticSuppressBegin();
      TypeRecord* base_deduced = TypeDeduceClassTemplateFromGuideFiltered(
          syntax, base_template, NULL, actuals, allow_explicit, NULL,
          &inherited_score, &inherited_explicit);
      DiagnosticSuppressEnd();
      if (base_deduced == NULL) {
        continue;
      }
      TypeRecord* candidate = CXXMapInheritedDeductionToDerived(
          syntax, class_template, decl->base_type, base_deduced);
      TypeRecordDelete(base_deduced);
      if (candidate == NULL) {
        continue;
      }
      if (placeholder != NULL &&
          !TypeClassTemplatePlaceholderAcceptsDeduced(placeholder, candidate)) {
        if (alias_rejected != NULL) {
          *alias_rejected = true;
        }
        TypeRecordDelete(candidate);
        continue;
      }
      if (result == NULL || inherited_score < best_score) {
        if (result != NULL) {
          TypeRecordDelete(result);
        }
        result = candidate;
        best_score = inherited_score;
        best_is_explicit = inherited_explicit;
        best_is_inherited = true;
        best_guide = NULL;
        ambiguous = false;
      } else if (inherited_score == best_score && best_is_inherited &&
                 !CXXDeductionCandidateEqual(result, candidate)) {
        ambiguous = true;
        TypeRecordDelete(candidate);
      } else {
        // A non-inherited candidate is preferred when otherwise tied.
        TypeRecordDelete(candidate);
      }
    }
  }
  TypeParserDestruct(&parser);
  if (ambiguous) {
    SyntaxError(syntax, "Ambiguous class template argument deduction for %s",
                class_template->name.value);
    if (result != NULL) {
      TypeRecordDelete(result);
    }
    return NULL;
  }
  if (result != NULL && best_is_explicit && !allow_explicit) {
    SyntaxError(syntax,
                "Explicit deduction guide cannot be used for copy-initialization");
    TypeRecordDelete(result);
    return NULL;
  }
  if (result != NULL) {
    if (selected_score != NULL) {
      *selected_score = best_score;
    }
    if (selected_explicit != NULL) {
      *selected_explicit = best_is_explicit;
    }
  }
  return result;
}

TypeRecord* TypeDeduceClassTemplateFromGuide(Syntax* syntax,
                                             Symbol* class_template,
                                             Vector* actuals,
                                             bool allow_explicit) {
  return TypeDeduceClassTemplateFromGuideFiltered(
      syntax, class_template, NULL, actuals, allow_explicit, NULL, NULL, NULL);
}

TypeRecord* TypeDeduceClassTemplateFromPlaceholder(Syntax* syntax,
                                                   TypeRecord* placeholder,
                                                   Vector* actuals,
                                                   bool allow_explicit,
                                                   bool* alias_rejected) {
  if (alias_rejected != NULL) {
    *alias_rejected = false;
  }
  if (placeholder == NULL || !TypeIsClassTemplatePlaceholder(placeholder)) {
    return NULL;
  }
  Symbol* class_template = TypeClassTemplatePlaceholderOrigin(placeholder);
  TypeRecord* alias_base = TypeClassTemplatePlaceholderBase(placeholder);
  TypeRecord* filter = alias_base != NULL && alias_base->template_arguments != NULL
                           ? placeholder
                           : NULL;
  return TypeDeduceClassTemplateFromGuideFiltered(
      syntax, class_template, filter, actuals, allow_explicit, alias_rejected,
      NULL, NULL);
}

/* Set up the source->target struct substitution and clone/queue the body of an
 * instantiated member function.  Factored out so it can run in a second pass,
 * after every member function signature has been added to `owner`. */
StructMember* InstantiateTemplateMemberFunction(TypeParser* parser,
                                                       Struct* owner,
                                                       StructMember* member,
                                                       Vector* args,
                                                       Vector* pending) {
  Struct* substitution_source =
      member->symbol != NULL && member->symbol->type != NULL
          ? member->symbol->type->info.function.cxx_member_owner
          : NULL;
  TypeSubstitutionScope substitution = TypeParserPushTemplateSubstitution(
      parser, substitution_source, owner);
  TypeRecord* func = InstantiateMemberFunctionType(
      parser, owner, member->is_static, member->symbol->type, args,
      member->symbol->location);
  const char* symbol_name = member->symbol->name.value;
  String destructor_name;
  StringInit(&destructor_name, NULL);
  // A conversion operator's name is derived from its result type, so once that
  // type has been substituted the name must be recomputed: a conversion to a
  // dependent target (e.g. `operator View<C, int>`) is spelled "operator View"
  // in the primary template but must become "operator View<char,int>" in the
  // `Str<char>` instantiation, or overload resolution's conversion-operator
  // matching (which compares the member name against the name derived from its
  // result type) fails to recognise it as a conversion function at all.
  String conversion_name;
  StringInit(&conversion_name, NULL);
  bool source_is_conversion_operator = false;
  if (!func->info.function.is_constructor &&
      !func->info.function.is_destructor &&
      member->symbol->type->next != NULL) {
    String source_expected;
    ConversionOperatorName(member->symbol->type->next, &source_expected);
    source_is_conversion_operator =
        strcmp(source_expected.value, member->symbol->name.value) == 0;
    StringDestruct(&source_expected);
  }
  if (func->info.function.is_constructor && owner->tag_name != NULL) {
    symbol_name = owner->tag_name->value;
  } else if (func->info.function.is_destructor && owner->tag_name != NULL) {
    StringSet(&destructor_name, "~");
    StringAppendString(&destructor_name, owner->tag_name);
    symbol_name = destructor_name.value;
  } else if (source_is_conversion_operator && func->next != NULL) {
    // Name the instantiated conversion operator from its substituted result
    // type.  This can still be a deferred template-id when the result names a
    // class template completed later in the TU; overload resolution
    // (ClassHasConversionOperatorTo) re-materializes the result type when it
    // matches, so it does not rely on this name being fully specialized.
    ConversionOperatorName(func->next, &conversion_name);
    symbol_name = conversion_name.value;
  }
  Symbol* symbol = NewSymbol(symbol_name, func,
                             member->symbol->storage);
  StringDestruct(&destructor_name);
  StringDestruct(&conversion_name);
  symbol->location = member->symbol->location;
  symbol->flags.is_template = member->symbol->flags.is_template;
  symbol->type->info.function.template_parameter_count =
      member->symbol->type->info.function.template_parameter_count;
  func->info.function.symbol = symbol;
  SymbolSetCXXMangledAsmName(symbol);
  Symbol* template_definition = member->symbol->value.func_defn;
  if (template_definition == NULL &&
      member->symbol->type->info.function.body != NULL) {
    template_definition = member->symbol;
  }
  // Keep the source definition reachable while this class specialization is
  // still being assembled. A later data member can use an earlier constexpr
  // member in a dependent template argument (for example
  // `array<T, rank_dynamic()>`); constexpr evaluation must be able to
  // instantiate that body before the normal pending-body pass runs.
  symbol->value.func_defn = template_definition;
  TypeParserPopTemplateSubstitution(&substitution);
  PendingMemberBody* pmb = malloc(sizeof(PendingMemberBody));
  pmb->symbol = symbol;
  pmb->template_definition = template_definition;
  pmb->substitution_source = substitution_source;
  pmb->pattern = member->symbol;
  VectorAppend(pending, pmb);
  StructMember* instantiated = NewStructMember(symbol);
  instantiated->is_member_function = true;
  instantiated->is_static = member->is_static;
  instantiated->access = member->access;
  return instantiated;
}

static TemplateArgument* NewDefaultTypeTemplateArgument(TypeRecord* type) {
  TemplateArgument* arg = TemplateArgumentAlloc();
  arg->kind = kTemplateParameterType;
  arg->is_pack_expansion = false;
  arg->type = type;
  arg->int_value = 0;
  arg->template_parameter_index = -1;
  arg->pack_arguments = NULL;
  arg->dependent_expr = NULL;
  arg->location = SOURCE_LOCATION_MISSING;
  return arg;
}

static TemplateArgument* NewDefaultNonTypeTemplateArgument(
    long long int_value, int template_parameter_index) {
  TemplateArgument* arg = TemplateArgumentAlloc();
  arg->kind = kTemplateParameterNonType;
  arg->is_pack_expansion = false;
  arg->type = NULL;
  arg->int_value = int_value;
  arg->value_kind = template_parameter_index < 0
                        ? kTemplateValueIntegral : kTemplateValueNone;
  arg->template_parameter_index = template_parameter_index;
  arg->pack_arguments = NULL;
  arg->dependent_expr = NULL;
  arg->location = SOURCE_LOCATION_MISSING;
  return arg;
}

TemplateArgument* NewEmptyPackTemplateArgument(
    TemplateParameterKind kind) {
  TemplateArgument* arg = TemplateArgumentAlloc();
  arg->kind = kind;
  arg->is_pack_expansion = false;
  arg->type = NULL;
  arg->int_value = 0;
  arg->template_parameter_index = -1;
  arg->pack_arguments = NewVector();
  arg->dependent_expr = NULL;
  arg->location = SOURCE_LOCATION_MISSING;
  return arg;
}

static int FindTemplateParameterPackIndex(Vector* template_parameters) {
  for (size_t i = 0; template_parameters != NULL &&
                     i < template_parameters->length; i++) {
    TemplateParameter* param = template_parameters->value.p[i];
    if (param != NULL && param->is_parameter_pack) {
      return (int)i;
    }
  }
  return -1;
}

static bool TemplateNonTypeArgumentMatchesParameter(
    TemplateParameter* param, TemplateArgument* arg) {
  if (param == NULL || arg == NULL ||
      param->kind != kTemplateParameterNonType ||
      arg->kind != kTemplateParameterNonType) {
    return false;
  }
  // A nested template-id can be completed while its non-type argument still
  // depends on an enclosing class template, for example
  // `array<index_type, rank_dynamic()>` inside `extents<...>`.  Its concrete
  // value and type are checked after substitution; rejecting the unresolved
  // expression here prevents the enclosing specialization from ever forming.
  if (arg->dependent_expr != NULL) {
    return true;
  }
  if (param->type == NULL || (param->type->type & kTypeAuto) != 0 ||
      TypeContainsTemplateParameter(param->type)) {
    return TemplateArgumentConcreteValueKind(arg) != kTemplateValueNone ||
           arg->template_parameter_index >= 0 ||
           arg->dependent_expr != NULL;
  }
  TemplateValueKind value_kind = TemplateArgumentConcreteValueKind(arg);
  if (value_kind == kTemplateValueObject) {
    return TypeIsStructOrUnion(param->type) && arg->type != NULL &&
           TypeEqualIgnoringTopLevelQualifierMask(
               arg->type, param->type, kQualConst | kQualVolatile);
  }
  if (value_kind == kTemplateValueIntegral) {
    return param->type->declarator == kDeclPrimitive &&
           (param->type->type &
            (kTypeFloat | kTypeDouble | kTypeLongDouble |
             kTypeFloat32 | kTypeFloat64 |
             kTypeStruct | kTypeUnion | kTypeVoid)) == 0;
  }
  if (arg->type == NULL) {
    return false;
  }
  // Converted constant expressions ([temp.arg.nontype]): an integral argument
  // (including a still-dependent integral parameter such as `template<int I>`)
  // may initialize a different integral non-type parameter (`size_t I`).
  if (TypeIsIntegral(param->type) && TypeIsIntegral(arg->type)) {
    return true;
  }
  if (value_kind == kTemplateValueNull) {
    return TypeIsPointer(param->type) || TypeIsMemberPointer(param->type) ||
           TypeIsNullPointer(param->type);
  }
  if (value_kind == kTemplateValuePointer) {
    if (!TypeIsPointer(arg->type) || !TypeIsPointer(param->type)) {
      return false;
    }
    bool argument_is_function =
        arg->type->next != NULL && TypeIsFunction(arg->type->next);
    bool parameter_is_function =
        param->type->next != NULL && TypeIsFunction(param->type->next);
    if (argument_is_function != parameter_is_function) {
      return false;
    }
    if (argument_is_function) {
      return arg->type->qualifiers == param->type->qualifiers &&
             TypeEqualIgnoringFunctionNoexcept(arg->type->next,
                                               param->type->next) &&
             (!param->type->next->info.function.is_noexcept ||
              arg->type->next->info.function.is_noexcept);
    }
    return TypeAssignmentCompatible(arg->type, param->type);
  }
  if (value_kind == kTemplateValueMemberPointer) {
    return TypeEqual(arg->type, param->type);
  }
  return TypeAssignmentCompatible(arg->type, param->type);
}

static bool ConvertClassNonTypeTemplateArgument(TypeParser* parser,
                                                TemplateParameter* param,
                                                TemplateArgument* arg) {
  if (parser == NULL || parser->syntax == NULL || param == NULL || arg == NULL ||
      param->type == NULL || !TypeIsStructOrUnion(param->type) ||
      TemplateArgumentConcreteValueKind(arg) == kTemplateValueObject ||
      arg->dependent_expr != NULL || arg->template_parameter_index >= 0) {
    return true;
  }
  ASTNode* source =
      TemplateArgumentMaterializeExpression(arg, arg->location);
  if (source == NULL) {
    return false;
  }
  compiler->constant_evaluation_required_depth++;
  ASTNode* initializer = AnalyzeInitializer(param->type, source, true);
  compiler->constant_evaluation_required_depth--;
  if (initializer == NULL) {
    ASTNodeDelete(source);
    return false;
  }
  Symbol* storage =
      SyntaxNewTemporary(parser->syntax, TypeRecordCopy(param->type));
  ASTNode* expression = NewCompoundLiteralASTNode(
      NewIdentifierASTNode(storage, arg->location), arg->location, initializer);
  ASTNodeSetType(expression, TypeRecordCopy(param->type));
  bool converted = TemplateArgumentSetFromExpression(arg, expression);
  ASTNodeDelete(expression);
  return converted;
}

static Vector* TemplateParameterListForSymbol(Symbol* symbol) {
  if (symbol == NULL) {
    return NULL;
  }
  // A class template's parameter list is attached at the closing brace.  Until
  // then the tag is not `is_template`, but a template template argument
  // written in the body (`Apply<Box>` inside `Box`) still has to match the
  // parameter list that is in scope.
  if (!symbol->flags.is_template && symbol->type != NULL &&
      TypeIsStructOrUnion(symbol->type) &&
      symbol->type->info.struct_info != NULL) {
    Struct* str = symbol->type->info.struct_info;
    Syntax* syntax = &compiler->syntax;
    if (str->tag_symbol == symbol && str->template_parameters.length == 0 &&
        str->defining_template_scope_count > 0 &&
        syntax->current_template_parameters != NULL &&
        syntax->current_template_parameter_count ==
            str->defining_template_scope_count) {
      return syntax->current_template_parameters;
    }
    return NULL;
  }
  if (!symbol->flags.is_template) {
    return NULL;
  }
  if (symbol->flags.is_template_template_parameter) {
    return symbol->template_template_parameters;
  }
  if (symbol->alias_template != NULL) {
    return &symbol->alias_template->parameters;
  }
  if (symbol->variable_template != NULL) {
    return &symbol->variable_template->parameters;
  }
  if (symbol->flags.is_concept && symbol->concept_definition != NULL) {
    return symbol->concept_definition->template_parameters;
  }
  if (symbol->type != NULL && TypeIsStructOrUnion(symbol->type) &&
      symbol->type->info.struct_info != NULL) {
    return &symbol->type->info.struct_info->template_parameters;
  }
  return NULL;
}

static bool TemplateParameterHasDefault(TemplateParameter* param) {
  if (param == NULL) {
    return false;
  }
  if (param->is_parameter_pack) {
    return true;
  }
  if (param->kind == kTemplateParameterType) {
    return param->default_type != NULL;
  }
  return param->default_argument != NULL || param->has_default_int;
}

static bool TemplateTemplateParameterListsCompatible(Vector* formal,
                                                       Vector* actual) {
  if (formal == NULL || actual == NULL) {
    return false;
  }
  size_t fi = 0;
  size_t ai = 0;
  while (fi < formal->length) {
    TemplateParameter* fp = formal->value.p[fi];
    if (fp == NULL) {
      return false;
    }
    if (fp->is_parameter_pack) {
      for (; ai < actual->length; ai++) {
        TemplateParameter* ap = actual->value.p[ai];
        if (ap == NULL || ap->kind != fp->kind ||
            (fp->kind == kTemplateParameterTemplate &&
             (fp->template_template_kind != ap->template_template_kind ||
              !TemplateTemplateParameterListsCompatible(
                  fp->template_parameters, ap->template_parameters)))) {
          return false;
        }
      }
      return true;
    }
    if (ai >= actual->length) {
      return false;
    }
    TemplateParameter* ap = actual->value.p[ai];
    if (ap == NULL || ap->kind != fp->kind ||
        (fp->kind == kTemplateParameterNonType &&
         fp->type != NULL && ap->type != NULL &&
         (fp->type->type & kTypeAuto) == 0 &&
         (ap->type->type & kTypeAuto) == 0 &&
         !TypeEqual(fp->type, ap->type)) ||
        (fp->kind == kTemplateParameterTemplate &&
         (fp->template_template_kind != ap->template_template_kind ||
          !TemplateTemplateParameterListsCompatible(
              fp->template_parameters, ap->template_parameters)))) {
      return false;
    }
    fi++;
    ai++;
  }
  for (; ai < actual->length; ai++) {
    if (!TemplateParameterHasDefault(actual->value.p[ai])) {
      return false;
    }
  }
  return true;
}

static bool TemplateTemplateArgumentMatchesParameter(
    TemplateParameter* param, TemplateArgument* arg) {
  if (param == NULL || arg == NULL ||
      param->kind != kTemplateParameterTemplate ||
      arg->kind != kTemplateParameterTemplate) {
    return false;
  }
  Symbol* symbol = arg->template_symbol;
  TemplateTemplateParameterKind actual_kind =
      symbol != NULL && symbol->flags.is_template_template_parameter
          ? symbol->template_template_parameter_kind
          : symbol != NULL && symbol->flags.is_concept
                ? kTemplateTemplateParameterConcept
                : symbol != NULL && symbol->variable_template != NULL
                      ? kTemplateTemplateParameterVariable
                      : kTemplateTemplateParameterType;
  if (actual_kind != param->template_template_kind) {
    return false;
  }
  Vector* actual = TemplateParameterListForSymbol(arg->template_symbol);
  if (actual == NULL && arg->template_parameter_index >= 0) {
    return true;
  }
  return TemplateTemplateParameterListsCompatible(
      param->template_parameters, actual);
}

static const char* TemplateArgumentKindError(TemplateParameterKind kind) {
  if (kind == kTemplateParameterType) {
    return "Template argument must name a type";
  }
  if (kind == kTemplateParameterTemplate) {
    return "Template argument must name a compatible template";
  }
  return "Template non-type argument must be an integer constant expression";
}

/* A non-type parameter whose own type is dependent is where the pre-C++20
 * constraint idiom puts its condition: in
 * `template <class T, typename enable_if<C<T>, int>::type = 0> void f(T);`
 * the enable_if has no member `type` when `C<T>` is false, and the candidate
 * must then be discarded.  Substitute each such type against the now-complete
 * argument vector and report whether all of them have a valid substitution.
 * Substituting requires the full vector, because the condition may name any
 * parameter, not only those declared before this one.  Without this the
 * condition is never evaluated and every candidate is accepted. */
static bool SubstituteDependentNonTypeParameterTypes(TypeParser* parser,
                                                     Vector* template_parameters,
                                                     Vector* completed) {
  if (parser == NULL || template_parameters == NULL || completed == NULL) {
    return true;
  }
  for (size_t i = 0; i < template_parameters->length; i++) {
    TemplateParameter* param = template_parameters->value.p[i];
    if (param == NULL || param->kind != kTemplateParameterNonType ||
        param->type == NULL || (param->type->type & kTypeAuto) != 0 ||
        !TypeContainsTemplateParameter(param->type)) {
      continue;
    }
    bool saved_substitution_failed = parser->template_substitution_failed;
    parser->template_substitution_failed = false;
    TypeRecord* substituted =
        SubstituteTemplateParameters(parser, param->type, completed);
    bool substitution_failed =
        parser->template_substitution_failed || substituted == NULL;
    parser->template_substitution_failed = saved_substitution_failed;
    if (substitution_failed) {
      TypeRecordDelete(substituted);
      return false;
    }
    // Record the concrete parameter type on the argument so later stages see
    // the substituted type rather than the dependent spelling.
    TemplateArgument* arg =
        i < completed->length ? completed->value.p[i] : NULL;
    if (arg != NULL && arg->dependent_expr == NULL &&
        !TypeContainsTemplateParameter(substituted)) {
      TypeRecordDelete(arg->type);
      arg->type = TypeRecordCopy(substituted);
    }
    TypeRecordDelete(substituted);
  }
  return true;
}

/* Produce a full template argument vector with one entry per declared
 * parameter: copy supplied `args`, substitute defaults (which may themselves
 * reference earlier parameters) for any omitted trailing parameters, and gather
 * leftover args into a trailing parameter pack. Emits `error_message` and
 * returns NULL if required arguments are missing. */
static Vector* CompleteTemplateArguments(TypeParser* parser,
                                         Vector* template_parameters,
                                         Vector* args,
                                         const char* error_message,
                                         bool emit_error,
                                         TemplateArgumentOwnership ownership) {
  if (args == NULL) {
    if (emit_error) {
      SyntaxError(parser->syntax, "%s", error_message);
    }
    return NULL;
  }
  int pack_index = FindTemplateParameterPackIndex(template_parameters);
  if (pack_index < 0 && args->length > template_parameters->length) {
    if (emit_error) {
      SyntaxError(parser->syntax, "Too many template arguments");
    }
    return NULL;
  }
  Vector* completed = NewVector();
  VectorReserve(completed, template_parameters->length);
  for (size_t i = 0; i < template_parameters->length; i++) {
    TemplateParameter* param = template_parameters->value.p[i];
    if (param != NULL && param->is_parameter_pack) {
      TemplateArgument* pack = NewEmptyPackTemplateArgument(param->kind);
      // Deduction produces a normalized vector with one bundled argument per
      // declared parameter. Preserve that one-to-one shape for a non-trailing
      // pack instead of greedily absorbing the arguments belonging to later
      // parameters. Raw explicit argument lists still use the trailing-pack
      // gathering path.
      bool normalized_pack =
          args->length == template_parameters->length && i < args->length &&
          args->value.p[i] != NULL &&
          ((TemplateArgument*)args->value.p[i])->pack_arguments != NULL;
      size_t pack_end = normalized_pack ? i + 1 : args->length;
      for (size_t j = i; j < pack_end; j++) {
        TemplateArgument* arg = args->value.p[j];
        bool incompatible_template =
            arg != NULL && arg->kind == kTemplateParameterTemplate &&
            arg->pack_arguments == NULL &&
            !TemplateTemplateArgumentMatchesParameter(param, arg);
        if (arg == NULL || arg->kind != param->kind ||
            incompatible_template) {
          if (emit_error) {
            SyntaxError(
                parser->syntax, "%s",
                incompatible_template
                    ? "Template argument does not match template parameter list"
                    : TemplateArgumentKindError(param->kind));
          }
          TemplateArgumentDelete(pack);
          VectorDeleteWithContents(completed,
                                   (VectorElementDestructor)TemplateArgumentDelete,
                                   /*free_element=*/false);
          return NULL;
        }
        if (arg->pack_arguments != NULL) {
          for (size_t k = 0; k < arg->pack_arguments->length; k++) {
            TemplateArgument* element = arg->pack_arguments->value.p[k];
            bool incompatible_element =
                element != NULL &&
                element->kind == kTemplateParameterTemplate &&
                !TemplateTemplateArgumentMatchesParameter(param, element);
            if (element == NULL || element->kind != param->kind ||
                incompatible_element) {
              if (emit_error) {
                SyntaxError(
                    parser->syntax, "%s",
                    incompatible_element
                        ? "Template argument does not match template parameter list"
                        : TemplateArgumentKindError(param->kind));
              }
              TemplateArgumentDelete(pack);
              VectorDeleteWithContents(
                  completed, (VectorElementDestructor)TemplateArgumentDelete,
                  /*free_element=*/false);
              return NULL;
            }
            if (ownership == kTemplateArgumentsConsume) {
              arg->pack_arguments->value.p[k] = NULL;
              VectorAppend(pack->pack_arguments, element);
            } else {
              VectorAppend(pack->pack_arguments,
                           TemplateArgumentCopy(element));
            }
          }
          if (ownership == kTemplateArgumentsConsume) {
            args->value.p[j] = NULL;
            TemplateArgumentDelete(arg);
          }
        } else {
          if (ownership == kTemplateArgumentsConsume) {
            args->value.p[j] = NULL;
            VectorAppend(pack->pack_arguments, arg);
          } else {
            VectorAppend(pack->pack_arguments, TemplateArgumentCopy(arg));
          }
        }
      }
      VectorAppend(completed, pack);
      continue;
    }
    bool argument_from_default = false;
    TemplateArgument* arg = i < args->length ? args->value.p[i] : NULL;
    if (arg != NULL) {
      if (ownership == kTemplateArgumentsConsume) {
        args->value.p[i] = NULL;
      } else {
        arg = TemplateArgumentCopy(arg);
      }
    }
    if (arg == NULL) {
      if (param->kind == kTemplateParameterType &&
          param->default_type != NULL) {
        bool saved_substitution_failed = parser->template_substitution_failed;
        parser->template_substitution_failed = false;
        TypeRecord* default_type =
            SubstituteTemplateParameters(parser, param->default_type,
                                         completed);
        bool substitution_failed =
            parser->template_substitution_failed || default_type == NULL;
        parser->template_substitution_failed = saved_substitution_failed;
        if (substitution_failed) {
          /* `typename = typename enable_if<C>::type`: when C is false the
           * default has no valid substitution and this candidate is SFINAE'd
           * out.  Do not diagnose; the caller discards the candidate. */
          TypeRecordDelete(default_type);
          VectorDeleteWithContents(
              completed, (VectorElementDestructor)TemplateArgumentDelete,
              /*free_element=*/false);
          return NULL;
        }
        arg = NewDefaultTypeTemplateArgument(default_type);
      } else if (param->kind == kTemplateParameterNonType &&
                 (param->default_argument != NULL ||
                  param->has_default_int)) {
        argument_from_default = true;
        TemplateArgument* default_argument = param->default_argument;
        int default_template_parameter_index =
            default_argument != NULL
                ? default_argument->template_parameter_index
                : param->default_template_parameter_index;
        if (default_template_parameter_index >= 0 &&
            (size_t)default_template_parameter_index < completed->length) {
          TemplateArgument* actual =
              completed->value.p[default_template_parameter_index];
          if (actual->kind == kTemplateParameterNonType) {
            arg = TemplateArgumentCopy(actual);
          }
        }
        if (arg == NULL) {
          bool saved_failed = parser->template_substitution_failed;
          parser->template_substitution_failed = false;
          arg = default_argument != NULL
                    ? NewSubstitutedTemplateArgument(parser, default_argument,
                                                     completed)
                    : NewDefaultNonTypeTemplateArgument(
                          param->default_int_value,
                          default_template_parameter_index);
          bool failed = parser->template_substitution_failed;
          // `int = enable_if_t<Trait<T>::value, int>()`: forming an invalid
          // type while substituting the default (`T*` when `T` is a reference)
          // or naming a missing `enable_if<false>::type` drops the candidate.
          // Sibling operands may still mention a template parameter; the
          // failure itself is enough.
          bool concrete_failure = failed;
          parser->template_substitution_failed = saved_failed ||
                                                 concrete_failure;
          if (concrete_failure) {
            TemplateArgumentDelete(arg);
            VectorDeleteWithContents(
                completed, (VectorElementDestructor)TemplateArgumentDelete,
                /*free_element=*/false);
            return NULL;
          }
        }
      } else if (param->kind == kTemplateParameterTemplate &&
                 param->default_argument != NULL) {
        arg = NewSubstitutedTemplateArgument(
            parser, param->default_argument, completed);
      } else {
        if (emit_error) {
          SyntaxError(parser->syntax, "%s", error_message);
        }
        VectorDeleteWithContents(completed,
                                 (VectorElementDestructor)TemplateArgumentDelete,
                                 /*free_element=*/false);
        return NULL;
      }
    }
    if (argument_from_default &&
        param->kind == kTemplateParameterNonType &&
        param->type != NULL && (param->type->type & kTypeAuto) == 0 &&
        !TypeContainsTemplateParameter(param->type) &&
        arg != NULL && arg->dependent_expr == NULL) {
      TypeRecordDelete(arg->type);
      arg->type = TypeRecordCopy(param->type);
    }
    if (param->kind == kTemplateParameterTemplate &&
        arg->kind == kTemplateParameterType && arg->type != NULL &&
        TypeIsStructOrUnion(arg->type) &&
        arg->type->info.struct_info != NULL) {
      // The injected-class-name denotes the class template when it is the
      // argument of a template template parameter (`Apply<Box>` inside
      // `Box`), and the current instantiation when it is a type argument
      // (`is_same<Round, Box>`).
      Struct* str = arg->type->info.struct_info;
      Symbol* origin = arg->type->template_origin;
      if (origin == NULL && str->tag_symbol != NULL &&
          (str->is_template || str->defining_template_scope_count > 0)) {
        origin = str->tag_symbol;
      }
      bool current_instantiation =
          origin != NULL &&
          (arg->type->template_origin == NULL ||
           (parser->template_substitution_target != NULL &&
            str == parser->template_substitution_target));
      if (current_instantiation) {
        arg->kind = kTemplateParameterTemplate;
        arg->template_symbol = origin;
        TypeRecordDelete(arg->type);
        arg->type = NULL;
      }
    }
    if (param->kind != arg->kind) {
      if (emit_error) {
        SyntaxError(parser->syntax, "%s",
                    TemplateArgumentKindError(param->kind));
      }
      TemplateArgumentDelete(arg);
      VectorDeleteWithContents(completed,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      return NULL;
    }
    if (param->kind == kTemplateParameterNonType &&
        !ConvertClassNonTypeTemplateArgument(parser, param, arg)) {
      if (emit_error) {
        SyntaxError(parser->syntax,
                    "Template non-type argument cannot initialize the parameter object");
      }
      TemplateArgumentDelete(arg);
      VectorDeleteWithContents(completed,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      return NULL;
    }
    if (param->kind == kTemplateParameterNonType &&
        !TemplateNonTypeArgumentMatchesParameter(param, arg)) {
      if (emit_error) {
        SyntaxError(parser->syntax,
                    "Template non-type argument is not compatible with parameter type");
      }
      TemplateArgumentDelete(arg);
      VectorDeleteWithContents(completed,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      return NULL;
    }
    if (param->kind == kTemplateParameterTemplate &&
        !TemplateTemplateArgumentMatchesParameter(param, arg)) {
      if (emit_error) {
        SyntaxError(parser->syntax,
                    "Template argument does not match template parameter list");
      }
      TemplateArgumentDelete(arg);
      VectorDeleteWithContents(completed,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      return NULL;
    }
    if (param->kind == kTemplateParameterNonType &&
        param->type != NULL && (param->type->type & kTypeAuto) == 0 &&
        !TypeContainsTemplateParameter(param->type) &&
        arg->template_parameter_index < 0 && arg->dependent_expr == NULL) {
      TypeRecordDelete(arg->type);
      arg->type = TypeRecordCopy(param->type);
    }
    VectorAppend(completed, arg);
  }
  if (!SubstituteDependentNonTypeParameterTypes(parser, template_parameters,
                                                completed)) {
    VectorDeleteWithContents(completed,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return NULL;
  }
  return completed;
}

/* Complete a concept-id's argument list against the concept's own template
 * parameters, applying trailing default arguments.  Concept default arguments
 * can name earlier parameters (e.g. `C = common_type_t<T, U>`), which
 * CompleteTemplateArguments resolves by substituting the arguments filled so
 * far.  Returns a freshly owned vector or NULL; never diagnoses. */
Vector* TypeCompleteConceptArguments(Syntax* syntax, Vector* concept_parameters,
                                     Vector* args) {
  if (syntax == NULL || concept_parameters == NULL || args == NULL) {
    return NULL;
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  Vector* completed = CompleteTemplateArguments(
      &parser, concept_parameters, args,
      "too few template arguments for concept", /*emit_error=*/false,
      kTemplateArgumentsBorrow);
  TypeParserDestruct(&parser);
  return completed;
}

/* Complete a class template's argument list against its parameters (filling in
 * defaults and gathering a trailing pack); see CompleteTemplateArguments. */
Vector* CompleteClassTemplateArguments(TypeParser* parser,
                                              Struct* template_struct,
                                              Vector* args) {
  return CompleteTemplateArguments(parser, &template_struct->template_parameters,
                                   args,
                                   "Too few template arguments for class template",
                                   /*emit_error=*/true,
                                   kTemplateArgumentsBorrow);
}

/* Register a partial specialization (its tag and argument pattern) on the
 * primary class template, rejecting an exact duplicate pattern. */
static void AddOwnedAssociatedConstraint(ConstraintExpr** target,
                                         ConstraintExpr* constraint) {
  if (target == NULL || constraint == NULL) {
    return;
  }
  ConstraintExpr* current = *target;
  if (current == NULL) {
    *target = constraint;
    return;
  }
  *target = NewConjunctionConstraint(current, constraint, constraint->location);
}

static void MoveTemplateParameterConstraints(Vector* parameters,
                                             ConstraintExpr** target) {
  if (parameters == NULL || target == NULL) {
    return;
  }
  for (size_t i = 0; i < parameters->length; i++) {
    TemplateParameter* param = parameters->value.p[i];
    if (param == NULL || param->associated_constraint == NULL) {
      continue;
    }
    ConstraintExpr* constraint = param->associated_constraint;
    param->associated_constraint = NULL;
    AddOwnedAssociatedConstraint(target, constraint);
  }
}

void AddClassTemplatePartialSpecialization(TypeParser* parser,
                                                  Symbol* primary,
                                                  Symbol* partial_tag,
                                                  Vector* pattern_args) {
  if (primary == NULL || primary->type == NULL ||
      !TypeIsStructOrUnion(primary->type) ||
      primary->type->info.struct_info == NULL || partial_tag == NULL ||
      pattern_args == NULL) {
    return;
  }
  Struct* primary_struct = primary->type->info.struct_info;
  // Build the new specialization (including its associated constraint) up front
  // so duplicate detection can compare constraints, not just the pattern.  Two
  // specializations that share a pattern but have *distinct* constraints are
  // valid constrained partial specializations (e.g. incrementable_traits<T>
  // constrained on `has-member-difference-type` vs. `subtractable-integral`);
  // only a same-pattern, constraint-equivalent redeclaration is a duplicate.
  ClassTemplatePartialSpecialization* partial =
      NewClassTemplatePartialSpecialization(
          partial_tag, parser->syntax->current_template_parameters,
          pattern_args);
  MoveTemplateParameterConstraints(&partial->template_parameters,
                                   &partial->associated_constraint);
  if (parser->syntax->current_template_requires_clause != NULL) {
    AddOwnedAssociatedConstraint(&partial->associated_constraint,
                                 parser->syntax->current_template_requires_clause);
    parser->syntax->current_template_requires_clause = NULL;
  }
  for (size_t i = 0; i < primary_struct->partial_specializations.length; i++) {
    ClassTemplatePartialSpecialization* existing =
        primary_struct->partial_specializations.value.p[i];
    if (existing != NULL &&
        TemplateArgumentPatternVectorEqual(&existing->pattern_arguments,
                                           &partial->pattern_arguments) &&
        ConceptsAssociatedConstraintsEquivalent(
            existing->associated_constraint, &existing->template_parameters,
            partial->associated_constraint, &partial->template_parameters)) {
      SyntaxError(parser->syntax,
                  "Duplicate class template partial specialization %s",
                  partial_tag->name.value);
      ClassTemplatePartialSpecializationDelete(partial);
      return;
    }
  }
  VectorAppend(&primary_struct->partial_specializations, partial);
}

/* Register a partial/explicit specialization of a *variable* template.  The
 * specialization's parameters are the current template parameters, its pattern
 * is `pattern_args` (the template-id written after the variable name), and its
 * value is `initializer` (kept unanalyzed, folded per use). */
void AddVariableTemplatePartialSpecialization(TypeParser* parser,
                                              Symbol* primary,
                                              Vector* pattern_args,
                                              struct ASTNode* initializer,
                                              struct TypeRecord* type) {
  if (primary == NULL || primary->variable_template == NULL ||
      pattern_args == NULL) {
    if (initializer != NULL) {
      ASTNodeDelete(initializer);
    }
    return;
  }
  VariableTemplate* vt = primary->variable_template;
  for (size_t i = 0; i < vt->partial_specializations.length; i++) {
    ClassTemplatePartialSpecialization* existing =
        vt->partial_specializations.value.p[i];
    if (existing != NULL &&
        TemplateArgumentPatternVectorEqual(&existing->pattern_arguments,
                                           pattern_args)) {
      SyntaxError(parser->syntax,
                  "Duplicate variable template specialization %s",
                  primary->name.value);
      if (initializer != NULL) {
        ASTNodeDelete(initializer);
      }
      return;
    }
  }
  ClassTemplatePartialSpecialization* partial =
      NewVariableTemplatePartialSpecialization(
          parser->syntax->current_template_parameters, pattern_args,
          initializer, type);
  MoveTemplateParameterConstraints(&partial->template_parameters,
                                   &partial->associated_constraint);
  if (parser->syntax->current_template_requires_clause != NULL) {
    AddOwnedAssociatedConstraint(&partial->associated_constraint,
                                 parser->syntax->current_template_requires_clause);
    parser->syntax->current_template_requires_clause = NULL;
  }
  VectorAppend(&vt->partial_specializations, partial);
}

/* Complete a function template's argument list against its parameters (filling
 * defaults / gathering a trailing pack). A type argument that is still a
 * template parameter is left dependent; an enclosing parameter can share this
 * function's parameter index (`WasDeduced<Arg>()` inside `Cleanup<Arg>`), and
 * erasing that placeholder would instantiate the function as if the argument
 * were `int`. Returns NULL if completion fails. */
static Vector* CompleteFunctionTemplateArguments(TypeParser* parser,
                                                 TypeRecord* func,
                                                 Vector* args,
                                                 bool emit_error,
                                                 TemplateArgumentOwnership ownership) {
  Vector* parameters = NULL;
  if (func != NULL && TypeIsFunction(func)) {
    parameters = &func->info.function.template_parameters;
    if (parameters->length == 0 && func->info.function.symbol != NULL &&
        func->info.function.symbol->imported_function_template_parameters_backup
                .length > 0) {
      parameters =
          &func->info.function.symbol->imported_function_template_parameters_backup;
    }
  }
  if (func == NULL || !TypeIsFunction(func) || parameters == NULL ||
      parameters->length == 0) {
    if (emit_error) {
      SyntaxError(parser->syntax,
                  "Function template instantiation is not supported yet");
    }
    return NULL;
  }
  // Call deduction records one argument per parameter of the member template,
  // numbered from 0.  A primary member template still spells those parameters
  // after the enclosing class (`T` at index 2 inside `template<class Policy,
  // class... Params>`).  Rebase a copy so `Trait<const T&>::value` folds
  // against the deduced argument; rebasing the primary would change later
  // instantiations.
  Vector* parameters_for_completion = parameters;
  Vector* rebased_parameters = NULL;
  int member_base = func->info.function.template_parameter_base;
  if (member_base > 0) {
    rebased_parameters = TemplateParameterVectorCopy(parameters);
    // The primary still numbers parameters after the enclosing class
    // (`Hash` at 0, `T` at 1, this template's `H` at 2).  Call completion
    // only has this template's own arguments.  Fill the enclosing indices
    // from the class being instantiated before rebasing `H` down to 0, or
    // `decltype(H::Invoke(declval<State>(), declval<const T&>()))` keeps `T`
    // and the SFINAE candidate is discarded.
    Vector* enclosing = NULL;
    Struct* owner =
        func->info.function.cxx_member_owner;
    Struct* target =
        parser != NULL ? parser->template_substitution_target : NULL;
    if (owner != NULL && target != NULL && target->tag_symbol != NULL &&
        target->tag_symbol->type != NULL &&
        (owner == target ||
         target->tag_symbol->type->template_origin == owner->tag_symbol)) {
      enclosing = StructConcreteTemplateArguments(target);
    }
    if ((enclosing == NULL || (int)enclosing->length < member_base) &&
        func->info.function.symbol != NULL) {
      Vector* owner_args = MemberFunctionEnclosingClassArguments(
          func->info.function.symbol);
      if (owner_args != NULL && (int)owner_args->length >= member_base) {
        enclosing = owner_args;
      }
    }
    if (enclosing == NULL || (int)enclosing->length < member_base) {
      Vector* stacked =
          NestedSubstitutionArgumentsForOwner(owner, member_base);
      if (stacked == NULL) {
        stacked = NestedSubstitutionArgumentsForOwner(target, member_base);
      }
      if (stacked != NULL) {
        enclosing = stacked;
      }
    }
    if (enclosing != NULL && (int)enclosing->length < member_base) {
      enclosing = NULL;
    }
    for (size_t i = 0; i < rebased_parameters->length; i++) {
      TemplateParameter* param = rebased_parameters->value.p[i];
      if (param == NULL) {
        continue;
      }
      if (enclosing != NULL && parser != NULL) {
        bool saved_failed = parser->template_substitution_failed;
        bool saved_only =
            parser->substituting_enclosing_template_arguments_only;
        parser->template_substitution_failed = false;
        parser->substituting_enclosing_template_arguments_only = true;
        if (param->type != NULL &&
            TypeContainsTemplateParameter(param->type)) {
          TypeRecord* substituted =
              SubstituteTemplateParameters(parser, param->type, enclosing);
          bool failed = parser->template_substitution_failed;
          bool original_has_own =
              TypeReferencesParameterAtOrAbove(param->type, member_base);
          bool substituted_has_own =
              substituted != NULL &&
              TypeReferencesParameterAtOrAbove(substituted, member_base);
          if (substituted != NULL && !failed &&
              (!original_has_own || substituted_has_own)) {
            TypeRecordDelete(param->type);
            param->type = substituted;
          } else {
            TypeRecordDelete(substituted);
          }
          parser->template_substitution_failed = false;
        }
        if (param->default_type != NULL &&
            TypeContainsTemplateParameter(param->default_type)) {
          TypeRecord* substituted = SubstituteTemplateParameters(
              parser, param->default_type, enclosing);
          bool failed = parser->template_substitution_failed;
          bool original_has_own = TypeReferencesParameterAtOrAbove(
              param->default_type, member_base);
          bool substituted_has_own =
              substituted != NULL &&
              TypeReferencesParameterAtOrAbove(substituted, member_base);
          if (substituted != NULL && !failed &&
              (!original_has_own || substituted_has_own)) {
            TypeRecordDelete(param->default_type);
            param->default_type = substituted;
          } else {
            TypeRecordDelete(substituted);
          }
        }
        parser->substituting_enclosing_template_arguments_only = saved_only;
        parser->template_substitution_failed = saved_failed;
      }
      RebaseTemplateParameterIndices(param->type, member_base);
      RebaseTemplateParameterIndices(param->default_type, member_base);
      if (param->default_template_parameter_index >= member_base) {
        param->default_template_parameter_index -= member_base;
      }
      if (param->index >= member_base) {
        param->index -= member_base;
      }
      if (param->default_argument != NULL &&
          param->default_argument->template_parameter_index >= member_base) {
        param->default_argument->template_parameter_index -= member_base;
      }
      if (param->default_argument != NULL &&
          param->default_argument->dependent_expr != NULL &&
          parser->syntax != NULL) {
        ASTNode* rebased = TypeSubstituteMemberTemplateExpressionAndRebase(
            parser->syntax, param->default_argument->dependent_expr,
            /*args=*/NULL, member_base,
            param->default_argument->dependent_expr->location, NULL, NULL);
        if (rebased != NULL) {
          param->default_argument->dependent_expr = rebased;
        }
      }
    }
    parameters_for_completion = rebased_parameters;
  }
  Vector* completed =
      CompleteTemplateArguments(parser, parameters_for_completion, args,
                                "Function template instantiation is not supported yet",
                                emit_error, ownership);
  if (rebased_parameters != NULL) {
    VectorDeleteWithContents(rebased_parameters,
                             (VectorElementDestructor)TemplateParameterDelete,
                             /*free_element=*/false);
  }
  if (completed == NULL) {
    return NULL;
  }
  return completed;
}

/* Instantiate the deferred friend functions a class template declared, mapping
 * each onto a concrete free function in the template's namespace.  The friend's
 * signature (and inline body, if any) is substituted with the instantiation
 * arguments; the resulting function is registered for overload resolution/ADL,
 * recorded as a friend of `str`, and queued for emission when it carries a body
 * (deduplicated against earlier specializations and existing declarations). */
/* Materialize the deferred friend functions listed on `friend_owner`, recording
 * each concrete friend on `record_into` and registering it for overload
 * resolution / ADL.  Dependent signatures and inline bodies are substituted with
 * `args`, resolving any nested-type references (e.g. a sentinel's friend naming
 * the iterator type) by name through the `subst_source`->`subst_target` mapping.
 * For a template's own friends all four structs coincide with (str, source);
 * for a nested class's friends, `record_into`/`friend_owner` are the nested
 * instantiated/template structs while `subst_source`/`subst_target` remain the
 * *enclosing* template/instantiation so sibling nested types remap correctly. */
static void InstantiateTemplateFriendFunctionsImpl(TypeParser* parser,
                                                   Struct* record_into,
                                                   Struct* friend_owner,
                                                   Struct* subst_source,
                                                   Struct* subst_target,
                                                   Vector* args) {
  for (size_t i = 0; i < friend_owner->friend_functions.length; i++) {
    Symbol* ftpl = friend_owner->friend_functions.value.p[i];
    if (ftpl == NULL || ftpl->type == NULL || !TypeIsFunction(ftpl->type)) {
      if (ftpl != NULL) {
        StructAddFriendFunction(record_into, ftpl);
      }
      continue;
    }

    Struct* saved_source = parser->template_substitution_source;
    Struct* saved_target = parser->template_substitution_target;
    parser->template_substitution_source = subst_source;
    parser->template_substitution_target = subst_target;

    TypeRecord* func = InstantiateMemberFunctionType(
        parser, /*owner=*/NULL, /*is_static_member=*/true, ftpl->type, args,
        ftpl->location);
    // Signature instantiation may recursively instantiate nested types and
    // temporarily replace the parser's active substitution. The inline friend
    // body still needs the enclosing source->specialization mapping.
    parser->template_substitution_source = subst_source;
    parser->template_substitution_target = subst_target;
    bool constraints_satisfied =
        func->info.function.associated_constraint == NULL ||
        ConceptsConstraintSatisfied(
            func->info.function.associated_constraint, args);
    func->info.function.cxx_member_owner = NULL;

    Symbol* sym = NewSymbol(ftpl->name.value, func, ftpl->storage);
    sym->flags = ftpl->flags;
    sym->location = ftpl->location;
    sym->namespace_ = ftpl->namespace_;
    func->info.function.symbol = sym;
    SymbolSetCXXMangledAsmName(sym);

    Symbol* in_scope = SyntaxRegisterInstantiatedFriendFunction(
        parser->syntax, ftpl->namespace_, sym);
    StructAddFriendFunction(record_into, in_scope != NULL ? in_scope : sym);

    bool is_new_symbol = (in_scope == sym);
    if (is_new_symbol && ftpl->type->info.function.body != NULL &&
        constraints_satisfied &&
        !PendingTemplateInstantiationHasAsmName(sym->asm_name.value)) {
      sym->type->info.function.body = CloneTemplateFunctionBody(
          parser, ftpl->type, sym->type, args);
      sym->type->info.function.definition = true;
      sym->flags.is_defined = true;
      if (sym->type->info.function.is_inline) {
        sym->flags.is_inline_defn = true;
      }
      if (!StorageIs(sym->storage, STO(static)) &&
          !sym->flags.is_explicit_specialization) {
        sym->flags.is_weak = true;
      }
      // A friend function template materialized for a class specialization is
      // still only a template definition. Keep its class-substituted body as
      // the pattern for per-call instantiation; analyzing or emitting that
      // dependent body now can bind its own parameters against unrelated class
      // arguments (for example CharT against an engine type).
      if (!sym->flags.is_template) {
        CompilerQueuePendingFunctionDefinition(sym);
        VectorAppend(&compiler->declaration_asts,
                     sym->type->info.function.body);
      }
    } else if (is_new_symbol && ftpl->type->info.function.body != NULL) {
      sym->value.func_defn = ftpl;
    }

    parser->template_substitution_source = saved_source;
    parser->template_substitution_target = saved_target;
  }
}

static void InstantiateTemplateFriendFunctions(TypeParser* parser, Struct* str,
                                               Struct* source_struct,
                                               Vector* args) {
  InstantiateTemplateFriendFunctionsImpl(parser, str, source_struct,
                                         source_struct, str, args);
}

/* True if `type` is a named alias-template specialization that has not yet
 * been expanded to its pattern (`Elem<T, U>` stored as origin+args rather
 * than the underlying `is_constructible<...>` class). */
static bool TypeIsUnexpandedAliasTemplateId(TypeRecord* type) {
  return CompilerIsCXX() && type != NULL && type->template_origin != NULL &&
         type->template_origin->flags.is_template &&
         StorageIs(type->template_origin->storage, STO(typedef)) &&
         type->template_arguments != NULL && !TypeIsStructOrUnion(type);
}

/* Expand a concrete alias template-id to its pattern.  Returns a new type
 * on success, or NULL if `type` is not a fully-concrete alias-id. */
static TypeRecord* ExpandConcreteAliasTemplateId(TypeParser* parser,
                                                 TypeRecord* type) {
  if (parser == NULL || !TypeIsUnexpandedAliasTemplateId(type) ||
      TemplateArgumentVectorContainsTemplateParameter(
          type->template_arguments)) {
    return NULL;
  }
  Symbol* alias = type->template_origin;
  Vector* grouped =
      CompleteAliasTemplateArguments(alias, type->template_arguments);
  Vector* pattern_args =
      grouped != NULL ? grouped : type->template_arguments;
  TypeRecord* subst =
      SubstituteTemplateParameters(parser, alias->type, pattern_args);
  if (subst != NULL) {
    subst->qualifiers |= type->qualifiers;
    subst = TypeMaterializeClassTemplateSpecialization(parser->syntax, subst);
  }
  if (grouped != NULL) {
    VectorDeleteWithContents(grouped,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  return subst;
}

/* Replace concrete alias-id type arguments with their expanded patterns so a
 * class template such as `conjunction<Elem<int, int&&>>` sees the underlying
 * class trait as `B1` rather than an unknown alias. */
static void ExpandConcreteAliasTemplateArguments(TypeParser* parser,
                                                 Vector* args) {
  if (parser == NULL || args == NULL) {
    return;
  }
  for (size_t i = 0; i < args->length; i++) {
    TemplateArgument* arg = args->value.p[i];
    if (arg == NULL) {
      continue;
    }
    if (arg->pack_arguments != NULL) {
      ExpandConcreteAliasTemplateArguments(parser, arg->pack_arguments);
      continue;
    }
    if (arg->kind != kTemplateParameterType || arg->type == NULL) {
      continue;
    }
    TypeRecord* expanded = ExpandConcreteAliasTemplateId(parser, arg->type);
    if (expanded != NULL) {
      TypeRecordDelete(arg->type);
      arg->type = expanded;
      continue;
    }
    if (arg->type->dependent_member_name != NULL &&
        arg->type->template_origin != NULL &&
        !TemplateArgumentVectorContainsTemplateParameter(
            arg->type->template_arguments)) {
      TypeRecord* materialized = TypeMaterializeClassTemplateSpecialization(
          parser->syntax, arg->type);
      if (materialized != arg->type) {
        TypeRecordDelete(arg->type);
        arg->type = materialized;
      }
    }
  }
}

static bool ClassTemplateArgumentIsStillDependent(TemplateArgument* arg) {
  if (TemplateArgumentContainsTemplateParameterForInstantiation(arg)) {
    return true;
  }
  if (arg == NULL || arg->type == NULL) {
    return false;
  }
  if (TypeIsUnexpandedAliasTemplateId(arg->type)) {
    return true;
  }
  if (arg->type->dependent_member_name != NULL &&
      arg->type->template_origin != NULL) {
    return true;
  }
  return false;
}

static bool ClassTemplateArgumentsAreStillDependent(Vector* args) {
  for (size_t i = 0; args != NULL && i < args->length; i++) {
    if (ClassTemplateArgumentIsStillDependent(args->value.p[i])) {
      return true;
    }
  }
  return false;
}

/* A non-type argument can be a fully substituted expression that was never
 * reduced to a value (`sizeof(const T& (*)()) != 0` after `T` is `int`).
 * Partial specializations such as `enable_if<true, T>` compare integral
 * values and miss that argument while it still carries the expression, so
 * the primary template is instantiated instead. */
static void FoldConcreteNonTypeTemplateArgument(TypeParser* parser,
                                                TemplateArgument* arg) {
  if (arg == NULL) {
    return;
  }
  if (arg->pack_arguments != NULL) {
    for (size_t i = 0; i < arg->pack_arguments->length; i++) {
      FoldConcreteNonTypeTemplateArgument(parser,
                                          arg->pack_arguments->value.p[i]);
    }
    return;
  }
  if (arg->kind != kTemplateParameterNonType || arg->dependent_expr == NULL ||
      DependentExpressionContainsTemplateParameter(arg->dependent_expr)) {
    return;
  }
  Vector empty_args;
  VectorInit(&empty_args);
  bool saved_failed = parser != NULL && parser->template_substitution_failed;
  if (parser != NULL) {
    parser->template_substitution_failed = false;
  }
  int64_t folded = 0;
  bool ok = TryFoldDependentTemplateArgument(parser, arg->dependent_expr,
                                             &empty_args, &folded);
  if (parser != NULL) {
    parser->template_substitution_failed = saved_failed;
  }
  VectorDestruct(&empty_args);
  if (!ok) {
    return;
  }
  ASTNodeDelete(arg->dependent_expr);
  arg->dependent_expr = NULL;
  arg->int_value = folded;
  arg->value_kind = kTemplateValueIntegral;
  arg->template_parameter_index = -1;
}

static void FoldConcreteNonTypeTemplateArguments(TypeParser* parser,
                                                 Vector* args) {
  for (size_t i = 0; args != NULL && i < args->length; i++) {
    FoldConcreteNonTypeTemplateArgument(parser, args->value.p[i]);
  }
}

// `Outer<X>::ErrorMaker<res>` members number enclosing parameters first.
// The nested specialization only carries `[res]`; prepend `Outer`'s arguments
// so `return res` substitutes.  Returns a new vector the caller must delete,
// or NULL when `own_args` is already complete.
static Vector* PrefixEnclosingClassTemplateArguments(Struct* enclosing,
                                                     Vector* own_parameters,
                                                     Vector* own_args) {
  if (enclosing == NULL || enclosing->tag_symbol == NULL ||
      enclosing->tag_symbol->type == NULL ||
      enclosing->tag_symbol->type->template_arguments == NULL ||
      enclosing->tag_symbol->type->template_arguments->length == 0) {
    return NULL;
  }
  Vector* parent_args = enclosing->tag_symbol->type->template_arguments;
  bool already_prefixed = own_args != NULL && own_args->length >= parent_args->length;
  // Comparing values cannot tell `Outer<1>::ErrorMaker<true>` apart from an
  // already-prefixed `[1, …]`.  `own_parameters` lists only the nested
  // template's own parameters, so an argument list no longer than it carries
  // no enclosing prefix.
  if (already_prefixed && own_parameters != NULL &&
      own_parameters->length > 0 && own_args->length <= own_parameters->length) {
    already_prefixed = false;
  }
  if (already_prefixed) {
    for (size_t i = 0; i < parent_args->length; i++) {
      if (!TemplateArgumentEqual(own_args->value.p[i], parent_args->value.p[i])) {
        already_prefixed = false;
        break;
      }
    }
  }
  if (already_prefixed) {
    return NULL;
  }
  Vector* combined = NewVector();
  for (size_t i = 0; i < parent_args->length; i++) {
    VectorAppend(combined, TemplateArgumentCopy(parent_args->value.p[i]));
  }
  if (own_args != NULL) {
    for (size_t i = 0; i < own_args->length; i++) {
      VectorAppend(combined, TemplateArgumentCopy(own_args->value.p[i]));
    }
  }
  return combined;
}

/* A substituted base that is not yet a class because it is still an alias-id
 * or a deferred `Trait<Args>::member` type.  Instantiating the enclosing
 * class would diagnose "base class must be a class" for dependent input. */
static bool TypeIsStillDependentClassBase(TypeRecord* type) {
  if (type == NULL) {
    return false;
  }
  if (TypeContainsTemplateParameter(type) ||
      TypeIsUnexpandedAliasTemplateId(type)) {
    return true;
  }
  return type->dependent_member_name != NULL && type->template_origin != NULL;
}

static TypeRecord* MaterializeClassBaseType(TypeParser* parser,
                                           TypeRecord* base_type) {
  if (parser == NULL || base_type == NULL) {
    return base_type;
  }
  TypeRecord* expanded = ExpandConcreteAliasTemplateId(parser, base_type);
  if (expanded != NULL) {
    TypeRecordDelete(base_type);
    base_type = expanded;
  }
  TypeRecord* materialized = TypeMaterializeClassTemplateSpecialization(
      parser->syntax, base_type);
  if (materialized != base_type) {
    TypeRecordDelete(base_type);
    base_type = materialized;
  }
  return base_type;
}

/* Look up typedef `name` on a specialization whose body is still being
 * substituted.  The published shell has no members yet, so read the matching
 * pattern (partial specialization or primary) and, when the typedef is only
 * inherited, the pattern's base. */
TypeRecord* ResolveInProgressClassMemberType(TypeParser* parser, Struct* str,
                                             String* name) {
  if (parser == NULL || str == NULL || name == NULL ||
      !str->instantiation_in_progress || str->resolving_in_progress_member ||
      str->tag_symbol == NULL || str->tag_symbol->type == NULL ||
      str->tag_symbol->type->template_origin == NULL) {
    return NULL;
  }
  Symbol* templ = str->tag_symbol->type->template_origin;
  Vector* completed = str->tag_symbol->type->template_arguments;
  if (templ->type == NULL || !TypeIsStructOrUnion(templ->type) ||
      templ->type->info.struct_info == NULL || completed == NULL) {
    return NULL;
  }
  str->resolving_in_progress_member = true;
  Vector* partial_args = NULL;
  ClassTemplatePartialSpecialization* partial =
      SelectClassTemplatePartialSpecialization(parser, templ, completed,
                                               &partial_args);
  Struct* source = templ->type->info.struct_info;
  Vector* source_args = completed;
  if (partial != NULL && partial->tag_symbol != NULL &&
      partial->tag_symbol->type != NULL &&
      TypeIsStructOrUnion(partial->tag_symbol->type) &&
      partial->tag_symbol->type->info.struct_info != NULL &&
      partial_args != NULL) {
    source = partial->tag_symbol->type->info.struct_info;
    source_args = partial_args;
  }
  TypeRecord* result = NULL;
  StructMember* member = source != NULL ? FindStructMember(source, name) : NULL;
  if (member != NULL && member->symbol != NULL &&
      member->symbol->type != NULL &&
      StorageIs(member->symbol->storage, STO(typedef))) {
    result = SubstituteTemplateParameters(parser, member->symbol->type,
                                          source_args);
  } else if (source != NULL) {
    for (size_t i = 0; i < source->bases.length && result == NULL; i++) {
      CXXBaseSpecifier* base = source->bases.value.p[i];
      if (base == NULL || base->type == NULL || base->is_pack_expansion) {
        continue;
      }
      TypeRecord* base_type =
          SubstituteTemplateParameters(parser, base->type, source_args);
      base_type = MaterializeClassBaseType(parser, base_type);
      if (base_type != NULL && TypeIsStructOrUnion(base_type) &&
          base_type->info.struct_info != NULL) {
        Struct* base_struct = base_type->info.struct_info;
        StructMember* inherited = FindStructMember(base_struct, name);
        if (inherited != NULL && inherited->symbol != NULL &&
            inherited->symbol->type != NULL &&
            StorageIs(inherited->symbol->storage, STO(typedef))) {
          result = TypeRecordCopy(inherited->symbol->type);
        } else if (base_struct->instantiation_in_progress) {
          result =
              ResolveInProgressClassMemberType(parser, base_struct, name);
        }
      }
      TypeRecordDelete(base_type);
    }
  }
  if (partial_args != NULL) {
    VectorDeleteWithContents(partial_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  str->resolving_in_progress_member = false;
  if (result != NULL && parser->syntax != NULL) {
    TypeRecord* materialized =
        TypeMaterializeClassTemplateSpecialization(parser->syntax, result);
    if (materialized != result) {
      TypeRecordDelete(result);
      result = materialized;
    }
  }
  return result;
}

/* Instantiate a class template `templ` with arguments `args`, returning the
 * concrete struct/union type (memoized by instantiation name so each unique
 * argument set is built once). Steps: handle alias templates; complete default
 * arguments; reuse an existing instantiation tag if present; select the best
 * partial specialization; create the instantiated struct tag; then instantiate
 * bases and members (expanding base/member packs) and lay out the struct. */
TypeRecord* InstantiateSimpleClassTemplate(TypeParser* parser,
                                                  Symbol* templ,
                                                  Vector* args) {
  return InstantiateSimpleClassTemplateImpl(parser, templ, args,
                                            /*emit_constraint_error=*/true);
}

static TypeRecord* InstantiateSimpleClassTemplateImpl(
    TypeParser* parser, Symbol* templ, Vector* args,
    bool emit_constraint_error) {
  TypeRecord* alias_type =
      InstantiateAliasClassTemplateImpl(parser, templ, args,
                                        emit_constraint_error);
  if (alias_type != NULL) {
    return alias_type;
  }
  if (templ == NULL || templ->type == NULL) {
    return NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  }
  bool saved_alias_failed = parser->template_substitution_failed;
  parser->template_substitution_failed = false;
  alias_type = InstantiateGenericAliasTemplate(parser, templ, args,
                                               emit_constraint_error);
  bool alias_failed = parser->template_substitution_failed;
  parser->template_substitution_failed = saved_alias_failed || alias_failed;
  if (alias_type != NULL) {
    return alias_type;
  }
  // `enable_if_t<false, int>` has no `::type`.  Returning the unresolved
  // pattern here made `enable_if_t<false, int>()` value-initialize as 0, so
  // every expression-form SFINAE overload stayed viable.
  if (alias_failed) {
    return NULL;
  }
  // A non-forwarding alias whose pattern is a class-template specialization
  // (`using CanConvert = TrueAlias<enable_if_t<...>>`, `using AlwaysTrue =
  // integral_constant<bool, true>`) must not fall through into class-template
  // instantiation: that would apply the alias's arguments to the *pattern's*
  // class.  Forwarding aliases (`using Vec = vector<T>`) still fall through
  // when the dedicated alias path above returns NULL.
  if (templ->flags.is_template && StorageIs(templ->storage, STO(typedef)) &&
      !CXXAliasTemplatePatternNamesClassTemplate(templ) &&
      TypeIsStructOrUnion(templ->type) &&
      templ->type->info.struct_info != NULL &&
      templ->type->info.struct_info->is_template &&
      templ->type->template_arguments != NULL) {
    return TypeRecordCopy(templ->type);
  }
  if (!TypeIsStructOrUnion(templ->type) ||
      templ->type->info.struct_info == NULL ||
      !templ->type->info.struct_info->is_template) {
    return TypeRecordCopy(templ->type);
  }
  Struct* template_struct = templ->type->info.struct_info;
  Vector* completed_args =
      CompleteClassTemplateArguments(parser, template_struct, args);
  if (completed_args == NULL) {
    return NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  }
  // Expand alias-id arguments (`Elem<int, int&&>`) to their underlying class
  // types before instantiation.  `conjunction<B1>` inherits from `B1`, so an
  // unexpanded alias is not a valid base.  If any argument is still dependent
  // after that (including an alias whose pattern mentions a template
  // parameter), keep the template-id deferred rather than instantiating the
  // primary or a partial spec against incomplete input.
  ExpandConcreteAliasTemplateArguments(parser, completed_args);
  FoldConcreteNonTypeTemplateArguments(parser, completed_args);
  if (ClassTemplateArgumentsAreStillDependent(completed_args)) {
    TypeRecord* deferred =
        NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
    deferred->template_origin = templ;
    deferred->template_arguments = TemplateArgumentVectorCopy(completed_args);
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return deferred;
  }

  Vector* partial_args = NULL;
  ClassTemplatePartialSpecialization* partial =
      SelectClassTemplatePartialSpecialization(parser, templ, completed_args,
                                               &partial_args);
  ConstraintExpr* active_constraint = template_struct->associated_constraint;
  Vector* constraint_args = completed_args;
  if (partial != NULL) {
    active_constraint = partial->associated_constraint;
    constraint_args = partial_args;
  }
  if (!ConceptsConstraintSatisfied(active_constraint, constraint_args)) {
    TypeRecord* failure = ClassTemplateConstraintFailureType(
        parser, templ, active_constraint, constraint_args,
        emit_constraint_error);
    if (partial_args != NULL) {
      VectorDeleteWithContents(partial_args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return failure;
  }

  String instantiated_name;
  StringInit(&instantiated_name, NULL);
  AppendTemplateInstantiationName(&instantiated_name, templ, completed_args);
  Symbol* existing =
      FindTemplateInstantiationTag(parser, templ, &instantiated_name);
  if (existing != NULL && existing->type != NULL &&
      TypeIsStructOrUnion(existing->type) &&
      existing->type->info.struct_info != NULL &&
      !existing->type->info.struct_info->is_template) {
    StringDestruct(&instantiated_name);
    if (partial_args != NULL) {
      VectorDeleteWithContents(partial_args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return TypeRecordCopy(existing->type);
  }
  if (existing != NULL) {
    SyntaxError(parser->syntax,
                "Class template instantiation conflicts with existing tag %s",
                instantiated_name.value);
    StringDestruct(&instantiated_name);
    if (partial_args != NULL) {
      VectorDeleteWithContents(partial_args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return TypeRecordCopy(templ->type);
  }
  Struct* source_struct = template_struct;
  Vector* source_args = completed_args;
  if (partial != NULL && partial->tag_symbol != NULL &&
      partial->tag_symbol->type != NULL &&
      TypeIsStructOrUnion(partial->tag_symbol->type) &&
      partial->tag_symbol->type->info.struct_info != NULL) {
    source_struct = partial->tag_symbol->type->info.struct_info;
    source_args = partial_args;
  }

  // When the primary template has only been forward-declared (its body not yet
  // parsed) and it has no partial specializations, do not materialize -- and
  // above all do not *cache* -- a concrete specialization now.  The primary's
  // member set is empty, so caching it would permanently shadow the real
  // specialization once the definition is seen.  This arises when another
  // template's member signature names a class template that is completed later
  // in the TU (e.g. std::basic_string's conversion to the still-forward-
  // declared std::basic_string_view).  Return the deferred template-id
  // representation (a copy of the primary template type carrying the concrete
  // arguments) exactly as SubstituteTemplateIdType does for still-dependent
  // arguments; TypeMaterializeClassTemplateSpecialization then re-instantiates
  // it against the completed definition once the type is required to be
  // complete.
  //
  // The no-partial-specialization guard is essential: a deliberately
  // incomplete traits primary (e.g. std::tuple_size, declared once and only
  // ever completed through partial/explicit specializations) must still
  // instantiate to its empty primary here for arguments that match no
  // specialization -- deferring it would leave consumers that immediately need
  // the (empty) type, such as the structured-bindings `tuple_size<E>::value`
  // lookup, unable to complete it.
  if (partial == NULL && template_struct->tag_symbol != NULL &&
      !template_struct->tag_symbol->flags.is_defined &&
      template_struct->partial_specializations.length == 0) {
    StringDestruct(&instantiated_name);
    TypeRecord* deferred = TypeRecordCopy(templ->type);
    if (deferred->template_arguments != NULL) {
      VectorDeleteWithContents(deferred->template_arguments,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    deferred->template_origin = templ;
    deferred->template_arguments = TemplateArgumentVectorCopy(completed_args);
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return deferred;
  }

  if (!ClassTemplateInstantiationMembersSupported(parser, source_struct)) {
    StringDestruct(&instantiated_name);
    if (partial_args != NULL) {
      VectorDeleteWithContents(partial_args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return TypeRecordCopy(templ->type);
  }

  Struct* str = NewStruct(source_struct->is_union);
  str->is_class = source_struct->is_class;
  str->lexical_parent = source_struct->lexical_parent;
  str->access_enclosing_function = source_struct->access_enclosing_function;
  // Only remap the parent when this specialization is a nested class of the
  // class template currently being instantiated.  Applying the enclosing
  // target to every class materialized during that instantiation would make
  // `InlinedVector` (and other unrelated templates) look nested in
  // `FormatSpecTemplate` and prefix the wrong arguments onto their members.
  if (parser->enclosing_template_substitution_target != NULL &&
      source_struct->lexical_parent != NULL &&
      source_struct->lexical_parent ==
          parser->enclosing_template_substitution_source) {
    str->lexical_parent = parser->enclosing_template_substitution_target;
  }
  str->packed = source_struct->packed;
  str->explicit_alignment = source_struct->explicit_alignment;
  str->pack = source_struct->pack;
  // Carry friend classes from the template to each instantiation verbatim;
  // friend *functions* are instantiated per specialization below, after the
  // members are in place, so their dependent signatures and inline bodies can
  // be substituted with the template arguments.
  for (size_t i = 0; i < source_struct->friend_classes.length; i++) {
    StructAddFriendClass(str, source_struct->friend_classes.value.p[i]);
  }
  TypeRecord* type = NewTypeRecord(source_struct->is_union ? kTypeUnion
                                                           : kTypeStruct,
                                  kQualPlain);
  TypeRecordSetStructInfo(type, str);
  type->template_origin = templ;
  type->template_arguments = TemplateArgumentVectorCopy(completed_args);
  Symbol* tag = NewSymbol(instantiated_name.value, type, STO(implicit));
  tag->flags.is_defined = true;
  if (source_struct->tag_symbol != NULL) {
    VectorDestruct(&tag->attributes);
    AttributeListClone(&tag->attributes,
                       &source_struct->tag_symbol->attributes);
    SubstituteDependentSymbolAlignment(parser, tag, source_args);
    if (tag->alignment > str->explicit_alignment) {
      str->explicit_alignment = tag->alignment;
    }
  }
  str->tag_name = &tag->name;
  str->tag_symbol = tag;
  if (!AddTemplateInstantiationTag(parser, templ, tag)) {
    SyntaxError(parser->syntax,
                "Class template instantiation conflicts with existing tag %s",
                instantiated_name.value);
    StringDestruct(&instantiated_name);
    if (partial_args != NULL) {
      VectorDeleteWithContents(partial_args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return TypeRecordCopy(templ->type);
  }
  str->instantiation_in_progress = true;

  Struct* saved_substitution_source = parser->template_substitution_source;
  Struct* saved_substitution_target = parser->template_substitution_target;
  Struct* saved_enclosing_substitution_source =
      parser->enclosing_template_substitution_source;
  Struct* saved_enclosing_substitution_target =
      parser->enclosing_template_substitution_target;
  // Member push/pop scopes below publish this class's substitution to the
  // syntax.  Put the caller's back afterwards: a later alias expansion in the
  // caller (`IteratorValueAdapter<A, I>(it)` in a member of InlinedVector)
  // would otherwise resolve members against this class.
  Struct* saved_syntax_substitution_source =
      parser->syntax->template_substitution_source;
  Struct* saved_syntax_substitution_target =
      parser->syntax->template_substitution_target;
  Struct* saved_syntax_enclosing_substitution_source =
      parser->syntax->enclosing_template_substitution_source;
  Struct* saved_syntax_enclosing_substitution_target =
      parser->syntax->enclosing_template_substitution_target;
  if (saved_substitution_source != NULL && saved_substitution_target != NULL) {
    parser->enclosing_template_substitution_source =
        saved_substitution_source;
    parser->enclosing_template_substitution_target =
        saved_substitution_target;
  }
  parser->template_substitution_source = source_struct;
  parser->template_substitution_target = str;
  // Member-template default arguments are completed against the primary,
  // whose parameter indices still include this class.  Remember the concrete
  // arguments so that completion can fill those indices.
  PushNestedSubstitution(source_struct, source_args, type);

  for (size_t i = 0; i < source_struct->bases.length; i++) {
    CXXBaseSpecifier* template_base = source_struct->bases.value.p[i];
    int pack_index = -1;
    size_t pack_length = 0;
    if (template_base->is_pack_expansion &&
        TypeIsTemplateParameterPlaceholder(template_base->type, &pack_index) &&
        pack_index >= 0 && (size_t)pack_index < source_args->length) {
      TemplateArgument* pack = source_args->value.p[pack_index];
      if (pack != NULL && pack->pack_arguments != NULL) {
        for (size_t j = 0; j < pack->pack_arguments->length; j++) {
          TemplateArgument* element = pack->pack_arguments->value.p[j];
          if (element == NULL || element->kind != kTemplateParameterType ||
              element->type == NULL || !TypeIsStructOrUnion(element->type)) {
            SyntaxError(parser->syntax,
                        "Base class pack expansion requires class types");
            continue;
          }
          TypeRecord* base_type = TypeRecordCopy(element->type);
          TypeRecordCalculateSize(base_type);
          VectorAppend(&str->bases,
                       NewCXXBaseSpecifier(base_type, template_base->access,
                                           template_base->is_virtual));
        }
        continue;
      }
    }
    if (template_base->is_pack_expansion &&
        FindPackExpansionInType(template_base->type, source_args, &pack_index,
                                &pack_length) &&
        pack_index >= 0 && pack_length > 0) {
      for (size_t j = 0; j < pack_length; j++) {
        TypeRecord* base_type = SubstituteTemplateParametersForPackElement(
            parser, template_base->type, source_args, pack_index, j);
        base_type = MaterializeClassBaseType(parser, base_type);
        if (!TypeIsStructOrUnion(base_type)) {
          if (TypeIsStillDependentClassBase(base_type)) {
            TypeRecordDelete(base_type);
            continue;
          }
          SyntaxError(parser->syntax,
                      "base class must be a class or struct type");
          TypeRecordDelete(base_type);
          continue;
        }
        TypeRecordCalculateSize(base_type);
        VectorAppend(&str->bases,
                     NewCXXBaseSpecifier(base_type, template_base->access,
                                         template_base->is_virtual));
      }
      continue;
    }
    TypeRecord* base_pattern = TypeRecordCopy(template_base->type);
    int nested_parameter_base = 0;
    if (source_struct->template_parameters.length > 0) {
      TemplateParameter* first = source_struct->template_parameters.value.p[0];
      if (first != NULL && first->index > 0) {
        nested_parameter_base = first->index;
      }
    }
    RebaseTemplateParameterIndices(base_pattern, nested_parameter_base);
    TypeRecord* base_type =
        SubstituteTemplateParameters(parser, base_pattern, source_args);
    TypeRecordDelete(base_pattern);
    base_type = MaterializeClassBaseType(parser, base_type);
    if (!TypeIsStructOrUnion(base_type)) {
      if (TypeIsStillDependentClassBase(base_type)) {
        TypeRecordDelete(base_type);
        continue;
      }
      SyntaxError(parser->syntax, "base class must be a class or struct type");
      TypeRecordDelete(base_type);
      continue;
    }
    TypeRecordCalculateSize(base_type);
    VectorAppend(&str->bases,
                 NewCXXBaseSpecifier(base_type, template_base->access,
                                     template_base->is_virtual));
  }
  CollectCXXVirtualBases(str);
  CopyCXXBaseVirtualMembers(str);
  LayoutCXXBaseSpecifiers(str);
  ApplyCXXMemberUsingDeclarations(parser, str, source_struct, source_args);
  ApplyFriendTypeDeclarations(parser, str, source_struct, source_args);

  // Member function bodies are cloned in a second pass, after every member
  // function signature has been added to `str`, so that a member's body may
  // reference other members declared later in the class (e.g. `operator=`
  // calling a later-declared `emplace`).
  Vector pending_member_bodies;
  VectorInit(&pending_member_bodies);
  EnsureDeferredNestedMemberBodies();
  size_t deferred_nested_body_mark = g_deferred_nested_member_bodies.length;
  // Nested classes whose deferred hidden friends must be materialized once the
  // enclosing instantiation is complete (parallel source/target vectors).
  Vector pending_nested_friend_sources;
  Vector pending_nested_friend_targets;
  VectorInit(&pending_nested_friend_sources);
  VectorInit(&pending_nested_friend_targets);
  for (size_t i = 0; i < source_struct->members.length; i++) {
    StructMember* member = source_struct->members.value.p[i];
    // Layout-only fields from the primary template are not source-level
    // members.  Recreate them for the specialization after its concrete base
    // graph and virtual-base set are known.
    if (member == source_struct->vptr_member ||
        member == source_struct->vbptr_member) {
      continue;
    }
    if (StructMemberIsNestedType(member)) {
      TypeRecord* nested_type =
          SubstituteTemplateParameters(parser, member->symbol->type,
                                       source_args);
      // A member type alias may substitute to a lazy dependent-member type such
      // as `typename ratio<1,1000>::type` (from `using period = typename
      // Period::type;`).  Now that the arguments are concrete, collapse it to
      // the real class specialization so later qualified accesses through this
      // alias (e.g. `To::period::num`) resolve to the specialization's members
      // and fold, instead of emitting an unresolved bare `num`/`den` symbol.
      if (!TypeContainsTemplateParameter(nested_type)) {
        nested_type = TypeMaterializeClassTemplateSpecialization(
            parser->syntax, nested_type);
      }
      if (TypeIsStructOrUnion(nested_type) &&
          nested_type->info.struct_info != NULL &&
          member->symbol->type != NULL &&
          TypeIsStructOrUnion(member->symbol->type) &&
          member->symbol->type->info.struct_info != NULL &&
          member->symbol->type->info.struct_info->lexical_parent ==
              source_struct) {
        Struct* nested_source = member->symbol->type->info.struct_info;
        Struct* nested_target = nested_type->info.struct_info;
        nested_target->lexical_parent = str;
        for (size_t j = 0; j < nested_target->members.length; j++) {
          StructMember* nested_func_member = nested_target->members.value.p[j];
          for (StructMember* overload = nested_func_member; overload != NULL;
               overload = overload->overload_next) {
            if (overload->is_member_function && overload->symbol != NULL) {
              SymbolSetCXXMangledAsmName(overload->symbol);
            }
          }
        }
        // A nested class (e.g. a view's __iterator/__sentinel) may declare
        // hidden-friend operators.  Defer materializing them until the whole
        // enclosing class is formed (below), so a sentinel's friend that names
        // the iterator type resolves against the already-instantiated sibling.
        VectorAppend(&pending_nested_friend_sources, nested_source);
        VectorAppend(&pending_nested_friend_targets, nested_target);
      }
      Symbol* nested_symbol =
          NewSymbol(member->symbol->name.value, nested_type, STO(typedef));
      nested_symbol->flags = member->symbol->flags;
      nested_symbol->location = member->symbol->location;
      // A member alias template (`template<int I> using StorageT = ...`)
      // keeps its own parameter list after the enclosing class is
      // instantiated.  Dropping it makes `StorageT<0>` complete against the
      // pattern (`Storage<ElemT<I>, I, Tag>`) and instantiate Storage with
      // `I` in the element-type slot.
      CopySymbolAliasTemplate(nested_symbol, member->symbol);
      StructMember* nested_member = NewStructMember(nested_symbol);
      nested_member->access = member->access;
      AddStructMember(parser, str, nested_member);
      continue;
    }
    if (member->is_member_function) {
      StructMember* instantiated = InstantiateTemplateMemberFunction(
          parser, str, member, source_args, &pending_member_bodies);
      StructMember* existing =
          MapFindPointerKey(&str->symbol_table, &instantiated->symbol->name);
      if (existing != NULL) {
        AppendStructMemberOverload(parser, str, existing, instantiated);
      } else {
        AddStructMember(parser, str, instantiated);
      }
      continue;
    }
    TypeRecord* member_type =
        SubstituteTemplateParameters(parser, member->symbol->type,
                                     source_args);
    TypeRecordCalculateSize(member_type);
    Symbol* member_symbol =
        NewSymbol(member->symbol->name.value, member_type, member->symbol->storage);
    member_symbol->location = member->symbol->location;
    member_symbol->flags = member->symbol->flags;
    CopySymbolAliasTemplate(member_symbol, member->symbol);
    member_symbol->value = member->symbol->value;
    member_symbol->dependent_value_template_parameter_index =
        member->symbol->dependent_value_template_parameter_index;
    VectorDestruct(&member_symbol->attributes);
    AttributeListClone(&member_symbol->attributes,
                       &member->symbol->attributes);
    SubstituteDependentSymbolValue(member_symbol, source_args);
    SubstituteDependentSymbolAlignment(parser, member_symbol, source_args);
    SubstituteStaticMemberInitializerValue(parser, member_symbol,
                                           member->default_initializer,
                                           source_args);
    if (!member_symbol->flags.value_set &&
        member->symbol->constexpr_initializer != NULL) {
      SubstituteStaticMemberInitializerValue(
          parser, member_symbol, member->symbol->constexpr_initializer,
          source_args);
    }
    StructMember* instantiated = NewStructMember(member_symbol);
    // Substitute template parameters in every member initializer. For a static
    // member the instantiated initializer is retained until an odr-use queues
    // its storage; for a non-static member it is used by constructor lowering.
    ASTNode* member_initializer = member->default_initializer;
    if (member->is_static && member->symbol->variable_template != NULL &&
        member->symbol->variable_template->initializer != NULL) {
      member_initializer = member->symbol->variable_template->initializer;
    }
    if (member_initializer != NULL) {
      ASTNode* substituted = CloneDependentExpressionWithArgs(
          parser, member_initializer, source_args);
      instantiated->default_initializer =
          substituted != NULL
              ? substituted
              : CloneCXXDefaultMemberInitializer(member_initializer);
    } else {
      instantiated->default_initializer =
          CloneCXXDefaultMemberInitializer(member_initializer);
    }
    if (member->is_static && !member_symbol->flags.value_set &&
        member_symbol->constexpr_initializer == NULL &&
        instantiated->default_initializer != NULL &&
        !DependentExpressionContainsTemplateParameter(
            instantiated->default_initializer)) {
      member_symbol->constexpr_initializer = ASTNodeClone(
          instantiated->default_initializer, IdentityCloneNode, NULL, NULL);
    }
    RecordStaticDataMemberPattern(member, member_symbol, source_args);
    instantiated->access = member->access;
    instantiated->is_anon = member->is_anon;
    instantiated->is_static = member->is_static;
    instantiated->is_mutable = member->is_mutable;
    instantiated->is_member_function = member->is_member_function;
    instantiated->is_using_declaration = member->is_using_declaration;
    instantiated->bit_size = member->bit_size;
    instantiated->is_bit_field = member->is_bit_field;
    instantiated->bit_offset = member->bit_offset;
    instantiated->cxx_vcall_offset = member->cxx_vcall_offset;
    if (!instantiated->is_static && !instantiated->is_using_declaration &&
        !StructMemberIsNestedType(instantiated)) {
      AlignNextOffsetForSymbol(str, member_symbol);
      instantiated->byte_offset = str->next_offset;
    } else {
      instantiated->byte_offset = member->byte_offset;
    }
    instantiated->index = str->members.length;
    if (instantiated->is_anon) {
      VectorAppend(&str->members, instantiated);
    } else {
      AddStructMember(parser, str, instantiated);
    }
    if (!instantiated->is_static && !instantiated->is_using_declaration &&
        !StructMemberIsNestedType(instantiated)) {
      UpdateStructSize(str, member_type, str->is_union);
    }
  }
  if (StructHasBitFieldMembers(source_struct)) {
    ResolveInstantiatedBitFieldWidths(parser, source_struct, str, source_args);
    RelayoutStruct(str);
  }
  InjectInstantiatedAnonymousMembers(parser, str);
  FinalizeStructAlignment(str);
  TypeRecordCalculateSize(type);
  ComputeCXXAggregateStatus(str);
  // Same as the nested-struct path: a class template can be instantiated
  // while an enclosing template is still being parsed.  Synthesize special
  // members for this concrete specialization anyway.
  bool saved_synthesize = parser->synthesize_instantiated_special_members;
  parser->synthesize_instantiated_special_members = true;
  AddImplicitCXXSpecialMembers(parser, str, tag);
  AddImplicitCXXDestructorIfNeeded(parser, str, tag);
  parser->synthesize_instantiated_special_members = saved_synthesize;
  AddImplicitCXXDeductionGuides(str, tag);
  // Complete the polymorphic layout of the instantiation exactly as a normal
  // class definition does (type_class.c): insert the hidden vptr/vbptr, lay out
  // virtual bases, and emit the vtable(s) and virtual-base table(s).  Without
  // this an instantiated class template with virtual members would carry a
  // virtual_members slot map but no vptr field and no emitted vtable, so its
  // constructors could not initialize the vptr and virtual calls would read
  // garbage.  This runs before the member-body clone pass below so that the
  // cloned constructor preambles observe vtables_registered and emit their vptr
  // initializers directly.
  CompleteInstantiatedPolymorphicLayout(parser, str);
  TypeRecordCalculateSize(type);
  // Nested hidden friends (iterator/const_iterator operator==) must exist
  // before the enclosing class's own friend bodies are cloned: those bodies
  // compare the nested iterators.  Sibling nested types are already formed, so
  // a sentinel friend that names the iterator type still resolves.
  for (size_t i = 0; i < pending_nested_friend_sources.length; i++) {
    Struct* nested_source = pending_nested_friend_sources.value.p[i];
    Struct* nested_target = pending_nested_friend_targets.value.p[i];
    InstantiateTemplateFriendFunctionsImpl(parser, nested_target, nested_source,
                                           source_struct, str, source_args);
  }
  InstantiateTemplateFriendFunctions(parser, str, source_struct, source_args);
  VectorDestruct(&pending_nested_friend_sources);
  VectorDestruct(&pending_nested_friend_targets);
  // Second pass: with the class fully formed (all members, layout, and implicit
  // special members in place), clone the deferred member function bodies so a
  // body may reference any other member regardless of declaration order.
  Vector* member_body_args = source_args;
  Struct* enclosing_for_args = NULL;
  if (parser->enclosing_template_substitution_target != NULL &&
      source_struct->lexical_parent ==
          parser->enclosing_template_substitution_source) {
    enclosing_for_args = parser->enclosing_template_substitution_target;
  } else if (str->lexical_parent != NULL && !str->lexical_parent->is_template) {
    enclosing_for_args = str->lexical_parent;
  }
  Vector* prefixed_member_body_args = PrefixEnclosingClassTemplateArguments(
      enclosing_for_args,
      partial != NULL ? NULL : &template_struct->template_parameters,
      source_args);
  if (prefixed_member_body_args != NULL) {
    member_body_args = prefixed_member_body_args;
  }
  for (size_t i = 0; i < pending_member_bodies.length; i++) {
    PendingMemberBody* pmb = pending_member_bodies.value.p[i];
    // A member template of a nested template cloned for the enclosing
    // instantiation (`Init<Arg>` in `MB<int>::VP`) already has the enclosing
    // arguments bound and numbers `Arg` after the nested template's own
    // parameters alone, so the enclosing prefix would shift every index.
    Vector* body_args = member_body_args;
    TypeRecord* definition_type = pmb->template_definition != NULL
                                      ? pmb->template_definition->type
                                      : NULL;
    if (prefixed_member_body_args != NULL && source_args != NULL &&
        definition_type != NULL && TypeIsFunction(definition_type) &&
        definition_type->info.function.template_parameters.length > 0 &&
        definition_type->info.function.template_parameter_base ==
            (int)source_args->length) {
      body_args = source_args;
    }
    if (pmb->template_definition == NULL) {
      RecordLateMemberBody(str, pmb, body_args);
    }
    CloneInstantiatedMemberFunctionBody(parser, str, pmb->symbol,
                                        pmb->template_definition,
                                        pmb->substitution_source, body_args);
    free(pmb);
  }
  FlushDeferredNestedMemberBodies(parser, deferred_nested_body_mark);
  EvaluateInstantiatedStaticAsserts(parser, source_struct, str,
                                    member_body_args);
  if (prefixed_member_body_args != NULL) {
    VectorDeleteWithContents(prefixed_member_body_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  VectorDestruct(&pending_member_bodies);
  str->instantiation_in_progress = false;
  PopNestedSubstitution(source_struct);
  parser->template_substitution_source = saved_substitution_source;
  parser->template_substitution_target = saved_substitution_target;
  parser->enclosing_template_substitution_source =
      saved_enclosing_substitution_source;
  parser->enclosing_template_substitution_target =
      saved_enclosing_substitution_target;
  parser->syntax->template_substitution_source =
      saved_syntax_substitution_source;
  parser->syntax->template_substitution_target =
      saved_syntax_substitution_target;
  parser->syntax->enclosing_template_substitution_source =
      saved_syntax_enclosing_substitution_source;
  parser->syntax->enclosing_template_substitution_target =
      saved_syntax_enclosing_substitution_target;
  StringDestruct(&instantiated_name);
  if (partial_args != NULL) {
    VectorDeleteWithContents(partial_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  VectorDeleteWithContents(completed_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return TypeRecordCopy(type);
}

/* Public entry point: instantiate class template `templ` with `args`. */
static void TypeParserCopySubstitutionFromSyntax(TypeParser* parser,
                                                 Syntax* syntax) {
  if (parser == NULL || syntax == NULL) {
    return;
  }
  parser->template_substitution_source = syntax->template_substitution_source;
  parser->template_substitution_target = syntax->template_substitution_target;
  parser->enclosing_template_substitution_source =
      syntax->enclosing_template_substitution_source;
  parser->enclosing_template_substitution_target =
      syntax->enclosing_template_substitution_target;
  if ((parser->template_substitution_target == NULL ||
       parser->template_substitution_source == NULL) &&
      compiler != NULL && compiler->current_function != NULL &&
      TypeIsFunction(compiler->current_function) &&
      compiler->current_function->info.function.cxx_member_owner != NULL) {
    Struct* owner = compiler->current_function->info.function.cxx_member_owner;
    parser->template_substitution_target = owner;
    if (owner->tag_symbol != NULL && owner->tag_symbol->type != NULL &&
        owner->tag_symbol->type->template_origin != NULL &&
        owner->tag_symbol->type->template_origin->type != NULL &&
        TypeIsStructOrUnion(
            owner->tag_symbol->type->template_origin->type)) {
      parser->template_substitution_source =
          owner->tag_symbol->type->template_origin->type->info.struct_info;
    }
  }
}

TypeRecord* TypeInstantiateClassTemplate(Syntax* syntax, Symbol* templ,
                                         Vector* args) {
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                 syntax->context);
  TypeParserCopySubstitutionFromSyntax(&parser, syntax);
  TypeRecord* type = InstantiateSimpleClassTemplateImpl(&parser, templ, args,
                                                        /*emit_constraint_error=*/true);
  TypeParserDestruct(&parser);
  return type;
}

/* Like TypeInstantiateClassTemplate, but rejects unsatisfied constraints
 * without emitting diagnostics (for speculative CTAD/deduction). */
TypeRecord* TypeInstantiateClassTemplateQuiet(Syntax* syntax, Symbol* templ,
                                              Vector* args) {
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                 syntax->context);
  TypeParserCopySubstitutionFromSyntax(&parser, syntax);
  TypeRecord* type = InstantiateSimpleClassTemplateImpl(&parser, templ, args,
                                                        /*emit_constraint_error=*/false);
  TypeParserDestruct(&parser);
  return type;
}

/* See TypeMaterializeClassTemplateSpecialization.  Recurses through
 * pointer/reference spines so a capture field typed `variant<int,long>*` is
 * rewritten to point at the concrete specialization. */
TypeRecord* TypeMaterializeClassTemplateSpecialization(Syntax* syntax,
                                                       TypeRecord* type) {
  if (syntax == NULL || type == NULL || !CompilerIsCXX()) {
    return type;
  }
  if (type->is_pack_index && type->pack_index_pack != NULL) {
    Vector no_arguments;
    VectorInit(&no_arguments);
    TypeRecord* selected =
        TypeSubstituteTemplateType(syntax, type, &no_arguments);
    VectorDestruct(&no_arguments);
    if (selected != NULL && !selected->is_pack_index) {
      return selected;
    }
    TypeRecordDelete(selected);
  }
  if (TypeIsPointer(type) || TypeIsReference(type)) {
    TypeRecord* next =
        TypeMaterializeClassTemplateSpecialization(syntax, type->next);
    if (next == type->next) {
      return type;
    }
    TypeRecord* copy = TypeRecordCopy(type);
    TypeRecordIncRef(next);
    TypeRecordDelete(copy->next);
    copy->next = next;
    copy->type = next != NULL ? next->type : copy->type;
    return TypeRecordCalculateSize(copy);
  }
  if (TypeIsFunction(type)) {
    TypeRecord* ret =
        TypeMaterializeClassTemplateSpecialization(syntax, type->next);
    TypeRecord* func = type;
    if (ret != type->next) {
      func = TypeRecordCopy(type);
      TypeRecordIncRef(ret);
      TypeRecordDelete(func->next);
      func->next = ret;
    }
    for (size_t i = 0; i < func->info.function.prototype.length; i++) {
      Symbol* formal = func->info.function.prototype.value.p[i];
      if (formal == NULL || formal->type == NULL) {
        continue;
      }
      TypeRecord* param =
          TypeMaterializeClassTemplateSpecialization(syntax, formal->type);
      if (param != formal->type) {
        // SymbolSetType retains `param`.  Deleting it frees the type the
        // parameter now points at.
        SymbolSetType(formal, param);
      }
    }
    return func;
  }
  if (type->template_origin != NULL && type->template_arguments != NULL &&
      !TemplateArgumentVectorContainsTemplateParameter(
          type->template_arguments) &&
      (type->dependent_member_name != NULL || !TypeIsStructOrUnion(type))) {
    TypeRecord* owner = TypeInstantiateClassTemplate(
        syntax, type->template_origin, type->template_arguments);
    if (owner != NULL && TypeIsStructOrUnion(owner) &&
        owner->info.struct_info != NULL) {
      if (type->dependent_member_name != NULL) {
        StructMember* member =
            FindStructMember(owner->info.struct_info, type->dependent_member_name);
        if (member != NULL && member->symbol != NULL &&
            StorageIs(member->symbol->storage, STO(typedef))) {
          TypeRecord* resolved = TypeRecordCopy(member->symbol->type);
          resolved->qualifiers |= type->qualifiers;
          TypeRecordDelete(owner);
          return TypeRecordCalculateSize(resolved);
        }
      }
      // Instantiated class is the scope for a non-type member such as
      // `conditional_t<B, T, F>::value` / `StorageT<I>::get`.  Also used when
      // the dependent name was encoded as an unknown-int template-id after
      // the member name was stripped for substitution.
      owner->qualifiers |= type->qualifiers;
      return TypeRecordCalculateSize(owner);
    }
    TypeRecordDelete(owner);
  }
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL) {
    return type;
  }
  Struct* str = type->info.struct_info;
  // Concrete specializations are not flagged as templates; only the primary
  // (still carrying unresolved-looking template_arguments) needs materializing.
  if (!str->is_template || str->tag_symbol == NULL) {
    return type;
  }
  Symbol* origin = type->template_origin;
  Vector* args = type->template_arguments;
  if (origin == NULL && str->tag_symbol->type != NULL) {
    origin = str->tag_symbol->type->template_origin;
    if (args == NULL) {
      args = str->tag_symbol->type->template_arguments;
    }
  }
  if (origin == NULL && str->tag_symbol->flags.is_template) {
    origin = str->tag_symbol;
  }
  if (origin == NULL || args == NULL ||
      TemplateArgumentVectorContainsTemplateParameter(args)) {
    return type;
  }
  TypeRecord* concrete = TypeInstantiateClassTemplate(syntax, origin, args);
  if (concrete == NULL) {
    return type;
  }
  concrete->qualifiers |= type->qualifiers;
  return concrete;
}

bool SymbolIsInStdNamespace(Symbol* symbol) {
  return symbol != NULL && symbol->namespace_ != NULL &&
         symbol->namespace_->qualified_name.value != NULL &&
         strcmp(symbol->namespace_->qualified_name.value, "std") == 0;
}

static bool CXXSymbolIsStdInitializerListTemplate(Symbol* symbol) {
  return symbol != NULL && strcmp(symbol->name.value, "initializer_list") == 0 &&
         SymbolIsInStdNamespace(symbol);
}

static bool CXXSymbolNamesStdInitializerList(Symbol* symbol) {
  if (symbol == NULL) {
    return false;
  }
  const char* name = symbol->name.value;
  if (name == NULL ||
      (strcmp(name, "initializer_list") != 0 &&
       strncmp(name, "initializer_list<", 17) != 0)) {
    return false;
  }
  return symbol->namespace_ == NULL ||
         strcmp(symbol->namespace_->qualified_name.value, "std") == 0;
}

bool TypeIsCXXInitializerList(TypeRecord* type) {
  if (!CompilerIsCXX() || type == NULL) {
    return false;
  }
  if (CXXSymbolIsStdInitializerListTemplate(type->template_origin)) {
    return true;
  }
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL ||
      type->info.struct_info->tag_symbol == NULL) {
    return false;
  }
  Symbol* tag = type->info.struct_info->tag_symbol;
  if (CXXSymbolNamesStdInitializerList(tag)) {
    return true;
  }
  if (CXXSymbolIsStdInitializerListTemplate(tag->type != NULL
                                                ? tag->type->template_origin
                                                : NULL)) {
    return true;
  }
  return CXXSymbolIsStdInitializerListTemplate(type->template_origin);
}

/* Public: is `info` an *initializer-list constructor* ([dcl.init.list]/2)?
 * That is a constructor whose first user parameter is `std::initializer_list<E>`
 * (or a reference to one) and whose remaining parameters all have default
 * arguments.  A constructor that merely mentions an initializer_list somewhere
 * later in its parameter list -- `expected(unexpect_t, initializer_list<U>,
 * Args&&...)`, say -- is not one, and must not make `T x{...}` pass the whole
 * braced-init-list as a single argument. */
bool CXXConstructorIsInitializerListConstructor(FunctionInfo* info) {
  if (info == NULL || !info->is_constructor) {
    return false;
  }
  size_t first = 0;
  while (first < info->prototype.length) {
    Symbol* formal = info->prototype.value.p[first];
    if (formal == NULL || (!StringEqual(&formal->name, "this") &&
                           !StringEqual(&formal->name, "__complete_object"))) {
      break;
    }
    first++;
  }
  if (first >= info->prototype.length) {
    return false;
  }
  Symbol* formal = info->prototype.value.p[first];
  if (formal == NULL) {
    return false;
  }
  TypeRecord* formal_type = formal->type;
  if (TypeIsReference(formal_type)) {
    formal_type = formal_type->next;
  }
  if (!TypeIsCXXInitializerList(formal_type)) {
    return false;
  }
  for (size_t i = first + 1; i < info->prototype.length; i++) {
    Symbol* rest = info->prototype.value.p[i];
    if (rest != NULL && rest->default_argument == NULL) {
      return false;
    }
  }
  return true;
}

/* Public: the element type T of a `std::initializer_list<T>` type, or NULL. */
TypeRecord* TypeCXXInitializerListElement(TypeRecord* type) {
  if (!TypeIsCXXInitializerList(type) || type->template_arguments == NULL ||
      type->template_arguments->length != 1) {
    return NULL;
  }
  TemplateArgument* arg = type->template_arguments->value.p[0];
  return arg != NULL && arg->kind == kTemplateParameterType ? arg->type : NULL;
}

/* Public: instantiate `std::initializer_list<element_type>` (used for braced
 * initializer expressions), looking up the std template. Returns NULL if the
 * std type is unavailable. */
TypeRecord* TypeInstantiateCXXInitializerList(Syntax* syntax,
                                              TypeRecord* element_type) {
  if (!CompilerIsCXX() || syntax == NULL || element_type == NULL) {
    return NULL;
  }
  Namespace* std_ns = NamespaceFindStdNamespace();
  if (std_ns == NULL) {
    return NULL;
  }
  String initializer_list_name;
  StringInit(&initializer_list_name, "initializer_list");
  NamespaceInlineSymbolLookup result =
      NamespaceResolveSymbolInInlineSet(std_ns, &initializer_list_name);
  Symbol* templ = result.status == kInlineLookupUnique ? result.symbol : NULL;
  StringDestruct(&initializer_list_name);
  if (!CXXSymbolIsStdInitializerListTemplate(templ) || !templ->flags.is_template) {
    return NULL;
  }
  Vector* args = NewVector();
  TemplateArgument* arg = TemplateArgumentAlloc();
  arg->kind = kTemplateParameterType;
  arg->is_pack_expansion = false;
  arg->type = TypeRecordCopy(element_type);
  arg->int_value = 0;
  arg->template_parameter_index = -1;
  arg->pack_arguments = NULL;
  arg->dependent_expr = NULL;
  arg->location = SOURCE_LOCATION_MISSING;
  VectorAppend(args, arg);
  TypeRecord* type = TypeInstantiateClassTemplate(syntax, templ, args);
  VectorDeleteWithContents(args, (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return type;
}

/* Public: look up a C++20 comparison-category type (e.g. "strong_ordering")
 * declared in namespace std.  Returns the struct TypeRecord, or NULL if the
 * type is not visible (typically because <compare> was not included). */
TypeRecord* TypeFindCXXComparisonCategory(const char* category_name) {
  if (!CompilerIsCXX() || compiler == NULL || category_name == NULL) {
    return NULL;
  }
  Namespace* std_ns = NamespaceFindStdNamespace();
  if (std_ns == NULL) {
    return NULL;
  }
  String name;
  StringInit(&name, category_name);
  NamespaceInlineTagLookup result = NamespaceResolveTagInInlineSet(std_ns, &name);
  Symbol* tag = result.status == kInlineLookupUnique ? result.tag : NULL;
  StringDestruct(&name);
  if (tag == NULL || tag->type == NULL || !TypeIsStructOrUnion(tag->type)) {
    return NULL;
  }
  return tag->type;
}

/* Public entry point: instantiate function template `templ` with `args`. */
Symbol* TypeInstantiateFunctionTemplate(Syntax* syntax, Symbol* templ,
                                        Vector* args) {
  if (!ConceptsFunctionTemplateConstraintsSatisfied(templ, args)) {
    return templ;
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                 syntax->context);
  Symbol* symbol =
      InstantiateSimpleFunctionTemplate(&parser, templ, args,
                                        /*args_are_completed=*/false);
  TypeParserDestruct(&parser);
  return symbol;
}

Symbol* TypeInstantiateFunctionTemplateWithCompletedArguments(
    Syntax* syntax, Symbol* templ, Vector* completed_args) {
  if (!ConceptsFunctionTemplateConstraintsSatisfied(templ, completed_args)) {
    return templ;
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                 syntax->context);
  Symbol* symbol =
      InstantiateSimpleFunctionTemplate(&parser, templ, completed_args,
                                        /*args_are_completed=*/true);
  TypeParserDestruct(&parser);
  return symbol;
}

/* True if a pattern argument is a bare template parameter (`T`, `I`,
 * `Ts...`) rather than a computed type (`ElemT<I>`, `Tag<Ts...>`).  A
 * forwarding alias (`using Vec = vector<T>`) uses only bare parameters;
 * `using StorageT = Storage<ElemT<I>, I, Tag>` does not. */
static bool AliasPatternArgumentIsBareParameter(TemplateArgument* arg) {
  if (arg == NULL) {
    return false;
  }
  if (arg->kind == kTemplateParameterNonType) {
    return arg->template_parameter_index >= 0 && arg->dependent_expr == NULL &&
           (arg->type == NULL || arg->type->template_origin == NULL);
  }
  if (arg->kind == kTemplateParameterTemplate) {
    return arg->template_parameter_index >= 0;
  }
  if (arg->kind != kTemplateParameterType || arg->type == NULL) {
    return false;
  }
  return TypeIsTemplateParameterPlaceholder(arg->type, NULL);
}

/* True if `alias` is an alias template whose right-hand side is itself a class
 * template specialization (e.g. `using X = vector<T>;`), as opposed to a plain
 * type alias. Such aliases participate in class-template instantiation/CTAD. */
bool CXXAliasTemplatePatternNamesClassTemplate(Symbol* alias) {
  if (!CompilerIsCXX() || alias == NULL || !alias->flags.is_template ||
      !StorageIs(alias->storage, STO(typedef)) || alias->type == NULL ||
      (alias->alias_template != NULL &&
       alias->alias_template->ctad_names_template_template_parameter) ||
      alias->type->dependent_member_name != NULL ||
      !TypeIsStructOrUnion(alias->type) ||
      alias->type->template_origin == NULL ||
      alias->type->template_arguments == NULL ||
      alias->type->template_origin->type == NULL ||
      !TypeIsStructOrUnion(alias->type->template_origin->type) ||
      alias->type->template_origin->type->info.struct_info == NULL ||
      !alias->type->template_origin->type->info.struct_info->is_template ||
      /* `using Can = TrueAlias<A, B>` stores the alias TrueAlias as origin.
       * TrueAlias expands to `integral_constant<bool, sizeof...>`, which has
       * two parameters, so an arity match would treat Can as a forwarding
       * alias of integral_constant and instantiate it with two *types*.
       * `alias_template != NULL` distinguishes that from a class template
       * (or its injected-class-name typedef). */
      alias->type->template_origin->alias_template != NULL) {
    return false;
  }
  size_t expected =
      (size_t)alias->type->template_origin->type->info.struct_info
          ->template_parameter_count;
  if (alias->type->template_arguments->length != expected) {
    return false;
  }
  // `using StorageT = Storage<ElemT<I>, I, Tag<Ts...>>` has the same arity
  // as Storage, but the arguments are computed.  Treating it as a
  // forwarding alias instantiates Storage with only `I` (`StorageT<0>` →
  // `Storage<0, …>`).  That accidentally works when the element type is
  // `int` (I binds as the first type argument) and fails for `long`.
  for (size_t i = 0; i < alias->type->template_arguments->length; i++) {
    if (!AliasPatternArgumentIsBareParameter(
            alias->type->template_arguments->value.p[i])) {
      return false;
    }
  }
  return TemplateArgumentVectorContainsTemplateParameter(
      alias->type->template_arguments);
}

/* Mark `type` as a CTAD placeholder for an alias template: point it at the
 * alias's underlying class template and copy the alias's argument pattern, so
 * deduction runs against the alias rather than the underlying template. */
void SetCXXAliasTemplatePlaceholderOrigin(Symbol* alias,
                                                 TypeRecord* type) {
  if (!CXXAliasTemplatePatternNamesClassTemplate(alias) || type == NULL) {
    return;
  }
  if (type->template_arguments != NULL) {
    VectorDeleteWithContents(type->template_arguments,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  type->template_arguments = TemplateArgumentVectorCopy(alias->type->template_arguments);
  type->template_origin = alias;
  TypeRecordSetStructInfo(
      type, alias->type->template_origin->type->info.struct_info);
}

/* True if an alias template argument is a pack expansion (`Ts...`), reporting
 * the referenced pack parameter index and its kind. */
bool AliasTemplateArgumentIsPackExpansion(TemplateArgument* arg,
                                                 int* pack_index,
                                                 TemplateParameterKind* kind) {
  if (arg == NULL || !arg->is_pack_expansion) {
    return false;
  }
  if (arg->kind == kTemplateParameterType) {
    int index = -1;
    if (TypeIsTemplateParameterPlaceholder(arg->type, &index) && index >= 0) {
      if (pack_index != NULL) {
        *pack_index = index;
      }
      if (kind != NULL) {
        *kind = kTemplateParameterType;
      }
      return true;
    }
  }
  if (arg->kind == kTemplateParameterNonType &&
      arg->template_parameter_index >= 0) {
    if (pack_index != NULL) {
      *pack_index = arg->template_parameter_index;
    }
    if (kind != NULL) {
      *kind = kTemplateParameterNonType;
    }
    return true;
  }
  if (arg->kind == kTemplateParameterTemplate &&
      arg->template_parameter_index >= 0) {
    if (pack_index != NULL) {
      *pack_index = arg->template_parameter_index;
    }
    if (kind != NULL) {
      *kind = kTemplateParameterTemplate;
    }
    return true;
  }
  return false;
}

/* Fill in an alias template's argument list from the supplied `actuals`,
 * applying defaults and gathering a trailing parameter pack, so the count
 * matches the alias's parameters. Returns NULL on an arity mismatch. */
TypeRecord* InstantiateAliasClassTemplate(TypeParser* parser,
                                                 Symbol* alias,
                                                 Vector* args) {
  return InstantiateAliasClassTemplateImpl(parser, alias, args,
                                           /*emit_constraint_error=*/true);
}

/* Members of a partial specialization (`template <class R, class X>
 * struct A<R(X)>`) are numbered against the partial's own parameters `[R, X]`,
 * not the primary's argument list `[R(X)]` recorded on the instantiated type.
 * Returns the partial's bindings for `owner` (caller deletes), or NULL when
 * `owner` was instantiated from the primary template. */
static Vector* PartialSpecializationMemberPatternArguments(TypeParser* parser,
                                                           Struct* owner) {
  TypeRecord* type = owner->tag_symbol->type;
  if (type->template_origin == NULL || type->template_arguments == NULL ||
      TemplateArgumentVectorContainsTemplateParameter(
          type->template_arguments)) {
    return NULL;
  }
  Vector* bindings = NULL;
  if (SelectClassTemplatePartialSpecialization(parser, type->template_origin,
                                               type->template_arguments,
                                               &bindings) == NULL) {
    return NULL;
  }
  return bindings;
}

int StructPartialSpecializationParameterCount(Syntax* syntax, Struct* owner) {
  if (syntax == NULL || owner == NULL || owner->tag_symbol == NULL ||
      owner->tag_symbol->type == NULL) {
    return -1;
  }
  Symbol* origin = owner->tag_symbol->type->template_origin;
  if (origin == NULL || origin->type == NULL ||
      !TypeIsStructOrUnion(origin->type) ||
      origin->type->info.struct_info == NULL ||
      origin->type->info.struct_info->partial_specializations.length == 0) {
    return -1;
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  Vector* bindings = PartialSpecializationMemberPatternArguments(&parser, owner);
  TypeParserDestruct(&parser);
  if (bindings == NULL) {
    return -1;
  }
  int count = (int)bindings->length;
  VectorDeleteWithContents(bindings,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return count;
}

static Vector* PrefixMemberAliasPatternArguments(Symbol* alias,
                                                 Vector* alias_args,
                                                 Vector* class_args);

/* `template<class U> using IsNotBitField = std::is_pointer<U*>` inside
 * `template<class T> struct Base` numbers `U` after `T`.  A use written
 * `IsNotBitField<Arg>` supplies only `Arg`.  When the enclosing class
 * arguments are not on the parser, place `Arg` at `U`'s index so `U*` is
 * formed from `Arg` rather than from whatever occupies slot 0. */
static Vector* AlignAliasArgumentsToParameterIndices(Symbol* alias,
                                                     Vector* alias_args) {
  if (alias == NULL || alias->alias_template == NULL || alias_args == NULL) {
    return NULL;
  }
  Vector* params = &alias->alias_template->parameters;
  if (params->length == 0 || alias_args->length == 0 ||
      alias_args->length > params->length) {
    return NULL;
  }
  size_t own_start = params->length - alias_args->length;
  TemplateParameter* first_own = params->value.p[own_start];
  if (first_own == NULL || first_own->index <= 0) {
    return NULL;
  }
  int base = first_own->index;
  for (size_t i = 0; i < alias_args->length; i++) {
    TemplateParameter* param = params->value.p[own_start + i];
    if (param == NULL || param->index != base + (int)i) {
      return NULL;
    }
  }
  Vector* aligned = NewVector();
  for (int i = 0; i < base; i++) {
    TemplateArgument* hole = TemplateArgumentAlloc();
    hole->kind = kTemplateParameterType;
    hole->location = SOURCE_LOCATION_MISSING;
    hole->template_parameter_index = -1;
    hole->type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
    hole->type->template_parameter_index = i;
    VectorAppend(aligned, hole);
  }
  for (size_t i = 0; i < alias_args->length; i++) {
    VectorAppend(aligned, TemplateArgumentCopy(alias_args->value.p[i]));
  }
  return aligned;
}

static Struct* StructDeclaringMember(Struct* str, String* name, int depth) {
  if (str == NULL || name == NULL || name->value == NULL || depth > 16) {
    return NULL;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member != NULL && member->symbol != NULL &&
        strcmp(member->symbol->name.value, name->value) == 0) {
      return str;
    }
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base == NULL || base->type == NULL ||
        !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL ||
        base->type->info.struct_info == str) {
      continue;
    }
    Struct* found = StructDeclaringMember(base->type->info.struct_info, name,
                                          depth + 1);
    if (found != NULL) {
      return found;
    }
  }
  return NULL;
}

/* A member alias template such as
 * `template<int I> using StorageT = Storage<T, I>` is parameterized by both
 * the enclosing class's arguments (`T`) and its own (`I`).  Named as
 * `StorageT<I>`, the supplied argument list is only `[I]`; substituting the
 * pattern with that list alone rebinds class parameter 0 to `I`.  Prepend the
 * concrete enclosing-class arguments so the pattern sees `[T, I]`.  Returns a
 * new vector the caller must delete, or NULL when `alias_args` is already the
 * full list (or `alias` is not a member of the active instantiation). */
Vector* MemberAliasPatternArguments(TypeParser* parser, Symbol* alias,
                                    Vector* alias_args) {
  if (parser == NULL || alias == NULL || alias_args == NULL ||
      !StorageIs(alias->storage, STO(typedef))) {
    return NULL;
  }
  Struct* source = parser->template_substitution_source;
  Struct* target = parser->template_substitution_target;
  if ((source == NULL || FindStructMember(source, &alias->name) == NULL) &&
      (target == NULL || FindStructMember(target, &alias->name) == NULL)) {
    source = parser->enclosing_template_substitution_source;
    target = parser->enclosing_template_substitution_target;
  }
  if ((source == NULL || FindStructMember(source, &alias->name) == NULL) &&
      (target == NULL || FindStructMember(target, &alias->name) == NULL) &&
      compiler != NULL && compiler->current_function != NULL &&
      TypeIsFunction(compiler->current_function) &&
      compiler->current_function->info.function.cxx_member_owner != NULL) {
    Struct* owner = compiler->current_function->info.function.cxx_member_owner;
    if (FindStructMember(owner, &alias->name) != NULL) {
      target = owner;
      source = NULL;
      if (owner->tag_symbol != NULL && owner->tag_symbol->type != NULL &&
          owner->tag_symbol->type->template_origin != NULL &&
          owner->tag_symbol->type->template_origin->type != NULL &&
          TypeIsStructOrUnion(
              owner->tag_symbol->type->template_origin->type)) {
        source = owner->tag_symbol->type->template_origin->type->info.struct_info;
      }
    }
  }
  // A namespace-scope alias (`internal_compressed_tuple::ElemT`) can share
  // a name with a member alias.  FindStructMember is name-only, so it would
  // treat the namespace alias as a member and prefix the class arguments
  // (`ElemT<CompressedTuple, I>` → `Elem<long, …>`).
  bool is_member = alias->namespace_ == NULL &&
      ((source != NULL && FindStructMember(source, &alias->name) != NULL) ||
       (target != NULL && FindStructMember(target, &alias->name) != NULL));
  // Do not infer membership from pattern arity.  `std::bool_constant<B>` is
  // `integral_constant<bool, B>` (two pattern arguments, one alias parameter)
  // and is a namespace-scope alias.  Treating it as a member of the class
  // currently being instantiated prefixes that class's arguments and drops
  // inherited `::value` (`is_convertible<A,B> : __is_convertible<A,B>`).
  if (!is_member && alias->namespace_ != NULL) {
    return NULL;
  }
  if (!is_member) {
    // Class-body scope aliases are not struct members, so name lookup on
    // `source`/`target` fails.  Member alias parameters are numbered after
    // the enclosing class parameters; that index offset is enough to prefix.
    if (target == NULL) {
      target = parser->template_substitution_target;
      if (target == NULL) {
        target = parser->enclosing_template_substitution_target;
      }
      if (target == NULL && compiler != NULL &&
          compiler->current_function != NULL &&
          TypeIsFunction(compiler->current_function)) {
        target = compiler->current_function->info.function.cxx_member_owner;
      }
    }
    if (target != NULL && alias->alias_template != NULL &&
        alias->alias_template->parameters.length > 0 &&
        target->tag_symbol != NULL && target->tag_symbol->type != NULL &&
        target->tag_symbol->type->template_arguments != NULL &&
        target->tag_symbol->type->template_arguments->length > 0) {
      TemplateParameter* first =
          alias->alias_template->parameters.value.p[0];
      size_t class_n = target->tag_symbol->type->template_arguments->length;
      Vector* partial_args =
          PartialSpecializationMemberPatternArguments(parser, target);
      if (partial_args != NULL) {
        class_n = partial_args->length;
        VectorDeleteWithContents(partial_args,
                                 (VectorElementDestructor)TemplateArgumentDelete,
                                 /*free_element=*/false);
      }
      if (first != NULL && first->index == (int)class_n) {
        is_member = true;
      }
    }
    if (!is_member) {
      return AlignAliasArgumentsToParameterIndices(alias, alias_args);
    }
  }
  // `using Base::insert` is instantiated on the derived class, but the alias
  // (`IsLifetimeBoundAssignmentFrom`) is declared on the base.  Prefix that
  // base's arguments.  The derived class's argument list is a different
  // template.
  if (target != NULL) {
    Struct* declaring = StructDeclaringMember(target, &alias->name, 0);
    if (declaring != NULL) {
      target = declaring;
    }
  }
  if (target == NULL || target->tag_symbol == NULL ||
      target->tag_symbol->type == NULL ||
      target->tag_symbol->type->template_arguments == NULL ||
      target->tag_symbol->type->template_arguments->length == 0) {
    return AlignAliasArgumentsToParameterIndices(alias, alias_args);
  }
  Vector* partial_args =
      PartialSpecializationMemberPatternArguments(parser, target);
  Vector* class_arguments =
      partial_args != NULL ? partial_args
                           : target->tag_symbol->type->template_arguments;
  // A variadic class stores `raw_hash_set<Policy, Hash, Eq, Alloc>` as four
  // arguments, while a member alias's parameters are numbered after the
  // declared list, where the pack is one slot.  Fold the pack back before
  // prefixing or the alias's own argument lands on a pack element.
  Symbol* class_template = target->tag_symbol->type->template_origin;
  if (class_template == NULL && source != NULL) {
    class_template = source->tag_symbol;
  }
  Vector* grouped =
      RegroupExpandedClassTemplateArguments(class_template, class_arguments);
  Vector* combined = PrefixMemberAliasPatternArguments(
      alias, alias_args, grouped != NULL ? grouped : class_arguments);
  if (grouped != NULL) {
    VectorDeleteWithContents(grouped,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  if (partial_args != NULL) {
    VectorDeleteWithContents(partial_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  return combined;
}

static Vector* PrefixMemberAliasPatternArguments(Symbol* alias,
                                                 Vector* alias_args,
                                                 Vector* class_args) {
  size_t alias_param_count =
      alias->alias_template != NULL ? alias->alias_template->parameters.length
                                    : 0;
  // After the enclosing class is instantiated, a member alias pattern may
  // already have had class parameters substituted and its own parameters
  // rebased to zero (`Storage<ElemT<I0>, I0, StorageTag<long>>`).  Prefixing
  // class arguments then rebinds `I` to the class pack.
  if (alias_args->length == alias_param_count && alias->type != NULL) {
    int max_index = -1;
    if (alias->type->template_arguments != NULL) {
      for (size_t i = 0; i < alias->type->template_arguments->length; i++) {
        MaxTemplateParameterIndexInArgument(
            alias->type->template_arguments->value.p[i], &max_index);
      }
    }
    if (max_index >= 0 && (size_t)max_index < alias_args->length) {
      TemplateParameter* first_own =
          alias->alias_template != NULL &&
                  alias->alias_template->parameters.length > 0
              ? alias->alias_template->parameters.value.p[0]
              : NULL;
      // The pattern's recorded arguments can name an enclosing class
      // parameter at a low index (`Tree` at 0 in `key_arg<K>`, while `K` is
      // numbered 1).  That index is below the supplied alias-argument count,
      // but it is not this alias's own parameter.  Skipping the class prefix
      // binds `Tree::key_type` to `K`.  Once the alias parameters themselves
      // occupy that low range, the pattern is already aligned.
      if (first_own == NULL || first_own->index <= max_index) {
        return NULL;
      }
    }
    if (alias->alias_template != NULL &&
        alias->alias_template->parameters.length > 0) {
      TemplateParameter* first =
          alias->alias_template->parameters.value.p[0];
      if (first != NULL && first->index == 0) {
        return NULL;
      }
    }
  }
  if (alias_param_count > 0 &&
      alias_args->length == class_args->length + alias_param_count) {
    return NULL;
  }
  // A leading argument that merely equals the enclosing class argument is
  // not a prefix.  `preserves_data<T, U>` is written with the class parameter
  // as its first alias argument, so after `T` is substituted the vector starts
  // with the class argument while still having exactly the alias's own arity.
  // Only a longer vector (class arguments already prepended) is already
  // aligned with the member alias's offset parameter indices.
  if (alias_args->length >= class_args->length &&
      (alias_param_count == 0 || alias_args->length > alias_param_count)) {
    bool already_prefixed = true;
    for (size_t i = 0; i < class_args->length; i++) {
      if (!TemplateArgumentEqual(alias_args->value.p[i],
                                 class_args->value.p[i])) {
        already_prefixed = false;
        break;
      }
    }
    if (already_prefixed) {
      return NULL;
    }
  }
  Vector* combined = TemplateArgumentVectorCopy(class_args);
  for (size_t i = 0; i < alias_args->length; i++) {
    VectorAppend(combined, TemplateArgumentCopy(alias_args->value.p[i]));
  }
  return combined;
}

static Vector* CompleteMemberAliasTemplateArguments(TypeParser* parser,
                                                    Symbol* alias,
                                                    Vector* args) {
  Vector* completed = CompleteAliasTemplateArguments(alias, args);
  if (completed != NULL) {
    return completed;
  }
  Vector* prefixed = MemberAliasPatternArguments(parser, alias, args);
  if (prefixed == NULL) {
    return NULL;
  }
  completed = CompleteAliasTemplateArguments(alias, prefixed);
  VectorDeleteWithContents(prefixed,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return completed;
}

static TypeRecord* InstantiateAliasClassTemplateImpl(TypeParser* parser,
                                                     Symbol* alias,
                                                     Vector* args,
                                                     bool emit_constraint_error) {
  if (!CXXAliasTemplatePatternNamesClassTemplate(alias) ||
      alias->type->template_origin == NULL ||
      alias->type->template_arguments == NULL) {
    return NULL;
  }
  Vector* completed_alias_args =
      CompleteMemberAliasTemplateArguments(parser, alias, args);
  if (completed_alias_args == NULL) {
    return NULL;
  }
  if (ClassTemplateArgumentsAreStillDependent(completed_alias_args)) {
    TypeRecord* deferred =
        NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
    deferred->template_origin = alias;
    deferred->template_arguments =
        TemplateArgumentVectorCopy(completed_alias_args);
    VectorDeleteWithContents(completed_alias_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return deferred;
  }
  if (!ConceptsConstraintSatisfied(alias->associated_constraint,
                                   completed_alias_args)) {
    if (emit_constraint_error) {
      ReportAliasTemplateConstraintFailure(parser, alias, completed_alias_args);
    }
    VectorDeleteWithContents(completed_alias_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  }
  Vector* pattern_args =
      MemberAliasPatternArguments(parser, alias, completed_alias_args);
  Vector* subst_args =
      pattern_args != NULL ? pattern_args : completed_alias_args;
  Vector* underlying_args =
      SubstituteTemplateArgumentVectorForTypes(parser,
                                              alias->type->template_arguments,
                                              subst_args);
  if (pattern_args != NULL) {
    VectorDeleteWithContents(pattern_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  TypeRecord* instantiated = InstantiateSimpleClassTemplateImpl(
      parser, alias->type->template_origin, underlying_args,
      emit_constraint_error);
  VectorDeleteWithContents(underlying_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  VectorDeleteWithContents(completed_alias_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return instantiated;
}

/* Expand an alias template whose pattern is not a bare class-template
 * template-id (the shape `InstantiateAliasClassTemplateImpl` handles), for
 * example `template <class R> using all_t = decltype(views::all(declval<R>()))`
 * whose pattern is an already-formed specialization such as `ref_view<R>`.
 *
 * The pattern must be substituted with the alias's own arguments.  Returning it
 * unsubstituted leaves the alias's parameter indices in the result, and those
 * indices then bind to whatever occupies the same index in the enclosing
 * template -- so inside a member function template of a class template, the
 * alias parameter silently resolves to the class's argument.
 *
 * Returns NULL when `templ` is not an alias template, leaving the caller's
 * class-template path in charge. */
static TypeRecord* InstantiateGenericAliasTemplate(TypeParser* parser,
                                                   Symbol* alias, Vector* args,
                                                   bool emit_constraint_error) {
  if (!CompilerIsCXX() || alias == NULL || !alias->flags.is_template ||
      !StorageIs(alias->storage, STO(typedef)) || alias->type == NULL) {
    return NULL;
  }
  // A forwarding alias whose pattern *is* the primary class template
  // (`using Vec = vector<T>`) belongs to the ordinary class-template path.
  // A member alias whose pattern is a different specialization
  // (`using StorageT = Storage<ElemT<I>, I, Tag>`) must substitute the
  // pattern with the alias's own arguments; treating it as a forwarding
  // alias would instantiate Storage with only `I`.
  if (TypeIsStructOrUnion(alias->type) &&
      alias->type->info.struct_info != NULL &&
      alias->type->info.struct_info->is_template &&
      (alias->type->template_arguments == NULL ||
       CXXAliasTemplatePatternNamesClassTemplate(alias))) {
    return NULL;
  }
  Vector* completed_args =
      CompleteMemberAliasTemplateArguments(parser, alias, args);
  if (completed_args == NULL) {
    return NULL;
  }
  if (!ConceptsConstraintSatisfied(alias->associated_constraint,
                                   completed_args)) {
    if (emit_constraint_error) {
      ReportAliasTemplateConstraintFailure(parser, alias, completed_args);
    }
    VectorDeleteWithContents(completed_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  }
  Vector* pattern_args =
      MemberAliasPatternArguments(parser, alias, completed_args);
  Vector* subst_args = pattern_args != NULL ? pattern_args : completed_args;
  bool saved_failed = parser->template_substitution_failed;
  parser->template_substitution_failed = false;
  TypeRecord* subst =
      SubstituteTemplateParameters(parser, alias->type, subst_args);
  bool substitution_failed =
      parser->template_substitution_failed || subst == NULL;
  // Building the class named by the alias walks that class's members.  A
  // nested SFINAE miss (`enable_if<false>::type`) sets the failure flag even
  // though the class itself was instantiated.  The flag belongs to that
  // nested lookup.  A dependent-member pattern (`enable_if<B, T>::type`)
  // still fails: there the flag is the alias's own result.
  if (substitution_failed && subst != NULL &&
      alias->type->dependent_member_name == NULL &&
      TypeIsStructOrUnion(subst) && !TypeIsUnknown(subst) &&
      !TypeContainsTemplateParameter(subst)) {
    substitution_failed = false;
  }
  parser->template_substitution_failed = saved_failed || substitution_failed;
  if (pattern_args != NULL) {
    VectorDeleteWithContents(pattern_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  VectorDeleteWithContents(completed_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  if (subst == NULL || substitution_failed) {
    // `enable_if<false, int>::type` has no member.  Substitution sets the
    // failure flag and returns the class itself; that is not the alias result.
    TypeRecordDelete(subst);
    return NULL;
  }
  // Alias patterns such as `typename __make_index_sequence<N>::type` stay a
  // lazy `Template<Args>::member` encoding after substitution.  Collapse that to
  // the concrete nested typedef so functional casts (`make_index_sequence<N>{}`)
  // construct `index_sequence<0..N-1>` rather than naming the alias at runtime.
  subst = TypeMaterializeClassTemplateSpecialization(parser->syntax, subst);
  return TypeRecordCalculateSize(subst);
}

/* Public: build a CTAD placeholder type from a class-template (or alias
 * template) symbol, used when a variable is declared with just the template
 * name (`Foo x = ...;`). Returns NULL if `symbol` is not a class template. */
TypeRecord* TypeClassTemplatePlaceholderFromSymbol(Symbol* symbol) {
  if (!CompilerIsCXX() || symbol == NULL || !symbol->flags.is_template ||
      symbol->type == NULL || !TypeIsStructOrUnion(symbol->type)) {
    return NULL;
  }
  TypeRecord* type = TypeRecordCopy(symbol->type);
  if (CXXAliasTemplatePatternNamesClassTemplate(symbol)) {
    SetCXXAliasTemplatePlaceholderOrigin(symbol, type);
  } else if (type->info.struct_info != NULL &&
             type->info.struct_info->is_template) {
    type->template_origin = symbol;
  }
  if (!TypeIsClassTemplatePlaceholder(type)) {
    TypeRecordDelete(type);
    return NULL;
  }
  return type;
}
