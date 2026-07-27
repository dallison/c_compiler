//
//  type_template_substitute.c
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
#include "member_pointer.h"
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
static Vector* CompleteAliasTemplateArgumentsFromPattern(Symbol* alias,
                                                         Vector* actuals);
static Vector* CompleteAliasTemplateArgumentsFromParameters(Vector* parameters,
                                                              Vector* actuals);

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
    if (TypeIsTemplateParameterPlaceholder(t, &index) &&
        RecordPackExpansionIndex(index, args, pack_index, pack_length)) {
      found = true;
    }
    if (t->declarator == kDeclArray &&
        RecordPackExpansionIndex(t->info.array.template_parameter_index, args,
                                 pack_index, pack_length)) {
      found = true;
    }
    if (t->template_arguments != NULL) {
      for (size_t i = 0; i < t->template_arguments->length; i++) {
        if (FindPackExpansionInTemplateArgument(t->template_arguments->value.p[i],
                                                args, pack_index,
                                                pack_length)) {
          found = true;
        }
      }
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

bool FindPackExpansionInTemplateArgument(TemplateArgument* arg,
                                                Vector* args,
                                                int* pack_index,
                                                size_t* pack_length) {
  if (arg == NULL) {
    return false;
  }
  if (arg->kind == kTemplateParameterNonType &&
      RecordPackExpansionIndex(arg->template_parameter_index, args, pack_index,
                               pack_length)) {
    return true;
  }
  if (arg->kind == kTemplateParameterType &&
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
  return FindPackExpansionInType(arg->type, args, pack_index, pack_length);
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

// Re-evaluate a value-dependent non-type template-argument expression (stored
// unevaluated at parse time, e.g. `!is_integral<It>::value`) against concrete
// template arguments; defined after the template-body clone machinery it relies
// on.
TemplateArgument* NewSubstitutedTemplateArgument(TypeParser* parser,
                                                 TemplateArgument* arg,
                                                 Vector* args) {
  TemplateArgument* concrete = malloc(sizeof(TemplateArgument));
  memset(concrete, 0, sizeof(*concrete));
  concrete->kind = arg->kind;
  concrete->is_pack_expansion = false;
  concrete->type =
      arg->kind == kTemplateParameterNonType && arg->type != NULL
          ? TypeRecordCopy(arg->type) : NULL;
  concrete->int_value = arg->int_value;
  concrete->template_parameter_index = arg->template_parameter_index;
  concrete->pack_arguments = NULL;
  concrete->dependent_expr = NULL;
  concrete->location = arg->location;
  concrete->value_kind = arg->value_kind;
  concrete->value_symbol = arg->value_symbol;
  concrete->value_offset = arg->value_offset;
  concrete->value_adjustment = arg->value_adjustment;
  concrete->member_function = arg->member_function;
  // A value-dependent non-type argument (e.g. an `enable_if` SFINAE condition):
  // try to fold it now that some parameters are concrete.  If it folds, the
  // argument becomes an ordinary integer; otherwise keep the expression so a
  // later, more-concrete substitution can complete it.
  if (arg->kind == kTemplateParameterNonType && arg->dependent_expr != NULL) {
    int64_t folded = 0;
    if (TryFoldDependentTemplateArgument(parser, arg->dependent_expr, args,
                                         &folded)) {
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
      concrete->dependent_expr =
          partial != NULL ? partial : arg->dependent_expr;
    }
    return concrete;
  }
  if (arg->kind == kTemplateParameterType && arg->type != NULL) {
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
        concrete->type = TypeRecordCopy(actual->type);
        concrete->type =
            TypeMaterializeClassTemplateSpecialization(parser->syntax,
                                                       concrete->type);
        concrete->template_parameter_index = -1;
      } else {
        concrete->template_parameter_index = actual->template_parameter_index;
      }
    } else if (actual != NULL && actual->kind == kTemplateParameterNonType &&
               actual->type != NULL) {
      concrete->type = TypeRecordCopy(actual->type);
      concrete->type =
          TypeMaterializeClassTemplateSpecialization(parser->syntax,
                                                     concrete->type);
      concrete->template_parameter_index = -1;
    }
  } else if (arg->kind == kTemplateParameterType && arg->type != NULL &&
             TypeIsTemplateParameterPlaceholder(arg->type, NULL) &&
             arg->type->template_parameter_index >= 0 &&
             (size_t)arg->type->template_parameter_index < args->length) {
    TemplateArgument* actual =
        args->value.p[arg->type->template_parameter_index];
    if (actual != NULL && actual->kind == kTemplateParameterNonType &&
        actual->type != NULL) {
      concrete->type = TypeRecordCopy(actual->type);
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
      symbol->dependent_value_template_parameter_index < 0) {
    return;
  }
  int index = symbol->dependent_value_template_parameter_index;
  if (args == NULL || (size_t)index >= args->length) {
    return;
  }
  TemplateArgument* arg = args->value.p[index];
  if (arg == NULL || arg->kind != kTemplateParameterNonType ||
      arg->pack_arguments != NULL) {
    return;
  }
  if (arg->template_parameter_index >= 0) {
    symbol->dependent_value_template_parameter_index =
        arg->template_parameter_index;
    symbol->flags.value_set = false;
    return;
  }
  symbol->value.ivalue = arg->int_value;
  symbol->flags.value_set = true;
  symbol->dependent_value_template_parameter_index = -1;
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
      TypeRecord* deduced = TypeDeduceAuto(symbol->type, cloned->type);
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
        symbol->dependent_value_template_parameter_index = -1;
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
    symbol->dependent_value_template_parameter_index = -1;
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
  }
  DiagnosticSuppressEnd();
  ASTNodeDelete(cloned);
  if (ok) {
    symbol->flags.value_set = true;
    symbol->dependent_value_template_parameter_index = -1;
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
        TypeIsTemplateParameterPlaceholder(arg->type, &index) &&
        index >= 0 && (size_t)index < args->length) {
      pack = args->value.p[index];
    } else if (arg->kind == kTemplateParameterNonType &&
               arg->template_parameter_index >= 0 &&
               (size_t)arg->template_parameter_index < args->length) {
      pack = args->value.p[arg->template_parameter_index];
    }
    if (pack == NULL && index >= 0 &&
        (args == NULL || (size_t)index >= args->length)) {
      VectorAppend(out, TemplateArgumentCopy(arg));
      return;
    }
    if (pack != NULL && pack->pack_arguments != NULL) {
      for (size_t i = 0; i < pack->pack_arguments->length; i++) {
        if (arg->kind == kTemplateParameterType && index >= 0 &&
            arg->type != NULL && arg->type->qualifiers != kQualPlain) {
          Vector* element_args = TemplateArgumentVectorCopyWithPackElement(
              args, index, pack->pack_arguments->value.p[i]);
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
    if (pack != NULL && pack->is_pack_expansion) {
      VectorAppend(out, TemplateArgumentCopy(pack));
      return;
    }
    int pattern_pack_index = -1;
    size_t pattern_pack_length = 0;
    if (arg->kind == kTemplateParameterType &&
        FindPackExpansionInType(arg->type, args, &pattern_pack_index,
                                &pattern_pack_length) &&
        pattern_pack_index >= 0 && (size_t)pattern_pack_index < args->length) {
      TemplateArgument* pattern_pack = args->value.p[pattern_pack_index];
      if (pattern_pack != NULL && pattern_pack->pack_arguments != NULL) {
        for (size_t i = 0; i < pattern_pack->pack_arguments->length; i++) {
          Vector* element_args = TemplateArgumentVectorCopyWithPackElement(
              args, pattern_pack_index, pattern_pack->pack_arguments->value.p[i]);
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
    TemplateArgument* pack = malloc(sizeof(TemplateArgument));
    memset(pack, 0, sizeof(*pack));
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

/* Replace a self-reference to a template definition with the concrete struct
 * currently being instantiated. This keeps injected-class-name uses (`map`,
 * `iterator`, etc.) tied to the active instantiation instead of the primary
 * template's generic struct. */
static TypeRecord* SubstituteTemplateSelfReference(TypeRecord* type,
                                                   Struct* source,
                                                   Struct* target) {
  if (source == NULL || target == NULL || type->declarator != kDeclPrimitive ||
      !TypeIsStructOrUnion(type) || type->info.struct_info != source ||
      type->template_arguments != NULL) {
    return NULL;
  }
  TypeRecord* subst = TypeRecordCopy(type);
  subst->info.struct_info = target;
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
    TypeParser* parser, TypeRecord* type) {
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
    StructMember* member =
        FindStructMember(target, type->info.struct_info->tag_name);
    if (member != NULL && member->symbol != NULL &&
        member->symbol->type != NULL && TypeIsStructOrUnion(member->symbol->type)) {
      TypeRecord* subst = TypeRecordCopy(member->symbol->type);
      subst->qualifiers |= type->qualifiers;
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

void DeleteStringVector(Vector* strings) {
  if (strings == NULL) {
    return;
  }
  VectorDeleteWithContents(strings, (VectorElementDestructor)StringDestruct,
                           /*free_element=*/true);
}

TypeRecord* SubstituteDependentMemberType(TypeParser* parser,
                                                 TypeRecord* type,
                                                 Vector* args) {
  if (type->declarator != kDeclPrimitive ||
      type->template_parameter_index < 0 ||
      type->dependent_member_name == NULL) {
    return NULL;
  }
  int index = type->template_parameter_index;
  if (args == NULL || index < 0 || (size_t)index >= args->length) {
    return TypeRecordCopy(type);
  }
  TemplateArgument* arg = args->value.p[index];
  if (arg == NULL || arg->kind != kTemplateParameterType ||
      arg->type == NULL || !TypeIsStructOrUnion(arg->type) ||
      arg->type->info.struct_info == NULL) {
    return TypeRecordCopy(type);
  }
  if (type->dependent_member_template_arguments != NULL) {
    Vector* components = SplitDependentMemberPath(type->dependent_member_name);
    TypeRecord* current = TypeRecordCopy(arg->type);
    TypeRecord* resolved = NULL;
    for (size_t i = 0; i < components->length; i++) {
      if (!TypeIsStructOrUnion(current) || current->info.struct_info == NULL) {
        break;
      }
      String* component = components->value.p[i];
      Vector* component_args =
          i < type->dependent_member_template_arguments->length
              ? type->dependent_member_template_arguments->value.p[i]
              : NULL;
      StructMember* member =
          FindStructMember(current->info.struct_info, component);
      TypeRecordDelete(current);
      current = NULL;
      if (member == NULL || member->symbol == NULL ||
          member->symbol->type == NULL ||
          (!StorageIs(member->symbol->storage, STO(typedef)) &&
           !TypeIsStructOrUnion(member->symbol->type))) {
        break;
      }
      if (component_args != NULL) {
        if (!member->symbol->flags.is_template &&
            (!TypeIsStructOrUnion(member->symbol->type) ||
             member->symbol->type->info.struct_info == NULL ||
             !member->symbol->type->info.struct_info->is_template)) {
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
        if (TemplateArgumentVectorContainsTemplateParameter(concrete_args)) {
          VectorDeleteWithContents(
              concrete_args, (VectorElementDestructor)TemplateArgumentDelete,
              /*free_element=*/false);
          resolved = TypeRecordCopy(type);
          break;
        }
        current = InstantiateSimpleClassTemplate(parser, member->symbol,
                                                 concrete_args);
        VectorDeleteWithContents(concrete_args,
                                 (VectorElementDestructor)TemplateArgumentDelete,
                                 /*free_element=*/false);
      } else {
        current = TypeRecordCopy(member->symbol->type);
      }
      if (i + 1 == components->length) {
        resolved = current;
        current = NULL;
      }
    }
    TypeRecordDelete(current);
    DeleteStringVector(components);
    if (resolved != NULL) {
      resolved->qualifiers |= type->qualifiers;
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
  StructMember* member =
      FindStructMember(owner_struct, type->dependent_member_name);
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
  TypeRecord* subst = TypeRecordCopy(member->symbol->type);
  subst->qualifiers |= type->qualifiers;
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
TypeRecord* SubstituteTemplateIdType(TypeParser* parser,
                                            TypeRecord* type,
                                            Vector* args,
                                            TemplateIdSubstitutionInfo info) {
  if (info.origin == NULL || info.args == NULL) {
    return NULL;
  }
  Vector* concrete_args =
      SubstituteTemplateArgumentVectorForTypes(parser, info.args, args);
  bool expandable_alias =
      CompilerIsCXX() && info.origin->flags.is_template &&
      StorageIs(info.origin->storage, STO(typedef)) &&
      !TypeIsStructOrUnion(info.origin->type) &&
      !CXXAliasTemplatePatternNamesClassTemplate(info.origin);
  bool dependent_args =
      TemplateArgumentVectorContainsTemplateParameter(concrete_args);
  if (dependent_args && !expandable_alias &&
      type->dependent_member_name != NULL &&
      TypeIsStructOrUnion(info.origin->type) &&
      info.origin->type->info.struct_info != NULL &&
      info.origin->type->info.struct_info->partial_specializations.length == 0) {
    // A dependent member of a primary class template with no partial
    // specializations can be resolved from the primary immediately.  This is
    // important for traits such as
    // `add_rvalue_reference_t<T> = add_rvalue_reference<T>::type`: preserving
    // the resulting `T&&` lets a surrounding dependent decltype retain the
    // correct value category instead of degrading to the parser's `int`
    // placeholder.
    StructMember* member = FindStructMember(
        info.origin->type->info.struct_info, type->dependent_member_name);
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
    if (member != NULL && member->symbol != NULL && !member_is_nested_type &&
        StorageIs(member->symbol->storage, STO(typedef))) {
      TypeRecord* subst = SubstituteTemplateParameters(
          parser, member->symbol->type, concrete_args);
      subst->qualifiers |= type->qualifiers;
      VectorDeleteWithContents(concrete_args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      return TypeRecordCalculateSize(subst);
    }
  }
  if (dependent_args && !expandable_alias) {
    TypeRecord* deferred = TypeRecordCopy(type);
    if (deferred->template_arguments != NULL) {
      VectorDeleteWithContents(
          deferred->template_arguments,
          (VectorElementDestructor)TemplateArgumentDelete,
          /*free_element=*/false);
    }
    deferred->template_arguments = concrete_args;
    deferred->qualifiers |= type->qualifiers;
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
        CompleteAliasTemplateArguments(info.origin, concrete_args);
    Vector* pattern_args = grouped_args != NULL ? grouped_args : concrete_args;
    TypeRecord* subst =
        SubstituteTemplateParameters(parser, info.origin->type, pattern_args);
    subst->qualifiers |= type->qualifiers;
    // The alias pattern may be a dependent-member type such as
    // `typename ratio<...>::type`.  Now that the arguments are concrete the
    // result is a real class type; collapse the lazy `Template<Args>::member`
    // encoding to that concrete specialization so downstream uses (e.g. as a
    // template argument whose members are later accessed) see a plain class.
    subst = TypeMaterializeClassTemplateSpecialization(parser->syntax, subst);
    // Once this alias has expanded to its concrete result, do not retain the
    // alias template-id on that result. Keeping `decay_t<const T>` metadata on
    // the resulting `T` makes later substitutions treat the concrete type as
    // dependent and prevents non-type trait arguments from folding.
    if (subst->template_origin == info.origin) {
      subst->template_origin = NULL;
      if (subst->template_arguments != NULL) {
        VectorDeleteWithContents(
            subst->template_arguments,
            (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
      }
      subst->template_arguments = NULL;
    }
    if (grouped_args != NULL) {
      VectorDeleteWithContents(grouped_args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    VectorDeleteWithContents(concrete_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return TypeRecordCalculateSize(subst);
  }

  TypeRecord* subst =
      InstantiateSimpleClassTemplate(parser, info.origin, concrete_args);
  if (subst == NULL) {
    VectorDeleteWithContents(concrete_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return TypeRecordCopy(type);
  }
  subst->qualifiers |= type->qualifiers;

  if (type->dependent_member_name != NULL &&
      TypeIsStructOrUnion(subst) && subst->info.struct_info != NULL) {
    StructMember* member =
        FindStructMember(subst->info.struct_info,
                         type->dependent_member_name);
    if (member != NULL && member->symbol != NULL &&
        StorageIs(member->symbol->storage, STO(typedef))) {
      TypeRecord* member_type = NULL;
      Vector* member_template_args =
          type->dependent_member_template_arguments != NULL &&
                  type->dependent_member_template_arguments->length > 0
              ? type->dependent_member_template_arguments->value.p[0]
              : NULL;
      if (member_template_args != NULL) {
        Vector* concrete_member_args =
            SubstituteTemplateArgumentVectorForTypes(parser, member_template_args,
                                                    args);
        Vector* combined_args = TemplateArgumentVectorCopy(concrete_args);
        for (size_t i = 0; i < concrete_member_args->length; i++) {
          VectorAppend(combined_args,
                       TemplateArgumentCopy(concrete_member_args->value.p[i]));
        }
        member_type = SubstituteTemplateParameters(
            parser, member->symbol->type, combined_args);
        // Expanding a member alias template can expose another deferred
        // template-id whose local parameter indices were not visible on the
        // outer dependent-member type (for example allocator_traits'
        // rebind_alloc<T>).  Resolve that newly exposed layer while the
        // combined enclosing/member argument list is still available.
        if (member_type != NULL &&
            TypeContainsTemplateParameter(member_type)) {
          TemplateIdSubstitutionInfo exposed =
              TemplateIdInfoForSubstitution(member_type);
          if (exposed.origin != NULL && exposed.args != NULL) {
            Vector* exposed_args = SubstituteTemplateArgumentVectorForTypes(
                parser, exposed.args, combined_args);
            if (!TemplateArgumentVectorContainsTemplateParameter(exposed_args)) {
              TypeRecord* resolved_member_type = InstantiateSimpleClassTemplate(
                  parser, exposed.origin, exposed_args);
              resolved_member_type->qualifiers |= member_type->qualifiers;
              TypeRecordDelete(member_type);
              member_type = resolved_member_type;
            }
            VectorDeleteWithContents(
                exposed_args, (VectorElementDestructor)TemplateArgumentDelete,
                /*free_element=*/false);
          }
        }
        VectorDeleteWithContents(
            combined_args, (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
        VectorDeleteWithContents(
            concrete_member_args, (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
      } else {
        member_type =
            SubstituteTemplateParameters(parser, member->symbol->type,
                                         concrete_args);
      }
      member_type->qualifiers |= type->qualifiers;
      TypeRecordDelete(subst);
      VectorDeleteWithContents(concrete_args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      return TypeRecordCalculateSize(member_type);
    }
    if (parser != NULL &&
        !StructContainsTemplateParameter(subst->info.struct_info)) {
      parser->template_substitution_failed = true;
    }
  }
  VectorDeleteWithContents(concrete_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return TypeRecordCalculateSize(subst);
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
  if (arg == NULL || arg->kind != kTemplateParameterType ||
      arg->type == NULL) {
    return TypeRecordCopy(type);
  }
  TypeRecord* subst = TypeRecordCopy(arg->type);
  subst->qualifiers |= type->qualifiers;
  subst = TypeMaterializeClassTemplateSpecialization(parser->syntax, subst);
  return subst;
}

/* Substitute a non-type template parameter used as an array bound. */
static void SubstituteArrayTemplateBound(TypeRecord* copy,
                                         TypeRecord* original,
                                         Vector* args) {
  if (copy->declarator != kDeclArray ||
      original->info.array.template_parameter_index < 0) {
    return;
  }
  int index = original->info.array.template_parameter_index;
  if (args == NULL || (size_t)index >= args->length) {
    return;
  }
  TemplateArgument* arg = args->value.p[index];
  if (arg == NULL || arg->kind != kTemplateParameterNonType) {
    return;
  }
  if (arg->template_parameter_index >= 0) {
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
  copy->info.struct_info = argument->type->info.struct_info;
  copy->template_parameter_index = -1;
  copy->type &= ~kTypeUnknown;
  copy->size = MemberPointerSize(copy);
}

/* Replace the `next` type in a copied pointer/reference/array/function spine.
 * C++ reference collapsing and pointer-to-reference cleanup both happen here so
 * callers see a valid post-substitution type chain. */
static void SubstituteTypeSpineNext(TypeParser* parser, TypeRecord* copy,
                                    TypeRecord* original, Vector* args) {
  if (copy->next == NULL) {
    return;
  }
  TypeRecord* original_next = original->next;
  TypeRecordIncRef(original_next);
  TypeRecordDelete(copy->next);
  copy->next = SubstituteTemplateParameters(parser, original_next, args);
  TypeRecordDelete(original_next);
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
  TypeRecord* copy = TypeRecordCopy(type);
  SubstituteMemberPointerClass(copy, args);
  SubstituteArrayTemplateBound(copy, type, args);
  SubstituteTypeSpineNext(parser, copy, type, args);
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
 * Returns a newly allocated, size-calculated type record. */
static Vector g_dependent_decltype_stack;
static bool g_dependent_decltype_stack_initialized;

static bool DependentDecltypeStackContains(ASTNode* expr) {
  for (size_t i = 0; i < g_dependent_decltype_stack.length; i++) {
    if (g_dependent_decltype_stack.value.p[i] == expr) {
      return true;
    }
  }
  return false;
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
static bool TypeIsDecltypeAliasTemplateId(TypeRecord* type) {
  return CompilerIsCXX() && type != NULL &&
         type->dependent_decltype_expr != NULL &&
         type->template_origin != NULL && type->template_arguments != NULL &&
         type->template_origin->flags.is_template &&
         StorageIs(type->template_origin->storage, STO(typedef)) &&
         type->template_origin->type != NULL &&
         type->template_origin->type != type &&
         type->template_origin->type->dependent_decltype_expr ==
             type->dependent_decltype_expr;
}

static void ClearDependentExpressionAnalysis(ASTNode* node, void* data,
                                             int child_id, VisitorMode mode) {
  (void)data;
  (void)child_id;
  if (mode == kVisitPreChildren && node != NULL) {
    node->flags &= ~kASTAnalyzed;
  }
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
  if (type->dependent_decltype_expr != NULL &&
      !DependentDecltypeStackContains(type->dependent_decltype_expr)) {
    VectorAppend(&g_dependent_decltype_stack, type->dependent_decltype_expr);
    // decltype is unevaluated: cloning and resolving a declaration-only
    // function such as std::declval must not require a function body.
    DiagnosticSuppressBegin();
    // Cloning the operand substitutes template arguments and may speculatively
    // instantiate declaration-only helpers (e.g. std::declval), which can
    // record recovered diagnostics in the enclosing error trap even though the
    // clone itself succeeds.  Isolate that noise so a SFINAE-sensitive caller
    // (e.g. a `requires { typename decltype(...); }` type-requirement) only
    // observes errors from the actual re-analysis below, not from cloning.
    bool clone_trap = DiagnosticErrorTrapBegin();
    ASTNode* expr = CloneDependentExpressionWithArgs(
        parser, type->dependent_decltype_expr, args);
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
      expr = AnalyzeExpression(expr);
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
            TypeIsUnknown(expr->type) || TypeContainsTemplateParameter(expr->type);
        if (operand_still_dependent) {
          TypeRecord* opaque = TypeRecordCopy(type);
          opaque->qualifiers |= type->qualifiers;
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
              parser, type->dependent_decltype_expr, args);
          if (deferred != NULL) {
            opaque->dependent_decltype_expr = deferred;
            ASTNodeDelete(expr);
          } else {
            opaque->dependent_decltype_expr = expr;
          }
          DiagnosticSuppressEnd();
          VectorPop(&g_dependent_decltype_stack);
          return TypeRecordCalculateSize(opaque);
        }
        TypeRecord* result = NULL;
        if (expr->value_category == kValueCategoryLvalue) {
          result = NewDecltypeReference(expr->type, false);
        } else if (expr->value_category == kValueCategoryXvalue) {
          result = NewDecltypeReference(expr->type, true);
        } else {
          result = TypeRecordCopy(expr->type);
        }
        if (result != NULL) {
          result->qualifiers |= type->qualifiers;
          ASTNodeDelete(expr);
          DiagnosticSuppressEnd();
          VectorPop(&g_dependent_decltype_stack);
          return TypeRecordCalculateSize(result);
        }
      }
      ASTNodeDelete(expr);
    }
    DiagnosticSuppressEnd();
    VectorPop(&g_dependent_decltype_stack);
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
    subst = SubstituteNestedTypeFromActiveInstantiation(parser, type);
    if (subst != NULL) {
      return subst;
    }
    if (TypeNamesActiveNestedTemplateStruct(parser, type)) {
      return SubstituteNestedStructTemplateParameters(parser, type, args);
    }
  }

  TypeRecord* subst = SubstituteDependentMemberType(parser, type, args);
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

  if (CompilerIsCXX() && TypeIsStructOrUnion(type) &&
      StructContainsTemplateParameter(type->info.struct_info)) {
    if (type->info.struct_info != NULL &&
        type->info.struct_info->is_template &&
        template_id.origin == NULL && template_id.args == NULL) {
      return TypeRecordCopy(type);
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
  clone->default_argument =
      ASTNodeClone(formal->default_argument, IdentityCloneNode, NULL, NULL);
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
  Vector element_args;
  VectorInit(&element_args);
  for (size_t i = 0; i < args->length; i++) {
    VectorAppend(&element_args,
                 i == (size_t)pack_index ? pack->pack_arguments->value.p[element_index]
                                          : args->value.p[i]);
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
  if (type == NULL || base <= 0) {
    return;
  }
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (t->template_parameter_index >= base) {
      t->template_parameter_index -= base;
    }
    if (t->declarator == kDeclArray &&
        t->info.array.template_parameter_index >= base) {
      t->info.array.template_parameter_index -= base;
    }
    if (t->template_arguments != NULL) {
      for (size_t i = 0; i < t->template_arguments->length; i++) {
        RebaseTemplateArgumentParameterIndices(t->template_arguments->value.p[i],
                                               base);
      }
    }
    // A deferred `decltype` operand (e.g. `iterator_t<R> =
    // decltype(ranges::begin(declval<R&>()))`) references template parameters
    // inside its expression AST, not the type spine.  Those must be rebased too
    // or a later substitution with the member's own arguments (numbered from 0)
    // would fail to reach the enclosing-relative parameter still named in the
    // operand, leaving the decltype permanently unresolved.
    if (t->dependent_decltype_expr != NULL) {
      t->dependent_decltype_expr =
          CloneAndRebaseDependentExpression(t->dependent_decltype_expr, base);
    }
  }
}

void RebaseTemplateParameterIndices(TypeRecord* type, int base) {
  if (type == NULL || base <= 0) {
    return;
  }
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (t->template_parameter_index >= base) {
      t->template_parameter_index -= base;
    }
    if (t->declarator == kDeclArray &&
        t->info.array.template_parameter_index >= base) {
      t->info.array.template_parameter_index -= base;
    }
    if (t->template_arguments != NULL) {
      for (size_t i = 0; i < t->template_arguments->length; i++) {
        RebaseTemplateArgumentParameterIndices(t->template_arguments->value.p[i],
                                               base);
      }
    }
    if (t->dependent_decltype_expr != NULL) {
      t->dependent_decltype_expr =
          CloneAndRebaseDependentExpression(t->dependent_decltype_expr, base);
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
  }
}

/* Rebase (see RebaseTemplateParameterIndices) the parameter indices inside a
 * template argument and its referenced type. */
void RebaseTemplateArgumentParameterIndices(TemplateArgument* arg,
                                                   int base) {
  if (arg == NULL || base <= 0) {
    return;
  }
  if (arg->template_parameter_index >= base) {
    arg->template_parameter_index -= base;
  }
  RebaseTemplateParameterIndices(arg->type, base);
  arg->dependent_expr =
      CloneAndRebaseDependentExpression(arg->dependent_expr, base);
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
  int base = func->info.function.template_parameter_base;
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
  for (size_t i = 0; i < func->info.function.template_parameters.length; i++) {
    TemplateParameter* param =
        func->info.function.template_parameters.value.p[i];
    if (param != NULL && param->index >= base) {
      param->index -= base;
    }
  }
  func->info.function.template_parameter_base = 0;
}

typedef struct {
  int base;
} RebaseDependentExpressionData;

static void RebaseDependentExpressionVisitor(ASTNode* node, void* data,
                                             int child_id, VisitorMode mode) {
  (void)child_id;
  (void)mode;
  if (node == NULL || node->op != AST_OP(identifier)) {
    return;
  }
  RebaseDependentExpressionData* rebase = data;
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  Symbol* old = id->symbol;
  if (old == NULL) {
    return;
  }
  bool needs_copy = old->template_parameter_index >= rebase->base ||
                    old->dependent_value_template_parameter_index >=
                        rebase->base ||
                    TypeContainsTemplateParameter(old->type) ||
                    (id->template_arguments != NULL &&
                     TemplateArgumentVectorContainsTemplateParameter(
                         id->template_arguments));
  if (!needs_copy) {
    return;
  }
  TypeRecord* type =
      old->type != NULL ? TypeRecordCopy(old->type) : NULL;
  RebaseTemplateParameterIndices(type, rebase->base);
  Symbol* copy = NewSymbol(old->name.value, type, old->storage);
  copy->namespace_ = old->namespace_;
  copy->flags = old->flags;
  copy->alignment = old->alignment;
  copy->template_parameter_index = old->template_parameter_index;
  if (copy->template_parameter_index >= rebase->base) {
    copy->template_parameter_index -= rebase->base;
  }
  copy->dependent_value_template_parameter_index =
      old->dependent_value_template_parameter_index;
  if (copy->dependent_value_template_parameter_index >= rebase->base) {
    copy->dependent_value_template_parameter_index -= rebase->base;
  }
  copy->location = old->location;
  copy->value = old->value;
  copy->stack_offset = old->stack_offset;
  copy->alias_target = old->alias_target;
  id->symbol = copy;
  ASTNodeSetType(node, copy->type);
  // Explicit template arguments on the id (e.g. the `R&` in `declval<R&>()`
  // inside `iterator_t<R> = decltype(ranges::begin(declval<R&>()))`) also carry
  // enclosing-relative parameter indices.  The clone owns its own copy of this
  // vector (IdentifierASTNodeCopy deep-copies it), so rebasing it in place is
  // safe and is required or the operand keeps naming the pre-rebase parameter
  // and never resolves once the member's own arguments arrive.
  if (id->template_arguments != NULL) {
    for (size_t i = 0; i < id->template_arguments->length; i++) {
      RebaseTemplateArgumentParameterIndices(id->template_arguments->value.p[i],
                                             rebase->base);
    }
  }
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
  clone.parser = parser;
  clone.args = args;
  clone.to_func = NULL;
  clone.rebase_template_parameter_base = 0;
  clone.from_owner = NULL;
  clone.to_owner = NULL;
  ASTNode* cloned =
      ASTNodeClone(expr, CloneDependentDecltypeNode, &clone, NULL);
  MapDestructWithContents(&clone.pack_symbol_map, DeleteMappedVector);
  MapDestruct(&clone.symbol_map);
  return cloned;
}

bool TryFoldDependentTemplateArgument(TypeParser* parser, ASTNode* expr,
                                             Vector* args, int64_t* out) {
  if (expr == NULL || args == NULL) {
    return false;
  }
  ASTNode* cloned = CloneDependentExpressionWithArgs(parser, expr, args);
  if (cloned == NULL) {
    return false;
  }
  // A dependent qualified name that survived cloning (its scope is still
  // dependent) must not be diagnosed here: this is a speculative fold, and an
  // unresolved name simply means the value stays dependent for now.
  DiagnosticSuppressBegin();
  cloned = AnalyzeExpression(cloned);
  bool ok = EvaluateIntegerExpression(cloned, out);
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
ASTNode* TypeSubstituteTemplateExpressionAndRebase(
    Syntax* syntax, ASTNode* expr, Vector* args, int rebase_base,
    SourceLocation location) {
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
  clone.to_func = NULL;
  clone.rebase_template_parameter_base = rebase_base;
  clone.from_owner = NULL;
  clone.to_owner = NULL;
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
      RebaseTemplateArgumentParameterIndices(concrete_args->value.p[j],
                                             rebase_base);
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
    TemplateArgument* arg = malloc(sizeof(TemplateArgument));
    memset(arg, 0, sizeof(*arg));
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
static Vector* AliasActualsWithDefaults(Symbol* alias, Vector* actuals) {
  if (alias == NULL || alias->alias_template == NULL) {
    return NULL;
  }
  Vector* parameters = &alias->alias_template->parameters;
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
  for (size_t i = actual_count; i < parameter_count; i++) {
    TemplateArgument* def =
        AliasDefaultTemplateArgument(parameters->value.p[i]);
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
      return NULL;
    }
    VectorAppend(extended, def);
  }
  return extended;
}

Vector* CompleteAliasTemplateArguments(Symbol* alias, Vector* actuals) {
  if (alias == NULL) {
    return NULL;
  }
  // Fill in trailing default arguments (e.g. the `void` in
  // `enable_if_t<B, class T = void>`) before dispatching so an alias named with
  // fewer arguments than it declares still expands fully.
  Vector* extended = AliasActualsWithDefaults(alias, actuals);
  Vector* effective = extended != NULL ? extended : actuals;
  Vector* result = NULL;
  if (alias->type != NULL && alias->type->template_arguments != NULL) {
    result = CompleteAliasTemplateArgumentsFromPattern(alias, effective);
  } else if (alias->alias_template != NULL &&
             alias->alias_template->parameters.length != 0) {
    result = CompleteAliasTemplateArgumentsFromParameters(
        &alias->alias_template->parameters, effective);
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
  int pack_index = -1;
  TemplateParameterKind pack_kind = kTemplateParameterType;
  for (size_t i = 0; i < parameters->length; i++) {
    TemplateParameter* param = parameters->value.p[i];
    if (param != NULL && param->is_parameter_pack) {
      pack_index = param->index;
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
