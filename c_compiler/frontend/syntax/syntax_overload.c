//
//  syntax_overload.c
//  c_compiler
//
//  File-scope symbol insertion, overload and redeclaration matching, and
//  explicit specialization and instantiation of function templates.
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

static bool OverloadTypesEqual(TypeRecord* left, TypeRecord* right);

// Looks up `name` in the current named namespace when inside one, otherwise
// in the global symbol table. Returns the symbol or NULL if it is not found.
Symbol* FindFileScopeSymbol(Syntax* syntax, String* name) {
  if (InNamedNamespace(syntax)) {
    Symbol* symbol = NamespaceFindSymbol(syntax->current_namespace, name);
    if (symbol != NULL) {
      return symbol;
    }
    return NULL;
  }
  return FindGlobalSymbol(name);
}

// Marks `symbol` exported when parsing inside an export block and attaches
// module identity from its storage class to the symbol.
void MarkExportedDeclaration(Syntax* syntax, Symbol* symbol) {
  if (syntax->export_depth > 0) {
    symbol->flags.is_exported = true;
  }
  SymbolAttachModuleContext(symbol, symbol->storage);
}

// Inserts `symbol` into the current namespace or the global table after
// applying export/module marking. Returns false if the name is already taken.
bool InsertFileScopeSymbol(Syntax* syntax, Symbol* symbol) {
  MarkExportedDeclaration(syntax, symbol);
  if (InNamedNamespace(syntax)) {
    return NamespaceInsertSymbol(syntax->current_namespace, symbol);
  }
  return InsertGlobalSymbol(symbol);
}

// Returns true when both symbols are C++ function types and may share an
// overload set.
bool CanOverloadFunctions(Symbol* a, Symbol* b) {
  return CompilerIsCXX() && a != NULL && b != NULL &&
         TypeIsFunction(a->type) && TypeIsFunction(b->type);
}

// The ordinary name of a C++ class or enumeration is a typedef alias of the
// tag.  A later function or object with that name hides the class
// ([basic.scope.hiding]); elaborated lookup still finds the tag.
bool SymbolIsHideableClassOrEnumName(Symbol* sym) {
  if (!CompilerIsCXX() || sym == NULL || sym->type == NULL ||
      !StorageIs(sym->storage, STO(typedef))) {
    return false;
  }
  if (TypeIsStructOrUnion(sym->type) &&
      sym->type->info.struct_info != NULL &&
      sym->type->info.struct_info->tag_name != NULL) {
    return StringEqual(sym->type->info.struct_info->tag_name,
                       sym->name.value);
  }
  if (TypeIsEnum(sym->type) && sym->type->info.enum_info != NULL &&
      sym->type->info.enum_info->tag_name != NULL) {
    return StringEqual(sym->type->info.enum_info->tag_name, sym->name.value);
  }
  return false;
}

// Removes the injected class or enumeration typedef from local, namespace, or
// global lookup so a later declaration with the same name can hide it.
void HideClassOrEnumName(Syntax* syntax, Symbol* injected) {
  if (syntax->local_symbol_stack != NULL) {
    UninstallLocalSymbol(syntax->local_symbol_stack, injected);
  }
  if (InNamedNamespace(syntax)) {
    UninstallNamespaceSymbol(syntax->current_namespace, injected, false);
  } else {
    UninstallGlobalSymbol(injected, false);
  }
}

// Rank a template overload against an explicit specialization's concrete
// parameter list. Concrete template parameters must match exactly; dependent
// parameters are left for normal specialization substitution.
static int TemplateOverloadConcreteMatchScore(Symbol* candidate,
                                              Symbol* specialization) {
  if (candidate == NULL || specialization == NULL ||
      candidate->type == NULL || specialization->type == NULL ||
      !TypeIsFunction(candidate->type) ||
      !TypeIsFunction(specialization->type) ||
      candidate->type->info.function.prototype.length !=
          specialization->type->info.function.prototype.length) {
    return -1;
  }
  int score = 0;
  for (size_t i = 0;
       i < candidate->type->info.function.prototype.length; i++) {
    Symbol* candidate_param =
        candidate->type->info.function.prototype.value.p[i];
    Symbol* specialization_param =
        specialization->type->info.function.prototype.value.p[i];
    if (candidate_param == NULL || specialization_param == NULL ||
        candidate_param->type == NULL || specialization_param->type == NULL) {
      return -1;
    }
    if (TypeContainsTemplateParameter(candidate_param->type)) {
      continue;
    }
    if (!TypeEqual(candidate_param->type, specialization_param->type)) {
      return -1;
    }
    score++;
  }
  return score;
}

// Among function-template overloads chained from `first`, returns the one
// whose non-dependent parameter types best match `specialization`, or NULL
// when no candidate matches.
static Symbol* FindFunctionTemplateOverload(Symbol* first,
                                            Symbol* specialization) {
  Symbol* best = NULL;
  int best_score = -1;
  for (Symbol* overload = first; overload != NULL;
       overload = overload->overload_next) {
    if (overload->flags.is_template && overload->type != NULL &&
        TypeIsFunction(overload->type)) {
      int score =
          TemplateOverloadConcreteMatchScore(overload, specialization);
      if (score > best_score) {
        best = overload;
        best_score = score;
      }
    }
  }
  return best;
}

// Same as FindFunctionTemplateOverload for member function templates linked
// from `first`. Returns NULL when no member template matches.
static Symbol* FindMemberFunctionTemplateOverload(
    StructMember* first, Symbol* specialization) {
  Symbol* best = NULL;
  int best_score = -1;
  for (StructMember* overload = first; overload != NULL;
       overload = overload->overload_next) {
    if (overload->symbol != NULL && overload->symbol->flags.is_template &&
        overload->symbol->type != NULL && TypeIsFunction(overload->symbol->type)) {
      int score = TemplateOverloadConcreteMatchScore(overload->symbol,
                                                     specialization);
      if (score > best_score) {
        best = overload->symbol;
        best_score = score;
      }
    }
  }
  return best;
}

// Appends `specialization` to the member overload chain headed by `first` and
// links it into the primary template's symbol overload list when present.
void AppendMemberFunctionSpecialization(Syntax* syntax,
                                        StructMember* first,
                                        StructMember* specialization) {
  StructMember* tail = first;
  while (tail->overload_next != NULL) {
    tail = tail->overload_next;
  }
  tail->overload_next = specialization;
  specialization->symbol->flags.is_overloaded = true;
  first->symbol->flags.is_overloaded = true;
  Symbol* templ = specialization->symbol->type->info.function.template_origin;
  if (templ != NULL) {
    Symbol* tail = templ;
    while (tail->overload_next != NULL) {
      tail = tail->overload_next;
    }
    tail->overload_next = specialization->symbol;
    templ->flags.is_overloaded = true;
  }
  (void)syntax;
}

// While parsing an explicit specialization, binds `sym` to its primary
// function template from `first_overload`, deduces template arguments when
// omitted, and emits a diagnostic if no template matches.
void MarkFunctionTemplateSpecialization(Syntax* syntax, Symbol* sym,
                                        Symbol* first_overload) {
  if (!syntax->parsing_template_specialization || sym == NULL ||
      sym->type == NULL || !TypeIsFunction(sym->type)) {
    return;
  }
  Symbol* templ = FindFunctionTemplateOverload(first_overload, sym);
  if (templ == NULL) {
    String suffix;
    StringInit(&suffix, NULL);
    SymbolFunctionDiagnosticSuffix(sym, &suffix);
    SyntaxError(syntax, "%s is not a function template%s",
                sym->name.value, suffix.value);
    StringDestruct(&suffix);
    return;
  }
  // An explicit specialization may omit the template-id when its arguments
  // can be deduced from the declared function type:
  // `template <> const Message* cast(const Base*)`.  Record those deduced
  // arguments so later `cast<Message>` lookup finds this specialization
  // instead of instantiating the primary template.
  if (sym->type->template_arguments == NULL) {
    sym->type->template_arguments =
        TypeDeduceFunctionTemplateArgumentsFromFunctionType(
            syntax, templ, /*explicit_args=*/NULL, sym->type);
  }
  sym->type->info.function.template_origin = templ;
  sym->flags.is_explicit_specialization = true;
}

// Like MarkFunctionTemplateSpecialization for an out-of-line member; does
// nothing when the declaration is a class-template member, not a member
// function template.
void MarkMemberFunctionTemplateSpecialization(
    Syntax* syntax, Symbol* sym, StructMember* first_overload) {
  // `template <> void Set(const string&, Arena*)` specializes a pack
  // (`template <typename...> void Set(const string&, Arena*)`) with an empty
  // argument list, so template_arguments may be NULL.
  if (!syntax->parsing_template_specialization || sym == NULL ||
      sym->type == NULL || !TypeIsFunction(sym->type)) {
    return;
  }
  Symbol* templ = FindMemberFunctionTemplateOverload(first_overload, sym);
  if (templ == NULL && first_overload != NULL &&
      first_overload->symbol != NULL &&
      first_overload->symbol->flags.is_template) {
    templ = first_overload->symbol;
  }
  if (templ == NULL) {
    // `template <> void S<T>::f()` specializes a class-template member, not a
    // member function template.
    return;
  }
  // As for a free function specialization, the template-id may be omitted
  // when its arguments are deducible from the declared member function type.
  // This is common when the parameter appears only in the return type.
  if (sym->type->template_arguments == NULL) {
    sym->type->template_arguments =
        TypeDeduceFunctionTemplateArgumentsFromFunctionType(
            syntax, templ, /*explicit_args=*/NULL, sym->type);
  }
  sym->type->info.function.template_origin = templ;
  sym->flags.is_explicit_specialization = true;
}

// The function template an explicit instantiation names.  Overloaded
// templates are told apart only by parameter count; anything still ambiguous
// is left uninstantiated.
static Symbol* ExplicitInstantiationTemplate(Symbol* first,
                                             StructMember* first_member,
                                             TypeRecord* declared) {
  Symbol* found = NULL;
  size_t declared_parameters = declared->info.function.prototype.length;
  for (;;) {
    Symbol* candidate = NULL;
    if (first_member != NULL) {
      candidate = first_member->symbol;
      first_member = first_member->overload_next;
    } else if (first != NULL) {
      candidate = first;
      first = first->overload_next;
    } else {
      break;
    }
    if (candidate == NULL || !candidate->flags.is_template ||
        candidate->type == NULL || !TypeIsFunction(candidate->type) ||
        candidate->type->info.function.prototype.length !=
            declared_parameters) {
      continue;
    }
    if (found != NULL) {
      return NULL;
    }
    found = candidate;
  }
  return found;
}

// `template C::C(std::string&&);`: the member function template
// specialization deduced from the declared signature, if exactly one matches.
static Symbol* DeduceExplicitMemberInstantiation(Syntax* syntax,
                                                 TypeParser* parser,
                                                 StructMember* member,
                                                 TypeRecord* declared) {
  Symbol* found = NULL;
  for (; member != NULL; member = member->overload_next) {
    Symbol* candidate = member->symbol;
    if (candidate == NULL || !candidate->flags.is_template ||
        candidate->type == NULL || !TypeIsFunction(candidate->type)) {
      continue;
    }
    Vector* args = TypeDeduceFunctionTemplateArgumentsFromFunctionType(
        syntax, candidate, NULL, declared);    if (args == NULL) {
      continue;
    }
    Symbol* instance = TypeInstantiateFunctionTemplateWithCompletedArguments(
        syntax, candidate, args);
    VectorDeleteWithContents(args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    if (instance == NULL || instance == candidate || instance->type == NULL ||
        !MemberDefinitionTypesEqual(parser, instance->type, declared)) {
      continue;
    }
    if (found != NULL && found != instance) {
      return NULL;
    }
    found = instance;
  }
  return found;
}

// `template R f<A>(P);` / `template R C<A>::m(P);`: emit the named
// specialization.  `sym` is the parsed declarator, which declares nothing.
void DefineExplicitFunctionInstantiation(Syntax* syntax,
                                         TypeParser* parser,
                                         Symbol* sym) {
  Vector* args = sym->type->template_arguments;
  StructMember* member = parser->cxx_member_definition;
  Symbol* instance = NULL;  if (member != NULL && args == NULL) {
    StructMember* match = FindStructMemberOverload(member, sym->type);
    instance = match != NULL ? match->symbol : NULL;
    if (instance == NULL || instance->flags.is_template) {
      instance =
          DeduceExplicitMemberInstantiation(syntax, parser, member, sym->type);
    }
  } else if (args != NULL) {
    Symbol* first = NULL;
    if (member == NULL) {
      if (parser->cxx_qualified_definition_namespace != NULL) {
        NamespaceInlineSymbolLookup found = NamespaceResolveSymbolInInlineSet(
            parser->cxx_qualified_definition_namespace, &sym->name);
        if (found.status == kInlineLookupUnique) {
          first = found.symbol;
        }
      }
      if (first == NULL) {
        first = FindFileScopeSymbol(syntax, &sym->name);
      }
    }
    Symbol* templ = ExplicitInstantiationTemplate(first, member, sym->type);
    if (templ != NULL) {
      instance = TypeInstantiateFunctionTemplate(syntax, templ, args);
      if (instance == templ) {
        instance = NULL;
      }
    }
  }
  if (instance != NULL && !instance->flags.is_template &&
      instance->type != NULL && TypeIsFunction(instance->type) &&
      !instance->type->info.function.is_deleted) {
    CompilerMarkFunctionReferenced(instance);
  }
}

// Compares two parameter types for overload resolution, ignoring top-level
// const and volatile on the parameter type itself.
static bool OverloadParameterTypesEqual(TypeRecord* left, TypeRecord* right) {
  if (left == NULL || right == NULL) {
    return false;
  }
  TypeRecord left_parameter = TypeRecordValue(left);
  TypeRecord right_parameter = TypeRecordValue(right);
  SetQualifiers(&left_parameter,
                GetQualifiers(&left_parameter) & ~(kQualConst | kQualVolatile));
  SetQualifiers(&right_parameter, GetQualifiers(&right_parameter) &
                                      ~(kQualConst | kQualVolatile));
  return OverloadTypesEqual(&left_parameter, &right_parameter);
}

// Returns true when two function prototypes match for overload purposes:
// parameter count, varargs, member cv/ref qualifiers, and parameter types.
static bool OverloadFunctionPrototypesEqual(FunctionInfo* left,
                                            FunctionInfo* right) {
  if (left->prototype.length != right->prototype.length ||
      left->varargs != right->varargs ||
      left->is_const_member != right->is_const_member ||
      left->is_volatile_member != right->is_volatile_member ||
      left->ref_qualifier != right->ref_qualifier) {
    return false;
  }
  size_t first_parameter = 0;
  if (left->prototype.length != 0) {
    Symbol* left_first = left->prototype.value.p[0];
    Symbol* right_first = right->prototype.value.p[0];
    bool left_has_this =
        left_first != NULL && StringEqual(&left_first->name, "this");
    bool right_has_this =
        right_first != NULL && StringEqual(&right_first->name, "this");
    if (left_has_this != right_has_this) {
      return false;
    }
    if (left_has_this) {
      first_parameter = 1;
    }
  }
  for (size_t i = first_parameter; i < left->prototype.length; i++) {
    Symbol* left_arg = left->prototype.value.p[i];
    Symbol* right_arg = right->prototype.value.p[i];
    if (left_arg == NULL || right_arg == NULL ||
        !OverloadParameterTypesEqual(left_arg->type, right_arg->type)) {
      return false;
    }
  }
  return true;
}

// Structural equality of types for overload and redeclaration matching;
// stricter than TypeEqual for arrays and class types with template arguments.
static bool OverloadTypesEqual(TypeRecord* left, TypeRecord* right) {
  if (left == NULL || right == NULL || left->declarator != right->declarator) {
    return false;
  }
  // Complex arithmetic types use a synthetic struct for storage, but their
  // identity is the element type rather than the per-declaration layout node.
  if (TypeIsComplex(left) || TypeIsComplex(right)) {
    return TypeEqual(left, right);
  }
  if (TypeIsStructOrUnion(left) || TypeIsStructOrUnion(right)) {
    if (!TypeIsStructOrUnion(left) || !TypeIsStructOrUnion(right) ||
        left->type != right->type ||
        GetQualifiers(left) != GetQualifiers(right)) {
      return false;
    }
    if (left->template_origin != NULL || right->template_origin != NULL) {
      if (left->template_origin == right->template_origin &&
          TypeTemplateArgumentVectorEqual(left->template_arguments,
                                          right->template_arguments)) {
        return true;
      }
      return left->info.struct_info == right->info.struct_info &&
             TypeTemplateArgumentVectorEqual(left->template_arguments,
                                             right->template_arguments);
    }
    return left->info.struct_info == right->info.struct_info;
  }
  switch (left->declarator) {
    case kDeclArray:
      return TypeArrayBoundsEqual(&left->info.array, &right->info.array) &&
             OverloadTypesEqual(left->next, right->next);
    case kDeclVector:
      return left->info.array.size.fixed == right->info.array.size.fixed &&
             OverloadTypesEqual(left->next, right->next);
    case kDeclPointer:
    case kDeclReference:
    case kDeclRValueReference:
      return OverloadTypesEqual(left->next, right->next);
    case kDeclMemberPointer:
      return TypeEqual(left, right);
    case kDeclFunction:
      return OverloadTypesEqual(left->next, right->next) &&
             OverloadFunctionPrototypesEqual(&left->info.function,
                                             &right->info.function);
    case kDeclPrimitive:
      return TypeEqual(left, right);
  }
  return false;
}

// Returns true when both types are functions and OverloadTypesEqual holds.
static bool OverloadFunctionTypesEqual(TypeRecord* left, TypeRecord* right) {
  return TypeIsFunction(left) && TypeIsFunction(right) &&
         OverloadTypesEqual(left, right);
}

// `extern const char name[17]; const char name[] = "...";` completes the
// unknown bound.  The definition is still an incomplete array at the
// redeclaration check (the string length is applied later), so compare element
// types and copy the known bound onto the incomplete array.
static bool ArrayHasUnknownBound(TypeRecord* array) {
  return array != NULL && array->declarator == kDeclArray &&
         !array->info.array.is_vla && !array->info.array.is_dependent_bound &&
         array->info.array.size.fixed <= 0;
}

// Returns true when one array type completes the other's unknown or flexible
// bound with the same element type, copying the known size onto incomplete.
static bool IncompleteArrayRedeclarationMatches(TypeRecord* left,
                                               TypeRecord* right) {
  if (left == NULL || right == NULL || left->declarator != kDeclArray ||
      right->declarator != kDeclArray) {
    return false;
  }
  TypeRecord* incomplete = NULL;
  TypeRecord* complete = NULL;
  if (left->info.array.is_flexible && !right->info.array.is_flexible) {
    incomplete = left;
    complete = right;
  } else if (right->info.array.is_flexible && !left->info.array.is_flexible) {
    incomplete = right;
    complete = left;
  } else if (ArrayHasUnknownBound(left) && !ArrayHasUnknownBound(right)) {
    incomplete = left;
    complete = right;
  } else if (ArrayHasUnknownBound(right) && !ArrayHasUnknownBound(left)) {
    incomplete = right;
    complete = left;
  } else {
    return false;
  }
  if (!TypeEqual(incomplete->next, complete->next)) {
    return false;
  }
  // `static const T a[];` stores an unknown bound as a non-flexible size of 0.
  // The out-of-line `const T C::a[] = {...}` is the flexible definition that
  // the initializer completes. Copying 0 onto it rejects every element.
  if (complete->info.array.size.fixed <= 0) {
    complete->info.array.is_flexible = true;
    return true;
  }
  incomplete->info.array.is_flexible = false;
  incomplete->info.array.size.fixed = complete->info.array.size.fixed;
  TypeRecordCalculateSize(incomplete);
  return true;
}

// Returns true when `left` and `right` denote the same type for a
// redeclaration, using overload rules for functions and flexible arrays.
bool RedeclarationTypesEqual(TypeRecord* left, TypeRecord* right) {
  if (TypeIsFunction(left) || TypeIsFunction(right)) {
    return OverloadFunctionTypesEqual(left, right);
  }
  if (IncompleteArrayRedeclarationMatches(left, right)) {
    return true;
  }
  return TypeEqual(left, right);
}

// `Big` inside the class template and `Big<N>` on the out-of-line definition
// are the same type when each argument is that template's own parameter.
static bool TemplateArgumentIsOwnParameter(TemplateArgument* arg,
                                          TemplateParameter* param) {
  if (arg == NULL || param == NULL || arg->kind != param->kind) {
    return false;
  }
  if (param->kind == kTemplateParameterType) {
    return arg->type != NULL &&
           arg->type->template_parameter_index == param->index &&
           arg->type->dependent_member_name == NULL;
  }
  return arg->template_parameter_index == param->index &&
         arg->dependent_expr == NULL;
}

// Returns true when every template argument of `type` is the corresponding
// template parameter of class template `str` (or the argument list is empty).
static bool TemplateArgumentsDenoteClassTemplate(TypeRecord* type,
                                                Struct* str) {
  if (type == NULL || str == NULL || !str->is_template) {
    return false;
  }
  Vector* args = type->template_arguments;
  if (args == NULL || args->length == 0) {
    return true;
  }
  if (args->length != str->template_parameters.length) {
    return false;
  }
  for (size_t i = 0; i < args->length; i++) {
    if (!TemplateArgumentIsOwnParameter(args->value.p[i],
                                       str->template_parameters.value.p[i])) {
      return false;
    }
  }
  return true;
}

// Returns true when two class types are the same template instance up to
// restrict, with template arguments that each name only that template's
// parameters.
static bool ClassTemplateIdentityTypesEqual(TypeRecord* left,
                                            TypeRecord* right) {
  if (left == NULL || right == NULL || !TypeIsStructOrUnion(left) ||
      !TypeIsStructOrUnion(right) || left->type != right->type ||
      (GetQualifiers(left) & ~kQualRestrict) !=
          (GetQualifiers(right) & ~kQualRestrict)) {
    return false;
  }
  Struct* str = left->info.struct_info;
  if (str == NULL || str != right->info.struct_info) {
    return false;
  }
  return TemplateArgumentsDenoteClassTemplate(left, str) &&
         TemplateArgumentsDenoteClassTemplate(right, str);
}

// Out-of-line member definitions of a class template often name a dependent
// typedef (`typename Class<T>::size_type`) whose in-class counterpart was
// recorded as a plain integer while the alias was still dependent.  Those
// declarations designate the same function.
static TypeRecord* PeelTypeForOutOfLineTypedefMatch(TypeRecord* type) {
  while (type != NULL) {
    if (TypeIsPointer(type) || TypeIsReference(type)) {
      type = type->next;
      continue;
    }
    if (GetQualifiers(type) & kQualConst) {
      TypeRecord* inner = type->next;
      if (inner == NULL) {
        break;
      }
      type = inner;
      continue;
    }
    break;
  }
  return type;
}

// Returns true when `dependent`'s dependent member name resolves to a class
// typedef that matches `decl` after peeling references and const.
static bool OutOfLineDependentMemberMatchesInClassTypedef(
    TypeParser* parser, TypeRecord* decl, TypeRecord* dependent) {
  if (parser == NULL || parser->cxx_member_owner == NULL || decl == NULL ||
      dependent == NULL || dependent->dependent_member_name == NULL) {
    return false;
  }
  StructMember* member = FindStructMember(parser->cxx_member_owner,
                                          dependent->dependent_member_name);
  if (member == NULL || member->symbol == NULL ||
      !StorageIs(member->symbol->storage, STO(typedef))) {
    return false;
  }
  decl = PeelTypeForOutOfLineTypedefMatch(decl);
  dependent = PeelTypeForOutOfLineTypedefMatch(dependent);
  TypeRecord* typedef_type = member->symbol->type;
  return OverloadTypesEqual(decl, typedef_type) ||
         TypeEqual(decl, typedef_type) ||
         OverloadTypesEqual(decl, dependent) || TypeEqual(decl, dependent);
}

// Compares in-class and out-of-line member types, including class-template
// identity and dependent typedef aliases; `parameter` selects overload-style
// parameter comparison and `allow_integral` permits matching integral types.
static bool OutOfLineMemberTypeEqual(TypeParser* parser, TypeRecord* left,
                                     TypeRecord* right, bool parameter,
                                     bool allow_integral) {
  if (parameter ? OverloadParameterTypesEqual(left, right)
                : OverloadTypesEqual(left, right)) {
    return true;
  }
  if (ClassTemplateIdentityTypesEqual(left, right)) {
    return true;
  }
  if (OutOfLineDependentMemberMatchesInClassTypedef(parser, left, right) ||
      OutOfLineDependentMemberMatchesInClassTypedef(parser, right, left)) {
    return true;
  }
  // `Iter&` in the class and `Iter<T>&` on the out-of-line definition name
  // the same object.  Compare the pointee; do not treat two different
  // integer pointees as the same type.
  if (left != NULL && right != NULL && left->declarator == right->declarator &&
      (TypeIsReference(left) ||
       (TypeIsPointer(left) && (GetQualifiers(left) & ~kQualRestrict) ==
                                   (GetQualifiers(right) & ~kQualRestrict)))) {
    return OutOfLineMemberTypeEqual(parser, left->next, right->next, parameter,
                                    /*allow_integral=*/false);
  }
  return allow_integral && TypeIsIntegral(left) && TypeIsIntegral(right);
}

// OutOfLineMemberTypeEqual with parameter comparison and integral widening.
static bool OutOfLineMemberParameterTypesEqual(TypeParser* parser,
                                               TypeRecord* left,
                                               TypeRecord* right) {
  return OutOfLineMemberTypeEqual(parser, left, right, /*parameter=*/true,
                                  /*allow_integral=*/true);
}

// Returns true when two member function types match for an out-of-line
// definition, including return type and prototype under member rules.
static bool OutOfLineMemberFunctionTypesEqual(TypeParser* parser,
                                              TypeRecord* left,
                                              TypeRecord* right) {
  if (!TypeIsFunction(left) || !TypeIsFunction(right)) {
    return false;
  }
  if (!OutOfLineMemberTypeEqual(parser, left->next, right->next,
                                /*parameter=*/false,
                                /*allow_integral=*/true)) {
    return false;
  }
  FunctionInfo* left_fn = &left->info.function;
  FunctionInfo* right_fn = &right->info.function;
  if (left_fn->prototype.length != right_fn->prototype.length ||
      left_fn->varargs != right_fn->varargs ||
      left_fn->is_const_member != right_fn->is_const_member ||
      left_fn->is_volatile_member != right_fn->is_volatile_member ||
      left_fn->ref_qualifier != right_fn->ref_qualifier) {
    return false;
  }
  size_t first_parameter = 0;
  if (left_fn->prototype.length != 0) {
    Symbol* left_first = left_fn->prototype.value.p[0];
    Symbol* right_first = right_fn->prototype.value.p[0];
    bool left_has_this =
        left_first != NULL && StringEqual(&left_first->name, "this");
    bool right_has_this =
        right_first != NULL && StringEqual(&right_first->name, "this");
    if (left_has_this != right_has_this) {
      return false;
    }
    if (left_has_this) {
      first_parameter = 1;
    }
  }
  for (size_t i = first_parameter; i < left_fn->prototype.length; i++) {
    Symbol* left_arg = left_fn->prototype.value.p[i];
    Symbol* right_arg = right_fn->prototype.value.p[i];
    if (left_arg == NULL || right_arg == NULL ||
        !OutOfLineMemberParameterTypesEqual(parser, left_arg->type,
                                          right_arg->type)) {
      return false;
    }
  }
  return true;
}

// Returns true when `left` and `right` match for a member definition,
// including out-of-line class-template cases when `parser` carries member
// context.
bool MemberDefinitionTypesEqual(TypeParser* parser, TypeRecord* left,
                                TypeRecord* right) {
  if (RedeclarationTypesEqual(left, right)) {
    return true;
  }
  return parser != NULL && parser->cxx_member_definition != NULL &&
         OutOfLineMemberFunctionTypesEqual(parser, left, right);
}

// Returns true when associated constraints on a template overload and an
// incoming function type with the same signature are equivalent, or when
// neither side is constrained.
static bool SameSignatureTemplateConstraintsAreEquivalent(Symbol* overload,
                                                          TypeRecord* type) {
  if (overload == NULL || overload->type == NULL || type == NULL ||
      !overload->flags.is_template || !TypeIsFunction(overload->type) ||
      !TypeIsFunction(type) || type->info.function.template_parameter_count <= 0) {
    return true;
  }
  bool overload_constrained =
      ConceptsFunctionTemplateHasAssociatedConstraint(overload);
  bool type_constrained = type->info.function.associated_constraint != NULL;
  if (!overload_constrained && !type_constrained) {
    return true;
  }
  Symbol scratch = {0};
  scratch.type = type;
  return ConceptsFunctionTemplateConstraintsEquivalent(overload, &scratch);
}

// Returns the first overload in the chain from `first` whose function type and
// template arguments match `type`, respecting module compatibility with
// `incoming`, or NULL if none match.
Symbol* FindMatchingOverload(Symbol* first, TypeRecord* type,
                             Symbol* incoming) {
  for (Symbol* overload = first; overload != NULL;
       overload = overload->overload_next) {
    if (incoming != NULL &&
        !SymbolCompatibleModuleRedeclaration(overload, incoming)) {
      continue;
    }
    bool overload_is_template = overload->flags.is_template;
    bool type_is_template =
        TypeIsFunction(type) && type->info.function.template_parameter_count > 0;
    if (overload_is_template != type_is_template) {
      continue;
    }
    if (OverloadFunctionTypesEqual(overload->type, type) &&
        SameSignatureTemplateConstraintsAreEquivalent(overload, type)) {
      // `template <> bool IsDigit<10>(char)` and `IsDigit<16>` share a
      // function type.  They are different explicit specializations.
      if (TypeIsFunction(overload->type) && TypeIsFunction(type) &&
          (overload->type->template_arguments != NULL ||
           type->template_arguments != NULL) &&
          !TemplateArgumentVectorEqual(overload->type->template_arguments,
                                      type->template_arguments)) {
        continue;
      }
      return overload;
    }
  }
  return NULL;
}

// Assigns the C++ mangled assembly name on `overload` for emission.
static void SetOverloadAsmName(Symbol* first, Symbol* overload) {
  (void)first;
  SymbolSetCXXMangledAsmName(overload);
}

// Links `overload` at the end of the overload chain headed by `first`, copies
// namespace from the primary, and refreshes mangled names for the set.
void AppendOverload(Symbol* first, Symbol* overload) {
  Symbol* tail = first;
  while (tail->overload_next != NULL) {
    tail = tail->overload_next;
  }
  overload->namespace_ = first->namespace_;
  tail->overload_next = overload;
  first->flags.is_overloaded = true;
  overload->flags.is_overloaded = true;
  SetOverloadAsmName(first, first);
  SetOverloadAsmName(first, overload);
}

// Returns true when `symbol` is or declares a function template by flag or
// template parameter metadata on its type.
static bool SymbolLooksLikeFunctionTemplate(Symbol* symbol) {
  return symbol != NULL && symbol->type != NULL &&
         TypeIsFunction(symbol->type) &&
         (symbol->flags.is_template ||
          symbol->type->info.function.template_parameter_count > 0 ||
          symbol->type->info.function.template_parameters.length > 0);
}

// Two function templates whose signatures are identical are still distinct
// templates if their template parameter lists differ.  That is how the
// pre-C++20 constraint idiom works: the condition lives in the type of a
// defaulted non-type parameter, as in
// `template <class T, typename enable_if<C<T>, int>::type = 0> void f(T);`,
// so the two overloads differ only in that type.
static bool TemplateParameterListsDiffer(Vector* left, Vector* right) {
  if (left == NULL || right == NULL) {
    return false;
  }
  if (left->length != right->length) {
    return true;
  }
  for (size_t i = 0; i < left->length; i++) {
    TemplateParameter* left_param = left->value.p[i];
    TemplateParameter* right_param = right->value.p[i];
    if (left_param == NULL || right_param == NULL) {
      continue;
    }
    if (left_param->kind != right_param->kind ||
        left_param->is_parameter_pack != right_param->is_parameter_pack) {
      return true;
    }
    if (left_param->kind == kTemplateParameterNonType &&
        !TypeEqual(left_param->type, right_param->type)) {
      return true;
    }
    // Defaulted type parameters encode SFINAE (`typename = enable_if_t<C>`).
    if (left_param->kind == kTemplateParameterType &&
        ((left_param->default_type != NULL) !=
             (right_param->default_type != NULL) ||
         (left_param->default_type != NULL &&
          !TypeEqual(left_param->default_type, right_param->default_type)))) {
      return true;
    }
  }
  return false;
}

// The parameters of the declaration being parsed have not reached its function
// type yet: MoveCurrentTemplateParametersToFunction hands them over later, so
// reading the type here yields an empty list and would make every redeclaration
// of a function template look like a new overload.  Until then they live in
// `syntax->current_template_parameters`.
Vector* PendingTemplateParameterList(Syntax* syntax, Symbol* symbol) {
  if (symbol != NULL && symbol->type != NULL && TypeIsFunction(symbol->type) &&
      symbol->type->info.function.template_parameters.length > 0) {
    return &symbol->type->info.function.template_parameters;
  }
  return syntax == NULL ? NULL : syntax->current_template_parameters;
}

// Returns true when the template parameter list on `candidate` differs from
// `overload_params` in a way that makes them distinct templates.
static bool FunctionTemplateParameterListsDiffer(Symbol* candidate,
                                                 Vector* overload_params) {
  if (candidate == NULL || candidate->type == NULL ||
      overload_params == NULL || overload_params->length == 0) {
    return false;
  }
  return TemplateParameterListsDiffer(
      &candidate->type->info.function.template_parameters, overload_params);
}

// Clones `constraint` and appends it to `target`, forming a conjunction when
// `target` already holds a constraint. Does nothing when `constraint` is NULL.
static void AppendClonedConstraint(ConstraintExpr** target,
                                   ConstraintExpr* constraint) {
  if (target == NULL || constraint == NULL) {
    return;
  }
  ConstraintExpr* clone = ConceptsCloneConstraint(constraint);
  if (*target == NULL) {
    *target = clone;
  } else {
    *target = NewConjunctionConstraint(*target, clone, clone->location);
  }
}

// Builds the combined associated constraint for a function template from the
// function's constraint, each template parameter's constraint, and
// `trailing_constraint`. Caller owns the returned expression tree.
ConstraintExpr* PendingFunctionTemplateConstraint(
    Symbol* symbol, Vector* parameters, ConstraintExpr* trailing_constraint) {
  ConstraintExpr* result = NULL;
  if (symbol != NULL && symbol->type != NULL && TypeIsFunction(symbol->type)) {
    AppendClonedConstraint(
        &result, symbol->type->info.function.associated_constraint);
  }
  if (parameters != NULL) {
    for (size_t i = 0; i < parameters->length; i++) {
      TemplateParameter* param = parameters->value.p[i];
      if (param != NULL) {
        AppendClonedConstraint(&result, param->associated_constraint);
      }
    }
  }
  AppendClonedConstraint(&result, trailing_constraint);
  return result;
}

// When `overload` is a constrained function template with the same signature
// as an existing overload but different constraints or template parameters,
// appends it as a distinct overload and returns true; otherwise returns false
// without linking.
bool TryAppendSameSignatureConstrainedTemplateOverload(
    Syntax* syntax, Symbol* first, Symbol* overload,
    ConstraintExpr* pending_constraint) {
  if (first == NULL || overload == NULL ||
      !SymbolLooksLikeFunctionTemplate(overload)) {
    return false;
  }
  Vector* overload_params = PendingTemplateParameterList(syntax, overload);
  ConstraintExpr* saved_constraint =
      overload->type->info.function.associated_constraint;
  if (saved_constraint == NULL && pending_constraint != NULL) {
    overload->type->info.function.associated_constraint = pending_constraint;
  }
  for (Symbol* candidate = first; candidate != NULL;
       candidate = candidate->overload_next) {
    if (!SymbolLooksLikeFunctionTemplate(candidate) ||
        !OverloadFunctionTypesEqual(candidate->type, overload->type)) {
      continue;
    }
    bool candidate_constrained =
        ConceptsFunctionTemplateHasAssociatedConstraint(candidate);
    bool overload_constrained =
        ConceptsFunctionTemplateHasAssociatedConstraint(overload);
    if (((candidate_constrained || overload_constrained) &&
         !ConceptsFunctionTemplateConstraintsEquivalent(candidate, overload)) ||
        FunctionTemplateParameterListsDiffer(candidate, overload_params)) {
      overload->type->info.function.associated_constraint = saved_constraint;
      AppendOverload(first, overload);
      return true;
    }
  }
  overload->type->info.function.associated_constraint = saved_constraint;
  return false;
}

// Follows a chain of using-declaration aliases on `symbol` until a non-alias
// or an overloaded symbol is reached, with a fixed depth limit.
Symbol* FollowAlias(Symbol* symbol) {
  int depth = 0;
  while (symbol != NULL && symbol->flags.is_using_alias &&
         symbol->alias_target != NULL && symbol->overload_next == NULL &&
         depth < 64) {
    symbol = symbol->alias_target;
    depth++;
  }
  return symbol;
}
