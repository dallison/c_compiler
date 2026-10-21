//
//  type_template_substitute.c
//  c_compiler
//

#include "type_template_internal.h"
#include "type_internal.h"

#include <stddef.h>
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
#include "member_pointer.h"
#include "reflection_semantics.h"
#include "statement_semantics.h"
#include "statement_parser.h"
#include "symbol_table.h"
#include "syntax.h"
#include "semantics.h"
#include "type_traits_semantics.h"
#include "errors.h"
#include "debug.h"
#include "rtti.h"
#include "set.h"

ASTNode* IdentityCloneNode(ASTNode* node, void* data) {
  (void)data;
  return node;
}

static ASTNode* CloneAndRebaseDependentExpression(ASTNode* expr, int base);
static TypeRecord* SubstituteTypePackIndex(TypeParser* parser,
                                           TypeRecord* type, Vector* args);
static Vector* CompleteAliasTemplateArgumentsFromPattern(Symbol* alias,
                                                         Vector* actuals);
static Vector* CompleteAliasTemplateArgumentsFromParameters(Vector* parameters,
                                                              Vector* actuals);
static Struct* StructDeclaringSymbol(Struct* str, Symbol* symbol, int depth);
static Struct* StructBaseDeclaringMember(Struct* str, String* name, int depth);
static bool TypeReferencesIndexAtLeast(TypeRecord* type, int min_index);
bool FindPackExpansionInExpression(ASTNode* expr, Vector* args,
                                   int* pack_index, size_t* pack_length);

static bool RecordPackExpansionIndex(int candidate, Vector* args,
                                     int* pack_index, size_t* pack_length) {
  if (candidate < 0 || args == NULL || (size_t)candidate >= args->length) {
    return false;
  }
  TemplateArgument* arg = args->value.p[candidate];
  if (arg == NULL || arg->pack_arguments == NULL) {
    return false;
  }
  if (*pack_index >= 0 && *pack_index != candidate) {
    return *pack_length == arg->pack_arguments->length;
  }
  *pack_index = candidate;
  *pack_length = arg->pack_arguments->length;
  return true;
}

bool FindPackExpansionInType(TypeRecord* type, Vector* args,
                                    int* pack_index, size_t* pack_length) {
  bool found = false;
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    int index = -1;
    if (t->is_pack_index && TypeDependentOf(t)->pack_index_expr != NULL &&
        FindPackExpansionInExpression(TypeDependentOf(t)->pack_index_expr, args,
                                      pack_index, pack_length)) {
      // In `Ts...[Is]...`, the outer expansion is driven by `Is`; `Ts` is
      // selected by the pack-indexing specifier itself and must remain whole.
      found = true;
    } else if (!t->is_pack_index &&
               TypeIsTemplateParameterPlaceholder(t, &index) &&
               RecordPackExpansionIndex(index, args, pack_index, pack_length)) {
      found = true;
    }
    if (t->template_origin != NULL &&
        t->template_origin->flags.is_template_template_parameter &&
        RecordPackExpansionIndex(
            t->template_parameter_index >= 0
                ? t->template_parameter_index
                : t->template_origin->template_parameter_index,
            args,
            pack_index, pack_length)) {
      found = true;
    }
    if (t->declarator == kDeclArray &&
        RecordPackExpansionIndex(t->info.array.template_parameter_index, args,
                                 pack_index, pack_length)) {
      found = true;
    }
    if (t->template_arguments != NULL) {
      for (size_t i = 0; i < t->template_arguments->length; i++) {
        TemplateArgument* arg = t->template_arguments->value.p[i];
        // `FlatTuple<T...>` in the pattern `ElemBase<FlatTuple<T...>, Idx>...`
        // expands `T` itself; only `Idx` belongs to the enclosing expansion.
        if (arg != NULL && arg->is_pack_expansion) {
          continue;
        }
        if (FindPackExpansionInTemplateArgument(arg, args, pack_index,
                                                pack_length)) {
          found = true;
        }
      }
    }
    if (TypeDependentOf(t)->dependent_decltype_expr != NULL &&
        FindPackExpansionInExpression(
            TypeDependentOf(t)->dependent_decltype_expr, args, pack_index,
            pack_length)) {
      found = true;
    }
    if (TypeIsFunction(t)) {
      for (size_t i = 0; i < t->info.function.prototype.length; i++) {
        Symbol* formal = t->info.function.prototype.value.p[i];
        if (formal != NULL &&
            FindPackExpansionInType(formal->type, args, pack_index,
                                    pack_length)) {
          found = true;
        }
      }
    }
  }
  return found;
}

typedef struct {
  Vector* args;
  int* pack_index;
  size_t* pack_length;
  bool found;
} PackExpansionExpressionSearch;

static void FindPackExpansionInExpressionNode(ASTNode* node, void* data,
                                              int child_id,
                                              VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL) {
    return;
  }
  PackExpansionExpressionSearch* search = data;
  if (node->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node;
    if (id->symbol != NULL && id->symbol->flags.is_parameter_pack &&
        RecordPackExpansionIndex(id->symbol->template_parameter_index,
                                 search->args, search->pack_index,
                                 search->pack_length)) {
      search->found = true;
    }
    if (id->symbol != NULL && id->symbol->type != NULL &&
        FindPackExpansionInType(id->symbol->type, search->args,
                                search->pack_index,
                                search->pack_length)) {
      search->found = true;
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
    if (FindPackExpansionInTemplateArgument(
            template_arguments->value.p[i], search->args, search->pack_index,
            search->pack_length)) {
      search->found = true;
    }
  }
  if ((node->op == AST_OP(sizeof) || node->op == AST_OP(alignof)) &&
      ((SizeofASTNode*)node)->type_operand != NULL &&
      FindPackExpansionInType(((SizeofASTNode*)node)->type_operand,
                              search->args, search->pack_index,
                              search->pack_length)) {
    search->found = true;
  }
  if (shape == kASTShapeCast &&
      FindPackExpansionInType(((CastASTNode*)node)->cast_type, search->args,
                              search->pack_index, search->pack_length)) {
    search->found = true;
  }
}

bool FindPackExpansionInExpression(ASTNode* expr, Vector* args,
                                   int* pack_index, size_t* pack_length) {
  PackExpansionExpressionSearch search = {
      .args = args,
      .pack_index = pack_index,
      .pack_length = pack_length,
      .found = false,
  };
  ASTNodeVisit(expr, FindPackExpansionInExpressionNode, 0, &search);
  return search.found;
}

bool FindPackExpansionInTemplateArgument(TemplateArgument* arg,
                                                Vector* args,
                                                int* pack_index,
                                                size_t* pack_length) {
  if (arg == NULL) {
    return false;
  }
  if (arg->kind == kTemplateParameterTemplate &&
      TemplateArgumentExtraOf(arg)->pack_index_expr != NULL) {
    return FindPackExpansionInExpression(
        TemplateArgumentExtraOf(arg)->pack_index_expr, args, pack_index,
        pack_length);
  }
  if (arg->kind == kTemplateParameterNonType &&
      RecordPackExpansionIndex(arg->template_parameter_index, args, pack_index,
                               pack_length)) {
    return true;
  }
  if ((arg->kind == kTemplateParameterType ||
       arg->kind == kTemplateParameterTemplate) &&
      RecordPackExpansionIndex(arg->template_parameter_index, args, pack_index,
                               pack_length)) {
    return true;
  }
  if (arg->pack_arguments != NULL) {
    bool found = false;
    for (size_t i = 0; i < arg->pack_arguments->length; i++) {
      if (FindPackExpansionInTemplateArgument(arg->pack_arguments->value.p[i],
                                              args, pack_index, pack_length)) {
        found = true;
      }
    }
    return found;
  }
  bool found =
      FindPackExpansionInType(arg->type, args, pack_index, pack_length);
  if (arg->dependent_expr != NULL &&
      FindPackExpansionInExpression(arg->dependent_expr, args, pack_index,
                                    pack_length)) {
    found = true;
  }
  return found;
}

/* Copy an argument vector but replace the pack at `pack_index` with a single
 * concrete `element`. Used to substitute one element at a time while expanding
 * a pack-dependent pattern. */
Vector* TemplateArgumentVectorCopyWithPackElement(Vector* args,
                                                         int pack_index,
                                                         TemplateArgument* element) {
  Vector* copy = TemplateArgumentVectorCopy(args);
  if (copy == NULL || pack_index < 0 || (size_t)pack_index >= copy->length) {
    return copy;
  }
  TemplateArgumentDelete(copy->value.p[pack_index]);
  VectorSet(copy, (size_t)pack_index, TemplateArgumentCopy(element));
  return copy;
}

/* Build the argument list for element `element_index` of a pack expansion.
 * Every bound pack of the same length is advanced in lockstep, as required for
 * patterns such as `Trait<Ts, Us&&>...`; unrelated packs remain intact. */
static Vector* TemplateArgumentVectorCopyForPackExpansionElement(
    Vector* args, int pack_index, size_t element_index) {
  if (args == NULL || pack_index < 0 || (size_t)pack_index >= args->length) {
    return TemplateArgumentVectorCopy(args);
  }
  TemplateArgument* driving_pack = args->value.p[pack_index];
  if (driving_pack == NULL || driving_pack->pack_arguments == NULL ||
      element_index >= driving_pack->pack_arguments->length) {
    return TemplateArgumentVectorCopy(args);
  }
  size_t pack_length = driving_pack->pack_arguments->length;
  Vector* copy = NewVector();
  for (size_t i = 0; i < args->length; i++) {
    TemplateArgument* arg = args->value.p[i];
    TemplateArgument* selected =
        arg != NULL && arg->pack_arguments != NULL &&
                arg->pack_arguments->length == pack_length &&
                element_index < arg->pack_arguments->length
            ? arg->pack_arguments->value.p[element_index]
            : arg;
    VectorAppend(copy, TemplateArgumentCopy(selected));
  }
  return copy;
}

// A class that is still being defined or instantiated has no members yet, so
// an operand such as `ranges::begin(declval<subrange<...>&>())` cannot be
// judged against it.  Constraint checks reached from inside that class (for
// example through its `view_interface<D>` base) must defer rather than report
// a substitution failure.
static bool TemplateArgumentVectorNamesIncompleteClass(Vector* args) {
  if (args == NULL) {
    return false;
  }
  for (size_t i = 0; i < args->length; i++) {
    TemplateArgument* arg = args->value.p[i];
    if (arg == NULL) {
      continue;
    }
    if (TemplateArgumentVectorNamesIncompleteClass(arg->pack_arguments)) {
      return true;
    }
    TypeRecord* base = arg->type;
    while (base != NULL && base->next != NULL) {
      base = base->next;
    }
    if (base != NULL && TypeIsStructOrUnion(base) &&
        base->info.struct_info != NULL &&
        (base->info.struct_info->instantiation_in_progress ||
         !TypeIsCompleteClass(base))) {
      return true;
    }
  }
  return false;
}

// Re-evaluate a value-dependent non-type template-argument expression (stored
// unevaluated at parse time, e.g. `!is_integral<It>::value`) against concrete
// template arguments; defined after the template-body clone machinery it relies
// on.
TemplateArgument* NewSubstitutedTemplateArgument(TypeParser* parser,
                                                 TemplateArgument* arg,
                                                 Vector* args) {
  TemplateArgument* concrete = TemplateArgumentAlloc();
  concrete->kind = arg->kind;
  concrete->is_pack_expansion = false;
  concrete->is_erroneous = arg->is_erroneous;
  concrete->type =
      arg->kind == kTemplateParameterNonType && arg->type != NULL
          ? TypeRecordInternOrCopy(arg->type) : NULL;
  concrete->int_value = arg->int_value;
  concrete->template_parameter_index = arg->template_parameter_index;
  concrete->pack_arguments = NULL;
  concrete->dependent_expr = NULL;
  concrete->location = arg->location;
  concrete->value_kind = arg->value_kind;
  TemplateArgumentCopyExtra(concrete, arg);
  if (concrete->extra != NULL) {
    concrete->extra->lvalue_symbol = NULL;
  }
  if (arg->kind == kTemplateParameterTemplate &&
      TemplateArgumentExtraOf(arg)->pack_index_expr != NULL) {
    int pack_parameter = arg->template_parameter_index;
    TemplateArgument* pack =
        args != NULL && pack_parameter >= 0 &&
                (size_t)pack_parameter < args->length
            ? args->value.p[pack_parameter]
            : NULL;
    int64_t index = 0;
    if (pack == NULL || pack->pack_arguments == NULL ||
        !TryFoldDependentTemplateArgument(
            parser, TemplateArgumentExtraOf(arg)->pack_index_expr, args,
            &index)) {
      ASTNode* partial = CloneDependentExpressionWithArgs(
          parser, TemplateArgumentExtraOf(arg)->pack_index_expr, args);
      TemplateArgumentMutableExtra(concrete)->pack_index_expr =
          partial != NULL ? partial
                          : TemplateArgumentExtraOf(arg)->pack_index_expr;
      return concrete;
    }
    if (index < 0 || (uint64_t)index >= pack->pack_arguments->length) {
      if (parser != NULL) {
        parser->template_substitution_failed = true;
      }
      SemanticError(TemplateArgumentExtraOf(arg)->pack_index_expr,
                    "pack index %" PRId64
                    " is out of bounds for a pack of length %zu",
                    index, pack->pack_arguments->length);
      return concrete;
    }
    TemplateArgument* selected =
        pack->pack_arguments->value.p[(size_t)index];
    if (selected == NULL ||
        selected->kind != kTemplateParameterTemplate) {
      if (parser != NULL) {
        parser->template_substitution_failed = true;
      }
      SemanticError(TemplateArgumentExtraOf(arg)->pack_index_expr,
                    "template-name pack indexing requires a template pack");
      return concrete;
    }
    TemplateArgumentDelete(concrete);
    return TemplateArgumentCopy(selected);
  }
  // A value-dependent non-type argument (e.g. an `enable_if` SFINAE condition):
  // try to fold it now that some parameters are concrete.  If it folds, the
  // argument becomes an ordinary integer; otherwise keep the expression so a
  // later, more-concrete substitution can complete it.
  if (arg->kind == kTemplateParameterNonType && arg->dependent_expr != NULL) {
    bool saved_failed =
        parser != NULL && parser->template_substitution_failed;
    if (parser != NULL) {
      parser->template_substitution_failed = false;
    }
    int64_t folded = 0;
    bool folded_ok =
        TryFoldDependentTemplateArgument(parser, arg->dependent_expr, args,
                                         &folded);
    bool subst_failed =
        parser != NULL && parser->template_substitution_failed;
    if (parser != NULL) {
      parser->template_substitution_failed = saved_failed || subst_failed;
    }
    if (folded_ok && !subst_failed) {
      concrete->int_value = folded;
      concrete->value_kind = kTemplateValueIntegral;
      concrete->template_parameter_index = -1;
    } else {
      // Cannot fold yet (e.g. `Target - I` where the member template's own
      // `Target` is still unknown but the enclosing class's `I` is concrete).
      // Bake the now-known parameters into a partially substituted copy so the
      // remaining parameter is the only one left symbolic; keeping the raw,
      // unsubstituted expression would leave the concrete parameter's index
      // dangling and later collide with the member template's own parameters
      // after rebasing.
      ASTNode* partial =
          CloneDependentExpressionWithArgs(parser, arg->dependent_expr, args);
      if (partial != NULL &&
          !DependentExpressionContainsTemplateParameter(partial)) {
        DiagnosticSuppressBegin();
        compiler->constant_evaluation_required_depth++;
        partial = AnalyzeExpression(partial);
        compiler->constant_evaluation_required_depth--;
        bool resolved = TemplateArgumentSetFromExpression(concrete, partial);
        DiagnosticSuppressEnd();
        if (resolved) {
          concrete->template_parameter_index = -1;
          ASTNodeDelete(partial);
          return concrete;
        }
        if (parser != NULL && partial != NULL && partial->type != NULL &&
            TypeIsUnknown(partial->type)) {
          parser->template_substitution_failed = true;
        }
      }
      concrete->dependent_expr = partial != NULL ? partial : arg->dependent_expr;
    }
    return concrete;
  }
  int placeholder_index = -1;
  TemplateArgument* placeholder_actual = NULL;
  if (arg->kind == kTemplateParameterType && arg->type != NULL &&
      TypeIsTemplateParameterPlaceholder(arg->type, &placeholder_index) &&
      placeholder_index >= 0 &&
      (size_t)placeholder_index < args->length) {
    placeholder_actual = args->value.p[placeholder_index];
  }
  if (arg->kind == kTemplateParameterTemplate &&
      arg->template_parameter_index >= 0 &&
      (size_t)arg->template_parameter_index < args->length) {
    TemplateArgument* actual = args->value.p[arg->template_parameter_index];
    if (actual != NULL && actual->kind == kTemplateParameterTemplate) {
      TemplateArgumentDelete(concrete);
      return TemplateArgumentCopy(actual);
    }
  } else if (placeholder_actual != NULL &&
             placeholder_actual->kind == kTemplateParameterNonType &&
             placeholder_actual->type != NULL) {
    // A type-constraint on an `auto` non-type parameter constrains the type of
    // the argument, not its value. Substitute the concrete value argument's
    // type into the concept-id's synthetic type argument.
    concrete->type = TypeRecordInternOrCopy(placeholder_actual->type);
    concrete->type =
        TypeMaterializeClassTemplateSpecialization(parser->syntax,
                                                   concrete->type);
    concrete->template_parameter_index = -1;
  } else if (arg->kind == kTemplateParameterType && arg->type != NULL) {
    concrete->type = SubstituteTemplateParameters(parser, arg->type, args);
    concrete->type =
        TypeMaterializeClassTemplateSpecialization(parser->syntax,
                                                   concrete->type);
    concrete->template_parameter_index = -1;
  } else if (arg->kind == kTemplateParameterType &&
             arg->template_parameter_index >= 0 &&
             (size_t)arg->template_parameter_index < args->length) {
    TemplateArgument* actual = args->value.p[arg->template_parameter_index];
    if (actual != NULL && actual->kind == kTemplateParameterType) {
      if (actual->pack_arguments != NULL) {
        TemplateArgumentDelete(concrete);
        return TemplateArgumentCopy(actual);
      }
      if (actual->type != NULL) {
        concrete->type = TypeRecordInternOrCopy(actual->type);
        concrete->type =
            TypeMaterializeClassTemplateSpecialization(parser->syntax,
                                                       concrete->type);
        concrete->template_parameter_index = -1;
      } else {
        concrete->template_parameter_index = actual->template_parameter_index;
      }
    } else if (actual != NULL && actual->kind == kTemplateParameterNonType &&
               actual->type != NULL) {
      concrete->type = TypeRecordInternOrCopy(actual->type);
      concrete->type =
          TypeMaterializeClassTemplateSpecialization(parser->syntax,
                                                     concrete->type);
      concrete->template_parameter_index = -1;
    }
  } else if (arg->kind == kTemplateParameterNonType &&
             arg->template_parameter_index >= 0 &&
             (size_t)arg->template_parameter_index < args->length) {
    TemplateArgument* actual = args->value.p[arg->template_parameter_index];
    if (actual != NULL && actual->kind == kTemplateParameterNonType &&
        actual->pack_arguments == NULL) {
      TemplateArgumentDelete(concrete);
      return TemplateArgumentCopy(actual);
    }
  }
  return concrete;
}

/* If `symbol`'s value depends on a non-type template parameter, resolve it
 * against `args`: either bind a concrete integer value or remap to another
 * parameter index (when the actual is itself still parameter-dependent). */
void SubstituteDependentSymbolValue(Symbol* symbol, Vector* args) {
  if (symbol == NULL ||
      SymbolDependentValueTemplateParameterIndex(symbol) < 0) {
    return;
  }
  int index = SymbolDependentValueTemplateParameterIndex(symbol);
  if (args == NULL || (size_t)index >= args->length) {
    return;
  }
  TemplateArgument* arg = args->value.p[index];
  if (arg == NULL || arg->kind != kTemplateParameterNonType ||
      arg->pack_arguments != NULL) {
    return;
  }
  if (arg->template_parameter_index >= 0) {
    SymbolSetDependentValueTemplateParameterIndex(
        symbol, arg->template_parameter_index);
    symbol->flags.value_set = false;
    return;
  }
  symbol->value.ivalue = arg->int_value;
  symbol->flags.value_set = true;
  SymbolSetDependentValueTemplateParameterIndex(symbol, -1);
}

// The alignment named by one element of a pack-expanded alignas operand.
// False while the element is still dependent.
static bool ResolveAlignasPackElement(TypeParser* parser, Attribute* attr,
                                      Vector* args, int pack_index,
                                      size_t element, int64_t* alignment) {
  bool resolved = false;
  if (attr->dependent_alignas_type != NULL) {
    TypeRecord* concrete = SubstituteTemplateParametersForPackElement(
        parser, attr->dependent_alignas_type, args, pack_index, element);
    if (concrete != NULL && !TypeContainsTemplateParameter(concrete)) {
      TypeRecordCalculateSize(concrete);
      *alignment = TypeRecordAlignment(concrete);
      resolved = true;
    }
    TypeRecordDelete(concrete);
    return resolved;
  }
  Vector* element_args =
      TemplateArgumentVectorCopyForPackExpansionElement(args, pack_index,
                                                        element);
  ASTNode* concrete = CloneDependentExpressionWithArgs(
      parser, attr->dependent_alignas_expr, element_args);
  if (concrete != NULL && !ExpressionIsTemplateDependent(concrete)) {
    concrete = AnalyzeExpression(concrete);
    resolved = EvaluateIntegerExpression(concrete, alignment);
  }
  ASTNodeDelete(concrete);
  VectorDeleteWithContents(element_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return resolved;
}

static bool IsValidAlignasValue(int64_t alignment) {
  return alignment >= 0 && alignment <= 2147483647 &&
         (alignment & (alignment - 1)) == 0;
}

void SubstituteDependentSymbolAlignment(TypeParser* parser, Symbol* symbol,
                                        Vector* args, int rebase_base) {
  if (parser == NULL || symbol == NULL) {
    return;
  }
  for (size_t i = 0; i < SymbolAttributes(symbol)->length; i++) {
    Attribute* attr = SymbolAttributes(symbol)->value.p[i];
    if (attr == NULL ||
        (attr->dependent_alignas_type == NULL &&
         attr->dependent_alignas_expr == NULL)) {
      continue;
    }

    int64_t alignment = 0;
    bool resolved = false;
    // The pack of a member template's `alignas(Us...)` is bound only when
    // that template is instantiated; until then the operand is substituted
    // and renumbered like any other.
    int pack_index = -1;
    size_t pack_length = 0;
    bool expand_pack =
        attr->dependent_alignas_is_pack_expansion &&
        (attr->dependent_alignas_type != NULL
             ? FindPackExpansionInType(attr->dependent_alignas_type, args,
                                       &pack_index, &pack_length)
             : FindPackExpansionInExpression(attr->dependent_alignas_expr,
                                             args, &pack_index, &pack_length));
    if (expand_pack) {
      // The strictest element alignment applies; an empty pack has no effect.
      resolved = true;
      for (size_t e = 0; resolved && e < pack_length; e++) {
        int64_t element = 0;
        resolved = ResolveAlignasPackElement(parser, attr, args, pack_index, e,
                                             &element);
        if (resolved && !IsValidAlignasValue(element)) {
          SyntaxError(parser->syntax,
                      "alignas specifier must name a power-of-two alignment");
          element = 0;
        }
        if (element > alignment) {
          alignment = element;
        }
      }
    } else if (attr->dependent_alignas_type != NULL) {
      TypeRecord* concrete = SubstituteTemplateParameters(
          parser, attr->dependent_alignas_type, args);
      if (concrete != NULL && !TypeContainsTemplateParameter(concrete)) {
        TypeRecordCalculateSize(concrete);
        alignment = TypeRecordAlignment(concrete);
        resolved = true;
      } else if (concrete != NULL) {
        RebaseTemplateParameterIndices(concrete, rebase_base);
        TypeRecordDelete(attr->dependent_alignas_type);
        attr->dependent_alignas_type = concrete;
        concrete = NULL;
      }
      TypeRecordDelete(concrete);
    } else {
      ASTNode* concrete =
          rebase_base > 0
              ? TypeSubstituteMemberTemplateExpressionAndRebase(
                    parser->syntax, attr->dependent_alignas_expr, args,
                    rebase_base, attr->dependent_alignas_expr->location, NULL,
                    NULL)
              : CloneDependentExpressionWithArgs(
                    parser, attr->dependent_alignas_expr, args);
      if (concrete != NULL && ExpressionIsTemplateDependent(concrete)) {
        ASTNodeDelete(attr->dependent_alignas_expr);
        attr->dependent_alignas_expr = concrete;
        concrete = NULL;
      } else if (concrete != NULL) {
        concrete = AnalyzeExpression(concrete);
        resolved = EvaluateIntegerExpression(concrete, &alignment);
      }
      ASTNodeDelete(concrete);
    }

    if (!resolved) {
      continue;
    }
    if (!IsValidAlignasValue(alignment)) {
      SyntaxError(parser->syntax,
                  "alignas specifier must name a power-of-two alignment");
      alignment = 0;
    }
    // The specifier is now an ordinary `aligned(N)`; alignas(0) has no effect.
    TypeRecordDelete(attr->dependent_alignas_type);
    attr->dependent_alignas_type = NULL;
    ASTNodeDelete(attr->dependent_alignas_expr);
    attr->dependent_alignas_expr = NULL;
    attr->dependent_alignas_is_pack_expansion = false;
    char text[24];
    int length = snprintf(text, sizeof(text), "%lld", (long long)alignment);
    AttributeAddArg(attr, text, (size_t)length);
  }
  // Instantiation builds a fresh symbol, so every specifier, dependent or
  // not, has to be applied here.
  int alignment =
      AttributeListAlignment(SymbolAttributes(symbol), compiler->alignment);
  if (alignment > SymbolAlignment(symbol)) {
    SymbolSetAlignment(symbol, alignment);
  }
}

void SubstituteStaticMemberInitializerValue(TypeParser* parser,
                                                   Symbol* symbol,
                                                   ASTNode* initializer,
                                                   Vector* args) {
  if (parser == NULL || symbol == NULL || initializer == NULL ||
      !CompilerIsCXX() ||
      (!symbol->flags.is_constexpr && !TypeIsConst(symbol->type))) {
    return;
  }
  ASTNode* expr = ConstexprInitializerExpression(initializer);
  if (expr == NULL) {
    return;
  }
  if (TypeContainsAuto(symbol->type)) {
    ASTNode* cloned = CloneDependentExpressionWithArgs(parser, expr, args);
    if (cloned == NULL) {
      return;
    }
    DiagnosticSuppressBegin();
    cloned = AnalyzeExpression(cloned);
    if (cloned != NULL && cloned->type != NULL) {
      bool decltype_auto =
          symbol->type->declarator == kDeclPrimitive &&
          (symbol->type->type & kTypeDecltypeAuto) != 0;
      TypeRecord* deduced =
          decltype_auto ? TypeDeduceDecltypeAuto(cloned)
                        : TypeDeduceAuto(symbol->type, cloned->type);
      if (deduced != NULL) {
        SymbolSetType(symbol, deduced);
      }
    }
    if (cloned != NULL && symbol->type != NULL &&
        TypeIsIntegral(symbol->type)) {
      int64_t ivalue = 0;
      if (EvaluateIntegerExpression(cloned, &ivalue)) {
        symbol->value.ivalue = ivalue;
        symbol->flags.value_set = true;
        SymbolSetDependentValueTemplateParameterIndex(symbol, -1);
      }
    }
    DiagnosticSuppressEnd();
    ASTNodeDelete(cloned);
    return;
  }
  if (!TypeIsIntegral(symbol->type) && !TypeIsFloatingPoint(symbol->type)) {
    return;
  }
  int64_t ivalue = 0;
  if (TypeIsIntegral(symbol->type) &&
      TryFoldDependentTemplateArgument(parser, expr, args, &ivalue)) {
    symbol->value.ivalue = ivalue;
    symbol->flags.value_set = true;
    SymbolSetDependentValueTemplateParameterIndex(symbol, -1);
    return;
  }
  ASTNode* cloned = CloneDependentExpressionWithArgs(parser, expr, args);
  if (cloned == NULL) {
    return;
  }
  DiagnosticSuppressBegin();
  cloned = AnalyzeExpression(cloned);
  bool ok = false;
  if (TypeIsFloatingPoint(symbol->type)) {
    ok = EvaluateFloatingPointExpression(cloned, &symbol->value.fvalue);
  } else if (TypeIsBool(symbol->type) && cloned != NULL &&
             TypeIsMemberPointer(cloned->type)) {
    // `static constexpr bool set = P;` for a member-pointer parameter `P`.
    cloned = AnalyzeExpression(NewCastASTNode(TypeRecordCopy(symbol->type),
                                              cloned->location, cloned));
    int64_t truth = 0;
    ok = cloned != NULL && EvaluateIntegerExpression(cloned, &truth);
    if (ok) {
      symbol->value.ivalue = truth;
    }
  }
  DiagnosticSuppressEnd();
  ASTNodeDelete(cloned);
  if (ok) {
    symbol->flags.value_set = true;
    SymbolSetDependentValueTemplateParameterIndex(symbol, -1);
  }
}

/* Diagnose an ill-formed pack expansion (a `...` whose pattern contains no
 * expandable parameter pack, e.g. `decltype(T())...` where `T` is not a pack).
 *
 * The offending pattern comes from the template *definition* and is
 * re-substituted for every member that mentions it and every time a member is
 * (re)analyzed; a type alias multiplies this further, since each use expands the
 * alias into a fresh copy of the pattern node.  A naive `SyntaxError` therefore
 * fires many times for a single source construct.  Recover by reporting at most
 * once per distinct diagnosis site: we key on both the pattern node's identity
 * (catches re-substitution of the same node) and the reported source location
 * (catches distinct copies produced at the same instantiation point).  A node /
 * location is only remembered once its diagnostic actually reaches the user
 * (i.e. it was not swallowed by a speculative SFINAE trap or a suppression
 * scope), so a genuine later error is still reported. */
static void ReportPackExpansionRequiresPack(TypeParser* parser,
                                            TemplateArgument* arg) {
  static Set reported_nodes;
  static Set reported_locations;
  static bool reported_init = false;
  if (!reported_init) {
    SetInitForPointers(&reported_nodes);
    SetInitForIntegers(&reported_locations);
    reported_init = true;
  }
  // Prefer the location recorded on the pattern node (where the construct was
  // written); fall back to the lexer's current position only when it is
  // unavailable (e.g. a synthesized argument).
  const char* filename;
  int lineno, start, end;
  DecodeSourceLocation(arg->location, &filename, &lineno, &start, &end);
  bool have_recorded_location =
      arg->location != SOURCE_LOCATION_MISSING && lineno > 0;
  // The location key must match the point we will actually report at, so that
  // copies of one pattern (produced e.g. by expanding a type alias at each use)
  // collapse to a single diagnosis while genuinely distinct occurrences stay
  // separate.  Fold the filename pointer and line together for a stable key.
  const char* key_file = have_recorded_location
                             ? filename
                             : parser->syntax->lex->source->filename.value;
  int key_line = have_recorded_location
                     ? lineno
                     : parser->syntax->lex->source->lineno;
  intptr_t location_key =
      (intptr_t)((((uintptr_t)key_file) << 20) ^ (uintptr_t)key_line);
  if (SetContains(&reported_nodes, arg) ||
      SetContains(&reported_locations, (void*)location_key)) {
    return;
  }
  int before = NumErrors();
  if (have_recorded_location) {
    SyntaxErrorAtLocation(
        parser->syntax, arg->location,
        "template argument pack expansion requires a parameter pack");
  } else {
    SyntaxError(parser->syntax,
                "template argument pack expansion requires a parameter pack");
  }
  if (NumErrors() > before) {
    SetInsert(&reported_nodes, arg);
    SetInsert(&reported_locations, (void*)location_key);
  }
}

/* Substitute one template argument and append the result(s) to `out`. A normal
 * argument appends a single substituted argument; a pack-expansion argument
 * (e.g. `Ts...`) expands into one substituted argument per pack element. */
/* A type pack expansion whose pack belongs to a member template not yet being
 * instantiated stays an expansion, but the enclosing arguments are bound in
 * its pattern now: `typename tuple_element<I, tuple<Args...>>::type...` keeps
 * `I` and becomes `tuple<int>`.  Left unbound, `Args` would collide with the
 * member's parameters once they are renumbered from 0. */
static void AppendPartiallySubstitutedPackExpansion(TypeParser* parser,
                                                    Vector* out,
                                                    TemplateArgument* arg,
                                                    Vector* args) {
  if (arg->kind != kTemplateParameterType || arg->type == NULL ||
      args == NULL || args->length == 0) {
    VectorAppend(out, TemplateArgumentCopy(arg));
    return;
  }
  TemplateArgument* partial = NewSubstitutedTemplateArgument(parser, arg, args);
  partial->is_pack_expansion = true;
  partial->references_parameter_pack = arg->references_parameter_pack;
  VectorAppend(out, partial);
}

static void AppendSubstitutedTemplateArgument(TypeParser* parser, Vector* out,
                                              TemplateArgument* arg,
                                              Vector* args) {
  if (arg == NULL) {
    return;
  }
  int index = -1;
  if (arg->is_pack_expansion) {
    TemplateArgument* pack = NULL;
    if (arg->kind == kTemplateParameterType &&
        (arg->type == NULL || !arg->type->is_pack_index) &&
        TypeIsTemplateParameterPlaceholder(arg->type, &index) &&
        index >= 0 && (size_t)index < args->length) {
      pack = args->value.p[index];
    } else if (arg->kind == kTemplateParameterNonType &&
               arg->template_parameter_index >= 0 &&
               (size_t)arg->template_parameter_index < args->length) {
      pack = args->value.p[arg->template_parameter_index];
    } else if (arg->kind == kTemplateParameterTemplate &&
               TemplateArgumentExtraOf(arg)->pack_index_expr == NULL &&
               arg->template_parameter_index >= 0 &&
               (size_t)arg->template_parameter_index < args->length) {
      pack = args->value.p[arg->template_parameter_index];
    }
    if (pack == NULL && index >= 0 &&
        (args == NULL || (size_t)index >= args->length)) {
      VectorAppend(out, TemplateArgumentCopy(arg));
      return;
    }
    if (pack == NULL && arg->template_parameter_index >= 0 &&
        (args == NULL ||
         (size_t)arg->template_parameter_index >= args->length)) {
      // This pack belongs to a nested member template rather than the
      // enclosing class template currently being substituted.  Preserve it
      // for the later member-template instantiation instead of diagnosing the
      // still-valid expansion as pack-free.
      VectorAppend(out, TemplateArgumentCopy(arg));
      return;
    }
    if (pack != NULL && pack->pack_arguments != NULL) {
      for (size_t i = 0; i < pack->pack_arguments->length; i++) {
        if (arg->kind == kTemplateParameterType && index >= 0 &&
            arg->type != NULL && GetQualifiers(arg->type) != kQualPlain) {
          Vector* element_args =
              TemplateArgumentVectorCopyForPackExpansionElement(args, index, i);
          TemplateArgument* subst =
              NewSubstitutedTemplateArgument(parser, arg, element_args);
          VectorAppend(out, subst);
          VectorDeleteWithContents(
              element_args, (VectorElementDestructor)TemplateArgumentDelete,
              /*free_element=*/false);
        } else {
          VectorAppend(out,
                       TemplateArgumentCopy(pack->pack_arguments->value.p[i]));
        }
      }
      return;
    }
    if (pack != NULL && pack->pack_arguments == NULL &&
        !pack->is_pack_expansion) {
      VectorAppend(out, TemplateArgumentCopy(pack));
      return;
    }
    if (pack != NULL && pack->is_pack_expansion) {
      VectorAppend(out, TemplateArgumentCopy(pack));
      return;
    }
    int pattern_pack_index = -1;
    size_t pattern_pack_length = 0;
    if (FindPackExpansionInTemplateArgument(
            arg, args, &pattern_pack_index, &pattern_pack_length) &&
        pattern_pack_index >= 0 && (size_t)pattern_pack_index < args->length) {
      TemplateArgument* pattern_pack = args->value.p[pattern_pack_index];
      if (pattern_pack != NULL && pattern_pack->pack_arguments != NULL) {
        for (size_t i = 0; i < pattern_pack->pack_arguments->length; i++) {
          Vector* element_args =
              TemplateArgumentVectorCopyForPackExpansionElement(
                  args, pattern_pack_index, i);
          TemplateArgument* subst =
              NewSubstitutedTemplateArgument(parser, arg, element_args);
          VectorAppend(out, subst);
          VectorDeleteWithContents(
              element_args, (VectorElementDestructor)TemplateArgumentDelete,
              /*free_element=*/false);
        }
        return;
      }
    }
    int unbound_index = FirstTemplateParameterIndexInType(arg->type);
    if (unbound_index >= 0 &&
        (args == NULL || (size_t)unbound_index >= args->length)) {
      // A qualified pattern such as `Args&&...` or
      // `decltype(f<Indices>())...` can name a pack owned by a nested member
      // template.  The enclosing class substitution has no binding for that
      // index yet, so retain the expansion for the member-template pass.
      AppendPartiallySubstitutedPackExpansion(parser, out, arg, args);
      return;
    }
    if (arg->references_parameter_pack) {
      // The pattern's syntax records a pack that is not bound by this
      // substitution pass (typically a nested member-template pack).
      AppendPartiallySubstitutedPackExpansion(parser, out, arg, args);
      return;
    }
    ReportPackExpansionRequiresPack(parser, arg);
    return;
  }

  // An already-bound parameter pack (its elements are populated but it is not
  // itself a pack expansion, e.g. the `Types...` binding carried by a concrete
  // `variant<int, char>`): re-substitute each element and keep the result as a
  // single pack argument.  Without this the pack -- whose own `type` is NULL --
  // would fall into the `type == NULL` early-return below and be dropped,
  // corrupting an already-concrete instantiation into an empty one when its
  // type is re-substituted in an unrelated context.
  if (arg->pack_arguments != NULL) {
    TemplateArgument* pack = TemplateArgumentAlloc();
    pack->kind = arg->kind;
    pack->is_pack_expansion = false;
    pack->type = NULL;
    pack->int_value = arg->int_value;
    pack->template_parameter_index = -1;
    pack->pack_arguments = NewVector();
    pack->dependent_expr = NULL;
    pack->location = arg->location;
    for (size_t i = 0; i < arg->pack_arguments->length; i++) {
      AppendSubstitutedTemplateArgument(parser, pack->pack_arguments,
                                        arg->pack_arguments->value.p[i], args);
    }
    VectorAppend(out, pack);
    return;
  }

  if (arg->kind == kTemplateParameterType && arg->type == NULL) {
    return;
  }

  VectorAppend(out, NewSubstitutedTemplateArgument(parser, arg, args));
}

/* Substitute every argument in `template_args` against the actuals `args`,
 * expanding pack expansions, to yield a fully concrete argument vector. */
Vector* SubstituteTemplateArgumentVectorForTypes(TypeParser* parser,
                                                        Vector* template_args,
                                                        Vector* args) {
  if (template_args == NULL) {
    return NULL;
  }
  Vector* concrete_args = NewVector();
  VectorReserve(concrete_args, template_args->length);
  for (size_t i = 0; i < template_args->length; i++) {
    AppendSubstitutedTemplateArgument(parser, concrete_args,
                                      template_args->value.p[i], args);
  }
  return concrete_args;
}

/* True if a function type has a parameter declared as a pack (variadic
 * template parameter), e.g. `void f(Ts... args)`. */
static bool FunctionTypeHasParameterPack(TypeRecord* type) {
  if (type == NULL || !TypeIsFunction(type)) {
    return false;
  }
  for (size_t i = 0; i < type->info.function.prototype.length; i++) {
    Symbol* formal = type->info.function.prototype.value.p[i];
    if (formal != NULL && formal->flags.is_parameter_pack) {
      return true;
    }
  }
  return false;
}

/* True when `str` is the class being instantiated: the substitution source,
 * the current specialization, or that specialization's primary template.
 * Member-function instantiation often sets source == target == the
 * specialization, so the primary's injected-class-name (`ElemT<Tuple, I>`)
 * would otherwise never match. */
static bool StructIsCurrentTemplateSelf(Struct* str, Struct* source,
                                        Struct* target) {
  if (str == NULL) {
    return false;
  }
  if (source != NULL && str == source) {
    return true;
  }
  if (target == NULL) {
    return false;
  }
  if (str == target) {
    return true;
  }
  if (target->tag_symbol == NULL || target->tag_symbol->type == NULL ||
      target->tag_symbol->type->template_origin == NULL ||
      target->tag_symbol->type->template_origin->type == NULL ||
      !TypeIsStructOrUnion(target->tag_symbol->type->template_origin->type)) {
    return false;
  }
  return target->tag_symbol->type->template_origin->type->info.struct_info ==
         str;
}

/* The pattern class and its nested classes are rewritten to the instantiation.
 * Any other concrete class (`Descriptor`, `string`) can be shared. */
static bool StructIsSubstitutionPattern(TypeParser* parser, Struct* str) {
  if (parser == NULL || str == NULL) {
    return false;
  }
  Struct* source = parser->template_substitution_source;
  Struct* enclosing = parser->enclosing_template_substitution_source;
  for (Struct* s = str; s != NULL; s = s->lexical_parent) {
    if (s == source || s == enclosing || s->instantiation_in_progress) {
      return true;
    }
  }
  if (StructIsCurrentTemplateSelf(str, source,
                                  parser->template_substitution_target)) {
    return true;
  }
  return StructIsCurrentTemplateSelf(
      str, enclosing, parser->enclosing_template_substitution_target);
}

static bool TypeMentionsSubstitutionPattern(TypeParser* parser,
                                            TypeRecord* type) {
  int guard = 0;
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (++guard > 64) {
      return true;
    }
    Struct* str = NULL;
    if (TypeIsStructOrUnion(t) || t->declarator == kDeclMemberPointer) {
      str = t->info.struct_info;
    }
    if (StructIsSubstitutionPattern(parser, str)) {
      return true;
    }
  }
  return false;
}

static bool SubstitutionCanShare(TypeParser* parser, TypeRecord* type) {
  return type != NULL && type->refs > 0 &&
         TypeRecordCanShareConcreteClass(type) &&
         !TypeMentionsSubstitutionPattern(parser, type);
}

/* Replace a self-reference to a template definition with the concrete struct
 * currently being instantiated. This keeps injected-class-name uses (`map`,
 * `iterator`, etc.) tied to the active instantiation instead of the primary
 * template's generic struct. */
static TypeRecord* SubstituteTemplateSelfReference(TypeRecord* type,
                                                   Struct* source,
                                                   Struct* target) {
  if (target == NULL || type->declarator != kDeclPrimitive ||
      !TypeIsStructOrUnion(type) || type->template_arguments != NULL ||
      !StructIsCurrentTemplateSelf(type->info.struct_info, source, target)) {
    return NULL;
  }
  // A lookup key borrowing `type`'s fields; interning copies it on a miss.
  // `type` may be a short record, so copy only the header.
  TypeRecord key = {0};
  memcpy(&key, type, offsetof(TypeRecord, info));
  key.shared_immutable = 0;
  key.refs = 0;
  key.template_parameter_summary = kTypeTemplateParameterSummaryUnknown;
  key.info.struct_info = target;
  if (target->tag_symbol != NULL && target->tag_symbol->type != NULL) {
    key.template_origin = target->tag_symbol->type->template_origin;
    key.template_arguments = target->tag_symbol->type->template_arguments;
  }
  if (TypeRecordCanIntern(&key)) {
    return TypeRecordIntern(&key);
  }
  TypeRecord* subst = TypeRecordCopy(type);
  TypeRecordSetStructInfo(subst, target);
  if (target->tag_symbol != NULL && target->tag_symbol->type != NULL) {
    if (subst->template_arguments != NULL) {
      VectorDeleteWithContents(subst->template_arguments,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    subst->template_origin = target->tag_symbol->type->template_origin;
    subst->template_arguments =
        TemplateArgumentVectorCopy(target->tag_symbol->type->template_arguments);
  }
  return TypeRecordCalculateSize(subst);
}

/* Look up a nested type name on the active instantiation. This handles cases
 * where the parsed type still points at the template definition's nested class,
 * but the concrete outer class already owns the substituted nested type. */
static TypeRecord* SubstituteNestedTypeFromActiveInstantiation(
    TypeParser* parser, TypeRecord* type, Vector* args) {
  if (!CompilerIsCXX() || parser == NULL ||
      type->declarator != kDeclPrimitive || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL || type->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  Struct* lookup_targets[] = {
      parser->template_substitution_target,
      parser->enclosing_template_substitution_target,
  };
  Struct* lookup_sources[] = {
      parser->template_substitution_source,
      parser->enclosing_template_substitution_source,
  };
  for (size_t i = 0; i < sizeof(lookup_targets) / sizeof(lookup_targets[0]);
       i++) {
    Struct* target = lookup_targets[i];
    Struct* source = lookup_sources[i];
    if (target == NULL || source == NULL ||
        type->info.struct_info->lexical_parent != source) {
      continue;
    }
    // `G<1>` instantiated from a member of `G<2>` has the same pattern as the
    // enclosing instantiation; its nested `Tag` is `G<1>::Tag`, even before
    // that member exists, never `G<2>::Tag`.
    if (i > 0 && source == lookup_sources[0]) {
      continue;
    }
    StructMember* member =
        FindStructMember(target, type->info.struct_info->tag_name);
    if (member != NULL && member->symbol != NULL &&
        member->symbol->type != NULL && TypeIsStructOrUnion(member->symbol->type)) {
      Struct* nested = member->symbol->type->info.struct_info;
      if (source != target && nested != NULL &&
          nested->lexical_parent == source &&
          StructHasMemberFunction(nested)) {
        return SubstituteNestedStructTemplateParameters(
            parser, member->symbol->type, args);
      }
      TypeRecord* subst = TypeRecordCopy(member->symbol->type);
      subst = TypeRecordAddQualifiers(subst, GetQualifiers(type));
      return TypeRecordCalculateSize(subst);
    }
  }
  return NULL;
}

/* True when `type` names a nested struct whose lexical parent is one of the
 * template definitions currently being substituted. Such structs must be
 * rebuilt member-by-member under the concrete parent. */
static bool TypeNamesActiveNestedTemplateStruct(TypeParser* parser,
                                                TypeRecord* type) {
  return CompilerIsCXX() && parser != NULL &&
         type->declarator == kDeclPrimitive && TypeIsStructOrUnion(type) &&
         type->info.struct_info != NULL &&
         ((parser->template_substitution_source != NULL &&
           type->info.struct_info->lexical_parent ==
               parser->template_substitution_source) ||
          (parser->enclosing_template_substitution_source != NULL &&
           type->info.struct_info->lexical_parent ==
               parser->enclosing_template_substitution_source));
}

/* Resolve a dependent member typedef such as `T::value_type` after `T` has been
 * substituted to a concrete class. Missing typedefs on concrete classes are
 * recorded as substitution failures for SFINAE-aware callers. */
Vector* SplitDependentMemberPath(String* path) {
  Vector* components = NewVector();
  if (path == NULL || path->value == NULL) {
    return components;
  }
  const char* start = path->value;
  const char* cursor = start;
  while (*cursor != '\0') {
    if (cursor[0] == ':' && cursor[1] == ':') {
      VectorAppend(components,
                   NewStringWithLength(start, (size_t)(cursor - start)));
      cursor += 2;
      start = cursor;
      continue;
    }
    cursor++;
  }
  VectorAppend(components,
               NewStringWithLength(start, (size_t)(cursor - start)));
  return components;
}

static bool DependentMemberPathHasMultipleComponents(String* path) {
  return path != NULL && path->value != NULL &&
         strstr(path->value, "::") != NULL;
}

void DeleteStringVector(Vector* strings) {
  if (strings == NULL) {
    return;
  }
  VectorDeleteWithContents(strings, (VectorElementDestructor)StringDestruct,
                           /*free_element=*/true);
}

static size_t DependentMemberPathLength(String* path) {
  Vector* components = SplitDependentMemberPath(path);
  size_t length = components->length;
  DeleteStringVector(components);
  return length;
}

/* `T::member` with `T` replaced by a type that is itself still dependent:
 * a parameter `U` gives `U::member`, `typename RR<U>::type` gives
 * `RR<U>::type::member`.  Returns NULL for any other replacement.  Keeping
 * `T::member` instead would leave `T`'s index to be read against whatever
 * arguments the result is substituted with next. */
static TypeRecord* ComposeDependentMemberPath(TypeRecord* root,
                                              TypeRecord* member) {
  if (root == NULL || root->declarator != kDeclPrimitive ||
      root->is_pack_index ||
      TypeDependentOf(root)->dependent_decltype_expr != NULL ||
      !TypeContainsTemplateParameter(root)) {
    return NULL;
  }
  bool parameter_root = root->template_parameter_index >= 0;
  bool template_id_root = root->template_parameter_index < 0 &&
                          root->template_origin != NULL &&
                          TypeDependentOf(root)->dependent_member_name != NULL;
  if (!parameter_root && !template_id_root) {
    return NULL;
  }
  TypeRecord* composed = TypeRecordCopy(root);
  SetQualifiers(composed, GetQualifiers(member));
  composed->template_parameter_summary = 0;
  if (TypeDependentOf(root)->dependent_member_name == NULL) {
    TypeMutableDependent(composed)->dependent_member_name =
        NewString(TypeDependentOf(member)->dependent_member_name->value);
    TypeMutableDependent(composed)->dependent_member_template_arguments =
        TemplateArgumentVectorListCopy(
            TypeDependentOf(member)->dependent_member_template_arguments);
    return TypeRecordCalculateSize(composed);
  }
  String path;
  StringInit(&path, TypeDependentOf(root)->dependent_member_name->value);
  StringAppend(&path, "::");
  StringAppend(&path, TypeDependentOf(member)->dependent_member_name->value);
  StringDelete(TypeMutableDependent(composed)->dependent_member_name);
  TypeMutableDependent(composed)->dependent_member_name = NewString(path.value);
  StringDestruct(&path);
  if (TypeDependentOf(root)->dependent_member_template_arguments != NULL ||
      TypeDependentOf(member)->dependent_member_template_arguments != NULL) {
    Vector* component_args = NewVector();
    Vector* parts[2] = {
        TypeDependentOf(root)->dependent_member_template_arguments,
        TypeDependentOf(member)->dependent_member_template_arguments};
    size_t counts[2] = {
        DependentMemberPathLength(TypeDependentOf(root)->dependent_member_name),
        DependentMemberPathLength(
            TypeDependentOf(member)->dependent_member_name)};
    for (int p = 0; p < 2; p++) {
      for (size_t i = 0; i < counts[p]; i++) {
        Vector* args = parts[p] != NULL && i < parts[p]->length
                           ? parts[p]->value.p[i]
                           : NULL;
        VectorAppend(component_args, TemplateArgumentVectorCopy(args));
      }
    }
    if (TypeDependentOf(composed)->dependent_member_template_arguments !=
        NULL) {
      VectorDeleteWithContents(
          TypeMutableDependent(composed)->dependent_member_template_arguments,
          (VectorElementDestructor)TemplateArgumentVectorDelete,
          /*free_element=*/false);
    }
    TypeMutableDependent(composed)->dependent_member_template_arguments =
        component_args;
  }
  return TypeRecordCalculateSize(composed);
}

Vector* RegroupExpandedClassTemplateArguments(Symbol* class_template,
                                              Vector* expanded);

TypeRecord* SubstituteDependentMemberType(TypeParser* parser,
                                                 TypeRecord* type,
                                                 Vector* args) {
  if (type->declarator != kDeclPrimitive ||
      type->template_parameter_index < 0 ||
      TypeDependentOf(type)->dependent_member_name == NULL ||
      type->is_pack_index) {
    return NULL;
  }
  int index = type->template_parameter_index;
  if (args == NULL || index < 0 || (size_t)index >= args->length) {
    return TypeRecordCopy(type);
  }
  TemplateArgument* arg = args->value.p[index];
  if (arg == NULL || arg->kind != kTemplateParameterType ||
      arg->type == NULL) {
    return TypeRecordCopy(type);
  }
  TypeRecord* composed = ComposeDependentMemberPath(arg->type, type);
  if (composed != NULL) {
    return composed;
  }
  // `typename T::member` when T has been replaced by a concrete non-class
  // (an integer, a pointer, an array) is ill-formed.  Detection idioms such
  // as `void_t<typename T::absl_container_hash>` rely on that substitution
  // failure to reject the partial specialization.  Copying the dependent
  // member type made the specialization match `uintptr_t`.  A T that is
  // still dependent or unknown keeps the member type for a later
  // substitution.
  if (!TypeIsStructOrUnion(arg->type) || arg->type->info.struct_info == NULL) {
    // A non-template nested class can retain an enclosing class parameter as
    // `T0::member` after its own parameter indices have been rebased away.
    // Resolve that member against the concrete enclosing specialization rather
    // than treating T0 as a (nonexistent) parameter of the nested class.
    Struct* source =
        parser != NULL ? parser->template_substitution_source : NULL;
    Struct* enclosing_source =
        parser != NULL ? parser->enclosing_template_substitution_source : NULL;
    Struct* enclosing_target =
        parser != NULL ? parser->enclosing_template_substitution_target : NULL;
    if (source != NULL && !source->is_template &&
        source->lexical_parent == enclosing_source &&
        enclosing_target != NULL) {
      StructMember* enclosing_member = FindStructMemberByName(
          enclosing_target,
          TypeDependentOf(type)->dependent_member_name->value);
      if (enclosing_member != NULL && enclosing_member->symbol != NULL &&
          StorageIs(enclosing_member->symbol->storage, STO(typedef)) &&
          enclosing_member->symbol->type != NULL) {
        TypeRecord* resolved = TypeRecordCopy(enclosing_member->symbol->type);
        resolved = TypeRecordAddQualifiers(resolved, GetQualifiers(type));
        return TypeRecordCalculateSize(resolved);
      }
    }
    if (!TypeContainsTemplateParameter(arg->type) &&
        !TypeIsUnknown(arg->type)) {
      if (parser != NULL) {
        parser->template_substitution_failed = true;
      }
    }
    return TypeRecordCopy(type);
  }
  bool multi_component_member = DependentMemberPathHasMultipleComponents(
      TypeDependentOf(type)->dependent_member_name);
  // `T::a::b` is stored as one member string.  Looking the whole string up as
  // a single name misses `a`.  Walk each component, the same way a path that
  // carries per-component template arguments already does.
  if (TypeDependentOf(type)->dependent_member_template_arguments != NULL ||
      multi_component_member) {
    Vector* components =
        SplitDependentMemberPath(TypeDependentOf(type)->dependent_member_name);
    TypeRecord* current = TypeRecordCopy(arg->type);
    TypeRecord* resolved = NULL;
    for (size_t i = 0; i < components->length; i++) {
      if (!TypeIsStructOrUnion(current) || current->info.struct_info == NULL) {
        break;
      }
      String* component = components->value.p[i];
      Vector* component_args = NULL;
      if (TypeDependentOf(type)->dependent_member_template_arguments != NULL &&
          i < TypeDependentOf(type)
                  ->dependent_member_template_arguments->length) {
        component_args = TypeDependentOf(type)
                             ->dependent_member_template_arguments->value.p[i];
      }
      TypeRecord* owner = current;
      StructMember* member =
          FindStructMember(owner->info.struct_info, component);
      current = NULL;
      if (member == NULL || member->symbol == NULL ||
          member->symbol->type == NULL ||
          (!StorageIs(member->symbol->storage, STO(typedef)) &&
           !TypeIsStructOrUnion(member->symbol->type))) {
        TypeRecordDelete(owner);
        break;
      }
      if (component_args != NULL) {
        if (!member->symbol->flags.is_template &&
            (!TypeIsStructOrUnion(member->symbol->type) ||
             member->symbol->type->info.struct_info == NULL ||
             !member->symbol->type->info.struct_info->is_template)) {
          TypeRecordDelete(owner);
          break;
        }
        Vector* concrete_args =
            SubstituteTemplateArgumentVectorForTypes(parser, component_args,
                                                    args);
        for (size_t j = 0; j < concrete_args->length; j++) {
          TemplateArgument* concrete_arg = concrete_args->value.p[j];
          if (concrete_arg != NULL &&
              concrete_arg->kind == kTemplateParameterType &&
              concrete_arg->type != NULL &&
              TypeContainsTemplateParameter(concrete_arg->type)) {
            TypeRecord* resolved_arg =
                SubstituteTemplateParameters(parser, concrete_arg->type, args);
            TypeRecordDelete(concrete_arg->type);
            concrete_arg->type = resolved_arg;
          }
        }
        // A member alias (`ElementType<N> = tuple_element<N, ElementTypes>::type`)
        // numbers `N` after the enclosing class.  Copying the pattern while the
        // use-site argument is still the member function's own parameter leaves
        // that class-relative index in the return type.  Substitute the pattern
        // instead, so the use-site parameter replaces the alias parameter.
        // An alias whose pattern is a class (`rebind_traits<T> =
        // AllocatorTraits<...>`) is still an alias, not a member class
        // template.
        bool substitutable_alias =
            member->symbol->flags.is_template &&
            StorageIs(member->symbol->storage, STO(typedef)) &&
            (SymbolExtraOf(member->symbol)->alias_template != NULL ||
             !TypeIsStructOrUnion(member->symbol->type));
        if (TemplateArgumentVectorContainsTemplateParameter(concrete_args) &&
            !substitutable_alias) {
          if (i + 1 == components->length) {
            resolved = TypeRecordCopy(member->symbol->type);
            if (resolved->template_arguments != NULL) {
              VectorDeleteWithContents(
                  resolved->template_arguments,
                  (VectorElementDestructor)TemplateArgumentDelete,
                  /*free_element=*/false);
            }
            resolved->template_origin = member->symbol;
            resolved->template_arguments = concrete_args;
          } else {
            VectorDeleteWithContents(
                concrete_args, (VectorElementDestructor)TemplateArgumentDelete,
                /*free_element=*/false);
            resolved = TypeRecordCopy(type);
          }
          TypeRecordDelete(owner);
          break;
        }
        // A member alias of a variadic class (`TypeList<T...>::Apply<Templ> =
        // Templ<T...>`) numbers Templ after the enclosing pack.  Instantiating
        // the alias with only `<Templ>` copies the pattern and leaves Templ's
        // index bound to whatever shares that slot.  Substitute the pattern
        // with the enclosing arguments folded back into one pack slot, then
        // the alias's own arguments.
        if (substitutable_alias) {
          Vector* class_args = owner->template_arguments;
          if (class_args == NULL &&
              owner->info.struct_info->tag_symbol != NULL &&
              owner->info.struct_info->tag_symbol->type != NULL) {
            class_args =
                owner->info.struct_info->tag_symbol->type->template_arguments;
          }
          Vector* grouped = RegroupExpandedClassTemplateArguments(
              owner->template_origin, class_args);
          Vector* pattern_args = TemplateArgumentVectorCopy(
              grouped != NULL ? grouped : class_args);
          if (grouped != NULL) {
            VectorDeleteWithContents(
                grouped, (VectorElementDestructor)TemplateArgumentDelete,
                /*free_element=*/false);
          }
          if (pattern_args == NULL) {
            pattern_args = NewVector();
          }
          for (size_t j = 0; j < concrete_args->length; j++) {
            VectorAppend(pattern_args,
                         TemplateArgumentCopy(concrete_args->value.p[j]));
          }
          current = SubstituteTemplateParameters(parser, member->symbol->type,
                                                 pattern_args);
          VectorDeleteWithContents(
              pattern_args, (VectorElementDestructor)TemplateArgumentDelete,
              /*free_element=*/false);
        } else {
          current = InstantiateSimpleClassTemplate(parser, member->symbol,
                                                   concrete_args);
        }
        VectorDeleteWithContents(concrete_args,
                                 (VectorElementDestructor)TemplateArgumentDelete,
                                 /*free_element=*/false);
        TypeRecordDelete(owner);
      } else {
        Vector* owner_args = owner->template_arguments;
        if (owner_args == NULL && owner->info.struct_info->tag_symbol != NULL &&
            owner->info.struct_info->tag_symbol->type != NULL) {
          owner_args =
              owner->info.struct_info->tag_symbol->type->template_arguments;
        }
        current = owner_args != NULL
                      ? SubstituteTemplateParameters(
                            parser, member->symbol->type, owner_args)
                      : TypeRecordCopy(member->symbol->type);
        TypeRecordDelete(owner);
      }
      if (i + 1 == components->length) {
        resolved = current;
        current = NULL;
      }
    }
    TypeRecordDelete(current);
    DeleteStringVector(components);
    if (resolved != NULL) {
      resolved = TypeRecordAddQualifiers(resolved, GetQualifiers(type));
      return TypeRecordCalculateSize(resolved);
    }
    if (parser != NULL &&
        !StructContainsTemplateParameter(arg->type->info.struct_info)) {
      parser->template_substitution_failed = true;
    }
    return TypeRecordCopy(type);
  }
  // The argument class may still be an uninstantiated specialization (e.g. it
  // was only ever named as a template argument, as in `f<wrap<ratio<...>>>()`).
  // Instantiate it so its member typedefs are present before looking one up.
  TypeRecord* owner_type =
      TypeMaterializeClassTemplateSpecialization(parser->syntax, arg->type);
  Struct* owner_struct = TypeIsStructOrUnion(owner_type) &&
                                 owner_type->info.struct_info != NULL
                             ? owner_type->info.struct_info
                             : arg->type->info.struct_info;
  StructMember* member = FindStructMember(
      owner_struct, TypeDependentOf(type)->dependent_member_name);
  if (member == NULL || member->symbol == NULL ||
      !StorageIs(member->symbol->storage, STO(typedef))) {
    if (parser != NULL &&
        !StructContainsTemplateParameter(arg->type->info.struct_info)) {
      parser->template_substitution_failed = true;
    }
    if (owner_type != arg->type) {
      TypeRecordDelete(owner_type);
    }
    return TypeRecordCopy(type);
  }
  TypeRecord* subst = NULL;
  // An inherited typedef can still carry parameter indices from the base
  // template that declared it. Substitute those indices with the concrete
  // base specialization's arguments rather than copying the base's pattern
  // type into the derived specialization.
  Struct* declaring = StructDeclaringSymbol(owner_struct, member->symbol, 0);
  TypeRecord* owner_tag_type =
      owner_struct->tag_symbol != NULL ? owner_struct->tag_symbol->type : NULL;
  Vector* owner_args =
      owner_tag_type != NULL ? owner_tag_type->template_arguments : NULL;
  if (declaring == owner_struct && owner_args != NULL &&
      TypeReferencesIndexAtLeast(member->symbol->type,
                                 (int)owner_args->length)) {
    Struct* inherited = StructBaseDeclaringMember(
        owner_struct, TypeDependentOf(type)->dependent_member_name, 0);
    if (inherited != NULL) {
      declaring = inherited;
    }
  }
  if (declaring != NULL && declaring != owner_struct &&
      declaring->tag_symbol != NULL &&
      declaring->tag_symbol->type != NULL &&
      declaring->tag_symbol->type->template_arguments != NULL &&
      TypeContainsTemplateParameter(member->symbol->type)) {
    TypeRecord* declaring_type = declaring->tag_symbol->type;
    Vector* declaring_args = declaring_type->template_arguments;
    Vector* grouped_args = RegroupExpandedClassTemplateArguments(
        declaring_type->template_origin, declaring_args);
    subst = SubstituteTemplateParameters(
        parser, member->symbol->type,
        grouped_args != NULL ? grouped_args : declaring_args);
    if (grouped_args != NULL) {
      VectorDeleteWithContents(
          grouped_args, (VectorElementDestructor)TemplateArgumentDelete,
          /*free_element=*/false);
    }
  }
  if (subst == NULL) {
    subst = TypeRecordCopy(member->symbol->type);
  }
  subst = TypeRecordAddQualifiers(subst, GetQualifiers(type));
  // The member typedef may itself resolve to a lazy dependent-member type such
  // as `typename ratio<1,1000>::type` (from `using period = typename
  // Period::type;`).  Collapse it to the concrete specialization so later
  // qualified accesses (`W::period::num`) resolve to real members and fold.
  subst = TypeMaterializeClassTemplateSpecialization(parser->syntax, subst);
  if (owner_type != arg->type) {
    TypeRecordDelete(owner_type);
  }
  return TypeRecordCalculateSize(subst);
}

static TypeRecord* SubstituteResolvedDependentMemberSuffix(
    TypeParser* parser, TypeRecord* pattern, TypeRecord* root, Vector* args) {
  if (pattern == NULL ||
      TypeDependentOf(pattern)->dependent_member_name == NULL || root == NULL) {
    return root;
  }
  Vector* path_args = TemplateArgumentVectorCopy(args);
  if (path_args == NULL) {
    path_args = NewVector();
  }
  int root_index = (int)path_args->length;
  VectorAppend(path_args, NewTypeTemplateArgument(root));
  TypeRecord* member_pattern = TypeRecordCopy(pattern);
  member_pattern->declarator = kDeclPrimitive;
  member_pattern->template_parameter_index = root_index;
  TypeRecord* result =
      SubstituteDependentMemberType(parser, member_pattern, path_args);
  TypeRecordDelete(member_pattern);
  VectorDeleteWithContents(
      path_args, (VectorElementDestructor)TemplateArgumentDelete,
      /*free_element=*/false);
  TypeRecordDelete(root);
  return result;
}

/* Recover the template-id represented by `type`. Some instantiated class types
 * carry the origin/arguments on the TypeRecord itself; others only have them on
 * the canonical tag type. This helper normalizes those two encodings. */
static TemplateIdSubstitutionInfo TemplateIdInfoForSubstitution(
    TypeRecord* type) {
  TemplateIdSubstitutionInfo info = {
      type != NULL ? type->template_origin : NULL,
      type != NULL ? type->template_arguments : NULL,
  };
  if (TypeIsStructOrUnion(type) && type->info.struct_info != NULL &&
      type->info.struct_info->tag_symbol != NULL &&
      type->info.struct_info->tag_symbol->type != NULL) {
    TypeRecord* tag_type = type->info.struct_info->tag_symbol->type;
    if (tag_type->template_origin != NULL &&
        (info.origin == NULL || tag_type->template_origin == info.origin)) {
      info.origin = tag_type->template_origin;
      if (info.args == NULL) {
        info.args = tag_type->template_arguments;
      }
    } else if (info.origin == NULL && info.args != NULL &&
               type->info.struct_info->is_template &&
               type->info.struct_info->tag_symbol->flags.is_template) {
      info.origin = type->info.struct_info->tag_symbol;
    }
  }
  return info;
}

/* Substitute a template-id (`Wrapper<T>`, alias templates, and dependent
 * `enable_if<T>::type`-style member typedefs). If any argument remains
 * dependent, keep the template-id deferred rather than instantiating the primary
 * template too early. */
/* Concrete class-template arguments are stored expanded (`Box<char, int>` is
 * `[char, int]`).  A member of that class numbers its parameters after the
 * enclosing list, where a pack occupies a single slot.  Rebuild `expanded`
 * into that shape, or return NULL when it already matches (no pack, or the
 * pack slot is already a pack argument).  The caller deletes the result. */
Vector* RegroupExpandedClassTemplateArguments(Symbol* class_template,
                                              Vector* expanded) {
  if (class_template == NULL || expanded == NULL ||
      class_template->type == NULL ||
      !TypeIsStructOrUnion(class_template->type) ||
      class_template->type->info.struct_info == NULL) {
    return NULL;
  }
  Vector* params =
      &class_template->type->info.struct_info->template_parameters;
  int pack_index = -1;
  for (size_t i = 0; i < params->length; i++) {
    TemplateParameter* param = params->value.p[i];
    if (param != NULL && param->is_parameter_pack) {
      pack_index = (int)i;
      break;
    }
  }
  if (pack_index < 0) {
    return NULL;
  }
  if (expanded->length == params->length) {
    TemplateArgument* slot = expanded->value.p[pack_index];
    if (slot != NULL && slot->pack_arguments != NULL) {
      return NULL;
    }
  }
  size_t fixed_after = params->length - (size_t)pack_index - 1;
  if (expanded->length < (size_t)pack_index + fixed_after) {
    return NULL;
  }
  Vector* grouped = NewVector();
  size_t ei = 0;
  for (size_t pi = 0; pi < params->length; pi++) {
    TemplateParameter* param = params->value.p[pi];
    if (param != NULL && param->is_parameter_pack) {
      TemplateArgument* pack = NewEmptyPackTemplateArgument(param->kind);
      size_t pack_end = expanded->length - fixed_after;
      while (ei < pack_end) {
        VectorAppend(pack->pack_arguments,
                     TemplateArgumentCopy(expanded->value.p[ei]));
        ei++;
      }
      VectorAppend(grouped, pack);
    } else if (ei < expanded->length) {
      VectorAppend(grouped, TemplateArgumentCopy(expanded->value.p[ei]));
      ei++;
    }
  }
  return grouped;
}

/* Return the active specialization when `class_template` is the class whose
 * member signature is currently being substituted.  Re-instantiating that
 * class while its member function is being formed can recursively rebuild its
 * bases and virtual overrides. */
static TypeRecord* ActiveClassTemplateSpecialization(
    TypeParser* parser, Symbol* class_template) {
  if (parser == NULL || class_template == NULL) {
    return NULL;
  }
  Struct* candidates[] = {
      parser->template_substitution_target,
      parser->enclosing_template_substitution_target,
  };
  for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
    Struct* str = candidates[i];
    if (str != NULL && str->tag_symbol != NULL &&
        str->tag_symbol->type != NULL &&
        str->tag_symbol->type->template_origin == class_template) {
      return TypeRecordCopy(str->tag_symbol->type);
    }
  }
  return NULL;
}

/* True when `name` is declared by a base of the primary class template.  The
 * ordinary member lookup skips dependent bases, but their primary member lists
 * are still useful for deciding whether a deferred member names an inherited
 * alias such as `raw_hash_map::reference`. */
static bool PrimaryTemplateBaseDeclaresMember(Struct* str, String* name) {
  if (str == NULL || name == NULL) {
    return false;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base == NULL || base->type == NULL ||
        !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    StructMember* member =
        FindStructMember(base->type->info.struct_info, name);
    if (member != NULL && member->symbol != NULL &&
        StorageIs(member->symbol->storage, STO(typedef)) &&
        member->symbol->type != NULL &&
        !TypeIsStructOrUnion(member->symbol->type)) {
      return true;
    }
  }
  return false;
}

/* Strip trailing member-template arguments when instantiating a non-variadic
 * class solely to resolve one of its inherited aliases. */
static Vector* NonVariadicClassTemplateArgumentPrefix(
    Symbol* class_template, Vector* args) {
  if (class_template == NULL || class_template->type == NULL ||
      !TypeIsStructOrUnion(class_template->type) ||
      class_template->type->info.struct_info == NULL || args == NULL) {
    return NULL;
  }
  Vector* params =
      &class_template->type->info.struct_info->template_parameters;
  if (args->length <= params->length) {
    return NULL;
  }
  for (size_t i = 0; i < params->length; i++) {
    TemplateParameter* param = params->value.p[i];
    if (param != NULL && param->is_parameter_pack) {
      return NULL;
    }
  }
  Vector* prefix = NewVector();
  VectorReserve(prefix, params->length);
  for (size_t i = 0; i < params->length; i++) {
    VectorAppend(prefix, TemplateArgumentCopy(args->value.p[i]));
  }
  return prefix;
}

static bool ExprReferencesIndexAtLeast(ASTNode* node, void* data) {
  int min_index = *(int*)data;
  if (node == NULL || node->op != AST_OP(identifier) || min_index <= 0) {
    return false;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  if (id->symbol == NULL) {
    return false;
  }
  if (id->symbol->flags.is_template_parameter &&
      id->symbol->template_parameter_index >= min_index) {
    return true;
  }
  if (SymbolDependentValueTemplateParameterIndex(id->symbol) >= min_index) {
    return true;
  }
  return id->symbol->type != NULL &&
         TypeReferencesIndexAtLeast(id->symbol->type, min_index);
}

/* True when `arg` names a template parameter that is not in a substitution
 * vector of length `min_index`.  Expanding an alias in that state binds the
 * missing parameter to whatever happens to sit at a lower index. */
static bool TemplateArgumentReferencesIndexAtLeast(TemplateArgument* arg,
                                                   int min_index) {
  if (arg == NULL || min_index <= 0) {
    return false;
  }
  if (arg->pack_arguments != NULL) {
    for (size_t i = 0; i < arg->pack_arguments->length; i++) {
      if (TemplateArgumentReferencesIndexAtLeast(arg->pack_arguments->value.p[i],
                                                min_index)) {
        return true;
      }
    }
    return false;
  }
  if (arg->template_parameter_index >= min_index) {
    return true;
  }
  if (TypeReferencesIndexAtLeast(arg->type, min_index)) {
    return true;
  }
  return arg->dependent_expr != NULL &&
         ASTNodeAny(arg->dependent_expr, ExprReferencesIndexAtLeast, &min_index);
}

static bool TypeReferencesIndexAtLeast(TypeRecord* type, int min_index) {
  if (type == NULL || min_index <= 0) {
    return false;
  }
  for (TypeRecord* current = type; current != NULL; current = current->next) {
    if (current->template_parameter_index >= min_index) {
      return true;
    }
    if (current->declarator == kDeclArray &&
        current->info.array.template_parameter_index >= min_index) {
      return true;
    }
    if (TypeDependentOf(current)->dependent_decltype_expr != NULL &&
        ASTNodeAny(TypeDependentOf(current)->dependent_decltype_expr,
                   ExprReferencesIndexAtLeast, &min_index)) {
      return true;
    }
    if (current->template_arguments == NULL) {
      continue;
    }
    for (size_t i = 0; i < current->template_arguments->length; i++) {
      if (TemplateArgumentReferencesIndexAtLeast(
              current->template_arguments->value.p[i], min_index)) {
        return true;
      }
    }
  }
  return false;
}

bool TypeNamesTemplateParameterAtOrAbove(TypeRecord* type, int min_index) {
  return TypeReferencesIndexAtLeast(type, min_index);
}

/* The struct whose member list directly contains `symbol`, searching bases.
 * Inherited alias templates (`Layout::ElementType` declared on LayoutImpl)
 * keep their parameter indices in that base's parameter space. */
static Struct* StructDeclaringSymbol(Struct* str, Symbol* symbol, int depth) {
  if (str == NULL || symbol == NULL || depth > 1024) {
    return NULL;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member != NULL && member->symbol == symbol) {
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
    Struct* found =
        StructDeclaringSymbol(base->type->info.struct_info, symbol, depth + 1);
    if (found != NULL) {
      return found;
    }
  }
  return NULL;
}

// Finds the concrete base specialization that directly declares `name`.
// Instantiated derived classes can contain an inherited typedef clone whose
// type still uses the base template's parameter indices.
static Struct* StructBaseDeclaringMember(Struct* str, String* name, int depth) {
  if (str == NULL || name == NULL || depth > 1024) {
    return NULL;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base == NULL || base->type == NULL ||
        !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL ||
        base->type->info.struct_info == str) {
      continue;
    }
    Struct* base_struct = base->type->info.struct_info;
    StructMember* direct = FindOwnStructMember(base_struct, name);
    if (direct != NULL) {
      return base_struct;
    }
    Struct* inherited =
        StructBaseDeclaringMember(base_struct, name, depth + 1);
    if (inherited != NULL) {
      return inherited;
    }
  }
  return NULL;
}

/* `Z<i>` with `i` a value-dependent expression (a constexpr local of a
 * template, `One<U>()`).  Substituting such an argument into one of Z's
 * members cannot replace Z's parameter, so the member must wait for a
 * concrete `i`. */
static bool TemplateArgumentsHaveValueDependentExpression(Vector* args) {
  for (size_t i = 0; args != NULL && i < args->length; i++) {
    TemplateArgument* arg = args->value.p[i];
    if (arg != NULL && arg->kind == kTemplateParameterNonType &&
        arg->template_parameter_index < 0 && arg->dependent_expr != NULL &&
        TemplateArgumentContainsTemplateParameter(arg)) {
      return true;
    }
  }
  return false;
}

/* `template <class U> using rt = Tr<ra<U>>` inside `template <class A> struct
 * Tr`, where `ra` is a sibling member alias template.  While `rt`'s pattern is
 * substituted for `Tr<Sym>`, `ra<U>` cannot expand because `U` belongs to `rt`,
 * so it stays a deferred alias-id.  Bound to the pattern's `ra`, it would
 * expand later (`Tr<Sym>::rt<int>`) with no enclosing class in scope and leave
 * `A` unsubstituted.  `Tr<Sym>`'s own `ra` already has `A` substituted, so the
 * deferred id must name it instead.
 *
 * Only member alias patterns need this.  A member function template's
 * signature (`operator[](key_arg<K>&&)`) keeps the pattern's alias: its
 * deduction and instantiation run with the class in scope and number `K` in
 * the pattern's parameter space.
 *
 * Returns NULL when no rebinding applies. */
static Symbol* ConcreteMemberAliasTemplate(TypeParser* parser, Symbol* origin) {
  if (parser == NULL || parser->member_alias_pattern_source == NULL ||
      parser->member_alias_pattern_source !=
          parser->template_substitution_source ||
      origin == NULL || SymbolExtraOf(origin)->alias_template == NULL ||
      !origin->flags.is_template || !StorageIs(origin->storage, STO(typedef))) {
    return NULL;
  }
  Struct* sources[2] = {parser->template_substitution_source,
                        parser->enclosing_template_substitution_source};
  Struct* targets[2] = {parser->template_substitution_target,
                        parser->enclosing_template_substitution_target};
  for (int i = 0; i < 2; i++) {
    if (sources[i] == NULL || targets[i] == NULL || sources[i] == targets[i]) {
      continue;
    }
    // The class-body lookup symbol is a separate Symbol from the member, but
    // both are created over the same pattern type.
    StructMember* pattern =
        MapFindPointerKey(&sources[i]->symbol_table, &origin->name);
    if (pattern == NULL || pattern->symbol == NULL ||
        (pattern->symbol != origin &&
         (origin->type == NULL || pattern->symbol->type != origin->type))) {
      continue;
    }
    StructMember* concrete =
        MapFindPointerKey(&targets[i]->symbol_table, &origin->name);
    if (concrete == NULL || concrete->symbol == NULL ||
        concrete->symbol == origin ||
        SymbolExtraOf(concrete->symbol)->alias_template == NULL ||
        !concrete->symbol->flags.is_template ||
        !StorageIs(concrete->symbol->storage, STO(typedef))) {
      return NULL;
    }
    return concrete->symbol;
  }
  return NULL;
}

TypeRecord* SubstituteTemplateIdType(TypeParser* parser,
                                            TypeRecord* type,
                                            Vector* args,
                                            TemplateIdSubstitutionInfo info) {
  if (info.origin == NULL || info.args == NULL) {
    return NULL;
  }
  if (info.origin->flags.is_template_template_parameter) {
    int index = type->template_parameter_index >= 0
                    ? type->template_parameter_index
                    : info.origin->template_parameter_index;
    if (index < 0 || args == NULL || (size_t)index >= args->length) {
      return TypeRecordCopy(type);
    }
    TemplateArgument* actual = args->value.p[index];
    if (actual == NULL || actual->kind != kTemplateParameterTemplate ||
        TemplateArgumentExtraOf(actual)->template_symbol == NULL) {
      return TypeRecordCopy(type);
    }
    info.origin = TemplateArgumentExtraOf(actual)->template_symbol;
  }
  bool remapped_enclosing_nested_template = false;
  // A deferred template-id (`VP<M>` in a member function template of the
  // enclosing class) has no struct of its own yet; the origin's struct says
  // which class template it is nested in.
  Struct* nested_struct = type->info.struct_info;
  if ((nested_struct == NULL || nested_struct->tag_name == NULL) &&
      info.origin->type != NULL && TypeIsStructOrUnion(info.origin->type)) {
    nested_struct = info.origin->type->info.struct_info;
  }
  if (parser != NULL && nested_struct != NULL &&
      nested_struct->tag_name != NULL &&
      ((parser->template_substitution_source != NULL &&
        parser->template_substitution_target != NULL &&
        nested_struct->lexical_parent ==
            parser->template_substitution_source) ||
       (parser->enclosing_template_substitution_source != NULL &&
        parser->enclosing_template_substitution_target != NULL &&
        nested_struct->lexical_parent ==
            parser->enclosing_template_substitution_source))) {
    Struct* concrete_parent =
        nested_struct->lexical_parent == parser->template_substitution_source
            ? parser->template_substitution_target
            : parser->enclosing_template_substitution_target;
    StructMember* concrete_nested =
        FindStructMember(concrete_parent, nested_struct->tag_name);
    if (concrete_nested != NULL && concrete_nested->symbol != NULL &&
        concrete_nested->symbol->flags.is_template) {
      info.origin = concrete_nested->symbol;
      remapped_enclosing_nested_template = true;
    }
  }
  // A template-id naming another specialization of the current class
  // (`cursor<Other>` inside `cursor<Const>`) must be rebound to the concrete
  // enclosing template's origin.  Keeping the primary nested-class symbol here
  // loses the already-substituted outer arguments, so deduction later compares
  // template-ids from different origins and rejects valid converting
  // constructors such as iterator-to-const_iterator.
  if (!remapped_enclosing_nested_template && parser != NULL &&
      type->info.struct_info != NULL &&
      parser->template_substitution_source != NULL &&
      parser->template_substitution_target != NULL &&
      type->info.struct_info == parser->template_substitution_source &&
      parser->template_substitution_target->tag_symbol != NULL &&
      parser->template_substitution_target->tag_symbol->type != NULL &&
      parser->template_substitution_target->tag_symbol->type->template_origin !=
          NULL) {
    info.origin =
        parser->template_substitution_target->tag_symbol->type->template_origin;
  }
  Symbol* concrete_alias = ConcreteMemberAliasTemplate(parser, info.origin);
  if (concrete_alias != NULL) {
    info.origin = concrete_alias;
  }
  Vector* concrete_args =
      SubstituteTemplateArgumentVectorForTypes(parser, info.args, args);
  // An alias such as `using key_arg = typename KeyArgImpl::template type<K,
  // key_type>` stores a struct-typed pattern (KeyArg) plus a dependent member
  // name.  That is still an alias pattern: expand it, instead of looking
  // `type` up on the primary and substituting it with only the alias arguments.
  bool alias_dependent_member_pattern =
      SymbolExtraOf(info.origin)->alias_template != NULL &&
      info.origin->type != NULL &&
      TypeDependentOf(info.origin->type)->dependent_member_name != NULL;
  bool expandable_alias =
      CompilerIsCXX() && info.origin->flags.is_template &&
      StorageIs(info.origin->storage, STO(typedef)) &&
      (!TypeIsStructOrUnion(info.origin->type) ||
       alias_dependent_member_pattern) &&
      !CXXAliasTemplatePatternNamesClassTemplate(info.origin);
  bool dependent_args =
      TemplateArgumentVectorContainsTemplateParameter(concrete_args);
  // An alias argument may name a parameter that this vector does not hold
  // (a member template's `T`, numbered after the enclosing class).  Expanding
  // the pattern now turns `Trait<const T&>` into the primary of the pattern.
  // Keep the alias-id so the call, which does have `T`, can expand it.
  bool alias_arg_not_in_vector = false;
  if (expandable_alias && args != NULL && args->length > 0) {
    int min_index = (int)args->length;
    for (size_t i = 0; concrete_args != NULL && i < concrete_args->length; i++) {
      if (TemplateArgumentReferencesIndexAtLeast(concrete_args->value.p[i],
                                                min_index)) {
        alias_arg_not_in_vector = true;
        break;
      }
    }
  }
  if (dependent_args && !expandable_alias &&
      TypeDependentOf(type)->dependent_member_name != NULL &&
      TypeIsStructOrUnion(info.origin->type) &&
      info.origin->type->info.struct_info != NULL &&
      info.origin->type->info.struct_info->partial_specializations.length ==
          0 &&
      !TemplateArgumentsHaveValueDependentExpression(concrete_args)) {
    // A dependent member of a primary class template with no partial
    // specializations can be resolved from the primary immediately.  This is
    // important for traits such as
    // `add_rvalue_reference_t<T> = add_rvalue_reference<T>::type`: preserving
    // the resulting `T&&` lets a surrounding dependent decltype retain the
    // correct value category instead of degrading to the parser's `int`
    // placeholder.
    StructMember* member =
        FindStructMember(info.origin->type->info.struct_info,
                         TypeDependentOf(type)->dependent_member_name);
    // A nested class/struct member (its injected type-name, stored as a
    // typedef to a struct lexically owned by the origin template) must NOT be
    // resolved from the primary while any argument is still dependent: doing so
    // yields the *primary's* single shared nested type and drops the differing
    // argument (e.g. a member function template's own parameter appearing in the
    // enclosing class-template's argument list, `Map<Key, C2>::iterator`).  Such
    // a type must stay deferred as `Origin<concrete_args>::member` so it later
    // resolves to the correct specialization's nested type.  The shortcut is
    // meant only for genuine alias/typedef members (e.g. `add_rvalue_reference<
    // T>::type = T&&`) whose definition substitutes correctly.
    bool member_is_nested_type =
        member != NULL && member->symbol != NULL &&
        member->symbol->type != NULL &&
        TypeIsStructOrUnion(member->symbol->type) &&
        member->symbol->type->info.struct_info != NULL &&
        member->symbol->type->info.struct_info->lexical_parent ==
            info.origin->type->info.struct_info;
    // A member alias template (`KeyArg<B>::type<K, key_type> = K`) numbers its
    // parameters after the enclosing class.  `concrete_args` is only that
    // class list; the `<K, key_type>` arguments are on the dependent member.
    // Substituting the pattern with the class arguments alone copies `K` at
    // its enclosing-relative index, and deduction of `find(const key_arg<K>&)`
    // then writes past the function's own argument vector.
    if (member != NULL && member->symbol != NULL && !member_is_nested_type &&
        member->symbol->flags.is_template &&
        StorageIs(member->symbol->storage, STO(typedef)) &&
        !TypeIsStructOrUnion(member->symbol->type) &&
        SymbolExtraOf(info.origin)->alias_template == NULL &&
        TypeDependentOf(type)->dependent_member_template_arguments != NULL &&
        TypeDependentOf(type)->dependent_member_template_arguments->length >
            0) {
      Vector* member_arg_list =
          TypeDependentOf(type)
              ->dependent_member_template_arguments->value.p[0];
      Vector* concrete_member_args = SubstituteTemplateArgumentVectorForTypes(
          parser, member_arg_list, args);
      Vector* grouped =
          RegroupExpandedClassTemplateArguments(info.origin, concrete_args);
      Vector* pattern_args = TemplateArgumentVectorCopy(
          grouped != NULL ? grouped : concrete_args);
      if (grouped != NULL) {
        VectorDeleteWithContents(
            grouped, (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
      }
      if (pattern_args == NULL) {
        pattern_args = NewVector();
      }
      if (concrete_member_args != NULL) {
        for (size_t j = 0; j < concrete_member_args->length; j++) {
          VectorAppend(pattern_args,
                       TemplateArgumentCopy(concrete_member_args->value.p[j]));
        }
        VectorDeleteWithContents(
            concrete_member_args,
            (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
      }
      TypeRecord* subst = SubstituteTemplateParameters(
          parser, member->symbol->type, pattern_args);
      subst = TypeRecordAddQualifiers(subst, GetQualifiers(type));
      VectorDeleteWithContents(
          pattern_args, (VectorElementDestructor)TemplateArgumentDelete,
          /*free_element=*/false);
      VectorDeleteWithContents(concrete_args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      return TypeRecordCalculateSize(subst);
    }
    if (member != NULL && member->symbol != NULL && !member_is_nested_type &&
        StorageIs(member->symbol->storage, STO(typedef))) {
      // `ElemFromList<J, char, Big>` arrives flat; the pattern expands `T...`
      // from a single pack argument.
      Vector* grouped =
          RegroupExpandedClassTemplateArguments(info.origin, concrete_args);
      TypeRecord* subst = SubstituteTemplateParameters(
          parser, member->symbol->type,
          grouped != NULL ? grouped : concrete_args);
      if (grouped != NULL) {
        VectorDeleteWithContents(
            grouped, (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
      }
      if (subst != NULL) {
        subst = TypeRecordAddQualifiers(subst, GetQualifiers(type));
        VectorDeleteWithContents(
            concrete_args, (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
        return TypeRecordCalculateSize(subst);
      }
    }
  }
  if (alias_arg_not_in_vector || (dependent_args && !expandable_alias)) {
    TypeRecord* deferred = TypeRecordCopy(type);
    if (deferred->template_arguments != NULL) {
      VectorDeleteWithContents(
          deferred->template_arguments,
          (VectorElementDestructor)TemplateArgumentDelete,
          /*free_element=*/false);
    }
    deferred->template_arguments = concrete_args;
    deferred->template_origin = info.origin;
    deferred = TypeRecordAddQualifiers(deferred, GetQualifiers(type));
    return deferred;
  }
  if (expandable_alias) {
    // Regroup the now-concrete arguments into the alias's declared parameter
    // shape before substituting the pattern.  A variadic alias such as
    // `template<class... T> using A = typename Box<T...>::type;` stores its
    // arguments ungrouped when first named with a dependent argument list
    // (e.g. `A<X, Y>` inside another template); substituting the pattern's
    // `Box<T...>` with the flat `[int, long]` would then fail to expand `T...`
    // ("pack expansion requires a parameter pack").  CompleteAliasTemplate-
    // Arguments folds the trailing arguments back into the pack so the pattern
    // sees a single pack argument.
    Vector* grouped_args =
        CompleteMemberAliasTemplateArguments(parser, info.origin,
                                             concrete_args);
    Vector* completed = grouped_args != NULL ? grouped_args : concrete_args;
    Vector* prefixed =
        MemberAliasPatternArguments(parser, info.origin, completed);
    Vector* pattern_args = prefixed != NULL ? prefixed : completed;
    TypeRecord* subst =
        SubstituteTemplateParameters(parser, info.origin->type, pattern_args);
    // A pattern such as `decltype(views::all(declval<R>()))` fails to
    // substitute when the call is ill-formed for these arguments.
    if (subst != NULL) {
      subst = TypeRecordAddQualifiers(subst, GetQualifiers(type));
      // The alias pattern may be a dependent-member type such as
      // `typename ratio<...>::type`.  Now that the arguments are concrete the
      // result is a real class type; collapse the lazy
      // `Template<Args>::member` encoding to that concrete specialization so
      // downstream uses (e.g. as a template argument whose members are later
      // accessed) see a plain class.
      subst = TypeMaterializeClassTemplateSpecialization(parser->syntax, subst);
    }
    // Once this alias has expanded to its concrete result, do not retain the
    // alias template-id on that result. Keeping `decay_t<const T>` metadata on
    // the resulting `T` makes later substitutions treat the concrete type as
    // dependent and prevents non-type trait arguments from folding.
    if (subst != NULL && subst->template_origin == info.origin) {
      subst->template_origin = NULL;
      if (subst->template_arguments != NULL) {
        VectorDeleteWithContents(
            subst->template_arguments,
            (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
      }
      subst->template_arguments = NULL;
    }
    // `typename alias_t<T>::member` stores the alias template-id plus the
    // member name. Expanding the alias yields the aliased class; the member
    // still has to be looked up on that class. Returning the class itself
    // made `remove_cvref_t<Box>::sender_concept` name `Box`.
    // The alias pattern itself is often `Class<T>::type`. That member is
    // already applied by the substitution above; looking it up again asks
    // the resulting class for `::type` and collapses the trait to `int`.
    bool member_is_alias_pattern =
        TypeDependentOf(type)->dependent_member_name != NULL &&
        info.origin != NULL && info.origin->type != NULL &&
        TypeDependentOf(info.origin->type)->dependent_member_name != NULL &&
        StringEqualString(
            TypeDependentOf(info.origin->type)->dependent_member_name,
            TypeDependentOf(type)->dependent_member_name);
    if (subst != NULL && TypeDependentOf(type)->dependent_member_name != NULL &&
        !member_is_alias_pattern && TypeIsStructOrUnion(subst) &&
        subst->info.struct_info != NULL &&
        !TypeContainsTemplateParameter(subst)) {
      Vector* member_args = NewVector();
      VectorAppend(member_args, NewTypeTemplateArgument(subst));
      TypeRecordDelete(subst);
      TypeRecord* pattern = TypeRecordCopy(type);
      pattern->declarator = kDeclPrimitive;
      pattern->template_parameter_index = 0;
      pattern->is_pack_index = false;
      pattern->template_origin = NULL;
      if (pattern->template_arguments != NULL) {
        VectorDeleteWithContents(
            pattern->template_arguments,
            (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
        pattern->template_arguments = NULL;
      }
      TypeRecord* member =
          SubstituteDependentMemberType(parser, pattern, member_args);
      TypeRecordDelete(pattern);
      VectorDeleteWithContents(
          member_args, (VectorElementDestructor)TemplateArgumentDelete,
          /*free_element=*/false);
      subst = member;
    }
    if (prefixed != NULL) {
      VectorDeleteWithContents(prefixed,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    if (grouped_args != NULL) {
      VectorDeleteWithContents(grouped_args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    VectorDeleteWithContents(concrete_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return subst != NULL ? TypeRecordCalculateSize(subst) : NULL;
  }

  bool inherited_member =
      TypeDependentOf(type)->dependent_member_name != NULL &&
      TypeIsStructOrUnion(info.origin->type) &&
      info.origin->type->info.struct_info != NULL &&
      PrimaryTemplateBaseDeclaresMember(
          info.origin->type->info.struct_info,
          TypeDependentOf(type)->dependent_member_name);
  Vector* class_args =
      inherited_member
          ? NonVariadicClassTemplateArgumentPrefix(info.origin, concrete_args)
          : NULL;
  TypeRecord* subst =
      inherited_member
          ? ActiveClassTemplateSpecialization(parser, info.origin)
          : NULL;
  if (subst == NULL) {
    subst = InstantiateSimpleClassTemplate(
        parser, info.origin, class_args != NULL ? class_args : concrete_args);
  }
  if (class_args != NULL) {
    VectorDeleteWithContents(class_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  if (subst == NULL) {
    VectorDeleteWithContents(concrete_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return TypeRecordCopy(type);
  }
  subst = TypeRecordAddQualifiers(subst, GetQualifiers(type));

  if (TypeDependentOf(type)->dependent_member_name != NULL &&
      ((TypeDependentOf(type)->dependent_member_template_arguments != NULL &&
        TypeDependentOf(type)->dependent_member_template_arguments->length >
            1) ||
       DependentMemberPathHasMultipleComponents(
           TypeDependentOf(type)->dependent_member_name))) {
    Vector* path_args = TemplateArgumentVectorCopy(args);
    if (path_args == NULL) {
      path_args = NewVector();
    }
    int root_index = (int)path_args->length;
    VectorAppend(path_args, NewTypeTemplateArgument(subst));
    TypeRecord* path_type = TypeRecordCopy(type);
    path_type->declarator = kDeclPrimitive;
    path_type->template_parameter_index = root_index;
    TypeRecord* member_type =
        SubstituteDependentMemberType(parser, path_type, path_args);
    TypeRecordDelete(path_type);
    VectorDeleteWithContents(
        path_args, (VectorElementDestructor)TemplateArgumentDelete,
        /*free_element=*/false);
    TypeRecordDelete(subst);
    VectorDeleteWithContents(
        concrete_args, (VectorElementDestructor)TemplateArgumentDelete,
        /*free_element=*/false);
    return TypeRecordCalculateSize(member_type);
  }

  if (TypeDependentOf(type)->dependent_member_name != NULL &&
      TypeIsStructOrUnion(subst) && subst->info.struct_info != NULL) {
    StructMember* member = FindStructMember(
        subst->info.struct_info, TypeDependentOf(type)->dependent_member_name);
    Symbol* injected = NULL;
    if (member == NULL || member->symbol == NULL ||
        !StorageIs(member->symbol->storage, STO(typedef))) {
      injected = FindInheritedInjectedClassName(
          subst->info.struct_info,
          TypeDependentOf(type)->dependent_member_name);
    }
    // The instantiation's base list is filled in after member typedefs are
    // substituted.  `typename Derived::Base` still names the injected-class-name
    // of a pattern base; substitute that base directly.
    if (injected == NULL && info.origin != NULL && info.origin->type != NULL &&
        TypeIsStructOrUnion(info.origin->type) &&
        info.origin->type->info.struct_info != NULL &&
        info.origin->type->info.struct_info != subst->info.struct_info &&
        subst->info.struct_info->bases.length == 0) {
      Struct* pattern = info.origin->type->info.struct_info;
      for (size_t i = 0; i < pattern->bases.length; i++) {
        CXXBaseSpecifier* base = pattern->bases.value.p[i];
        if (base == NULL || base->type == NULL) {
          continue;
        }
        TypeRecord* concrete =
            SubstituteTemplateParameters(parser, base->type, concrete_args);
        if (concrete == NULL) {
          continue;
        }
        TypeRecord* materialized = concrete;
        if (parser != NULL && parser->syntax != NULL) {
          materialized = TypeMaterializeClassTemplateSpecialization(
              parser->syntax, concrete);
        }
        TypeRecord* candidate = materialized != NULL ? materialized : concrete;
        bool matches = TypeIsStructOrUnion(candidate) &&
                       candidate->info.struct_info != NULL &&
                       CurrentClassNameMatchesTypeName(
                           candidate->info.struct_info,
                           TypeDependentOf(type)->dependent_member_name);
        if (matches) {
          TypeRecord* member_type = TypeRecordCopy(candidate);
          member_type =
              TypeRecordAddQualifiers(member_type, GetQualifiers(type));
          if (materialized != concrete) {
            TypeRecordDelete(materialized);
          }
          TypeRecordDelete(concrete);
          TypeRecordDelete(subst);
          VectorDeleteWithContents(
              concrete_args, (VectorElementDestructor)TemplateArgumentDelete,
              /*free_element=*/false);
          return TypeRecordCalculateSize(member_type);
        }
        if (materialized != concrete) {
          TypeRecordDelete(materialized);
        }
        TypeRecordDelete(concrete);
      }
    }
    if (injected != NULL && injected->type != NULL) {
      TypeRecord* injected_type = FindInheritedInjectedClassType(
          subst->info.struct_info,
          TypeDependentOf(type)->dependent_member_name);
      if (injected_type == NULL) {
        injected_type = injected->type;
      }
      TypeRecord* member_type = TypeRecordCopy(injected_type);
      member_type = TypeRecordAddQualifiers(member_type, GetQualifiers(type));
      TypeRecordDelete(subst);
      VectorDeleteWithContents(concrete_args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      return TypeRecordCalculateSize(member_type);
    }
    if (member != NULL && member->symbol != NULL &&
        StorageIs(member->symbol->storage, STO(typedef))) {
      TypeRecord* member_type = NULL;
      Vector* member_template_args =
          TypeDependentOf(type)->dependent_member_template_arguments != NULL &&
                  TypeDependentOf(type)
                          ->dependent_member_template_arguments->length > 0
              ? TypeDependentOf(type)
                    ->dependent_member_template_arguments->value.p[0]
              : NULL;
      if (member_template_args != NULL) {
        Vector* concrete_member_args =
            SubstituteTemplateArgumentVectorForTypes(parser, member_template_args,
                                                    args);
        Vector* combined_args = NULL;
        // A member alias template can name a class (`using WithStaticSizes =
        // LayoutWithStaticSizes<...>`).  That is not a nested class template:
        // its pattern still mentions the enclosing class's parameters, so it
        // must be substituted with those arguments prefixed.  Instantiating it
        // with only the member arguments binds the enclosing pack to whatever
        // shares index 0.
        if (member->symbol->flags.is_template &&
            SymbolExtraOf(member->symbol)->alias_template == NULL &&
            TypeIsStructOrUnion(member->symbol->type)) {
          member_type = InstantiateSimpleClassTemplate(
              parser, member->symbol, concrete_member_args);
        } else {
          // A member alias of a variadic class numbers its own parameters
          // after the enclosing list, where the pack is one slot
          // (`template<class... Ts> struct Box { template<template<class...>
          // class Templ> using Apply = Templ<Ts...>; }`).  The instantiation
          // stores `Box<char, int>` as `[char, int]`, so Templ's index 1 would
          // otherwise bind to `int`.  Fold the pack back into one argument
          // before appending the member arguments.
          Vector* class_args = concrete_args;
          Symbol* class_template = info.origin;
          Struct* alias_class = subst->info.struct_info;
          // `Layout<Ts...>::ElementType<N>` is declared on the LayoutImpl base,
          // whose parameters are a different list from Layout's.  Prefix that
          // base specialization's arguments so the alias parameter (numbered
          // after those parameters) lines up with the use-site argument.
          int alias_index = -1;
          if (SymbolExtraOf(member->symbol)->alias_template != NULL &&
              SymbolExtraOf(member->symbol)->alias_template->parameters.length >
                  0) {
            TemplateParameter* alias_param =
                SymbolExtraOf(member->symbol)
                    ->alias_template->parameters.value.p[0];
            if (alias_param != NULL) {
              alias_index = alias_param->index;
            }
          }
          if (alias_index > 0 && subst->info.struct_info != NULL) {
            Struct* declaring = StructDeclaringSymbol(
                subst->info.struct_info, member->symbol, 0);
            if (declaring != NULL && declaring->tag_symbol != NULL &&
                declaring->tag_symbol->type != NULL &&
                declaring->tag_symbol->type->template_arguments != NULL) {
              alias_class = declaring;
              class_args = declaring->tag_symbol->type->template_arguments;
              if (declaring->tag_symbol->type->template_origin != NULL) {
                class_template = declaring->tag_symbol->type->template_origin;
              }
            }
          }
          Vector* grouped_class_args =
              RegroupExpandedClassTemplateArguments(class_template, class_args);
          if (grouped_class_args != NULL) {
            class_args = grouped_class_args;
          }
          combined_args = TemplateArgumentVectorCopy(class_args);
          if (grouped_class_args != NULL) {
            VectorDeleteWithContents(
                grouped_class_args,
                (VectorElementDestructor)TemplateArgumentDelete,
                /*free_element=*/false);
          }
          // `<1, 0, 4>` supplied to `template <size_t... StaticSizes>` is three
          // arguments.  The alias pattern names that pack as one parameter
          // (numbered after the enclosing class).  Fold the member arguments
          // into the alias's parameter list before appending them, or the pack
          // binds only the first size.
          Vector* completed_member_args = CompleteAliasTemplateArguments(
              parser, member->symbol, concrete_member_args);
          Vector* member_args_for_pattern = completed_member_args != NULL
                                                ? completed_member_args
                                                : concrete_member_args;
          for (size_t i = 0;
               member_args_for_pattern != NULL &&
               i < member_args_for_pattern->length;
               i++) {
            VectorAppend(combined_args, TemplateArgumentCopy(
                                            member_args_for_pattern->value.p[i]));
          }
          if (completed_member_args != NULL) {
            VectorDeleteWithContents(
                completed_member_args,
                (VectorElementDestructor)TemplateArgumentDelete,
                /*free_element=*/false);
          }
          // A nested member alias (`WithStaticSizes` → `WithStaticSizeSequence`)
          // is expanded while the active class is still the caller (`Node<T>`).
          // Its parameters are numbered after its declaring class (`Layout`),
          // which can have the same arity, so the expansion would prefix
          // `Node`'s arguments.  Expand it against the declaring class.
          Struct* saved_alias_source = NULL;
          Struct* saved_alias_target = NULL;
          bool switched_alias_class = false;
          if (parser != NULL && alias_class != NULL) {
            saved_alias_source = parser->template_substitution_source;
            saved_alias_target = parser->template_substitution_target;
            parser->template_substitution_target = alias_class;
            if (alias_class->tag_symbol != NULL &&
                alias_class->tag_symbol->type != NULL &&
                alias_class->tag_symbol->type->template_origin != NULL &&
                alias_class->tag_symbol->type->template_origin->type != NULL &&
                TypeIsStructOrUnion(
                    alias_class->tag_symbol->type->template_origin->type)) {
              parser->template_substitution_source =
                  alias_class->tag_symbol->type->template_origin->type
                      ->info.struct_info;
            }
            switched_alias_class = true;
          }
          member_type = SubstituteTemplateParameters(
              parser, member->symbol->type, combined_args);
          if (switched_alias_class) {
            parser->template_substitution_source = saved_alias_source;
            parser->template_substitution_target = saved_alias_target;
          }
        }
        // Expanding a member alias template can expose another deferred
        // template-id whose local parameter indices were not visible on the
        // outer dependent-member type (for example allocator_traits'
        // rebind_alloc<T>).  Resolve that newly exposed layer while the
        // combined enclosing/member argument list is still available.
        if (combined_args != NULL && member_type != NULL &&
            TypeContainsTemplateParameter(member_type)) {
          TemplateIdSubstitutionInfo exposed =
              TemplateIdInfoForSubstitution(member_type);
          if (exposed.origin != NULL && exposed.args != NULL) {
            Vector* exposed_args = SubstituteTemplateArgumentVectorForTypes(
                parser, exposed.args, combined_args);
            if (!TemplateArgumentVectorContainsTemplateParameter(exposed_args)) {
              TypeRecord* resolved_member_type = InstantiateSimpleClassTemplate(
                  parser, exposed.origin, exposed_args);
              resolved_member_type = TypeRecordAddQualifiers(
                  resolved_member_type, GetQualifiers(member_type));
              TypeRecordDelete(member_type);
              member_type = resolved_member_type;
            }
            VectorDeleteWithContents(
                exposed_args, (VectorElementDestructor)TemplateArgumentDelete,
                /*free_element=*/false);
          }
        }
        if (combined_args != NULL) {
          VectorDeleteWithContents(
              combined_args, (VectorElementDestructor)TemplateArgumentDelete,
              /*free_element=*/false);
        }
        VectorDeleteWithContents(
            concrete_member_args, (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
      } else {
        member_type =
            SubstituteTemplateParameters(parser, member->symbol->type,
                                         concrete_args);
      }
      if (member_type == NULL) {
        if (parser != NULL) {
          parser->template_substitution_failed = true;
        }
        member_type =
            NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
      }
      member_type = TypeRecordAddQualifiers(member_type, GetQualifiers(type));
      TypeRecordDelete(subst);
      VectorDeleteWithContents(concrete_args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      return TypeRecordCalculateSize(member_type);
    }
    if (subst->info.struct_info->instantiation_in_progress &&
        TypeDependentOf(type)->dependent_member_name != NULL) {
      TypeRecord* pending = ResolveInProgressClassMemberType(
          parser, subst->info.struct_info,
          TypeDependentOf(type)->dependent_member_name);
      if (pending != NULL) {
        pending = TypeRecordAddQualifiers(pending, GetQualifiers(type));
        TypeRecordDelete(subst);
        VectorDeleteWithContents(
            concrete_args, (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
        return TypeRecordCalculateSize(pending);
      }
    }
    if (parser != NULL &&
        !StructContainsTemplateParameter(subst->info.struct_info)) {
      parser->template_substitution_failed = true;
    }
    TypeRecordDelete(subst);
    subst = TypeRecordCopy(type);
    if (subst->template_arguments != NULL) {
      VectorDeleteWithContents(
          subst->template_arguments,
          (VectorElementDestructor)TemplateArgumentDelete,
          /*free_element=*/false);
    }
    subst->template_origin = info.origin;
    subst->template_arguments = TemplateArgumentVectorCopy(concrete_args);
  }
  VectorDeleteWithContents(concrete_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return TypeRecordCalculateSize(subst);
}

/* Resolve a C++26 pack-indexing specifier `Ts...[I]` once both the pack and
 * index are known.  A partially substituted index is retained for a later
 * nested-template instantiation. */
static TypeRecord* SubstituteTypePackIndex(TypeParser* parser,
                                           TypeRecord* type, Vector* args) {
  if (!type->is_pack_index) {
    return NULL;
  }
  int pack_index = type->template_parameter_index;
  TemplateArgument* pack = TypeDependentOf(type)->pack_index_pack;
  if (pack == NULL && args != NULL && pack_index >= 0 &&
      (size_t)pack_index < args->length) {
    pack = args->value.p[pack_index];
  }
  if (pack == NULL) {
    return TypeRecordCopy(type);
  }
  if (pack->pack_arguments == NULL) {
    return TypeRecordCopy(type);
  }

  int64_t element_index = 0;
  if (!TryFoldDependentTemplateArgument(parser,
                                        TypeDependentOf(type)->pack_index_expr,
                                        args, &element_index)) {
    ASTNode* partial = CloneDependentExpressionWithArgs(
        parser, TypeDependentOf(type)->pack_index_expr, args);
    if (partial != NULL &&
        DependentExpressionContainsTemplateParameter(partial)) {
      TypeRecord* deferred = TypeRecordCopy(type);
      ASTNodeDelete(TypeMutableDependent(deferred)->pack_index_expr);
      TypeMutableDependent(deferred)->pack_index_expr = partial;
      if (TypeDependentOf(deferred)->pack_index_pack == NULL) {
        TypeMutableDependent(deferred)->pack_index_pack =
            TemplateArgumentCopy(pack);
      }
      return deferred;
    }
    ASTNodeDelete(partial);
    if (parser != NULL) {
      parser->template_substitution_failed = true;
    }
    SemanticError(TypeDependentOf(type)->pack_index_expr,
                  "pack index must be a constant expression");
    TypeRecord* fallback =
        NewTypeRecordWithSize(kTypeInt | kTypeUnknown, GetQualifiers(type));
    return fallback;
  }

  if (element_index < 0 ||
      (uint64_t)element_index >= pack->pack_arguments->length) {
    if (parser != NULL) {
      parser->template_substitution_failed = true;
    }
    SemanticError(TypeDependentOf(type)->pack_index_expr,
                  "pack index %" PRId64
                  " is out of bounds for a pack of length %zu",
                  element_index, pack->pack_arguments->length);
    return NewTypeRecordWithSize(kTypeInt | kTypeUnknown, GetQualifiers(type));
  }

  TemplateArgument* element =
      pack->pack_arguments->value.p[(size_t)element_index];
  if (element != NULL && element->kind == kTemplateParameterTemplate &&
      TemplateArgumentExtraOf(element)->template_symbol != NULL) {
    if (type->template_arguments == NULL) {
      if (parser != NULL) {
        parser->template_substitution_failed = true;
      }
      SemanticError(TypeDependentOf(type)->pack_index_expr,
                    "Indexed template name requires a template argument list "
                    "when used as a type");
      return NewTypeRecordWithSize(kTypeInt | kTypeUnknown,
                                   GetQualifiers(type));
    }
    Vector* concrete_args = SubstituteTemplateArgumentVectorForTypes(
        parser, type->template_arguments, args);
    TypeRecord* result = InstantiateSimpleClassTemplate(
        parser, TemplateArgumentExtraOf(element)->template_symbol,
        concrete_args);
    VectorDeleteWithContents(
        concrete_args, (VectorElementDestructor)TemplateArgumentDelete,
        /*free_element=*/false);
    if (result == NULL) {
      if (parser != NULL) {
        parser->template_substitution_failed = true;
      }
      return NewTypeRecordWithSize(kTypeInt | kTypeUnknown,
                                   GetQualifiers(type));
    }
    result->is_pack_index = false;
    if (result->dependent != NULL) {
      TypeMutableDependent(result)->pack_index_expr = NULL;
    }
    result->template_parameter_index = -1;
    result = TypeRecordAddQualifiers(result, GetQualifiers(type));
    return TypeRecordCalculateSize(result);
  }
  if (element == NULL || element->kind != kTemplateParameterType ||
      element->type == NULL) {
    if (parser != NULL) {
      parser->template_substitution_failed = true;
    }
    SemanticError(TypeDependentOf(type)->pack_index_expr,
                  "pack indexing specifier requires a type pack");
    return NewTypeRecordWithSize(kTypeInt | kTypeUnknown, GetQualifiers(type));
  }
  if (TypeDependentOf(type)->dependent_member_name != NULL) {
    Vector* element_args = TemplateArgumentVectorCopyWithPackElement(
        args, pack_index, element);
    TypeRecord* member_pattern = TypeRecordCopy(type);
    member_pattern->is_pack_index = false;
    ASTNodeDelete(TypeMutableDependent(member_pattern)->pack_index_expr);
    TypeMutableDependent(member_pattern)->pack_index_expr = NULL;
    TypeRecord* member = SubstituteDependentMemberType(
        parser, member_pattern, element_args);
    TypeRecordDelete(member_pattern);
    VectorDeleteWithContents(
        element_args, (VectorElementDestructor)TemplateArgumentDelete,
        /*free_element=*/false);
    return member;
  }
  TypeRecord* result = TypeContainsTemplateParameter(element->type)
                           ? SubstituteTemplateParameters(parser, element->type,
                                                          args)
                           : TypeRecordCopy(element->type);
  result = TypeRecordAddQualifiers(result, GetQualifiers(type));
  return TypeMaterializeClassTemplateSpecialization(parser->syntax, result);
}

/* Replace a bare type parameter `T` with its actual type argument, preserving
 * cv-qualifiers from the use site. */
static TypeRecord* SubstituteBareTemplateParameter(TypeParser* parser,
                                                   TypeRecord* type,
                                                   Vector* args) {
  if (type->declarator != kDeclPrimitive ||
      type->template_parameter_index < 0) {
    return NULL;
  }
  int index = type->template_parameter_index;
  if (args == NULL || index < 0 || (size_t)index >= args->length) {
    return TypeRecordCopy(type);
  }
  TemplateArgument* arg = args->value.p[index];
  if (type->template_origin != NULL &&
      type->template_origin->flags.is_template_template_parameter &&
      arg != NULL && arg->kind == kTemplateParameterTemplate &&
      TemplateArgumentExtraOf(arg)->template_symbol != NULL &&
      TemplateArgumentExtraOf(arg)->template_symbol->type != NULL &&
      TypeIsStructOrUnion(
          TemplateArgumentExtraOf(arg)->template_symbol->type)) {
    TypeRecord* placeholder =
        TypeRecordCopy(TemplateArgumentExtraOf(arg)->template_symbol->type);
    placeholder->template_origin =
        TemplateArgumentExtraOf(arg)->template_symbol;
    if (placeholder->template_arguments != NULL) {
      VectorDeleteWithContents(
          placeholder->template_arguments,
          (VectorElementDestructor)TemplateArgumentDelete,
          /*free_element=*/false);
      placeholder->template_arguments = NULL;
    }
    placeholder = TypeRecordAddQualifiers(placeholder, GetQualifiers(type));
    return placeholder;
  }
  if (arg == NULL || arg->kind != kTemplateParameterType ||
      arg->type == NULL) {
    return TypeRecordCopy(type);
  }
  TypeRecord* subst = NULL;
  if (SubstitutionCanShare(parser, arg->type)) {
    subst = TypeRecordShareExisting(arg->type);
    subst = TypeRecordAddQualifiers(subst, GetQualifiers(type));
  } else {
    subst = TypeRecordCopy(arg->type);
    subst = TypeRecordAddQualifiers(subst, GetQualifiers(type));
  }
  subst = TypeMaterializeClassTemplateSpecialization(parser->syntax, subst);
  return subst;
}

/* Substitute a non-type template parameter used as an array bound. */
static void SubstituteArrayTemplateBound(TypeParser* parser,
                                         TypeRecord* copy,
                                         TypeRecord* original,
                                         Vector* args) {
  if (copy->declarator != kDeclArray) {
    return;
  }
  if (original->info.array.is_dependent_bound) {
    int64_t folded = 0;
    bool saved_trap = DiagnosticErrorTrapBegin();
    bool folded_ok = TryFoldDependentTemplateArgument(
        parser, original->info.array.size.vla.size, args, &folded);
    bool bound_failed = DiagnosticErrorTrapped();
    DiagnosticErrorTrapEndPropagating(saved_trap);
    if (bound_failed) {
      // `char (*)[sizeof(declval<U>().f())]` with no such `f`: once nothing in
      // the bound is dependent, an invalid bound is a substitution failure.
      ASTNode* substituted = CloneDependentExpressionWithArgs(
          parser, original->info.array.size.vla.size, args);
      bool still_dependent =
          substituted == NULL ||
          DependentExpressionContainsTemplateParameter(substituted);
      ASTNodeDelete(substituted);
      if (!still_dependent) {
        parser->template_substitution_failed = true;
        copy->size = 0;
        return;
      }
    }
    if (folded_ok) {
      if (folded < 0) {
        SyntaxError(parser->syntax, "Array with negative size");
        folded = 1;
      }
      copy->info.array.size.fixed = (int)folded;
      copy->info.array.is_dependent_bound = false;
      copy->size = 0;
    } else {
      ASTNode* partial = CloneDependentExpressionWithArgs(
          parser, original->info.array.size.vla.size, args);
      if (partial != NULL) {
        copy->info.array.size.vla.size = partial;
      }
      copy->size = 0;
    }
    return;
  }
  if (original->info.array.template_parameter_index < 0) {
    return;
  }
  int index = original->info.array.template_parameter_index;
  if (args == NULL || (size_t)index >= args->length) {
    return;
  }
  TemplateArgument* arg = args->value.p[index];
  if (arg == NULL) {
    return;
  }
  if (arg->pack_arguments != NULL) {
    copy->info.array.size.fixed = (int)arg->pack_arguments->length;
    copy->info.array.template_parameter_index = -1;
    copy->size = 0;
  } else if (arg->kind != kTemplateParameterNonType) {
    return;
  } else if (arg->template_parameter_index >= 0) {
    copy->info.array.template_parameter_index = arg->template_parameter_index;
  } else {
    copy->info.array.size.fixed = (int)arg->int_value;
    copy->info.array.template_parameter_index = -1;
    copy->size = 0;
  }
}

static void SubstituteMemberPointerClass(TypeRecord* copy, Vector* args) {
  if (copy == NULL || copy->declarator != kDeclMemberPointer ||
      copy->template_parameter_index < 0 || args == NULL ||
      (size_t)copy->template_parameter_index >= args->length) {
    return;
  }
  TemplateArgument* argument =
      args->value.p[copy->template_parameter_index];
  if (argument == NULL || argument->kind != kTemplateParameterType ||
      argument->type == NULL || !TypeIsStructOrUnion(argument->type) ||
      argument->type->info.struct_info == NULL) {
    return;
  }
  TypeRecordSetStructInfo(copy, argument->type->info.struct_info);
  copy->template_parameter_index = -1;
  copy->type &= ~kTypeUnknown;
  copy->size = MemberPointerSize(copy);
}

static TypeRecord* SubstituteOriginalSpineNext(TypeParser* parser,
                                               TypeRecord* original,
                                               Vector* args) {
  TypeRecord* original_next = original->next;
  TypeRecordIncRef(original_next);
  TypeRecord* next = SubstituteTemplateParameters(parser, original_next, args);
  TypeRecordDelete(original_next);
  return next;
}

/* Replace the `next` type in a copied pointer/reference/array/function spine
 * with `next`, the substituted original `next`.  C++ reference collapsing and
 * pointer-to-reference cleanup both happen here so callers see a valid
 * post-substitution type chain. */
static void AttachSubstitutedSpineNext(TypeParser* parser, TypeRecord* copy,
                                       TypeRecord* next) {
  TypeRecordDelete(copy->next);
  copy->next = TypeRecordAdopt(next);
  if (copy->next == NULL) {
    return;
  }

  bool collapsed_reference = false;
  if (TypeIsReference(copy) && TypeIsReference(copy->next)) {
    TypeRecord* nested = copy->next;
    TypeRecord* collapsed_next = nested->next;
    bool rvalue = copy->declarator == kDeclRValueReference &&
                  nested->declarator == kDeclRValueReference;
    TypeRecordIncRef(collapsed_next);
    TypeRecordDelete(copy->next);
    copy->next = collapsed_next;
    copy->declarator = rvalue ? kDeclRValueReference : kDeclReference;
    collapsed_reference = true;
  }
  if (!collapsed_reference && TypeIsPointer(copy) &&
      TypeIsReference(copy->next)) {
    // Forming a pointer to reference during substitution is an invalid type.
    // Report substitution failure so this function-template candidate is
    // discarded rather than silently normalizing `T*` with `T = U&` to `U*`.
    parser->template_substitution_failed = true;
    return;
  }
  if (copy->next == NULL) {
    return;
  }
  if (!collapsed_reference) {
    TypeRecordIncRef(copy->next);
  }
  copy->type = copy->next->type;
}

/* Rebuild a function prototype after substituting its return/parameter types.
 * If a formal is a parameter pack, it expands into one concrete formal per pack
 * element; otherwise formals are substituted in place. */
static void SubstituteFunctionPrototype(TypeParser* parser, TypeRecord* copy,
                                        TypeRecord* original, Vector* args) {
  if (!TypeIsFunction(copy)) {
    return;
  }
  if (FunctionTypeHasParameterPack(original)) {
    VectorDestructWithContents(&copy->info.function.prototype,
                               (VectorElementDestructor)SymbolDestruct,
                               /*free_element=*/true);
    VectorInit(&copy->info.function.prototype);
    for (size_t i = 0; i < original->info.function.prototype.length; i++) {
      Symbol* formal = original->info.function.prototype.value.p[i];
      if (formal == NULL || formal->type == NULL) {
        continue;
      }
      AppendSubstitutedFormalParameter(parser, &copy->info.function.prototype,
                                       formal, args, 0);
    }
    for (size_t i = 0; i < copy->info.function.prototype.length; i++) {
      Symbol* formal = copy->info.function.prototype.value.p[i];
      formal->value.arg_number = (int32_t)i;
    }
    return;
  }
  for (size_t i = 0; i < copy->info.function.prototype.length; i++) {
    Symbol* formal = copy->info.function.prototype.value.p[i];
    Symbol* source_formal = original->info.function.prototype.value.p[i];
    if (formal == NULL || source_formal == NULL ||
        source_formal->type == NULL) {
      continue;
    }
    // TypeRecordCopy already shared a parameter type that cannot change.
    if (formal->type == source_formal->type) {
      continue;
    }
    TypeRecord* formal_type =
        SubstituteTemplateParameters(parser, source_formal->type, args);
    SymbolSetType(formal, formal_type);
  }
}

/* Fallback for non-special cases: copy the type and substitute all recursively
 * contained pieces (array bounds, next-chain, and function prototype). */
TypeRecord* SubstituteCopiedTypeRecord(TypeParser* parser,
                                              TypeRecord* type,
                                              Vector* args) {
  // The same pattern type is substituted once per instantiation.  Copying it
  // when nothing in it depends on the arguments retained a new record every
  // time; the arena never frees those records.
  if (SubstitutionCanShare(parser, type)) {
    return TypeRecordShareExisting(type);
  }
  if (TypeRecordCanIntern(type) &&
      !TypeMentionsSubstitutionPattern(parser, type)) {
    return TypeRecordIntern(type);
  }
  if (type->next != NULL &&
      (type->declarator == kDeclPointer || TypeIsReference(type))) {
    TypeRecord* next = SubstituteOriginalSpineNext(parser, type, args);
    if (next != NULL && !TypeIsReference(next) && TypeRecordCanIntern(next)) {
      TypeRecord* canonical_next = TypeRecordIntern(next);
      TypeRecordDelete(next);
      // A lookup key borrowing `type`'s fields; interning copies it on a
      // miss.  `type` may be a short record, so copy only the header.
      TypeRecord key = {0};
      memcpy(&key, type, offsetof(TypeRecord, info));
      key.shared_immutable = 0;
      key.refs = 0;
      key.template_parameter_summary = kTypeTemplateParameterSummaryUnknown;
      key.next = canonical_next;
      key.type = canonical_next->type;
      if (TypeRecordCanIntern(&key)) {
        return TypeRecordIntern(&key);
      }
      next = canonical_next;
    }
    TypeRecord* copy = TypeRecordCopy(type);
    AttachSubstitutedSpineNext(parser, copy, next);
    return TypeRecordCalculateSize(copy);
  }
  TypeRecord* copy = TypeRecordCopy(type);
  SubstituteMemberPointerClass(copy, args);
  SubstituteArrayTemplateBound(parser, copy, type, args);
  if (copy->next != NULL) {
    AttachSubstitutedSpineNext(parser, copy,
                               SubstituteOriginalSpineNext(parser, type, args));
  }
  SubstituteFunctionPrototype(parser, copy, type, args);
  return TypeRecordCalculateSize(copy);
}

/* Core type substitution: produce a concrete copy of `type` with every
 * template-parameter reference resolved against the actual arguments `args`.
 * Handles, in order:
 *   - rewriting a self-referential class type to the instantiation in progress;
 *   - dependent member typedefs (`T::member`);
 *   - nested class-template / alias-template instantiations;
 *   - bare type parameters (`T`) and non-type array bounds (`T[N]`);
 *   - structs that still contain parameters, pointer/reference chains (with
 *     reference collapsing), and function prototypes (expanding parameter
 *     packs).
 * Returns a size-calculated type record.  A type that substitution cannot
 * change is shared with its existing owners instead of copied. */
/* The decltype operands being substituted, as (operand, arguments) pairs.  Only
 * the same operand with the same arguments is a recursion: one partial
 * specialization's operand legitimately recurs with other arguments
 * (`result_of<C(A...)>` whose call operator is constrained on
 * `result_of<Compare(A...)>`). */
static Vector g_dependent_decltype_stack;
static bool g_dependent_decltype_stack_initialized;

static bool DependentDecltypeStackContains(ASTNode* expr, Vector* args) {
  for (size_t i = 0; i + 1 < g_dependent_decltype_stack.length; i += 2) {
    if (g_dependent_decltype_stack.value.p[i] == expr &&
        TemplateArgumentVectorEqual(g_dependent_decltype_stack.value.p[i + 1],
                                    args)) {
      return true;
    }
  }
  return false;
}

static void DependentDecltypeStackPush(ASTNode* expr, Vector* args) {
  VectorAppend(&g_dependent_decltype_stack, expr);
  VectorAppend(&g_dependent_decltype_stack, args);
}

static void DependentDecltypeStackPop(void) {
  VectorPop(&g_dependent_decltype_stack);
  VectorPop(&g_dependent_decltype_stack);
}

/* A call whose explicit template arguments still name a template parameter
 * (`IDT<Alloc>()` while only the enclosing class arguments are bound) is not
 * resolved to a specialization; its type is the template's declared return
 * type, which has none of the call's arguments. */
static bool NodeCallsUnresolvedFunctionTemplate(ASTNode* node, void* data) {
  (void)data;
  if (node == NULL || node->op != AST_OP(call)) {
    return false;
  }
  ASTNode* callee = ((VectorASTNode*)node)->left;
  if (callee == NULL || callee->op != AST_OP(identifier)) {
    return false;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)callee;
  return id->symbol != NULL && id->symbol->flags.is_template &&
         id->symbol->type != NULL && TypeIsFunction(id->symbol->type) &&
         TemplateArgumentVectorContainsTemplateParameter(id->template_arguments);
}

/* True when `type` is a *named use* of an alias template whose pattern is a
 * dependent `decltype`, e.g. `all_t<R>` where
 * `template<class T> using all_t = decltype(views::all(declval<T>()))`.
 *
 * Naming such an alias inside another template copies the alias's pattern (the
 * deferred decltype operand) onto the use site and records origin/arguments on
 * the side.  The copied operand still refers to the *alias's* parameters, so it
 * must not be substituted with the enclosing template's argument list: alias
 * parameter 0 would silently bind to the enclosing template's argument 0.  The
 * arguments recorded on the use site (`[R]`) are what map the alias's parameter
 * space onto the enclosing one, so the template-id path -- which substitutes
 * those first and only then expands the pattern -- must handle this. */
bool TypeIsDecltypeAliasTemplateId(TypeRecord* type) {
  return CompilerIsCXX() && type != NULL &&
         TypeDependentOf(type)->dependent_decltype_expr != NULL &&
         type->template_origin != NULL && type->template_arguments != NULL &&
         type->template_origin->flags.is_template &&
         StorageIs(type->template_origin->storage, STO(typedef)) &&
         type->template_origin->type != NULL &&
         type->template_origin->type != type &&
         TypeDependentOf(type->template_origin->type)
                 ->dependent_decltype_expr ==
             TypeDependentOf(type)->dependent_decltype_expr;
}

static void ClearDependentExpressionAnalysis(ASTNode* node, void* data,
                                             int child_id, VisitorMode mode) {
  (void)data;
  (void)child_id;
  if (mode == kVisitPreChildren && node != NULL) {
    node->flags &= ~kASTAnalyzed;
  }
}

typedef struct {
  TypeParser* parser;
  Vector* args;
} DecltypeParameterRemap;

/* `decltype(c.data())` names the function parameter `c`.  The return type is
 * substituted before the instantiated parameter symbols exist, so `c` still
 * has the primary template's dependent type.  Give the cloned identifier a
 * parameter symbol whose type has already been substituted; otherwise the
 * decltype stays unknown and the function template is never instantiated
 * (`GetData` inside `absl::Span`'s converting constructor). */
/* `this` in a member's trailing return type points to the class pattern, a
 * type with no template parameter in it.  Re-analysis looks members up
 * through it, so `obj_ptr->f()` would resolve against the pattern's `C*`
 * member; returns `this` retyped to the class being substituted into. */
static TypeRecord* DecltypeThisForSubstitutionTarget(TypeParser* parser,
                                                     Symbol* symbol) {
  if (parser == NULL || symbol == NULL || symbol->type == NULL ||
      !StringEqual(&symbol->name, "this") ||
      parser->template_substitution_source == NULL ||
      parser->template_substitution_target == NULL ||
      parser->template_substitution_target->tag_symbol == NULL ||
      parser->template_substitution_target->tag_symbol->type == NULL) {
    return NULL;
  }
  TypeRecord* pointee = symbol->type->next;
  if (!TypeIsPointer(symbol->type) || !TypeIsStructOrUnion(pointee) ||
      pointee->info.struct_info != parser->template_substitution_source ||
      parser->template_substitution_source ==
          parser->template_substitution_target) {
    return NULL;
  }
  TypeRecord* target =
      TypeRecordCopy(parser->template_substitution_target->tag_symbol->type);
  SetQualifiers(target, GetQualifiers(pointee));
  return NewPointerTo(GetQualifiers(symbol->type), target);
}

static void RemapDecltypeFunctionParameter(ASTNode* node, void* data,
                                           int child_id, VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(identifier)) {
    return;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  Symbol* symbol = id->symbol;
  DecltypeParameterRemap* remap = data;
  TypeRecord* subst = DecltypeThisForSubstitutionTarget(remap->parser, symbol);
  if (subst != NULL) {
    Symbol* copy = NewSymbol(symbol->name.value, subst, symbol->storage);
    copy->flags = symbol->flags;
    copy->location = symbol->location;
    copy->value.arg_number = symbol->value.arg_number;
    id->symbol = copy;
    node->flags &= ~kASTAnalyzed;
    ASTNodeClearType(node);
    return;
  }
  if (symbol == NULL || !symbol->flags.is_argument || symbol->type == NULL ||
      !TypeContainsTemplateParameter(symbol->type)) {
    return;
  }
  subst =
      SubstituteTemplateParameters(remap->parser, symbol->type, remap->args);
  if (subst == NULL || TypeContainsTemplateParameter(subst) ||
      TypeIsUnknown(subst)) {
    TypeRecordDelete(subst);
    return;
  }
  Symbol* copy = NewSymbol(symbol->name.value, subst, symbol->storage);
  copy->flags = symbol->flags;
  copy->location = symbol->location;
  copy->value.arg_number = symbol->value.arg_number;
  id->symbol = copy;
  node->flags &= ~kASTAnalyzed;
  ASTNodeClearType(node);
}

static bool ClassSubstitutionSourceSeesFunction(Struct* source,
                                                Symbol* function) {
  Struct* owner = function->type != NULL && TypeIsFunction(function->type)
                      ? function->type->info.function.cxx_member_owner
                      : NULL;
  for (Struct* s = source; s != NULL; s = s->lexical_parent) {
    if (s->access_enclosing_function == function) {
      return true;
    }
    for (Struct* o = owner; o != NULL; o = o->lexical_parent) {
      if (s == o) {
        return true;
      }
    }
  }
  return false;
}

/* A local class of a function template depends only on that function's
 * template parameters.  Instantiating an unrelated class template that names
 * it (`std::vector<Info>` while the function is still a pattern) substitutes
 * that class's arguments, which must not rebuild the local class: parameter
 * indices are positional, so `Info`'s `Container` would become `vector`'s
 * `T`, and the stray copies are distinct types. */
static bool LocalClassUnrelatedToClassSubstitution(TypeParser* parser,
                                                   Struct* str) {
  if (!StructIsFunctionTemplateLocalClass(str)) {
    return false;
  }
  Struct* source = parser->template_substitution_source;
  Struct* enclosing = parser->enclosing_template_substitution_source;
  if (source == NULL && enclosing == NULL) {
    return false;
  }
  Symbol* function = str->access_enclosing_function;
  return !ClassSubstitutionSourceSeesFunction(source, function) &&
         !ClassSubstitutionSourceSeesFunction(enclosing, function);
}

TypeRecord* SubstituteTemplateParameters(TypeParser* parser,
                                         TypeRecord* type,
                                         Vector* args) {
  if (type == NULL) {
    return NULL;
  }
  if (TypeRecordIsInvokeResultPlaceholder(type)) {
    TypeRecord* resolved =
        TypeRecordSubstituteInvokeResultPlaceholder(parser, type, args);
    if (resolved != NULL) {
      return resolved;
    }
  }
  if (TypeRecordIsCommonTypePlaceholder(type)) {
    TypeRecord* resolved =
        TypeRecordSubstituteCommonTypePlaceholder(parser, type, args);
    if (resolved != NULL) {
      return resolved;
    }
    if (parser->template_substitution_failed) {
      return NewTypeRecordWithSize(kTypeInt | kTypeUnknown,
                                   GetQualifiers(type));
    }
  }
  if (!g_dependent_decltype_stack_initialized) {
    VectorInit(&g_dependent_decltype_stack);
    g_dependent_decltype_stack_initialized = true;
  }
  if (TypeIsDecltypeAliasTemplateId(type)) {
    TemplateIdSubstitutionInfo alias_id = {type->template_origin,
                                           type->template_arguments};
    TypeRecord* subst = SubstituteTemplateIdType(parser, type, args, alias_id);
    if (subst != NULL) {
      return subst;
    }
  }
  if (TypeDependentOf(type)->dependent_splice_expr != NULL) {
    ASTNode* splice = CloneDependentExpressionWithArgs(
        parser, TypeDependentOf(type)->dependent_splice_expr, args);
    if (splice != NULL && splice->op == AST_OP(splice)) {
      ASTNodeVisit(splice, ClearDependentExpressionAnalysis, 0, NULL);
      SpliceASTNode* splice_node = (SpliceASTNode*)splice;
      splice_node->reflection = AnalyzeExpression(splice_node->reflection);
      ReflectionValue* reflection =
          SemanticEvaluateReflection(splice_node->reflection);
      if (reflection != NULL) {
        TypeRecord* result = SemanticMaterializeReflectedType(
            reflection, splice_node->base.location);
        if (result != NULL) {
          result = TypeRecordAddQualifiers(result, GetQualifiers(type));
          ASTNodeDelete(splice);
          return TypeRecordCalculateSize(result);
        }
        SemanticError(splice,
                      "Type splice operand does not reflect a type");
      }
    }
    TypeRecord* dependent =
        NewTypeRecordWithSize(kTypeInt | kTypeUnknown, GetQualifiers(type));
    TypeMutableDependent(dependent)->dependent_splice_expr = splice;
    return dependent;
  }
  if (TypeDependentOf(type)->dependent_decltype_expr != NULL &&
      !DependentDecltypeStackContains(
          TypeDependentOf(type)->dependent_decltype_expr, args)) {
    DependentDecltypeStackPush(TypeDependentOf(type)->dependent_decltype_expr,
                               args);
    if ((TypeDependentOf(type)->dependent_decltype_expr->flags &
         kASTUnparenthesizedDecltypeEntity) != 0 &&
        TypeDependentOf(type)->dependent_decltype_expr->op ==
            AST_OP(identifier)) {
      Symbol* entity =
          ((IdentifierASTNode*)TypeDependentOf(type)->dependent_decltype_expr)
              ->symbol;
      // An undeduced `auto` entity is not a type yet.  Returning the
      // placeholder makes `const T&` print as `const ` and rejects the real
      // initializer type.  Keep the decltype opaque until deduction replaces
      // the placeholder.
      if (entity != NULL && entity->type != NULL && entity->type != type &&
          !TypeContainsAuto(entity->type)) {
        // An unparenthesized id-expression uses the entity's declared type.
        // Resolve that type directly so a parameter-pack element such as
        // `decltype(args)...` is substituted against the selected pack element
        // rather than reanalyzing an identifier that still denotes the whole
        // function parameter pack.
        bool saved_substitution_failed =
            parser != NULL && parser->template_substitution_failed;
        TypeRecord* declared =
            SubstituteTemplateParameters(parser, entity->type, args);
        if (declared != NULL && !TypeContainsTemplateParameter(declared) &&
            !TypeContainsAuto(declared)) {
          declared = TypeRecordAddQualifiers(declared, GetQualifiers(type));
          declared = SubstituteResolvedDependentMemberSuffix(
              parser, type, declared, args);
          DependentDecltypeStackPop();
          return TypeRecordCalculateSize(declared);
        }
        TypeRecordDelete(declared);
        // The entity's declared type may not be substitutable on its own
        // (`decltype(M<T>::value)` names the pattern's member); re-analyzing
        // the operand below decides whether substitution fails.
        if (parser != NULL) {
          parser->template_substitution_failed = saved_substitution_failed;
        }
      }
    }
    // decltype is unevaluated: cloning and resolving a declaration-only
    // function such as std::declval must not require a function body.
    DiagnosticSuppressBegin();
    // Cloning the operand substitutes template arguments and may speculatively
    // instantiate declaration-only helpers (e.g. std::declval).  That clone
    // is unevaluated: a friend function template named by the operand must
    // not have its body instantiated here (`AbslHashValue` for `Cord` is
    // only needed for its declared return type).  Isolate clone diagnostics
    // so a SFINAE-sensitive caller observes errors from the re-analysis
    // below, not from cloning.  The same holds for the substitution-failure
    // flag: the definition-time callee can be an overload that the
    // re-analysis rejects (`enable_if_t<is_same_v<B, bool>, H>
    // AbslHashValue(H, B)` while a hidden friend is the real match).
    compiler->speculative_template_instantiation_depth++;
    bool clone_trap = DiagnosticErrorTrapBegin();
    bool saved_substitution_failed =
        parser != NULL && parser->template_substitution_failed;
    ASTNode* expr = CloneDependentExpressionWithArgs(
        parser, TypeDependentOf(type)->dependent_decltype_expr, args);
    if (parser != NULL) {
      parser->template_substitution_failed = saved_substitution_failed;
    }
    DiagnosticErrorTrapEnd(clone_trap);
    if (expr != NULL) {
      // The definition-time expression was analyzed while its operands were
      // still dependent, so its nodes carry kASTAnalyzed with dependent types.
      // AnalyzeExpression is a no-op on an already-analyzed node, so unless we
      // clear the flag across the freshly substituted clone the arithmetic/
      // conversion rules never re-run against the now-concrete operand types
      // (e.g. decltype(declval<int>() + declval<long>()) would keep the stale
      // type instead of resolving to long).
      ASTNodeVisit(expr, ClearDependentExpressionAnalysis, 0, NULL);
      DecltypeParameterRemap parameter_remap = {parser, args};
      ASTNodeVisit(expr, RemapDecltypeFunctionParameter, 0,
                   &parameter_remap);
      bool analysis_trap = DiagnosticErrorTrapBegin();
      ASTNode* saved_decltype_operand = compiler->decltype_operand;
      compiler->decltype_operand = expr;
      expr = AnalyzeExpression(expr);
      compiler->decltype_operand = saved_decltype_operand;
      bool analysis_failed = DiagnosticErrorTrapped();
      DiagnosticErrorTrapEnd(analysis_trap);
      if (analysis_failed) {
        // A still-dependent operand (ElemFromList<I, T...>::type while I is
        // not yet a constant) can fail overload resolution.  That is not
        // ill-formed; keep the decltype opaque for a later substitution.
        bool arguments_are_concrete =
            args != NULL &&
            !TemplateArgumentVectorContainsTemplateParameter(args) &&
            !TemplateArgumentVectorNamesIncompleteClass(args);
        bool structurally_dependent =
            expr != NULL &&
            DependentExpressionContainsTemplateParameter(expr);
        bool still_dependent =
            expr != NULL &&
            (structurally_dependent ||
             (!arguments_are_concrete &&
              ExpressionIsTemplateDependent(expr)));
        if (still_dependent) {
          TypeRecord* opaque = TypeRecordCopy(type);
          opaque = TypeRecordAddQualifiers(opaque, GetQualifiers(type));
          ASTNode* deferred = CloneDependentExpressionWithArgs(
              parser, TypeDependentOf(type)->dependent_decltype_expr, args);
          if (deferred != NULL) {
            ASTNodeVisit(deferred, RemapDecltypeFunctionParameter, 0,
                         &parameter_remap);
            TypeMutableDependent(opaque)->dependent_decltype_expr = deferred;
            ASTNodeDelete(expr);
          } else {
            TypeMutableDependent(opaque)->dependent_decltype_expr = expr;
          }
          compiler->speculative_template_instantiation_depth--;
          DiagnosticSuppressEnd();
          DependentDecltypeStackPop();
          return TypeRecordCalculateSize(opaque);
        }
        if (parser != NULL) {
          parser->template_substitution_failed = true;
        }
        ASTNodeDelete(expr);
        compiler->speculative_template_instantiation_depth--;
        DiagnosticSuppressEnd();
        DependentDecltypeStackPop();
        return NULL;
      }
      if (expr != NULL && expr->type != NULL) {
        // If the substituted operand is still type-dependent, the `decltype` is
        // not yet evaluable: only some enclosing template arguments were
        // supplied (e.g. substituting the class parameter `D` while a member
        // template's own parameter `R` survives).  The re-analyzed operand type
        // here is a *partially-resolved spine* (e.g. `ranges::begin(declval<R&>())`
        // resolving to `R&`).  Returning that spine lets an enclosing alias
        // textually reduce it -- `iter_difference_t<iterator_t<R>>` collapsing
        // via `iterator_traits<remove_cvref_t<R&>>::difference_type` down to
        // `R::difference_type`, which bakes in the *primary* iterator_traits body
        // and never re-dispatches to the `T*` specialization once `R` becomes
        // concrete.  Keep the `decltype` opaque (carry the substituted operand as
        // the deferred expression) so a later substitution with a concrete `R`
        // re-evaluates the whole chain from scratch.
        bool operand_still_dependent =
            TypeIsUnknown(expr->type) ||
            TypeContainsTemplateParameter(expr->type) ||
            TypeContainsAuto(expr->type) ||
            ASTNodeAny(expr, NodeCallsUnresolvedFunctionTemplate, NULL);
        if (operand_still_dependent) {
          TypeRecord* opaque = TypeRecordCopy(type);
          opaque = TypeRecordAddQualifiers(opaque, GetQualifiers(type));
          // Carry a FRESH, UNANALYZED clone of the substituted operand as the
          // deferred expression, not the just-analyzed `expr`.  The re-analysis
          // above may have resolved to an unknown/dependent type only because a
          // type involved was still incomplete at this substitution point (e.g. a
          // CRTP Derived being completed while its view_interface base's member
          // constraint `contiguous_iterator<iterator_t<Derived>>` is substituted):
          // the CPO call `ranges::begin(declval<Derived&>())` cannot deduce its
          // `auto` return yet, so `expr` becomes a call node baked to `unknown`.
          // Re-analyzing that stale, already-resolved tree later never recovers.
          // A clean clone re-analyzes from scratch once Derived is complete and
          // then resolves correctly (e.g. to `int*`).
          ASTNode* deferred = CloneDependentExpressionWithArgs(
              parser, TypeDependentOf(type)->dependent_decltype_expr, args);
          if (deferred != NULL) {
            // The class's parameters (`v` of type `T`) are bound now; a later
            // substitution supplies only the member template's own.
            ASTNodeVisit(deferred, RemapDecltypeFunctionParameter, 0,
                         &parameter_remap);
            TypeMutableDependent(opaque)->dependent_decltype_expr = deferred;
            ASTNodeDelete(expr);
          } else {
            TypeMutableDependent(opaque)->dependent_decltype_expr = expr;
          }
          compiler->speculative_template_instantiation_depth--;
          DiagnosticSuppressEnd();
          DependentDecltypeStackPop();
          return TypeRecordCalculateSize(opaque);
        }
        TypeRecord* result = NULL;
        StructMember* accessed_member = NULL;
        if ((expr->flags & kASTParenthesized) == 0 &&
            (expr->op == AST_OP(dot) || expr->op == AST_OP(arrow)) &&
            ((BinaryASTNode*)expr)->right != NULL &&
            ((BinaryASTNode*)expr)->right->op == AST_OP(structmember)) {
          accessed_member =
              ((StructMemberASTNode*)((BinaryASTNode*)expr)->right)->member;
        }
        if ((expr->flags & kASTUnparenthesizedDecltypeEntity) != 0) {
          result = TypeRecordCopy(expr->type);
        } else if (accessed_member != NULL && accessed_member->symbol != NULL &&
                   accessed_member->symbol->type != NULL &&
                   !accessed_member->is_member_function) {
          // [dcl.type.decltype]: an unparenthesized class member access names
          // the member's declared type.
          result = TypeRecordCopy(accessed_member->symbol->type);
        } else if (expr->value_category == kValueCategoryLvalue) {
          result = NewDecltypeReference(expr->type, false);
        } else if (expr->value_category == kValueCategoryXvalue) {
          result = NewDecltypeReference(expr->type, true);
        } else {
          result = NewDecltypePrvalueType(expr->type);
        }
        if (result != NULL) {
          result = TypeRecordAddQualifiers(result, GetQualifiers(type));
          result = SubstituteResolvedDependentMemberSuffix(
              parser, type, result, args);
          ASTNodeDelete(expr);
          compiler->speculative_template_instantiation_depth--;
          DiagnosticSuppressEnd();
          DependentDecltypeStackPop();
          return TypeRecordCalculateSize(result);
        }
      }
      ASTNodeDelete(expr);
    }
    compiler->speculative_template_instantiation_depth--;
    DiagnosticSuppressEnd();
    DependentDecltypeStackPop();
  }
  if (CompilerIsCXX() && parser != NULL) {
    TypeRecord* subst = SubstituteTemplateSelfReference(
        type, parser->template_substitution_source,
        parser->template_substitution_target);
    if (subst != NULL) {
      return subst;
    }
    subst = SubstituteTemplateSelfReference(
        type, parser->enclosing_template_substitution_source,
        parser->enclosing_template_substitution_target);
    if (subst != NULL) {
      return subst;
    }
    // An explicitly specialized nested template (for example
    // `nested<Outer>`) must pass through template-id substitution so its
    // arguments are completed and validated.  The active-nested shortcuts
    // below are only valid for an unadorned injected nested type name.
    bool explicit_nested_template_id =
        type->template_arguments != NULL && type->info.struct_info != NULL &&
        type->info.struct_info->is_template;
    if (!explicit_nested_template_id) {
      subst = SubstituteNestedTypeFromActiveInstantiation(parser, type, args);
      if (subst != NULL) {
        return subst;
      }
      if (TypeNamesActiveNestedTemplateStruct(parser, type)) {
        return SubstituteNestedStructTemplateParameters(parser, type, args);
      }
    }
  }

  // `SizeType<A>::size_type` stores the alias pattern's parameter index (0)
  // alongside the use-site arguments (`A`, which may be a later parameter of
  // the enclosing class).  Substituting that index against the enclosing
  // argument list binds `size_type` to the wrong parameter.  The template-id
  // path below applies the recorded arguments first.
  bool alias_member_template_id =
      type->template_origin != NULL && type->template_arguments != NULL &&
      TypeDependentOf(type)->dependent_member_name != NULL;
  TypeRecord* subst = NULL;
  if (!alias_member_template_id) {
    subst = SubstituteDependentMemberType(parser, type, args);
    if (subst != NULL) {
      return subst;
    }
  }

  subst = SubstituteTypePackIndex(parser, type, args);
  if (subst != NULL) {
    return subst;
  }

  TemplateIdSubstitutionInfo template_id =
      TemplateIdInfoForSubstitution(type);
  subst = SubstituteTemplateIdType(parser, type, args, template_id);
  if (subst != NULL) {
    return subst;
  }

  subst = SubstituteBareTemplateParameter(parser, type, args);
  if (subst != NULL) {
    return subst;
  }

  bool nested_in_active_instantiation =
      CompilerIsCXX() && TypeIsStructOrUnion(type) &&
      type->info.struct_info != NULL &&
      ((parser->template_substitution_source != NULL &&
        type->info.struct_info->lexical_parent ==
            parser->template_substitution_source) ||
       (parser->enclosing_template_substitution_source != NULL &&
        type->info.struct_info->lexical_parent ==
            parser->enclosing_template_substitution_source));
  // A partial local class copy is rebuilt only by its member template's own
  // instantiation; anywhere else (another class-stage visit, or a function
  // template it was passed to) its renumbered parameters are not these `args`.
  bool partial_local_class =
      CompilerIsCXX() && TypeIsStructOrUnion(type) &&
      StructIsPartialMemberTemplateLocalClass(type->info.struct_info);
  Symbol* partial_owner =
      partial_local_class
          ? type->info.struct_info->local_class_partial_owner
          : NULL;
  bool rebuild_partial_local_class =
      partial_local_class &&
      !parser->substituting_enclosing_template_arguments_only &&
      (partial_owner == NULL ||
       partial_owner == parser->body_clone_source_function);
  if (CompilerIsCXX() && TypeIsStructOrUnion(type) &&
      (StructContainsTemplateParameter(type->info.struct_info) ||
       StructIsFunctionTemplateLocalClass(type->info.struct_info) ||
       rebuild_partial_local_class ||
       (nested_in_active_instantiation &&
        StructHasMemberFunction(type->info.struct_info)))) {
    if (type->info.struct_info != NULL &&
        type->info.struct_info->is_template &&
        template_id.origin == NULL && template_id.args == NULL) {
      return TypeRecordCopy(type);
    }
    // A complete class such as Cord can still contain a template parameter in
    // some member type.  Rebuilding it while substituting an unrelated
    // function template (std::move) instantiates those members with the wrong
    // arguments: enable_if<false, int>::type fails, and that shared failure
    // flag discards a candidate whose own signature substituted successfully.
    // Copy the class instead.  Nested classes of the template being
    // substituted, local classes, and lambda closures still need a rebuild.
    Symbol* tag_symbol = type->info.struct_info->tag_symbol;
    bool unrelated_complete_class =
        !nested_in_active_instantiation && !rebuild_partial_local_class &&
        (partial_local_class || tag_symbol == NULL ||
         (!tag_symbol->flags.invented && !tag_symbol->flags.is_block_scope));
    if (unrelated_complete_class ||
        LocalClassUnrelatedToClassSubstitution(parser, type->info.struct_info)) {
      return SubstituteCopiedTypeRecord(parser, type, args);
    }
    return SubstituteNestedStructTemplateParameters(parser, type, args);
  }

  return SubstituteCopiedTypeRecord(parser, type, args);
}

TypeRecord* TypeSubstituteFunctionTemplateReturnType(
    Syntax* syntax, Symbol* function_template, Vector* explicit_args) {
  if (function_template != NULL && function_template->type != NULL &&
      TypeIsFunction(function_template->type) &&
      function_template->type->info.function.template_origin != NULL) {
    function_template =
        function_template->type->info.function.template_origin;
  }
  if (syntax == NULL || function_template == NULL ||
      function_template->type == NULL ||
      !TypeIsFunction(function_template->type) ||
      function_template->type->next == NULL || explicit_args == NULL ||
      explicit_args->length == 0) {
    return NULL;
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  TypeRecord* result = SubstituteTemplateParameters(
      &parser, function_template->type->next, explicit_args);
  TypeParserDestruct(&parser);
  return result;
}

/* Append a clone of formal parameter `formal` with the already-substituted
 * `formal_type` to the prototype `out`, optionally rebasing any remaining
 * template-parameter indices in the type by `rebase_base`. */
static void AppendFormalClone(Vector* out, Symbol* formal,
                              TypeRecord* formal_type,
                              int rebase_base) {
  if (formal == NULL || formal_type == NULL) {
    return;
  }
  if (rebase_base > 0) {
    RebaseTemplateParameterIndices(formal_type, rebase_base);
  }
  Symbol* clone = NewSymbol(formal->name.value, formal_type, formal->storage);
  clone->flags = formal->flags;
  clone->flags.is_argument = true;
  clone->flags.is_parameter_pack = false;
  clone->location = formal->location;
  SymbolCloneDefaultArgument(clone, formal);
  VectorAppend(out, clone);
}

/* Substitute one formal parameter against `args` and append the result(s) to
 * the prototype `out`. A pack parameter (`Ts... xs`) expands into one formal
 * per pack element; a non-pack parameter substitutes to a single formal. */
void AppendSubstitutedFormalParameter(TypeParser* parser, Vector* out,
                                             Symbol* formal, Vector* args,
                                             int rebase_base) {
  int index = -1;
  if (formal != NULL && formal->flags.is_parameter_pack &&
      TypeIsTemplateParameterPlaceholder(formal->type, &index) &&
      index >= 0 && (size_t)index < args->length) {
    TemplateArgument* pack = args->value.p[index];
    if (pack != NULL && pack->pack_arguments != NULL) {
      for (size_t i = 0; i < pack->pack_arguments->length; i++) {
        TemplateArgument* element = pack->pack_arguments->value.p[i];
        if (element == NULL || element->kind != kTemplateParameterType ||
            element->type == NULL) {
          continue;
        }
        TypeRecord* formal_type = TypeRecordCopy(element->type);
        AppendFormalClone(out, formal, formal_type, rebase_base);
      }
      return;
    }
  }
  int pattern_pack_index = -1;
  size_t pattern_pack_length = 0;
  if (formal != NULL && formal->flags.is_parameter_pack &&
      FindPackExpansionInType(formal->type, args, &pattern_pack_index,
                              &pattern_pack_length) &&
      pattern_pack_index >= 0 && (size_t)pattern_pack_index < args->length) {
    TemplateArgument* pack = args->value.p[pattern_pack_index];
    if (pack != NULL && pack->pack_arguments != NULL) {
      for (size_t i = 0; i < pattern_pack_length; i++) {
        TypeRecord* formal_type =
            SubstituteTemplateParametersForPackElement(
                parser, formal->type, args, pattern_pack_index, i);
        AppendFormalClone(out, formal, formal_type, rebase_base);
      }
      return;
    }
  }

  TypeRecord* formal_type =
      SubstituteTemplateParameters(parser, formal->type, args);
  size_t before = out->length;
  AppendFormalClone(out, formal, formal_type, rebase_base);
  if (formal != NULL && formal->flags.is_parameter_pack && out->length > before) {
    // Reached only when the pack could not be expanded here because its
    // argument is not among `args` (e.g. instantiating an enclosing class
    // template whose member function template still has an unexpanded pack).
    // AppendFormalClone clears is_parameter_pack for expanded elements, but this
    // formal is still a pack and must stay one so the later member-template
    // instantiation can recognize and deduce it.
    Symbol* appended = out->value.p[out->length - 1];
    appended->flags.is_parameter_pack = true;
  }
}

/* Build the synthetic name for the `index`-th element of a lambda capture pack,
 * e.g. base "xs" -> "xs$pack0". */
void LambdaCapturePackElementName(String* name, const char* base,
                                         size_t index) {
  StringPrintf(name, "%s$pack%zu", base, index);
}

/* True if `name` is a synthetic element name (base + "$packN") of capture pack
 * `base`. */
bool LambdaCapturePackElementMatches(const char* name, const char* base) {
  if (name == NULL || base == NULL) {
    return false;
  }
  size_t base_len = strlen(base);
  if (strncmp(name, base, base_len) != 0) {
    return false;
  }
  const char* suffix = name + base_len;
  if (strncmp(suffix, "$pack", 5) != 0) {
    return false;
  }
  suffix += 5;
  if (*suffix == '\0') {
    return false;
  }
  while (*suffix != '\0') {
    if (*suffix < '0' || *suffix > '9') {
      return false;
    }
    suffix++;
  }
  return true;
}

/* Return the first template-parameter index referenced anywhere in `type`
 * (the type chain or its template arguments), or -1 if none. */
int FirstTemplateParameterIndexInType(TypeRecord* type) {
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (t->template_parameter_index >= 0) {
      return t->template_parameter_index;
    }
    if (t->template_origin != NULL &&
        t->template_origin->flags.is_template_template_parameter) {
      return t->template_origin->template_parameter_index;
    }
    if (t->template_arguments != NULL) {
      for (size_t i = 0; i < t->template_arguments->length; i++) {
        TemplateArgument* arg = t->template_arguments->value.p[i];
        int nested = FirstTemplateParameterIndexInType(
            arg != NULL ? arg->type : NULL);
        if (nested >= 0) {
          return nested;
        }
      }
    }
  }
  return -1;
}

/* True if the pattern `type` itself expands the pack argument at `index`, as
 * opposed to only naming it inside a nested expansion. */
static bool PatternExpandsPackArgument(TypeRecord* type, Vector* args,
                                       size_t index) {
  TemplateArgument unrelated = {0};
  Vector only;
  VectorInit(&only);
  for (size_t i = 0; i < args->length; i++) {
    VectorAppend(&only, i == index ? args->value.p[i] : &unrelated);
  }
  int found_index = -1;
  size_t found_length = 0;
  bool found = FindPackExpansionInType(type, &only, &found_index,
                                       &found_length) &&
               found_index == (int)index;
  VectorDestruct(&only);
  return found;
}

/* Substitute `type` as if the pack at `pack_index` were the single element
 * `element_index` of that pack. Used to expand a pack-dependent pattern into
 * one type per element. */
TypeRecord* SubstituteTemplateParametersForPackElement(
    TypeParser* parser, TypeRecord* type, Vector* args, int pack_index,
    size_t element_index) {
  if (args == NULL || pack_index < 0 || (size_t)pack_index >= args->length) {
    return SubstituteTemplateParameters(parser, type, args);
  }
  TemplateArgument* pack = args->value.p[pack_index];
  if (pack == NULL || pack->pack_arguments == NULL ||
      element_index >= pack->pack_arguments->length) {
    return SubstituteTemplateParameters(parser, type, args);
  }
  size_t pack_length = pack->pack_arguments->length;
  Vector element_args;
  VectorInit(&element_args);
  for (size_t i = 0; i < args->length; i++) {
    TemplateArgument* arg = args->value.p[i];
    if (arg != NULL && arg->pack_arguments != NULL &&
        arg->pack_arguments->length == pack_length &&
        element_index < arg->pack_arguments->length &&
        (i == (size_t)pack_index || PatternExpandsPackArgument(type, args, i))) {
      VectorAppend(&element_args, arg->pack_arguments->value.p[element_index]);
    } else {
      VectorAppend(&element_args, arg);
    }
  }
  TypeRecord* result = SubstituteTemplateParameters(parser, type,
                                                    &element_args);
  VectorDestruct(&element_args);
  return result;
}

/* Substitute a struct/union type that still contains template parameters by
 * rebuilding it member-by-member: each non-pack data member is substituted and
 * re-laid-out, member packs expand into one synthetically-named field per pack
 * element, and member functions are instantiated last (deferred so the layout
 * is finalized first). Returns a copy whose struct_info is the new struct. */
static void RebaseTemplateParameterIndicesSpine(TypeRecord* type, int base) {
  if (type == NULL || base <= 0 || TypeRecordCanShareConcreteClass(type)) {
    return;
  }
  TypeRecordInvalidateTemplateParameterSummary(type);
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    assert(!t->shared_immutable);
    if (t->template_parameter_index >= base) {
      t->template_parameter_index -= base;
    }
    if (t->declarator == kDeclArray &&
        t->info.array.template_parameter_index >= base) {
      t->info.array.template_parameter_index -= base;
    }
    if (t->declarator == kDeclArray && t->info.array.is_dependent_bound) {
      t->info.array.size.vla.size =
          CloneAndRebaseDependentExpression(t->info.array.size.vla.size, base);
    }
    RebaseTemplateArgumentVectorParameterIndices(t->template_arguments, base);
    if (TypeDependentOf(t)->dependent_member_template_arguments != NULL) {
      Vector* lists =
          TypeMutableDependent(t)->dependent_member_template_arguments;
      for (size_t i = 0; i < lists->length; i++) {
        RebaseTemplateArgumentVectorParameterIndices(lists->value.p[i], base);
      }
    }
    // A deferred `decltype` operand (e.g. `iterator_t<R> =
    // decltype(ranges::begin(declval<R&>()))`) references template parameters
    // inside its expression AST, not the type spine.  Those must be rebased too
    // or a later substitution with the member's own arguments (numbered from 0)
    // would fail to reach the enclosing-relative parameter still named in the
    // operand, leaving the decltype permanently unresolved.  An alias-id's
    // operand is the alias's own pattern, numbered in the alias's parameter
    // space; its recorded arguments (rebased above) carry the mapping.
    if (TypeDependentOf(t)->dependent_decltype_expr != NULL &&
        !TypeIsDecltypeAliasTemplateId(t)) {
      TypeMutableDependent(t)->dependent_decltype_expr =
          CloneAndRebaseDependentExpression(
              TypeDependentOf(t)->dependent_decltype_expr, base);
    }
    if (TypeDependentOf(t)->pack_index_expr != NULL) {
      TypeDependentInfo* dependent = TypeMutableDependent(t);
      ASTNode* old_index = dependent->pack_index_expr;
      dependent->pack_index_expr =
          CloneAndRebaseDependentExpression(old_index, base);
      ASTNodeDelete(old_index);
    }
  }
}

void RebaseTemplateParameterIndices(TypeRecord* type, int base) {
  if (type == NULL || base <= 0) {
    return;
  }
  // Nothing in this type is numbered relative to an enclosing template.
  // Walking it would copy shared pointer targets that do not change.
  if (TypeRecordCanShareConcreteClass(type)) {
    return;
  }
  TypeRecordInvalidateTemplateParameterSummary(type);
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    bool rest_is_concrete =
        t->next != NULL && TypeRecordCanShareConcreteClass(t->next);
    // TypeRecordCopy shares `next`.  Rebasing a copied pointer or reference
    // must not renumber the original pointee: a member function template's
    // parameter types (`F&&`, `Ts&&`) are that shared pointee, and sliding
    // their indices makes a later class instantiation treat them as the
    // enclosing class parameter.
    if (!rest_is_concrete && t->next != NULL && t->next->refs > 1) {
      TypeRecord* shared = t->next;
      TypeRecord* unique = TypeRecordCopy(shared);
      TypeRecordIncRef(unique);
      t->next = unique;
      TypeRecordDelete(shared);
    }
    assert(!t->shared_immutable);
    if (t->template_parameter_index >= base) {
      t->template_parameter_index -= base;
    }
    if (t->declarator == kDeclArray &&
        t->info.array.template_parameter_index >= base) {
      t->info.array.template_parameter_index -= base;
    }
    if (t->declarator == kDeclArray && t->info.array.is_dependent_bound) {
      t->info.array.size.vla.size =
          CloneAndRebaseDependentExpression(t->info.array.size.vla.size, base);
    }
    RebaseTemplateArgumentVectorParameterIndices(t->template_arguments, base);
    if (TypeDependentOf(t)->dependent_member_template_arguments != NULL) {
      Vector* lists =
          TypeMutableDependent(t)->dependent_member_template_arguments;
      for (size_t i = 0; i < lists->length; i++) {
        RebaseTemplateArgumentVectorParameterIndices(lists->value.p[i], base);
      }
    }
    // Function types can occur as ordinary template arguments, for example in
    // `is_same<void(Box), void(decay_t<T>)>`.  Their parameter symbols own
    // separate type records, so walking only the function's return-type spine
    // leaves `T` at its old enclosing-relative index.  Rebase those parameter
    // types as part of the function type.
    if (TypeIsFunction(t)) {
      for (size_t i = 0; i < t->info.function.prototype.length; i++) {
        Symbol* formal = t->info.function.prototype.value.p[i];
        if (formal != NULL) {
          RebaseTemplateParameterIndices(formal->type, base);
        }
      }
    }
    // See RebaseTemplateParameterIndicesSpine for alias-id operands.
    if (TypeDependentOf(t)->dependent_decltype_expr != NULL &&
        !TypeIsDecltypeAliasTemplateId(t)) {
      TypeMutableDependent(t)->dependent_decltype_expr =
          CloneAndRebaseDependentExpression(
              TypeDependentOf(t)->dependent_decltype_expr, base);
    }
    // Nested lambda closures embed placeholder capture-field types numbered
    // relative to an enclosing template.  After substituting the enclosing
    // arguments, those own-parameter placeholders must be renumbered too or
    // later instantiation still sees them as enclosing-relative.
    // Only walk non-function data members, and only along each field's type
    // spine: descending into a pointed-to/referenced struct would corrupt
    // captured visitor closures' own template parameters.
    if (TypeIsStructOrUnion(t) && t->info.struct_info != NULL &&
        t->info.struct_info->tag_symbol != NULL &&
        t->info.struct_info->tag_symbol->flags.invented) {
      Struct* str = t->info.struct_info;
      for (size_t i = 0; i < str->members.length; i++) {
        StructMember* member = str->members.value.p[i];
        if (member == NULL || member->symbol == NULL ||
            member->is_member_function || member->is_static ||
            member->is_using_declaration || StructMemberIsNestedType(member)) {
          continue;
        }
        RebaseTemplateParameterIndicesSpine(member->symbol->type, base);
      }
    }
    if (rest_is_concrete) {
      break;
    }
  }
}

/* Rebase (see RebaseTemplateParameterIndices) the parameter indices inside a
 * template argument and its referenced type. */
void RebaseTemplateArgumentParameterIndices(TemplateArgument* arg,
                                                   int base) {
  if (arg == NULL || base <= 0) {
    return;
  }
  assert(arg->shares == 0);
  if (arg->template_parameter_index >= base) {
    arg->template_parameter_index -= base;
  }
  if (!TypeRecordCanShareConcreteClass(arg->type)) {
    TypeRecord* independent_type = TypeRecordCloneSpine(arg->type);
    TypeRecordDelete(arg->type);
    arg->type = independent_type;
    RebaseTemplateParameterIndices(arg->type, base);
  }
  arg->dependent_expr =
      CloneAndRebaseDependentExpression(arg->dependent_expr, base);
  if (TemplateArgumentExtraOf(arg)->pack_index_expr != NULL) {
    TemplateArgumentMutableExtra(arg)->pack_index_expr =
        CloneAndRebaseDependentExpression(arg->extra->pack_index_expr, base);
  }
}

void RebaseTemplateArgumentVectorParameterIndices(Vector* args, int base) {
  if (args == NULL || base <= 0) {
    return;
  }
  for (size_t i = 0; i < args->length; i++) {
    TemplateArgument* arg = args->value.p[i];
    if (arg != NULL &&
        (arg->template_parameter_index >= base ||
         !TypeRecordCanShareConcreteClass(arg->type) ||
         arg->dependent_expr != NULL ||
         TemplateArgumentExtraOf(arg)->pack_index_expr != NULL)) {
      RebaseTemplateArgumentParameterIndices(
          TemplateArgumentVectorMutableAt(args, i), base);
    }
  }
}

/* A generic lambda written inside another template numbers its invented `auto`
 * parameters after the enclosing template's parameters and records that offset
 * as the call operator's template-parameter base (see NewLambdaCallOperator).
 * A closure that captures template-dependent state is rebuilt per enclosing
 * instantiation, and that rebuild rebases its operator to a 0-based standalone
 * template.  A closure that captures nothing template-dependent, however, is
 * genuinely identical across every instantiation of the enclosing template and
 * is therefore never rebuilt, so its operator keeps the enclosing-relative
 * numbering forever and neither deduction nor substitution (both of which
 * assume a 0-based own-parameter list) can instantiate it.
 *
 * Rebase such an operator to a 0-based standalone template in place, once: the
 * closure does not depend on the enclosing template, so this is the numbering
 * it should have had all along.  Dependent-capture closures are left untouched
 * (base > 0) so the rebuild path can still expand them safely. */
void TypeRebaseNonDependentLambdaCallOperator(Struct* closure, Symbol* op) {
  if (closure == NULL || op == NULL || op->type == NULL ||
      !TypeIsFunction(op->type)) {
    return;
  }
  TypeRecord* func = op->type;
  int base = FunctionExtraOf(&func->info.function)->template_parameter_base;
  if (base <= 0 || StructContainsTemplateParameter(closure)) {
    return;
  }
  RebaseTemplateParameterIndices(func->next, base);
  for (size_t i = 0; i < func->info.function.prototype.length; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    if (formal != NULL) {
      RebaseTemplateParameterIndices(formal->type, base);
    }
  }
  for (size_t i = 0;
       i < FunctionExtraOf(&func->info.function)->template_parameters.length;
       i++) {
    TemplateParameter* param =
        FunctionExtraOf(&func->info.function)->template_parameters.value.p[i];
    if (param != NULL && param->index >= base) {
      param->index -= base;
    }
  }
  if (func->info.function.extra != NULL) {
    func->info.function.extra->template_parameter_base = 0;
  }
}

typedef struct {
  int base;
} RebaseDependentExpressionData;

// The clone shares a written type (`sizeof(T)`, `(T*)p`) with the original
// expression, so rebase a copy.
static void RebaseDependentExpressionOwnedType(TypeRecord** slot, int base) {
  if (!TypeContainsTemplateParameter(*slot)) {
    return;
  }
  TypeRecord* rebased = TypeRecordCloneSpine(*slot);
  RebaseTemplateParameterIndices(rebased, base);
  TypeRecordIncRef(rebased);
  TypeRecordDelete(*slot);
  *slot = rebased;
}

static void RebaseDependentExpressionVisitor(ASTNode* node, void* data,
                                             int child_id, VisitorMode mode) {
  (void)child_id;
  (void)mode;
  if (node != NULL &&
      (node->op == AST_OP(sizeof) || node->op == AST_OP(alignof))) {
    RebaseDependentExpressionOwnedType(
        &((SizeofASTNode*)node)->type_operand,
        ((RebaseDependentExpressionData*)data)->base);
    return;
  }
  if (node != NULL && node->op == AST_OP(cast)) {
    RebaseDependentExpressionOwnedType(
        &((CastASTNode*)node)->cast_type,
        ((RebaseDependentExpressionData*)data)->base);
    return;
  }
  if (node == NULL || node->op != AST_OP(identifier)) {
    return;
  }
  RebaseDependentExpressionData* rebase = data;
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  Symbol* old = id->symbol;
  if (old == NULL) {
    return;
  }
  bool needs_copy =
      old->template_parameter_index >= rebase->base ||
      SymbolDependentValueTemplateParameterIndex(old) >= rebase->base ||
      TypeContainsTemplateParameter(old->type) ||
      (id->template_arguments != NULL &&
       TemplateArgumentVectorContainsTemplateParameter(id->template_arguments));
  if (!needs_copy) {
    return;
  }
  // A variable template-id (`is_invocable_r_v<R, F, A>`) is folded by looking
  // up `symbol->variable_template`.  An alias template (`enable_if_t<B, T>`)
  // is folded from the primary's pattern.  Copying either symbol and rebasing
  // that pattern slides the alias's own parameters (`T` at index 1) into the
  // enclosing class's argument (`T` becomes index 0, the same slot as `B`),
  // so `enable_if_t<false, int>` never forms `enable_if<false, int>::type`.
  // The template arguments live on the identifier and are rebased below; the
  // symbol itself is the shared definition and must stay.  A namespace-scope
  // function template (`Sz<U>()`) is numbered in its own parameter space; a
  // copy would be a second template whose specializations duplicate its own.
  if (SymbolExtraOf(old)->variable_template != NULL ||
      (old->flags.is_template && StorageIs(old->storage, STO(typedef))) ||
      (old->flags.is_template && TypeIsFunction(old->type) &&
       old->type->info.function.cxx_member_owner == NULL)) {
    RebaseTemplateArgumentVectorParameterIndices(id->template_arguments,
                                                 rebase->base);
    return;
  }
  // The symbol's type spine may link records it does not count references
  // to, so a shallow copy would rebase the shared pattern in place.
  TypeRecord* type =
      old->type != NULL ? TypeRecordCloneSpine(old->type) : NULL;
  RebaseTemplateParameterIndices(type, rebase->base);
  Symbol* copy = NewSymbol(old->name.value, type, old->storage);
  copy->namespace_ = old->namespace_;
  copy->flags = old->flags;
  SymbolSetAlignment(copy, SymbolAlignment(old));
  copy->template_parameter_index = old->template_parameter_index;
  if (copy->template_parameter_index >= rebase->base) {
    copy->template_parameter_index -= rebase->base;
  }
  SymbolSetDependentValueTemplateParameterIndex(
      copy, SymbolDependentValueTemplateParameterIndex(old));
  if (SymbolDependentValueTemplateParameterIndex(copy) >= rebase->base) {
    SymbolSetDependentValueTemplateParameterIndex(
        copy, SymbolDependentValueTemplateParameterIndex(copy) - rebase->base);
  }
  copy->location = old->location;
  copy->value = old->value;
  copy->stack_offset = old->stack_offset;
  SymbolCopyAliasTarget(copy, old);
  SymbolCopyLambdaCaptureSource(copy, old);
  copy->lambda_capture_by_reference =
      old->lambda_capture_by_reference;
  id->symbol = copy;
  ASTNodeSetType(node, copy->type);
  // Explicit template arguments on the id (e.g. the `R&` in `declval<R&>()`
  // inside `iterator_t<R> = decltype(ranges::begin(declval<R&>()))`) also carry
  // enclosing-relative parameter indices.  The clone owns its own copy of this
  // vector (IdentifierASTNodeCopy deep-copies it), so rebasing it in place is
  // safe and is required or the operand keeps naming the pre-rebase parameter
  // and never resolves once the member's own arguments arrive.
  RebaseTemplateArgumentVectorParameterIndices(id->template_arguments,
                                               rebase->base);
}

static ASTNode* CloneAndRebaseDependentExpression(ASTNode* expr, int base) {
  if (expr == NULL || base <= 0) {
    return expr;
  }
  ASTNode* clone = ASTNodeClone(expr, IdentityCloneNode, NULL, NULL);
  RebaseDependentExpressionData rebase = {.base = base};
  ASTNodeVisit(clone, RebaseDependentExpressionVisitor, 0, &rebase);
  return clone;
}

static bool FunctionParameterPackIsConcrete(Symbol* formal, Vector* args) {
  int pack_index = -1;
  size_t pack_length = 0;
  if (!TypeIsTemplateParameterPlaceholder(formal->type, &pack_index) &&
      !FindPackExpansionInType(formal->type, args, &pack_index,
                               &pack_length)) {
    return false;
  }
  if (pack_index < 0 || (size_t)pack_index >= args->length) {
    return false;
  }
  TemplateArgument* pack = args->value.p[pack_index];
  return pack != NULL && pack->pack_arguments != NULL;
}

/* A dependent expression outside a function body (a trailing `decltype`,
 * a `noexcept` operand) can name a function parameter pack:
 * `decltype(P::apply(std::forward<Ts>(ts)...))`.  There is no instantiated
 * prototype to take the elements from, so substitute one parameter per pack
 * element.  Without them `ts` keeps its dependent pack type and the expanded
 * call cannot be resolved. */
static void MapExpressionFunctionParameterPack(ASTNode* node, void* data,
                                               int child_id,
                                               VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(identifier)) {
    return;
  }
  TemplateFunctionBodyClone* clone = data;
  Symbol* formal = ((IdentifierASTNode*)node)->symbol;
  if (formal == NULL || formal->type == NULL || !formal->flags.is_argument ||
      !formal->flags.is_parameter_pack ||
      MapFindPointerKey(&clone->pack_symbol_map, formal) != NULL ||
      !FunctionParameterPackIsConcrete(formal, clone->args)) {
    return;
  }
  Vector* elements = NewVector();
  AppendSubstitutedFormalParameter(clone->parser, elements, formal,
                                   clone->args, /*rebase_base=*/0);
  for (size_t i = 0; i < elements->length; i++) {
    Symbol* element = elements->value.p[i];
    if (element == NULL || element->flags.is_parameter_pack) {
      VectorDelete(elements);
      return;
    }
  }
  MapKeyValue kv;
  kv.key.p = formal;
  kv.value.p = elements;
  MapInsert(&clone->pack_symbol_map, kv);
}

/* Copy `from`'s function template-parameter list onto `to`, rebasing all
 * indices by `rebase_base` (used when cloning a member function template whose
 * parameters trail the enclosing class template's parameters). */
ASTNode* CloneDependentExpressionWithArgs(TypeParser* parser,
                                                 ASTNode* expr, Vector* args) {
  if (expr == NULL || args == NULL) {
    return NULL;
  }
  TemplateFunctionBodyClone clone;
  MapInitForPointerKeys(&clone.symbol_map);
  MapInitForPointerKeys(&clone.pack_symbol_map);
  clone.args = args;
  clone.parser = parser;
  if (parser != NULL) {
    ASTNodeVisit(expr, MapExpressionFunctionParameterPack, 0, &clone);
  }
  clone.args = args;
  clone.from_func = NULL;
  clone.to_func = NULL;
  clone.rebase_template_parameter_base = 0;
  clone.substitution_source =
      parser != NULL ? parser->template_substitution_source : NULL;
  clone.substitution_target =
      parser != NULL ? parser->template_substitution_target : NULL;
  // Dependent initializers and constraints need the same member-symbol
  // retargeting as function bodies.  For example, cloning
  // `Trait<K, V>::value = check<Pair>()` for `Trait<int, int>` must bind
  // `check` to the specialization; otherwise its body is instantiated without
  // the enclosing class arguments.
  clone.from_owner = clone.substitution_source;
  clone.to_owner = clone.substitution_target;
  ASTNode* cloned =
      ASTNodeClone(expr, CloneDependentDecltypeNode, &clone, NULL);
  MapDestructWithContents(&clone.pack_symbol_map, DeleteMappedVector);
  MapDestruct(&clone.symbol_map);
  if (cloned != NULL) {
    // The definition-time expression was analyzed while its operands were
    // still dependent.  Substitution can make those operands concrete, so the
    // cloned tree must be reanalyzed rather than retaining stale dependent
    // types on its parent nodes.
    ASTNodeVisit(cloned, ClearDependentExpressionAnalysis, 0, NULL);
  }
  return cloned;
}

bool FoldDependentTemplateArgumentUncached(TypeParser* parser, ASTNode* expr,
                                           Vector* args, int64_t* out) {
  if (expr == NULL || args == NULL) {
    return false;
  }
  ASTNode* cloned = CloneDependentExpressionWithArgs(parser, expr, args);
  if (cloned == NULL) {
    return false;
  }
  // Substituting an enclosing class template can leave references to a member
  // function template's own parameters unresolved.  Such an expression is
  // still dependent and must be carried forward, not sent through semantic
  // analysis as though it were concrete.
  if ((cloned->flags & kASTFoldExpression) != 0 &&
      DependentExpressionContainsTemplateParameter(cloned)) {
    ASTNodeDelete(cloned);
    return false;
  }
  // A non-type parameter declared `size_t` can be materialized from an `int`
  // literal (`0` passed as `N`).  `sizeof...` is `size_t`.  Those two types
  // do not deduce the same `T` for `min(N, sizeof...(Params))`.  Convert the
  // integer constant to `size_t` before analysis so the call can instantiate.
  if (cloned != NULL && cloned->op == AST_OP(call)) {
    VectorASTNode* call = (VectorASTNode*)cloned;
    IdentifierASTNode* id =
        call->left != NULL && call->left->op == AST_OP(identifier)
            ? (IdentifierASTNode*)call->left
            : NULL;
    if (id != NULL && id->symbol != NULL && id->symbol->flags.is_template &&
        id->symbol->type != NULL && TypeIsFunction(id->symbol->type) &&
        call->children != NULL && call->children->length == 2) {
      ASTNode* args[2] = {call->children->value.p[0],
                          call->children->value.p[1]};
      if (args[0] != NULL && args[1] != NULL &&
          args[0]->op == AST_OP(number) && args[1]->op == AST_OP(number) &&
          args[0]->type != NULL && args[1]->type != NULL &&
          TypeIsIntegral(args[0]->type) && TypeIsIntegral(args[1]->type) &&
          !TypeEqual(args[0]->type, args[1]->type)) {
        TypeRecord* size_type = NewSizeTypeRecord();
        int size_index = TypeEqual(args[0]->type, size_type)   ? 0
                         : TypeEqual(args[1]->type, size_type) ? 1
                                                               : -1;
        TypeRecordDelete(size_type);
        if (size_index >= 0) {
          int other = 1 - size_index;
          int64_t ivalue = ((ConstantASTNode*)args[other])->value.ivalue;
          SourceLocation location = args[other]->location;
          int child_id = args[other]->child_id;
          ASTNode* replaced =
              NewIntConstantASTNode(ivalue, NewSizeTypeRecord(), location);
          replaced->parent = cloned;
          replaced->child_id = child_id;
          call->children->value.p[other] = replaced;
          ASTNodeDelete(args[other]);
        }
      }
    }
  }
  // A dependent qualified name that survived cloning (its scope is still
  // dependent) must not be diagnosed here: this is a speculative fold, and an
  // unresolved name simply means the value stays dependent for now.
  DiagnosticSuppressBegin();
  compiler->constant_evaluation_required_depth++;
  cloned = AnalyzeExpression(cloned);
  compiler->constant_evaluation_required_depth--;
  bool ok = EvaluateIntegerExpression(cloned, out);
  // Overload resolution can leave a constexpr function template uninstantiated
  // (`const T` from `std::min`) even though every argument is a constant.
  // Deduce and instantiate it before giving up on the fold.
  if (!ok && cloned != NULL && cloned->op == AST_OP(call)) {
    VectorASTNode* call = (VectorASTNode*)cloned;
    if (call->left != NULL && call->left->op == AST_OP(identifier) &&
        call->children != NULL) {
      IdentifierASTNode* id = (IdentifierASTNode*)call->left;
      Symbol* callee = id->symbol;
      if (callee != NULL && callee->flags.is_template &&
          callee->type != NULL && TypeIsFunction(callee->type) &&
          callee->type->info.function.template_origin == NULL) {
        Symbol* selected = CXXResolveOverloadedFunctionTemplateCall(
            callee, id->template_arguments, call->children);
        if (selected != NULL && selected->type != NULL &&
            TypeIsFunction(selected->type) &&
            selected->type->info.function.template_origin != NULL &&
            selected->type->template_arguments != NULL) {
          Symbol* instantiated =
              TypeInstantiateFunctionTemplateWithCompletedArguments(
                  &compiler->syntax,
                  selected->type->info.function.template_origin,
                  selected->type->template_arguments);
          if (instantiated != NULL && instantiated->type != NULL &&
              TypeIsFunction(instantiated->type)) {
            id->symbol = instantiated;
            ASTNodeSetInstantiatedCalleeType(call->left, instantiated->type);
            TypeRecord* return_type = instantiated->type->next;
            if (return_type != NULL && TypeIsReference(return_type)) {
              ASTNodeSetType(cloned, return_type->next);
              cloned->value_category =
                  return_type->declarator == kDeclRValueReference
                      ? kValueCategoryXvalue
                      : kValueCategoryLvalue;
            } else if (return_type != NULL) {
              ASTNodeSetType(cloned, return_type);
              cloned->value_category = kValueCategoryPrvalue;
            }
            ok = EvaluateIntegerExpression(cloned, out);
          }
        }
      }
    }
  }
  DiagnosticSuppressEnd();
  ASTNodeDelete(cloned);
  return ok;
}

/* When cloning a value-dependent expression during class-template
 * instantiation, resolve a bare reference to a value-dependent static data
 * member of the template being instantiated (found via the parser's active
 * substitution source) by re-folding that member's own initializer against the
 * same concrete arguments.  This lets member typedefs whose non-type arguments
 * name sibling members fold, e.g. `using type = ratio<num, den>;` where `num`
 * and `den` are themselves computed from the template parameters.  Returns a
 * fresh constant node on success, or NULL to leave the reference unchanged. */
ASTNode* TypeSubstituteMemberTemplateExpressionAndRebase(
    Syntax* syntax, ASTNode* expr, Vector* args, int rebase_base,
    SourceLocation location, Struct* from_owner, Struct* to_owner) {
  if (syntax == NULL || expr == NULL) {
    return NULL;
  }
  if (args == NULL) {
    return CloneAndRebaseDependentExpression(expr, rebase_base);
  }

  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                 syntax->context);
  TemplateFunctionBodyClone clone;
  MapInitForPointerKeys(&clone.symbol_map);
  MapInitForPointerKeys(&clone.pack_symbol_map);
  clone.parser = &parser;
  clone.args = args;
  clone.from_func = NULL;
  clone.to_func = NULL;
  clone.rebase_template_parameter_base = rebase_base;
  // Member-template defaults (`enable_if_t<Trait<U>::value>()`) still name
  // the member's own parameters.  Resolving those traits against only the
  // enclosing class arguments binds `::value` on the primary template.
  if (rebase_base > 0) {
    parser.substituting_enclosing_template_arguments_only = true;
  }
  clone.from_owner = from_owner;
  clone.to_owner = to_owner;
  clone.substitution_source =
      from_owner != NULL && to_owner != NULL
          ? from_owner
          : parser.template_substitution_source;
  clone.substitution_target =
      from_owner != NULL && to_owner != NULL
          ? to_owner
          : parser.template_substitution_target;
  ASTNode* cloned = ASTNodeClone(expr, CloneTemplateFunctionBodyNode,
                                 &clone, NULL);
  MapDestructWithContents(&clone.pack_symbol_map, DeleteMappedVector);
  MapDestruct(&clone.symbol_map);
  TypeParserDestruct(&parser);
  if (cloned != NULL) {
    cloned->location = location;
  }
  return cloned;
}

ASTNode* TypeSubstituteTemplateExpressionAndRebase(
    Syntax* syntax, ASTNode* expr, Vector* args, int rebase_base,
    SourceLocation location) {
  return TypeSubstituteMemberTemplateExpressionAndRebase(
      syntax, expr, args, rebase_base, location, NULL, NULL);
}

ASTNode* TypeSubstituteTemplateExpression(Syntax* syntax, ASTNode* expr,
                                          Vector* args,
                                          SourceLocation location) {
  return TypeSubstituteTemplateExpressionAndRebase(
      syntax, expr, args, 0, location);
}

TypeRecord* TypeSubstituteTemplateTypeAndRebase(Syntax* syntax,
                                                TypeRecord* type,
                                                Vector* args,
                                                int rebase_base) {
  TypeRecord* result = TypeSubstituteTemplateType(syntax, type, args);
  RebaseTemplateParameterIndices(result, rebase_base);
  return result;
}

TypeRecord* TypeSubstituteTemplateType(Syntax* syntax, TypeRecord* type,
                                       Vector* args) {
  if (type == NULL) {
    return NULL;
  }
  if (syntax == NULL || args == NULL) {
    return TypeRecordCopy(type);
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                 syntax->context);
  TypeRecord* result = SubstituteTemplateParameters(&parser, type, args);
  TypeParserDestruct(&parser);
  return result;
}

TypeRecord* TypeSubstituteTemplateTypeOrFail(Syntax* syntax, TypeRecord* type,
                                             Vector* args) {
  if (type == NULL || syntax == NULL || args == NULL) {
    return TypeSubstituteTemplateType(syntax, type, args);
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                 syntax->context);
  TypeRecord* result = SubstituteTemplateParameters(&parser, type, args);
  if (parser.template_substitution_failed) {
    TypeRecordDelete(result);
    result = NULL;
  }
  TypeParserDestruct(&parser);
  return result;
}

Vector* TypeSubstituteTemplateArgumentVector(Syntax* syntax,
                                             Vector* template_args,
                                             Vector* args) {
  return TypeSubstituteTemplateArgumentVectorAndRebase(
      syntax, template_args, args, 0);
}

Vector* TypeSubstituteTemplateArgumentVectorAndRebase(
    Syntax* syntax, Vector* template_args, Vector* args, int rebase_base) {
  if (template_args == NULL) {
    return NULL;
  }
  if (args == NULL) {
    Vector empty;
    VectorInit(&empty);
    Vector* result = TypeSubstituteTemplateArgumentVectorAndRebase(
        syntax, template_args, &empty, rebase_base);
    VectorDestruct(&empty);
    return result;
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                 syntax->context);
  Vector* result =
      SubstituteTemplateArgumentVector(&parser, template_args, args,
                                       rebase_base);
  TypeParserDestruct(&parser);
  return result;
}

Vector* TypeSubstituteTemplateArgumentVectorOrFail(Syntax* syntax,
                                                   Vector* template_args,
                                                   Vector* args,
                                                   bool* failed) {
  *failed = false;
  if (template_args == NULL || args == NULL) {
    return TypeSubstituteTemplateArgumentVector(syntax, template_args, args);
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                 syntax->context);
  Vector* result =
      SubstituteTemplateArgumentVector(&parser, template_args, args, 0);
  *failed = parser.template_substitution_failed;
  TypeParserDestruct(&parser);
  return result;
}

/* Return the constructor name (== the class tag name) for a struct type, or
 * NULL if `type` is not a named C++ class. */
const char* CXXConstructorNameForRecord(TypeRecord* type) {
  if (!CompilerIsCXX() || type == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL ||
      type->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  return type->info.struct_info->tag_name->value;
}

/* True if `type` is a class type that declares a constructor member (named
 * after its tag).  Used to decide whether value-initialization of a resolved
 * template-parameter type must run a default constructor (class with a
 * constructor) or zero-initialize the storage (scalar or aggregate class). */
Vector* SubstituteTemplateArgumentVector(TypeParser* parser,
                                                Vector* template_args,
                                                Vector* args,
                                                int rebase_base) {
  if (template_args == NULL) {
    return NULL;
  }
  Vector* concrete_args = NewVector();
  for (size_t i = 0; i < template_args->length; i++) {
    size_t start = concrete_args->length;
    AppendSubstitutedTemplateArgument(parser, concrete_args,
                                      template_args->value.p[i], args);
    for (size_t j = start; j < concrete_args->length; j++) {
      RebaseTemplateArgumentParameterIndices(
          TemplateArgumentVectorMutableAt(concrete_args, j), rebase_base);
    }
  }
  return concrete_args;
}

/* True if `node` is a pack-expansion of a single parameter-pack identifier,
 * i.e. the `xs` in `xs...`. */
/* Build the template-argument that supplies `param`'s default value, or NULL if
 * the parameter has no default or its default is still dependent (references an
 * earlier parameter).  Dependent defaults are left unfilled so the caller keeps
 * its previous behaviour rather than baking in an unresolved parameter. */
static TemplateArgument* AliasDefaultTemplateArgument(TemplateParameter* param) {
  if (param == NULL || param->is_parameter_pack) {
    return NULL;
  }
  if (param->kind == kTemplateParameterType && param->default_type != NULL &&
      !TypeContainsTemplateParameter(param->default_type)) {
    TemplateArgument* arg = TemplateArgumentAlloc();
    arg->kind = kTemplateParameterType;
    arg->is_pack_expansion = false;
    arg->type = TypeRecordCopy(param->default_type);
    arg->int_value = 0;
    arg->template_parameter_index = -1;
    arg->pack_arguments = NULL;
    arg->dependent_expr = NULL;
    arg->location = SOURCE_LOCATION_MISSING;
    return arg;
  }
  if (param->kind == kTemplateParameterNonType &&
      param->default_argument != NULL) {
    return TemplateArgumentCopy(param->default_argument);
  }
  if (param->kind == kTemplateParameterNonType && param->has_default_int &&
      param->default_template_parameter_index < 0) {
    return NewIntegralTemplateArgument(param->default_int_value);
  }
  if (param->kind == kTemplateParameterTemplate &&
      param->default_argument != NULL &&
      param->default_argument->template_parameter_index < 0) {
    return TemplateArgumentCopy(param->default_argument);
  }
  return NULL;
}

/* Return `actuals` extended with trailing default arguments taken from the
 * alias's declared parameters, or NULL if no extension is needed (or possible).
 * A freshly allocated vector is returned that the caller owns; existing elements
 * are copied so the original `actuals` is untouched.  This lets an alias such as
 * `enable_if_t<B, T = void>` be named with just `enable_if_t<true>` and still
 * expand to `enable_if<true, void>::type` rather than leaking the alias's own
 * `T` parameter (which would otherwise collide with the enclosing template's
 * parameters during substitution). */
static Vector* AliasActualsWithDefaults(TypeParser* parser, Symbol* alias,
                                        Vector* actuals) {
  if (alias == NULL || SymbolExtraOf(alias)->alias_template == NULL) {
    return NULL;
  }
  Vector* parameters = &SymbolExtraOf(alias)->alias_template->parameters;
  size_t parameter_count = parameters->length;
  size_t actual_count = actuals != NULL ? actuals->length : 0;
  if (parameter_count == 0 || actual_count >= parameter_count) {
    return NULL;
  }
  // Do not try to fill defaults across a parameter pack; pack handling is left
  // to the existing count-based logic.
  for (size_t i = 0; i < parameter_count; i++) {
    TemplateParameter* param = parameters->value.p[i];
    if (param != NULL && param->is_parameter_pack) {
      return NULL;
    }
  }
  Vector* extended = NewVector();
  for (size_t i = 0; i < actual_count; i++) {
    VectorAppend(extended, TemplateArgumentCopy(actuals->value.p[i]));
  }
  // A member alias's defaults retain their enclosing-relative parameter
  // indices. Prefix the concrete class arguments for substitution, while the
  // returned vector continues to contain only the alias's written arguments.
  Vector* aligned =
      parser != NULL ? MemberAliasPatternArguments(parser, alias, extended)
                     : NULL;
  Vector* substitution_actuals = aligned != NULL ? aligned : extended;
  for (size_t i = actual_count; i < parameter_count; i++) {
    TemplateParameter* param = parameters->value.p[i];
    TemplateArgument* def = AliasDefaultTemplateArgument(param);
    if (def == NULL && parser != NULL && param != NULL &&
        param->kind == kTemplateParameterType && param->default_type != NULL &&
        param->index >= 0 &&
        (size_t)param->index <= substitution_actuals->length &&
        !TemplateArgumentVectorContainsTemplateParameter(
            substitution_actuals)) {
      // A default naming an earlier parameter (`class C = less<K>`) is
      // substituted with the arguments so far. Member aliases use the aligned
      // class-plus-alias vector prepared above.
      bool saved_failed = parser->template_substitution_failed;
      parser->template_substitution_failed = false;
      TypeRecord* type =
          SubstituteTemplateParameters(parser, param->default_type,
                                       substitution_actuals);
      bool failed = parser->template_substitution_failed;
      parser->template_substitution_failed = saved_failed;
      if (type != NULL && !failed && !TypeContainsTemplateParameter(type)) {
        def = TemplateArgumentAlloc();
        def->kind = kTemplateParameterType;
        def->is_pack_expansion = false;
        def->type = type;
        def->int_value = 0;
        def->template_parameter_index = -1;
        def->pack_arguments = NULL;
        def->dependent_expr = NULL;
        def->location = SOURCE_LOCATION_MISSING;
      } else {
        TypeRecordDelete(type);
      }
    }
    if (def != NULL && def->kind == kTemplateParameterNonType &&
        def->template_parameter_index >= 0 &&
        (size_t)def->template_parameter_index < extended->length) {
      TemplateArgument* actual =
          extended->value.p[def->template_parameter_index];
      TemplateArgumentDelete(def);
      def = actual != NULL ? TemplateArgumentCopy(actual) : NULL;
    }
    if (def == NULL) {
      // A trailing parameter has no usable (concrete) default; abandon the
      // extension and let the caller fall back to its prior behaviour.
      VectorDeleteWithContents(extended,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      if (aligned != NULL) {
        VectorDeleteWithContents(
            aligned, (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
      }
      return NULL;
    }
    VectorAppend(extended, def);
    if (aligned != NULL) {
      VectorAppend(aligned, TemplateArgumentCopy(def));
    }
  }
  if (aligned != NULL) {
    VectorDeleteWithContents(aligned,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  return extended;
}

Vector* CompleteAliasTemplateArguments(TypeParser* parser, Symbol* alias,
                                       Vector* actuals) {
  if (alias == NULL) {
    return NULL;
  }
  // Fill in trailing default arguments (e.g. the `void` in
  // `enable_if_t<B, class T = void>`) before dispatching so an alias named with
  // fewer arguments than it declares still expands fully.
  Vector* extended = AliasActualsWithDefaults(parser, alias, actuals);
  Vector* effective = extended != NULL ? extended : actuals;
  Vector* result = NULL;
  // A member alias such as `template<int I> using StorageT = Storage<T, I>`
  // mentions enclosing-class parameters in its pattern.  Completing from those
  // pattern indices would demand `[T, I]` while the name is written `StorageT<I>`.
  // Prefer the alias's own parameter list when it is known.
  if (SymbolExtraOf(alias)->alias_template != NULL &&
      SymbolExtraOf(alias)->alias_template->parameters.length != 0) {
    result = CompleteAliasTemplateArgumentsFromParameters(
        &SymbolExtraOf(alias)->alias_template->parameters, effective);
  } else if (alias->type != NULL && alias->type->template_arguments != NULL) {
    result = CompleteAliasTemplateArgumentsFromPattern(alias, effective);
  }
  if (extended != NULL) {
    VectorDeleteWithContents(extended,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  return result;
}

static Vector* CompleteAliasTemplateArgumentsFromParameters(Vector* parameters,
                                                              Vector* actuals) {
  if (parameters == NULL || parameters->length == 0) {
    return NULL;
  }
  // Position within the alias's own parameter list: a member alias's
  // `param->index` is offset by the enclosing class parameters.
  int pack_index = -1;
  TemplateParameterKind pack_kind = kTemplateParameterType;
  for (size_t i = 0; i < parameters->length; i++) {
    TemplateParameter* param = parameters->value.p[i];
    if (param != NULL && param->is_parameter_pack) {
      pack_index = (int)i;
      pack_kind = param->kind;
      break;
    }
  }
  size_t parameter_count = parameters->length;
  size_t actual_count = actuals != NULL ? actuals->length : 0;
  if (pack_index < 0 && actual_count != parameter_count) {
    return NULL;
  }
  if (pack_index >= 0 && actual_count < (size_t)pack_index) {
    return NULL;
  }

  Vector* completed = NewVector();
  VectorReserve(completed, parameter_count);
  for (size_t i = 0; i < parameter_count; i++) {
    TemplateParameter* param = parameters->value.p[i];
    if (param != NULL && param->is_parameter_pack) {
      TemplateArgument* pack = NewEmptyPackTemplateArgument(pack_kind);
      for (size_t j = i; j < actual_count; j++) {
        TemplateArgument* actual = actuals->value.p[j];
        if (actual == NULL || actual->kind != pack_kind) {
          TemplateArgumentDelete(pack);
          VectorDeleteWithContents(completed,
                                   (VectorElementDestructor)TemplateArgumentDelete,
                                   /*free_element=*/false);
          return NULL;
        }
        VectorAppend(pack->pack_arguments, TemplateArgumentCopy(actual));
      }
      VectorAppend(completed, pack);
      continue;
    }
    if (actuals == NULL || i >= actual_count) {
      VectorDeleteWithContents(completed,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      return NULL;
    }
    VectorAppend(completed, TemplateArgumentCopy(actuals->value.p[i]));
  }
  return completed;
}

static Vector* CompleteAliasTemplateArgumentsFromPattern(Symbol* alias,
                                                         Vector* actuals) {
  if (alias == NULL || alias->type == NULL ||
      alias->type->template_arguments == NULL) {
    return NULL;
  }
  int max_index = -1;
  int pack_index = -1;
  TemplateParameterKind pack_kind = kTemplateParameterType;
  for (size_t i = 0; i < alias->type->template_arguments->length; i++) {
    TemplateArgument* pattern_arg = alias->type->template_arguments->value.p[i];
    MaxTemplateParameterIndexInArgument(pattern_arg, &max_index);
    int current_pack_index = -1;
    TemplateParameterKind current_pack_kind = kTemplateParameterType;
    if (AliasTemplateArgumentIsPackExpansion(pattern_arg, &current_pack_index,
                                             &current_pack_kind)) {
      if (pack_index >= 0 && pack_index != current_pack_index) {
        return NULL;
      }
      pack_index = current_pack_index;
      pack_kind = current_pack_kind;
    }
  }
  if (max_index < 0) {
    return NULL;
  }
  size_t parameter_count = (size_t)max_index + 1;
  size_t actual_count = actuals != NULL ? actuals->length : 0;
  if (pack_index < 0 && actual_count != parameter_count) {
    return NULL;
  }
  if (pack_index >= 0 && actual_count < (size_t)pack_index) {
    return NULL;
  }

  Vector* completed = NewVector();
  VectorReserve(completed, parameter_count);
  for (size_t i = 0; i < parameter_count; i++) {
    if (pack_index >= 0 && i == (size_t)pack_index) {
      TemplateArgument* pack = NewEmptyPackTemplateArgument(pack_kind);
      for (size_t j = i; j < actual_count; j++) {
        TemplateArgument* actual = actuals->value.p[j];
        if (actual == NULL || actual->kind != pack_kind) {
          TemplateArgumentDelete(pack);
          VectorDeleteWithContents(completed,
                                   (VectorElementDestructor)TemplateArgumentDelete,
                                   /*free_element=*/false);
          return NULL;
        }
        if (actual->pack_arguments != NULL) {
          for (size_t k = 0; k < actual->pack_arguments->length; k++) {
            VectorAppend(pack->pack_arguments,
                         TemplateArgumentCopy(
                             actual->pack_arguments->value.p[k]));
          }
        } else {
          VectorAppend(pack->pack_arguments, TemplateArgumentCopy(actual));
        }
      }
      VectorAppend(completed, pack);
      continue;
    }

    if (actuals == NULL || i >= actual_count) {
      VectorDeleteWithContents(completed,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      return NULL;
    }
    VectorAppend(completed, TemplateArgumentCopy(actuals->value.p[i]));
  }
  return completed;
}

/* If `alias` is an alias template whose pattern names a class template (e.g.
 * `template<class T> using Vec = vector<T>;`), instantiate it by substituting
 * the alias arguments into the underlying template's argument list and
 * instantiating that class template. Returns NULL if `alias` is not such an
 * alias (so the caller falls through to normal class instantiation). */
