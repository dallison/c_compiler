//
//  syntax_external_decl.c
//  c_compiler
//
//  External (namespace-scope) declarations: the top-level declaration
//  parser, linkage specifications and out-of-class special members.
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

// How the external declarator loop continues after a step.
typedef enum {
  kExternalDeclaratorProceed,  // Keep processing the current declarator.
  kExternalDeclaratorNext,     // Parse the next declarator in the list.
  kExternalDeclaratorStop,     // The declarator list is finished.
} ExternalDeclaratorFlow;

// One declarator of a namespace-scope declaration, as it is matched against
// earlier declarations of the same name.
typedef struct {
  Symbol* sym;      // The symbol this declarator declares.
  // A previous declaration of the same entity, if found.  The declarator is
  // merged into it and `sym` is discarded.
  Symbol* old_sym;
  // `sym` was added to an existing overload set, so it must not also be
  // inserted into the symbol table as a new name.
  bool overload_was_appended;
  // True once we discover this declarator is an out-of-class definition of a
  // static data member that was already defined in-class (a C++17 inline /
  // constexpr static data member).  Such an out-of-class `T C::m;` is a
  // deprecated non-defining redeclaration and must not emit a second symbol.
  bool redundant_static_member_redefinition;
} ExternalDeclarator;

// Resets the parser after a declarator that produced no declaration and
// reports whether a comma introduces another declarator.
static ExternalDeclaratorFlow FinishSkippedExternalDeclarator(
    TypeParser* parser) {
  TypeParserReset(parser);
  if (LexMatch(parser->syntax->lex, TOK(comma))) {
    return kExternalDeclaratorNext;
  }
  return kExternalDeclaratorStop;
}

// Handles declarators that are not ordinary declarations: the declarators of
// an explicit instantiation, and variable-template specializations
// (`template<...> T name<pattern> = ...`).  Returns Proceed for anything else.
static ExternalDeclaratorFlow ParseSpecialExternalDeclarator(
    TypeParser* parser, Symbol* sym, Storage storage) {
  Syntax* syntax = parser->syntax;
  if (syntax->parsing_explicit_instantiation) {
    if (sym != NULL && TypeIsFunction(sym->type)) {
      DefineExplicitFunctionInstantiation(syntax, parser, sym);
    }
    SymbolDelete(sym);
    return FinishSkippedExternalDeclarator(parser);
  }
  // A variable-template specialization: `template<...> T name<pattern> = ...`.
  // The declarator carried a template-argument list and an existing variable
  // template of the same name is in scope.  Register it as a partial/explicit
  // specialization rather than treating the reused name as a redefinition.
  if (CompilerIsCXX() && sym != NULL && !TypeIsFunction(sym->type) &&
      parser->declarator_template_arguments != NULL &&
      (syntax->parsing_template_declaration ||
       syntax->parsing_template_specialization)) {
    Symbol* primary = SyntaxFindSymbol(syntax, &sym->name);
    if (primary != NULL && SymbolExtraOf(primary)->variable_template != NULL) {
      ASTNode* initializer = NULL;
      if (LexMatch(syntax->lex, TOK(equal))) {
        syntax->init_storage = storage;
        initializer = SyntaxParseInitializer(syntax, sym, storage);
      } else {
        initializer = ParseCXXDirectInitializer(syntax, sym, false);
        if (initializer == NULL && CompilerIsCXX() &&
            LexMatch(syntax->lex, TOK(lbrace))) {
          initializer =
              MarkDirectListInitializer(ParseBracedInitializer(syntax));
        }
        if (initializer == NULL) {
          SyntaxError(syntax,
                      "variable template specialization requires an initializer");
        }
      }
      AddVariableTemplatePartialSpecialization(
          parser, primary, parser->declarator_template_arguments, initializer,
          sym->type);
      SymbolDelete(sym);
      return FinishSkippedExternalDeclarator(parser);
    }
  }
  return kExternalDeclaratorProceed;
}

// Applies what the declaration context implies to a new declarator's symbol:
// C linkage, export, internal linkage in an unnamed namespace (which also
// changes `*storage` for the rest of the list), module attachment, the const of
// constexpr objects, template-ness of function templates, and `explicit`.
// Returns false when the declarator was a deduction guide, which is consumed.
static bool PrepareExternalDeclaratorSymbol(TypeParser* parser, Symbol* sym,
                                            Storage* storage,
                                            bool is_explicit) {
  Syntax* syntax = parser->syntax;
  // A declaration appearing inside an `extern "C"` linkage specification
  // has C language linkage, so its name is not mangled.
  if (syntax->c_linkage) {
    sym->flags.is_c_linkage = true;
  }
  if (syntax->export_depth > 0) {
    sym->flags.is_exported = true;
  }
  if (CXXDeclarationInUnnamedNamespace(syntax, parser, sym, *storage)) {
    // An `extern` declaration stays a non-defining declaration.
    if (*storage == STO(implicit)) {
      *storage = STO(static);
    }
    sym->storage = STO(static);
  }
  SymbolAttachModuleContext(sym, *storage);
  ApplyExplicitCXXLinkageAttachment(syntax, sym, *storage);
  // `constexpr` on an object implies `const` on its type.  Apply it before
  // matching against any previous declaration so that an out-of-class
  // definition (`constexpr T C::x;`) compares equal to the in-class
  // `static constexpr` member, whose type is already const-qualified.
  if (!TypeIsFunction(sym->type) && !TypeIsReference(sym->type) &&
      parser->is_constexpr) {
    SymbolSetType(sym, TypeRecordAddQualifiers(sym->type, kQualConst));
  }
  if (TypeIsFunction(sym->type) &&
      FunctionExtraOf(&sym->type->info.function)->template_parameters.length >
          0 &&
      !sym->flags.is_template) {
    sym->flags.is_template = true;
    FunctionMutableExtra(&sym->type->info.function)->template_parameter_count =
        (int)FunctionExtraOf(&sym->type->info.function)
            ->template_parameters.length;
    FunctionMutableExtra(&sym->type->info.function)->template_parameter_base =
        0;
  }
  if (syntax->parsing_template_declaration && TypeIsFunction(sym->type)) {
    sym->flags.is_template = true;
    FunctionMutableExtra(&sym->type->info.function)->template_parameter_count =
        CurrentTemplateParameterListLength(syntax);
    FunctionMutableExtra(&sym->type->info.function)->template_parameter_base =
        CurrentTemplateParameterBase(syntax);
  }
  ValidateCXX26MainDeclaration(syntax, sym, *storage);
  if (TypeIsFunction(sym->type)) {
    sym->type->info.function.is_explicit = is_explicit;
    if (syntax->pending_explicit_condition != NULL) {
      FunctionMutableExtra(&sym->type->info.function)->explicit_condition =
          syntax->pending_explicit_condition;
      syntax->pending_explicit_condition = NULL;
    }
  }
  if (TryParseCXXDeductionGuide(syntax, sym)) {
    return false;
  }
  if (is_explicit) {
    SyntaxError(syntax,
                "explicit is only supported on deduction guides");
  }
  return true;
}

// Finds the earlier declaration `sym` redeclares: the class member named by
// an out-of-class definition, or a namespace-scope symbol of the same name
// that the module rules allow it to merge with.  May set
// parser->cxx_member_definition for a member function of a known class.
static Symbol* FindPreviousExternalDeclaration(TypeParser* parser,
                                               Symbol* sym) {
  Syntax* syntax = parser->syntax;
  Symbol* old_sym = parser->cxx_member_definition != NULL &&
                            !syntax->parsing_template_specialization
                        ? parser->cxx_member_definition->symbol
                        : NULL;
  if (parser->cxx_member_definition == NULL && sym != NULL &&
      TypeIsFunction(sym->type) &&
      sym->type->info.function.cxx_member_owner != NULL) {
    StructMember* member_head = FindStructMember(
        sym->type->info.function.cxx_member_owner, &sym->name);
    if (member_head != NULL) {
      parser->cxx_member_definition = member_head;
      if (!syntax->parsing_template_specialization) {
        old_sym = member_head->symbol;
      }
    }
  }
  if (parser->cxx_member_definition == NULL) {
    Symbol* raw_old = NULL;
    if (parser->cxx_qualified_definition_namespace != NULL) {
      NamespaceInlineSymbolLookup found = NamespaceResolveSymbolInInlineSet(
          parser->cxx_qualified_definition_namespace, &sym->name);
      if (found.status == kInlineLookupUnique) {
        raw_old = found.symbol;
      }
    }
    if (raw_old == NULL) {
      raw_old = FindFileScopeSymbol(syntax, &sym->name);
    }
    if (raw_old != NULL && TypeIsFunction(sym->type) &&
        sym->type->info.function.cxx_member_owner != NULL &&
        TypeIsFunction(raw_old->type) &&
        raw_old->type->info.function.cxx_member_owner == NULL) {
      raw_old = NULL;
    }
    old_sym = SymbolFindModuleCompatibleOverload(raw_old, sym);
    if (raw_old != NULL && old_sym == NULL) {
      SyntaxError(syntax,
                  "Declaration of '%s' conflicts with an unrelated module "
                  "entity",
                  sym->name.value);
    }
  }
  return old_sym;
}

// An explicit specialization of a member function (`template<> R C<T>::f()`):
// finds the matching specialization among the member's overloads, or adds a
// new specialization member, and makes it the definition target.
static void MatchExplicitMemberSpecialization(TypeParser* parser,
                                              ExternalDeclarator* d) {
  Syntax* syntax = parser->syntax;
  Symbol* sym = d->sym;
  if (TypeIsFunction(sym->type) &&
      parser->declarator_template_arguments != NULL &&
      sym->type->template_arguments == NULL) {
    sym->type->template_arguments = TemplateArgumentVectorCopy(
        parser->declarator_template_arguments);
    parser->declarator_template_arguments = NULL;
  }
  MarkMemberFunctionTemplateSpecialization(
      syntax, sym, parser->cxx_member_definition);
  StructMember* matching_specialization =
      FindMemberFunctionTemplateSpecialization(
          parser->cxx_member_definition, sym->type);
  if (matching_specialization != NULL) {
    parser->cxx_member_definition = matching_specialization;
    d->old_sym = matching_specialization->symbol;
  } else if (TypeIsFunction(sym->type) &&
             sym->type->info.function.template_origin != NULL) {
    StructMember* specialization = NewStructMember(sym);
    specialization->is_member_function = true;
    specialization->is_static = parser->cxx_member_definition->is_static;
    specialization->access = parser->cxx_member_definition->access;
    AppendMemberFunctionSpecialization(
        syntax, parser->cxx_member_definition, specialization);
    parser->cxx_member_definition = specialization;
    d->old_sym = NULL;
    d->overload_was_appended = true;
  } else if (TypeIsFunction(sym->type) &&
             parser->cxx_member_owner != NULL) {
    // Explicit specialization of a static member of a class template
    // specialization, e.g. `template<> Ret Traits<T>::fn() { ... }`.
    StructMember* member_head =
        FindStructMember(parser->cxx_member_owner, &sym->name);
    if (member_head == NULL) {
      member_head = parser->cxx_member_definition;
    }
    if (member_head != NULL) {
      StructMember* matching =
          FindStructMemberOverload(member_head, sym->type);
      if (matching != NULL) {
        parser->cxx_member_definition = matching;
        d->old_sym = matching->symbol;
      } else {
        StructMember* specialization = NewStructMember(sym);
        specialization->is_member_function = true;
        specialization->is_static = member_head->is_static;
        specialization->access = member_head->access;
        AppendStructMemberOverload(parser, parser->cxx_member_owner,
                                   member_head, specialization);
        parser->cxx_member_definition = specialization;
        d->old_sym = NULL;
        d->overload_was_appended = true;
      }
    }
  }
}

// `sym` and the previous declaration can be overloads: either finds the
// overload `sym` redeclares or appends `sym` to the overload set.  The
// comparison uses the pending template constraint so that functions differing
// only in their constraints stay distinct.
static void AddExternalFunctionOverload(TypeParser* parser,
                                        ExternalDeclarator* d) {
  Syntax* syntax = parser->syntax;
  Symbol* sym = d->sym;
  ConstraintExpr* saved_constraint =
      FunctionExtraOf(&sym->type->info.function)->associated_constraint;
  ConstraintExpr* comparison_constraint =
      PendingFunctionTemplateConstraint(
          sym, PendingTemplateParameterList(syntax, sym),
          syntax->current_template_requires_clause);
  if (comparison_constraint != NULL) {
    FunctionMutableExtra(&sym->type->info.function)->associated_constraint =
        comparison_constraint;
  }
  if (TryAppendSameSignatureConstrainedTemplateOverload(
          syntax, d->old_sym, sym, NULL)) {
    d->old_sym = NULL;
    d->overload_was_appended = true;
  } else {
    Symbol* matching_overload = FindMatchingOverload(d->old_sym, sym->type, sym);
    if (matching_overload != NULL) {
      d->old_sym = matching_overload;
    } else {
      AppendOverload(d->old_sym, sym);
      d->old_sym = NULL;
      d->overload_was_appended = true;
    }
  }
  FunctionSetAssociatedConstraint(&sym->type->info.function, saved_constraint);
  ConstraintExprDelete(comparison_constraint);
}

// The qualified-name lookup that decided whether this definition gets
// an implicit `this` saw only the first overload.  `AddData` is both a
// non-static member template and a static one; a definition of the
// static overload was therefore given `this` and matched neither
// signature.  Drop or add `this` and search the overload set again.
static StructMember* MatchMemberDefinitionTogglingThis(TypeParser* parser,
                                                       Symbol* sym) {
  StructMember* matching_member = NULL;
  bool had_this = FunctionHasImplicitThisParameter(sym->type);
  if (had_this) {
    TypeRecordRemoveImplicitThisParameter(sym->type);
    StructMember* static_match = FindStructMemberOverload(
        parser->cxx_member_definition, sym->type);
    if (static_match != NULL && static_match->is_static) {
      matching_member = static_match;
    } else {
      Struct* owner = sym->type->info.function.cxx_member_owner;
      if (owner == NULL) {
        owner = parser->cxx_member_owner;
      }
      sym->type->info.function.cxx_member_owner = NULL;
      TypeRecordAddCXXThisParameter(sym->type, owner, sym->location);
    }
  } else {
    Struct* owner = sym->type->info.function.cxx_member_owner;
    if (owner == NULL) {
      owner = parser->cxx_member_owner;
    }
    sym->type->info.function.cxx_member_owner = NULL;
    TypeRecordAddCXXThisParameter(sym->type, owner, sym->location);
    StructMember* instance_match = FindStructMemberOverload(
        parser->cxx_member_definition, sym->type);
    if (instance_match != NULL && !instance_match->is_static) {
      matching_member = instance_match;
    } else if (FunctionHasImplicitThisParameter(sym->type)) {
      TypeRecordRemoveImplicitThisParameter(sym->type);
      sym->type->info.function.cxx_member_owner = owner;
    }
  }
  return matching_member;
}

// An out-of-class member definition: finds the member overload whose
// signature it defines and makes that member the previous declaration.
static void MatchOutOfLineMemberDefinition(TypeParser* parser,
                                           ExternalDeclarator* d) {
  Symbol* sym = d->sym;
  StructMember* matching_member = FindStructMemberOverload(
      parser->cxx_member_definition, sym->type);
  // A class template's out-of-line member carries the class's template
  // head, which FindStructMemberOverload reads as a member template.  An
  // exact signature still wins over the looser comparison below, which
  // cannot tell `f(const X&)` from `f(X&)`.
  for (StructMember* candidate = parser->cxx_member_definition;
       matching_member == NULL && candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->symbol != NULL && candidate->symbol->type != NULL &&
        TypeEqual(candidate->symbol->type, sym->type)) {
      matching_member = candidate;
    }
  }
  if (matching_member == NULL) {
    for (StructMember* candidate = parser->cxx_member_definition;
         candidate != NULL; candidate = candidate->overload_next) {
      if (candidate->symbol != NULL &&
          MemberDefinitionTypesEqual(
              parser, candidate->symbol->type, sym->type)) {
        matching_member = candidate;
        break;
      }
    }
  }
  if (matching_member == NULL && TypeIsFunction(sym->type) &&
      parser->cxx_member_owner != NULL) {
    matching_member = MatchMemberDefinitionTogglingThis(parser, sym);
  }
  if (matching_member != NULL) {
    parser->cxx_member_definition = matching_member;
    d->old_sym = matching_member->symbol;
  }
}

// Reports a redefinition of a symbol that is already defined, except for the
// cases that are not redefinitions: an `extern` redeclaration, a different
// explicit specialization of a member, and an out-of-line explicit
// specialization definition, which replaces a body attached from a header.
// May clear d->old_sym.
static void CheckExternalRedefinition(TypeParser* parser,
                                      ExternalDeclarator* d,
                                      Storage storage) {
  Syntax* syntax = parser->syntax;
  Symbol* sym = d->sym;
  Symbol* old_sym = d->old_sym;
  if (StorageIs(storage, STO(extern))) {
    // This might be a declaration, only if there is no initializer
    if (LexLookingAt(parser->lex, TOK(equal))) {
      // This is 'extern int foo = xxx', a definition
      String symbol_name;
      StringInit(&symbol_name, NULL);
      SymbolFunctionDiagnosticName(sym, &symbol_name);
      SyntaxError(syntax, "Duplicate definition of symbol %s",
                  symbol_name.value);
      StringDestruct(&symbol_name);
    }
    return;
  }
  if (!IsDefinition(parser, old_sym, storage)) {
    return;
  }
  bool distinct_member_specialization =
      syntax->parsing_template_specialization &&
      parser->cxx_member_definition != NULL &&
      TypeIsFunction(old_sym->type) && TypeIsFunction(sym->type) &&
      old_sym->type->template_arguments != NULL &&
      sym->type->template_arguments != NULL &&
      !TemplateArgumentVectorEqual(
          old_sym->type->template_arguments,
          sym->type->template_arguments);
  bool ool_explicit_specialization_definition =
      syntax->parsing_template_specialization &&
      parser->cxx_member_definition != NULL &&
      LexLookingAt(parser->lex, TOK(lbrace)) &&
      TypeIsFunction(old_sym->type) && TypeIsFunction(sym->type) &&
      old_sym->type->info.function.template_origin != NULL &&
      old_sym->type->template_arguments != NULL &&
      sym->type->template_arguments != NULL &&
      TemplateArgumentVectorEqual(
          old_sym->type->template_arguments,
          sym->type->template_arguments);
  if (distinct_member_specialization) {
    d->old_sym = NULL;
  } else if (ool_explicit_specialization_definition) {
    // A header reference can attach the primary template body to
    // the explicit specialization declaration.  The out-of-line
    // definition in a .cc file is authoritative.
    old_sym->flags.is_defined = false;
    old_sym->type->info.function.definition = false;
    if (old_sym->type->info.function.body != NULL) {
      ASTNodeDelete(old_sym->type->info.function.body);
      old_sym->type->info.function.body = NULL;
    }
  } else {
    // This is a declaration of a previously known definition.
    String symbol_name;
    StringInit(&symbol_name, NULL);
    SymbolFunctionDiagnosticName(sym, &symbol_name);
    SyntaxError(syntax, "Duplicate definition of symbol %s",
                symbol_name.value);
    StringDestruct(&symbol_name);
  }
}

// Merges a redeclaration into the previous declaration d->old_sym: checks for
// redefinition, marks the previous symbol defined or tentative, and checks
// that type and linkage agree before merging default arguments, contracts
// and constexpr-ness.
static void MergeWithPreviousExternalDeclaration(TypeParser* parser,
                                                 ExternalDeclarator* d,
                                                 Storage storage) {
  Syntax* syntax = parser->syntax;
  Symbol* sym = d->sym;
  // `extern Flag<T> name;` is parsed while `Flag` is only
  // forward-declared, so the symbol keeps a deferred template-id.
  // A later definition, after the class body, must re-instantiate
  // both sides or the redeclaration looks like a different type.
  MaterializeDeferredClassTemplateType(syntax, d->old_sym);
  MaterializeDeferredClassTemplateType(syntax, sym);
  // We have this symbol already.  If it's a declaration then it's
  // OK to declare (and define) it now.  If it's a definition then
  // this must be a declaration.
  if (d->old_sym->flags.is_defined) {
    CheckExternalRedefinition(parser, d, storage);
  } else {
    // Old sym is a declaration.
    if (IsDefinition(parser, sym,  storage)) {
      d->old_sym->flags.is_defined = true;
    } else if (!StorageIs(storage, STO(extern))) {
      d->old_sym->flags.is_tentative_decl = true;
    }
  }
  Symbol* old_sym = d->old_sym;
  if (!d->redundant_static_member_redefinition &&
      !MemberDefinitionTypesEqual(parser, sym->type, old_sym->type)) {
    String suffix;
    StringInit(&suffix, NULL);
    SymbolFunctionDiagnosticSuffix(sym, &suffix);
    SyntaxError(syntax, "Symbol %s redeclared with different type%s",
                sym->name.value, suffix.value);
    StringDestruct(&suffix);
    TypeErrorDetails(syntax->lex->current_token_location,
                     sym->type, old_sym->type);
    ReportNoteAtLocation(old_sym->location, "Previously declared here");
  } else {
    MergeCXXDefaultArguments(syntax, old_sym, sym);
    MergeCXXContractAssertions(syntax, old_sym, sym);
    CheckMatchingConstexprConsteval(syntax, old_sym->type, sym->type);
    // Symbol declaration is the same type as the definition, make sure
    // the linkage matches.
    Storage old_storage = old_sym->storage & ~STO(extern);
    Storage new_storage = sym->storage & ~STO(extern);

    if (old_storage != new_storage) {
      SyntaxError(syntax, "Symbol %s redeclared with different linkage",
                  sym->name.value);
    }
    sym->flags.is_defined = true;
  }
}

// The first declaration of a name: inserts it into the symbol table (unless
// it joined an overload set or defines a class member) and marks it defined
// or tentative.
static void RegisterNewExternalDeclaration(TypeParser* parser,
                                           ExternalDeclarator* d,
                                           Storage storage) {
  Syntax* syntax = parser->syntax;
  Symbol* sym = d->sym;
  if (!d->overload_was_appended) {
    if (parser->cxx_member_definition == NULL) {
      // This is the first declaration of this symbol, add to the symbol
      // table.
      bool inserted = InsertFileScopeSymbol(syntax, sym);
      if (!inserted) {
        SyntaxError(syntax, "Duplicate symbol %s",
                    sym->name.value);
      }
    }
  }
  if (IsDefinition(parser, sym, storage)) {
    sym->flags.is_defined = true;
  } else if (!StorageIs(storage, STO(extern))) {
    sym->flags.is_tentative_decl = true;
  }
}

// Matches a prepared declarator against earlier declarations of its name:
// explicit specializations, hidden class names, overload sets and
// out-of-class member definitions.  Then merges it into the previous
// declaration found, or registers it as a new one.
static void ResolveExternalDeclarator(TypeParser* parser,
                                      ExternalDeclarator* d,
                                      Storage storage) {
  Syntax* syntax = parser->syntax;
  Symbol* sym = d->sym;
  d->old_sym = FindPreviousExternalDeclaration(parser, sym);
  if (IsC23InferredAutoType(sym->type) && d->old_sym != NULL) {
    SyntaxError(syntax,
                "C23 inferred auto declaration cannot redeclare '%s'",
                sym->name.value);
  }
  if (CXXConstVariableHasInternalLinkage(syntax, parser, sym, d->old_sym)) {
    sym->storage = STO(static);
  }
  if (parser->cxx_member_definition != NULL &&
      syntax->parsing_template_specialization) {
    MatchExplicitMemberSpecialization(parser, d);
  } else {
    MarkFunctionTemplateSpecialization(syntax, sym, d->old_sym);
  }
  if (CompilerIsCXX() && parser->cxx_member_definition == NULL &&
      d->old_sym != NULL && !StorageIs(sym->storage, STO(typedef)) &&
      SymbolIsHideableClassOrEnumName(d->old_sym)) {
    HideClassOrEnumName(syntax, d->old_sym);
    d->old_sym = NULL;
  }
  if (parser->cxx_member_definition == NULL &&
      CanOverloadFunctions(d->old_sym, sym)) {
    AddExternalFunctionOverload(parser, d);
  }
  if (parser->cxx_member_definition != NULL &&
      !syntax->parsing_template_specialization) {
    MatchOutOfLineMemberDefinition(parser, d);
  }
  // Capture whether the in-class declaration already defined this static
  // data member (only inline / constexpr members are defined in-class).
  // This must be read before the block below can set `is_defined`, so that
  // a genuine out-of-class definition of a non-inline member is not skipped.
  if (CompilerIsCXX() && parser->cxx_member_definition != NULL &&
      d->old_sym != NULL && !TypeIsFunction(d->old_sym->type) &&
      d->old_sym->flags.is_defined) {
    d->redundant_static_member_redefinition = true;
  }
  if (d->old_sym != NULL) {
    MergeWithPreviousExternalDeclaration(parser, d, storage);
  } else {
    RegisterNewExternalDeclaration(parser, d, storage);
  }
}

// Parses any alignas, __attribute__, __declspec and [[...]] attributes at the
// current token into `attributes`.
static void ParseExternalDeclaratorAttributeList(Syntax* syntax,
                                                 Vector* attributes) {
  while (true) {
    if (SyntaxParseCXXAlignas(syntax, attributes)) {
      continue;
    }
    if (SyntaxParseAnyAttribute(syntax, attributes)) {
      continue;
    }
    break;
  }
}

// Parses a GCC-style assembler name after a declarator and stores it as the
// symbol's asm name:
//   int x asm("external_name");
//   void f(void) asm("external_name");
static void ParseDeclaratorAsmLabel(Syntax* syntax, Symbol* sym) {
  if (!LexMatch(syntax->lex, TOK(asm))) {
    return;
  }
  SyntaxNeedBracket(syntax, TOK(lparen), TC(openbra));
  String asm_name;
  StringInit(&asm_name, "");
  while (LexLookingAt(syntax->lex, TOK(string))) {
    StringAppend(&asm_name, syntax->lex->spelling.value);
    LexNextToken(syntax->lex);
  }
  SyntaxNeedBracket(syntax, TOK(rparen), TC(exprsep) | TC(decl));
  if (AsmLabelIsMachOSymbolName(syntax) && asm_name.value[0] == '_') {
    String stripped;
    StringInit(&stripped, asm_name.value + 1);
    StringSetString(&sym->asm_name, &stripped);
    StringDestruct(&stripped);
  } else {
    StringSetString(&sym->asm_name, &asm_name);
  }
  StringDestruct(&asm_name);
}

// Parses what may follow a declarator before its initializer or body:
// attributes, an assembler name and more attributes.  Applies the leading
// declaration `attributes` and any placeholder constraint to `sym`, and
// checks thread_local.
static void ParseExternalDeclaratorSuffix(TypeParser* parser, Symbol* sym,
                                          Vector* attributes) {
  Syntax* syntax = parser->syntax;
  // Parse common __attribute__ / __declspec / C++ attribute syntax.
  ParseExternalDeclaratorAttributeList(syntax, attributes);

  VectorAppendVector(&sym->attributes, attributes);
  VectorClear(attributes);
  SyntaxApplyDeclarationAttributes(syntax, sym);
  if (parser->placeholder_variable_constraint != NULL) {
    SymbolMutableExtra(sym)->associated_constraint =
        parser->placeholder_variable_constraint;
    parser->placeholder_variable_constraint = NULL;
  } else if (syntax->pending_placeholder_variable_constraint != NULL) {
    SymbolMutableExtra(sym)->associated_constraint =
        syntax->pending_placeholder_variable_constraint;
    syntax->pending_placeholder_variable_constraint = NULL;
  }

  SyntaxCheckThreadLocal(syntax, sym, kParsingFileScope,
                   parser->cxx_member_definition != NULL &&
                       parser->cxx_member_definition->is_static,
                   parser->cxx_member_definition != NULL &&
                       !parser->cxx_member_definition->is_static &&
                       !TypeIsFunction(sym->type));

  ParseDeclaratorAsmLabel(syntax, sym);

  // GNU permits declaration attributes after an assembler name:
  // `int x asm("name") __attribute__((section("cold"))) = 1;`.
  ParseExternalDeclaratorAttributeList(syntax, attributes);
  if (attributes->length != 0) {
    VectorAppendVector(&sym->attributes, attributes);
    VectorClear(attributes);
    SyntaxApplyDeclarationAttributes(syntax, sym);
  }
}

// Copies what a redeclaration adds onto the previous declaration `old_sym`
// (and the reverse for the asm name): module-private, noreturn, deprecated,
// = default / = delete, weak and C linkage.  Also parses = default /
// = delete and computes mangled names for functions.
static void MergeDeclaratorIntoPrevious(Syntax* syntax, Symbol* sym,
                                        Symbol* old_sym) {
  if (old_sym != NULL) {
    if (sym->flags.is_module_private && !old_sym->flags.is_exported) {
      old_sym->flags.is_module_private = true;
    }
    if (sym->flags.noreturn) {
      old_sym->flags.noreturn = true;
      if (!AttributeListHas(&old_sym->attributes, "noreturn")) {
        Attribute* noreturn =
            AttributeListFind(&sym->attributes, "noreturn");
        if (noreturn != NULL) {
          VectorAppend(&old_sym->attributes, AttributeClone(noreturn));
        }
      }
    }
    Attribute* deprecated =
        AttributeListFind(&sym->attributes, "deprecated");
    if (deprecated != NULL &&
        !AttributeListHas(&old_sym->attributes, "deprecated")) {
      VectorAppend(&old_sym->attributes, AttributeClone(deprecated));
    }
    if (sym->asm_name.length != 0) {
      StringSetString(&old_sym->asm_name, &sym->asm_name);
    } else if (old_sym->asm_name.length != 0) {
      StringSetString(&sym->asm_name, &old_sym->asm_name);
    }
  }
  if (TypeIsFunction(sym->type)) {
    ParseCXXDefaultDeleteFunctionSpecifier(syntax, sym->type,
                                           old_sym == NULL);
    ValidateCXX26DeletedMain(syntax, sym);
    if (old_sym != NULL && TypeIsFunction(old_sym->type)) {
      if (sym->type->info.function.is_defaulted) {
        old_sym->type->info.function.is_defaulted = true;
      }
      if (sym->type->info.function.is_deleted) {
        old_sym->type->info.function.is_deleted = true;
        old_sym->type->info.function.is_explicitly_deleted =
            sym->type->info.function.is_explicitly_deleted;
        if (FunctionExtraOf(&old_sym->type->info.function)->deleted_reason !=
            NULL) {
          StringDelete(
              FunctionExtraOf(&old_sym->type->info.function)->deleted_reason);
        }
        FunctionMutableExtra(&old_sym->type->info.function)->deleted_reason =
            FunctionExtraOf(&sym->type->info.function)->deleted_reason != NULL
                ? NewStringWithLength(FunctionExtraOf(&sym->type->info.function)
                                          ->deleted_reason->value,
                                      FunctionExtraOf(&sym->type->info.function)
                                          ->deleted_reason->length)
                : NULL;
      }
      if (sym->flags.is_weak) {
        old_sym->flags.is_weak = true;
      }
    }
  }
  if (TypeIsFunction(sym->type)) {
    if (old_sym != NULL && sym->flags.is_c_linkage) {
      old_sym->flags.is_c_linkage = true;
    }
    SymbolSetCXXMangledAsmName(sym);
    if (old_sym != NULL && old_sym->asm_name.length == 0) {
      SymbolSetCXXMangledAsmName(old_sym);
    }
  }
}

// Declares or defines the function `sym`.  Returns the definition's AST when
// a body followed, otherwise NULL (the declarator continues as a declaration).
static ASTNode* DeclareExternalFunction(TypeParser* parser,
                                        Vector* declarations, Symbol* sym,
                                        Symbol* old_sym) {
  Syntax* syntax = parser->syntax;
  if (!CompilerIsCXX() && parser->is_constexpr) {
    SyntaxError(syntax, "C constexpr functions are not supported");
  }
  if (TypeContainsAuto(sym->type) &&
      !TypeFunctionReturnContainsAuto(sym->type)) {
    SyntaxError(syntax, "auto function parameter type is not supported yet");
  }
  ASTNode *result = DeclareOrDefineFunction(syntax, declarations, sym, old_sym);
  if (result != NULL) {
    // A function definition whose name was already declared: `old_sym`
    // stays in the symbol table and `sym` (this definition, holding the
    // body and its own function type) is referenced only by the AST and
    // by old_sym->value.func_defn.  It never enters the table, so track it
    // for teardown instead of leaking the symbol<->type cycle.
    if (old_sym != NULL) {
      VectorAppend(&compiler->orphan_function_symbols, sym);
    }
  }
  return result;
}

// Rejects an object declaration (file or block scope) whose type cannot be
// instantiated: an abstract class, or a class template no specialization was
// made for.  Materializes a deferred class template specialization first.
void CheckDeclaredObjectType(Syntax* syntax, Symbol* sym) {
  // A type alias formed while its class template was only forward-declared
  // carries a *deferred* template-id (see the forward-declaration path in
  // InstantiateSimpleClassTemplateImpl): the primary's members were not yet
  // known, so the specialization was recorded but not materialized.  A
  // variable declaration requires a complete type, so materialize the
  // specialization now that the definition is available, instead of rejecting
  // the still-"template" type in the guard below.
  MaterializeDeferredClassTemplateType(syntax, sym);
  bool declares_object = !StorageIs(sym->storage, STO(typedef));
  if (declares_object && !TypeIsClassTemplatePlaceholder(sym->type) &&
      TypeIsAbstractClass(sym->type)) {
    SyntaxError(syntax, "Cannot declare object of abstract class %s",
                sym->type->info.struct_info->tag_name != NULL
                    ? sym->type->info.struct_info->tag_name->value
                    : "<anonymous>");
  }
  // `extern absl::Flag<T> FLAGS_name;` is well-formed while `Flag` is
  // only forward-declared.  The type stays a deferred template-id until
  // a later declaration can materialize it.  A definition still needs
  // a complete specialization.
  bool extern_incomplete_declaration =
      StorageIs(sym->storage, STO(extern)) &&
      !LexLookingAt(syntax->lex, TOK(equal)) &&
      !LexLookingAt(syntax->lex, TOK(lbrace));
  if (declares_object && !syntax->parsing_template_declaration &&
      TypeIsClassTemplateObject(sym->type) &&
      !extern_incomplete_declaration) {
    SyntaxError(syntax,
                "Cannot declare an object of class template '%s'; no "
                "specialization was instantiated",
                ClassTemplateObjectName(sym->type));
  }
}

// Applies constexpr, constinit, consteval and C++ inline to an object symbol
// (an inline variable is a weak definition), or rejects constinit on a
// function.
static void ApplyExternalObjectSpecifiers(TypeParser* parser, Symbol* sym) {
  Syntax* syntax = parser->syntax;
  if (TypeIsFunction(sym->type)) {
    if (parser->is_constinit) {
      SyntaxError(syntax, "'constinit' cannot be applied to a function");
    }
  } else {
    sym->flags.is_constexpr = parser->is_constexpr;
    sym->flags.is_constinit = parser->is_constinit;
    DiagnoseConstevalOnNonFunction(syntax, sym->type, parser->is_consteval);
    if (CompilerIsCXX() && parser->is_inline) {
      sym->flags.is_defined = true;
      if (!StorageIs(sym->storage, STO(static))) {
        sym->flags.is_weak = true;
        SymbolSetCXXDataAsmName(sym, NULL);
      }
    }
    if (sym->flags.is_constexpr && !TypeIsReference(sym->type)) {
      SymbolSetType(sym, TypeRecordAddQualifiers(sym->type, kQualConst));
    }
  }
}

// Checks a deduced `auto` object's placeholder constraint
// (`Concept auto x = ...`) against the deduced type.
static void CheckDeducedObjectConstraint(Syntax* syntax, Symbol* sym) {
  if (SymbolExtraOf(sym)->associated_constraint == NULL || sym->type == NULL) {
    return;
  }
  Vector* constraint_args = NewVector();
  TemplateArgument* type_arg = TemplateArgumentAlloc();
  type_arg->kind = kTemplateParameterType;
  type_arg->type = TypeRecordCopy(sym->type);
  VectorAppend(constraint_args, type_arg);
  if (!ConceptsConstraintSatisfied(SymbolExtraOf(sym)->associated_constraint,
                                   constraint_args)) {
    SyntaxError(syntax, "constraints not satisfied");
    ConceptsReportAssociatedConstraintFailure(
        SymbolExtraOf(sym)->associated_constraint, constraint_args,
        sym->location, NULL);
  }
  VectorDeleteWithContents(
      constraint_args, (VectorElementDestructor)TemplateArgumentDelete,
      /*free_element=*/false);
}

// Parses an object's initializer (`= x`, `(args)` or `{args}`), if any, and
// deduces an `auto` type from it.  Returns the initializer or NULL.
static ASTNode* ParseExternalObjectInitializer(TypeParser* parser,
                                               Symbol* sym, Storage storage) {
  Syntax* syntax = parser->syntax;
  ASTNode* initializer = NULL;
  if (LexMatch(syntax->lex, TOK(equal))) {
    syntax->init_storage = storage;
    initializer = SyntaxParseInitializer(syntax, sym, storage);
    ResolveCXXClassTemplateArgumentDeductionFromInitializer(
        syntax, sym, initializer);
  } else {
    initializer = ParseCXXDirectInitializer(syntax, sym, false);
    if (initializer != NULL) {
      if (!StorageIs(storage, STO(extern))) {
        sym->flags.is_defined = true;
      }
    } else if (CompilerIsCXX() && LexMatch(syntax->lex, TOK(lbrace))) {
      initializer = MarkDirectListInitializer(ParseBracedInitializer(syntax));
      if (!StorageIs(storage, STO(extern))) {
        sym->flags.is_defined = true;
      }
    }
  }
  ValidateC23AutoInitializer(syntax, sym, initializer);
  ValidateC23ConstexprObject(syntax, sym, initializer);
  if (!TypeIsFunction(sym->type) && TypeContainsAuto(sym->type) &&
      initializer == NULL) {
    SyntaxError(syntax, "auto variable requires an initializer");
  } else if (!TypeIsFunction(sym->type) && TypeContainsAuto(sym->type) &&
             syntax->parsing_template_declaration) {
    // A variable template's type is deduced for each instantiation.
    // Analyzing `auto` here resolves dependent calls such as
    // `std::get<T>(std::tuple{...})` against the primary template.
  } else if (!TypeIsFunction(sym->type) && TypeContainsAuto(sym->type)) {
    initializer = SemanticAnalyzeDeducedInitializer(sym, initializer);
    SemanticDeduceAutoType(sym, initializer, (ASTNode*)initializer);
    CheckDeducedObjectConstraint(syntax, sym);
  }
  return initializer;
}

// Supplies the default initializer a constexpr or constinit object without an
// initializer needs, diagnosing one that has none, and rejects class template
// argument deduction without an initializer.  Returns the initializer.
static ASTNode* SupplyConstexprDefaultInitializer(TypeParser* parser,
                                                  Symbol* sym, Storage storage,
                                                  ASTNode* initializer) {
  Syntax* syntax = parser->syntax;
  // Unlike constexpr, constinit may appear on a non-defining declaration
  // (`constinit extern int x;`); the definition carries the initializer.
  bool constinit_declaration = sym->flags.is_constinit &&
                               !sym->flags.is_constexpr &&
                               StorageIs(storage, STO(extern));
  if ((sym->flags.is_constexpr || sym->flags.is_constinit) &&
      !constinit_declaration &&
      initializer == NULL && parser->cxx_member_definition == NULL &&
      !CXXDeferConstexprDefaultInitialization(syntax, sym->type)) {
    // An out-of-class definition of a static data member
    // (`constexpr T C::x;`) needs no initializer: the required initializer is
    // supplied by the in-class `static constexpr` declaration.
    initializer = CompilerIsCXX() && !StorageIs(storage, STO(extern))
                      ? SyntaxNewCXXConstexprDefaultInitializer(
                            syntax, sym, /*local=*/false)
                      : NULL;
    if (initializer != NULL) {
      sym->flags.is_defined = true;
    } else {
      SyntaxError(syntax, sym->flags.is_constinit
                              ? "constinit variable requires an initializer"
                              : "constexpr variable requires an initializer");
    }
  }
  if (CompilerCXXAtLeast(kLanguageStandardCXX17) &&
      !syntax->parsing_template_declaration &&
      TypeIsClassTemplatePlaceholder(sym->type)) {
    SyntaxError(syntax, "Class template argument deduction requires an initializer");
  }
  return initializer;
}

// `const int C::b;` defines a member whose value came from its in-class
// initializer (`static const int b = 3;`); the definition carries it.  Returns
// that initializer, or `initializer` unchanged.
static ASTNode* InheritStaticMemberInitializer(
    TypeParser* parser, Symbol* sym, Symbol* old_sym, ASTNode* initializer,
    bool skip_redundant_static_member_definition) {
  Syntax* syntax = parser->syntax;
  if (CompilerIsCXX() && initializer == NULL &&
      !skip_redundant_static_member_definition &&
      !syntax->parsing_template_declaration &&
      parser->cxx_member_definition != NULL && old_sym != NULL &&
      old_sym->type != NULL &&
      !TypeIsFunction(old_sym->type) && old_sym->flags.value_set &&
      TypeIsConst(old_sym->type) &&
      (TypeIsIntegral(old_sym->type) || TypeIsEnum(old_sym->type))) {
    SourceLocation location = syntax->lex->current_token_location;
    initializer = NewExpressionInitializerASTNode(
        NewIntConstantASTNode(old_sym->value.ivalue,
                              TypeRecordCopy(sym->type), location),
        location);
  }
  return initializer;
}

// Appends the variable declaration for `sym` to `declarations`, records an
// out-of-class static data member definition on its class, and analyzes
// constant variables immediately so they can be used in constant expressions.
static void AppendExternalVariableDeclaration(TypeParser* parser,
                                              Vector* declarations,
                                              Symbol* sym, Symbol* old_sym,
                                              ASTNode* initializer) {
  Syntax* syntax = parser->syntax;
  ASTNode* decl = NewVariableDeclarationASTNode(
      sym, initializer, syntax->lex->current_token_location);
  VectorAppend(declarations, decl);
  if (CompilerIsCXX() && parser->cxx_member_definition != NULL &&
      old_sym != NULL && old_sym->type != NULL &&
      !TypeIsFunction(old_sym->type)) {
    TypeRecordStaticDataMemberDefinition(
        syntax, old_sym, decl,
        syntax->parsing_template_declaration &&
            !syntax->parsing_template_specialization);
  }
  // A variable template's initializer is value-dependent on its template
  // parameters; it is analyzed and folded per use (see
  // MarkTemplateDeclaration / TypeInstantiateVariableTemplateConstant),
  // never eagerly at the point of definition.
  if (!syntax->parsing_template_declaration &&
      (sym->flags.is_constexpr || sym->flags.is_constinit ||
       (TypeIsConst(sym->type) && !TypeIsStructOrUnion(sym->type)))) {
    SemanticAnalyzeVariableDefinition(syntax,
                                      (VariableDeclarationASTNode*)decl);
  }
}

// Parses the init-declarator list of a namespace-scope declaration whose
// specifiers (`type`, `storage`, `is_explicit`, leading `attributes`) have
// been parsed.  Each declarator is matched against earlier declarations,
// then becomes a function declaration or definition, or a variable
// declaration appended to `declarations`.  Returns a function definition's
// AST (which ends the list), otherwise NULL.
static ASTNode* ParseExternalDeclarationList(TypeParser* parser,
                                         TypeRecord* type,
                                         Storage storage,
                                         bool is_explicit,
                                         Vector* attributes,
                                         Vector* declarations) {
  Syntax* syntax = parser->syntax;
  while (!LexEof(parser->syntax->lex)) {
    if (LexLookingAt(syntax->lex, TOK(semicolon))) {
      // We don't need to have a variable declaration.
      break;
    }

    Symbol* sym = TypeParserParseDeclarator(parser, type);
    ValidateC23AutoDeclarator(syntax, sym);
    ExternalDeclaratorFlow flow =
        ParseSpecialExternalDeclarator(parser, sym, storage);
    if (flow == kExternalDeclaratorNext) {
      continue;
    }
    if (flow == kExternalDeclaratorStop) {
      break;
    }
    ExternalDeclarator d = {.sym = sym};
    if (sym != NULL) {
      if (!PrepareExternalDeclaratorSymbol(parser, sym, &storage,
                                           is_explicit)) {
        TypeParserReset(parser);
        continue;
      }
      ResolveExternalDeclarator(parser, &d, storage);
    }
    Symbol* old_sym = d.old_sym;

    // If we don't have a symbol in the type declarator we look for a comma and
    // continue parsing.
    if (sym == NULL) {
      if (!LexMatch(syntax->lex, TOK(comma))) {
        break;
      }
      TypeParserReset(parser);
      continue;
    }

    ParseExternalDeclaratorSuffix(parser, sym, attributes);
    MergeDeclaratorIntoPrevious(syntax, sym, old_sym);

    // Declaring or defining a function?
    if (TypeIsFunction(sym->type)) {
      ASTNode* result =
          DeclareExternalFunction(parser, declarations, sym, old_sym);
      if (result != NULL) {
        return result;
      }
    } else if (parser->is_inline && !CompilerIsCXX()) {
      SyntaxError(syntax, "inline can only be applied to functions");
    }
    CheckDeclaredObjectType(syntax, sym);

    if (old_sym != NULL) {
      // We now refer to the previously defined symbol rather than this new
      // one.
      if (sym->asm_name.length != 0) {
        StringSetString(&old_sym->asm_name, &sym->asm_name);
      }
      SymbolDelete(sym);
      sym = old_sym;
    }
    bool skip_cxx_function_redeclaration =
        CompilerIsCXX() && old_sym != NULL && TypeIsFunction(sym->type) &&
        sym->flags.is_defined && sym->type->info.function.definition;
    ApplyExternalObjectSpecifiers(parser, sym);

    ASTNode* initializer = ParseExternalObjectInitializer(parser, sym, storage);
    initializer =
        SupplyConstexprDefaultInitializer(parser, sym, storage, initializer);
    // A `T C::m;` out-of-class redeclaration of an already-in-class-defined
    // inline/constexpr static data member (no initializer of its own) is not a
    // definition: emitting it would duplicate the symbol produced by the
    // in-class initializer.  Drop it from code generation.
    bool skip_redundant_static_member_definition =
        d.redundant_static_member_redefinition && initializer == NULL;
    initializer = InheritStaticMemberInitializer(
        parser, sym, old_sym, initializer,
        skip_redundant_static_member_definition);
    if (!skip_cxx_function_redeclaration &&
        !skip_redundant_static_member_definition) {
      AppendExternalVariableDeclaration(parser, declarations, sym, old_sym,
                                        initializer);
    }

    if (IsC23InferredAutoType(type) &&
        LexLookingAt(syntax->lex, TOK(comma))) {
      SyntaxError(syntax,
                  "C23 inferred auto declaration must contain one declarator");
    }
    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
    TypeParserReset(parser);
    
    // All declarations in the list share the same storage.
    parser->storage = storage;
  }
  if (type != NULL && (TypeIsEnum(type) || TypeIsStructOrUnion(type))) {
    // Declaring a struct/union/enum with no symbol still needs to
    // output debug information.
    if (compiler->debug_output) {
      BuildTypeDebugInfo(&compiler->debug_builder, type);
    }
  }
  return NULL;
}

// Appends `node`, or every declaration in a decl-list node, to `declarations`.
// For a decl list, transfers ownership into `declarations` and clears the
// child vector so teardown does not double-free.
void AppendDeclarationsFromNode(Vector* declarations, ASTNode* node) {
  if (node == NULL) {
    return;
  }
  if (node->op != AST_OP(decl_list)) {
    VectorAppend(declarations, node);
    return;
  }

  DeclarationListASTNode* list = (DeclarationListASTNode*)node;
  VectorAppendVector(declarations, list->declarations);
  // The declarations have been transferred to the containing namespace list.
  // Leave the child list empty so its idempotent teardown does not delete them.
  VectorClear(list->declarations);
}

// Returns an empty declaration-list AST at `location` (a no-op declaration).
ASTNode* EmptyDeclarationList(SourceLocation location) {
  return NewDeclarationListASTNode(NewVector(), location);
}

// Parses an out-of-line C++ special member definition (constructor,
// destructor, or assignment) and defines the matching in-class declaration.
// Returns the function body AST, or an empty declaration list on error or
// explicit instantiation.
static ASTNode* ParseCXXSpecialMemberDefinition(Syntax* syntax) {
  SourceLocation location = syntax->lex->current_token_location;
  Vector* declarations = NewVector();
  Vector attributes = {0};
  while (SyntaxParseAnyAttribute(syntax, &attributes)) {
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), kParsingFileScope);
  ApplyCXXSpecialMemberDeclSpecifiers(&parser);

  Symbol* sym = TypeParserParseCXXSpecialMemberDeclarator(&parser);
  if (sym != NULL && attributes.length > 0) {
    VectorAppendVector(&sym->attributes, &attributes);
    SyntaxApplyDeclarationAttributes(syntax, sym);
  }
  // `template <typename T> inline Condition::Condition(...)` is parsed here,
  // not on the ordinary declarator path that records template_parameter_count.
  // Without that count the definition is not recognized as a member template
  // and is compared against the first constructor overload.  A class-template
  // constructor (`template <class T> Matcher<T>::Matcher(T)`) already consumed
  // that head in the qualifier; copying it on would hide the matching
  // declaration.
  if (sym != NULL && TypeIsFunction(sym->type) &&
      syntax->parsing_template_declaration &&
      !parser.qualifier_consumed_template_parameters &&
      FunctionExtraOf(&sym->type->info.function)->template_parameter_count <=
          0) {
    int listed = CurrentTemplateParameterListLength(syntax);
    int moved = (int)FunctionExtraOf(&sym->type->info.function)
                    ->template_parameters.length;
    int count = listed > 0 ? listed : moved;
    if (count > 0) {
      sym->flags.is_template = true;
      FunctionMutableExtra(&sym->type->info.function)
          ->template_parameter_count = count;
      FunctionMutableExtra(&sym->type->info.function)->template_parameter_base =
          listed > 0 ? CurrentTemplateParameterBase(syntax)
                     : syntax->current_template_parameter_count - count;
    }
  }
  Symbol* old_sym = parser.cxx_member_definition != NULL
      ? parser.cxx_member_definition->symbol
      : NULL;
  if (sym != NULL && parser.cxx_member_definition != NULL) {
    StructMember* matching_member =
        FindStructMemberOverload(parser.cxx_member_definition, sym->type);
    if (matching_member != NULL) {
      parser.cxx_member_definition = matching_member;
      old_sym = matching_member->symbol;
    }
  }
  if (sym == NULL || old_sym == NULL) {
    if (sym != NULL) {
      SymbolDelete(sym);
    }
    VectorDelete(declarations);
    TypeParserDestruct(&parser);
    return EmptyDeclarationList(location);
  }

  if (syntax->parsing_explicit_instantiation) {
    DefineExplicitFunctionInstantiation(syntax, &parser, sym);
    SymbolDelete(sym);
    VectorDelete(declarations);
    TypeParserDestruct(&parser);
    SyntaxNeedSemicolon(syntax, TC(decl));
    return EmptyDeclarationList(location);
  }

  if (old_sym->flags.is_defined && LexLookingAt(syntax->lex, TOK(lbrace))) {
    String symbol_name;
    StringInit(&symbol_name, NULL);
    SymbolFunctionDiagnosticName(sym, &symbol_name);
    SyntaxError(syntax, "Duplicate definition of symbol %s",
                symbol_name.value);
    StringDestruct(&symbol_name);
  } else if (!MemberDefinitionTypesEqual(&parser, sym->type, old_sym->type)) {
    String suffix;
    StringInit(&suffix, NULL);
    SymbolFunctionDiagnosticSuffix(sym, &suffix);
    SyntaxError(syntax, "Symbol %s redeclared with different type%s",
                sym->name.value, suffix.value);
    StringDestruct(&suffix);
    TypeErrorDetails(syntax->lex->current_token_location,
                     sym->type, old_sym->type);
  } else if (LexLookingAt(syntax->lex, TOK(lbrace))) {
    MergeCXXDefaultArguments(syntax, old_sym, sym);
    MergeCXXContractAssertions(syntax, old_sym, sym);
    CheckMatchingConstexprConsteval(syntax, old_sym->type, sym->type);
    old_sym->flags.is_defined = true;
  }

  ASTNode* result = DeclareOrDefineFunction(syntax, declarations, sym, old_sym);
  if (result != NULL) {
    VectorAppend(&compiler->orphan_function_symbols, sym);
    TypeParserDestruct(&parser);
    return result;
  }

  SyntaxError(syntax, "Special member definition requires a function body");
  SymbolDelete(sym);
  VectorDelete(declarations);
  TypeParserDestruct(&parser);
  return EmptyDeclarationList(location);
}

// If we are looking at a C++ linkage specification (`extern "C"` or
// `extern "C++"`, optionally with a brace-enclosed declaration sequence),
// parse it and return the resulting declaration(s).  Returns NULL (leaving the
// lexer untouched) when the current tokens are not a linkage specification, so
// the caller can treat a bare `extern` as an ordinary storage-class specifier.
static ASTNode* ParseCXXLinkageSpecification(Syntax* syntax) {
  if (!CompilerIsCXX() || !LexLookingAt(syntax->lex, TOK(extern))) {
    return NULL;
  }

  // Peek past `extern` to see whether a string-literal linkage name follows.
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // extern
  if (!LexLookingAtStringLiteral(syntax->lex)) {
    LexCheckpointRestore(syntax->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
    return NULL;
  }
  LexCheckpointDestruct(&checkpoint);

  String linkage;
  StringInit(&linkage, "");
  while (LexLookingAtStringLiteral(syntax->lex)) {
    LexValidateUnevaluatedString(syntax->lex, "linkage specification",
                                 /*allow_user_defined_suffix=*/false);
    StringAppend(&linkage, syntax->lex->spelling.value);
    LexNextToken(syntax->lex);
  }
  bool is_c = StringEqual(&linkage, "C");
  bool is_cpp = StringEqual(&linkage, "C++");
  if (!is_c && !is_cpp) {
    SyntaxError(syntax, "Unknown linkage specification \"%s\"", linkage.value);
  }
  StringDestruct(&linkage);

  // A nested linkage specification overrides the surrounding linkage while
  // its declaration sequence is parsed.
  bool saved_c_linkage = syntax->c_linkage;
  bool saved_explicit_cxx_linkage = syntax->explicit_cxx_linkage;

  if (LexMatch(syntax->lex, TOK(lbrace))) {
    Vector* declarations = NewVector();
    syntax->c_linkage = is_c;
    syntax->explicit_cxx_linkage = is_cpp;
    while (!LexEof(syntax->lex) && !LexLookingAt(syntax->lex, TOK(rbrace))) {
      ASTNode* node = SyntaxParseExternalDeclaration(syntax);
      AppendDeclarationsFromNode(declarations, node);
    }
    syntax->c_linkage = saved_c_linkage;
    syntax->explicit_cxx_linkage = saved_explicit_cxx_linkage;
    SyntaxNeedBracket(syntax, TOK(rbrace), TC(closebrace) | TC(decl));
    return NewDeclarationListASTNode(declarations, location);
  }

  // Single-declaration form: `extern "C" <declaration>`.
  syntax->c_linkage = is_c;
  syntax->explicit_cxx_linkage = is_cpp;
  ASTNode* node = SyntaxParseExternalDeclaration(syntax);
  syntax->c_linkage = saved_c_linkage;
  syntax->explicit_cxx_linkage = saved_explicit_cxx_linkage;
  return node;
}

// Parses a file-scope declaration introduced by a keyword or recognized by
// lookahead: module declarations, consteval blocks, static_assert,
// namespaces, templates, using-declarations, explicit instantiations,
// linkage specifications and out-of-line special member definitions.
// Returns true with the result in `*result`, or false when the declaration
// is an ordinary simple declaration.
static bool ParseKeywordExternalDeclaration(Syntax* syntax, ASTNode** result) {
  if (CompilerCXXAtLeast(kLanguageStandardCXX20)) {
    if (LexLookingAt(syntax->lex, TOK(export))) {
      *result = ModuleSyntaxParseExportDeclaration(syntax);
      return true;
    }
    ASTNode* module_node = ModuleSyntaxParseExternalDeclaration(syntax);
    if (module_node != NULL) {
      *result = module_node;
      return true;
    }
  }

  ModuleSyntaxNoteNonImportDeclaration(syntax);

  if (CompilerCXXAtLeast(kLanguageStandardCXX26) &&
      LexLookingAt(syntax->lex, TOK(consteval))) {
    LexCheckpoint block_checkpoint;
    LexCheckpointSave(syntax->lex, &block_checkpoint);
    LexNextToken(syntax->lex);
    bool is_consteval_block = LexLookingAt(syntax->lex, TOK(lbrace));
    LexCheckpointRestore(syntax->lex, &block_checkpoint);
    LexCheckpointDestruct(&block_checkpoint);
    if (is_consteval_block) {
      *result = SyntaxParseConstevalBlock(syntax, TC(decl) | TC(closebrace));
      return true;
    }
  }

  if (LexLookingAt(syntax->lex, TOK(static_assert))) {
    SourceLocation location = syntax->lex->current_token_location;
    ASTNode* node = SyntaxParseStaticAssert(syntax);
    ASTNodeDelete(node);
    *result = EmptyDeclarationList(location);
    return true;
  }
  if (LexLookingAt(syntax->lex, TOK(namespace))) {
    *result = ParseNamespaceDeclaration(syntax, /*leading_inline=*/false);
    return true;
  }
  if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(inline))) {
    LexCheckpoint checkpoint;
    LexCheckpointSave(syntax->lex, &checkpoint);
    LexNextToken(syntax->lex);
    if (LexLookingAt(syntax->lex, TOK(namespace))) {
      LexCheckpointDestruct(&checkpoint);
      *result = ParseNamespaceDeclaration(syntax, /*leading_inline=*/true);
      return true;
    }
    LexCheckpointRestore(syntax->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
  }
  if (LexLookingAt(syntax->lex, TOK(template))) {
    *result = ParseTemplateDeclaration(syntax);
    return true;
  }
  if (LexLookingAt(syntax->lex, TOK(using))) {
    *result = ParseUsingDeclaration(syntax);
    return true;
  }
  if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(extern))) {
    LexCheckpoint extern_checkpoint;
    LexCheckpointSave(syntax->lex, &extern_checkpoint);
    SourceLocation extern_location = syntax->lex->current_token_location;
    LexNextToken(syntax->lex);
    if (LexLookingAt(syntax->lex, TOK(template))) {
      LexCheckpointDestruct(&extern_checkpoint);
      LexNextToken(syntax->lex);
      *result = ParseExplicitTemplateInstantiation(syntax, extern_location,
                                                   /*is_extern=*/true);
      return true;
    }
    LexCheckpointRestore(syntax->lex, &extern_checkpoint);
    LexCheckpointDestruct(&extern_checkpoint);
    ASTNode* linkage = ParseCXXLinkageSpecification(syntax);
    if (linkage != NULL) {
      *result = linkage;
      return true;
    }
  }
  if (CurrentLineLooksLikeSpecialMemberDefinition(syntax)) {
    *result = ParseCXXSpecialMemberDefinition(syntax);
    return true;
  }
  return false;
}

// Parses a file-scope simple declaration: attributes, declaration
// specifiers and the init-declarator list (or a function definition).
// Returns the declaration list, including any inline static data member
// definitions queued while parsing it.
static ASTNode* ParseSimpleExternalDeclaration(Syntax* syntax) {
  Vector* declarations = NewVector();
  Vector attributes = {0};
  
  // Parse common __attribute__ / __declspec / C++ attribute syntax.
  while (SyntaxParseAnyAttribute(syntax, &attributes)) {
  }

  Storage storage = STO(implicit);
  bool is_inline = false;
  bool is_constexpr = false;
  bool is_consteval = false;
  bool is_constinit = false;
  bool is_explicit = false;
  TypeRecord* type = NULL;
  ParseDeclarationSpecifier(syntax, &storage, &is_inline, &is_constexpr,
                            &is_consteval, &is_constinit,
                            &is_explicit, &type, &attributes,
                            kParsingFileScope);

  if (!CompilerIsCXX() && is_constexpr) {
    if (StorageIs(storage, STO(extern))) {
      SyntaxError(syntax,
                  "file-scope C constexpr object cannot have external linkage");
    }
    storage &= ~STO(extern);
    storage |= STO(static);
  }

  if (StorageIs(storage, STO(auto)|STO(register))) {
    SyntaxError(syntax, "Illegal global storage specified: %s",
                StorageIs(storage, STO(register)) ? "register" : "auto");
    storage = STO(implicit);
  }

  // Parse common __attribute__ / __declspec / C++ attribute syntax.
  while (SyntaxParseAnyAttribute(syntax, &attributes)) {
  }

  // Create a type parser for the declarators.
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, storage, kParsingFileScope);
  parser.is_inline = is_inline;
  parser.is_constexpr = is_constexpr;
  parser.is_consteval = is_consteval;
  parser.is_constinit = is_constinit;
 
  // Open a scope (local symbol stack) for the symbols declared in the
  // type declaration list.
  SyntaxOpenScope(syntax);

  // Claim a reference on the freshly built base type for the duration of
  // declarator parsing.  Each declarator that adopts it takes its own
  // reference, so releasing ours afterwards frees the base type when no
  // declarator used it (e.g. a bare `struct S { ... };` or `enum E { ... };`).
  TypeRecordIncRef(type);

  // Now we get a sequence of declarations, separated by commas.
  ASTNode* result = ParseExternalDeclarationList(&parser,
                                                 type, storage,
                                                 is_explicit,
                                                 &attributes,
                                                 declarations);
  SyntaxCloseScope(syntax);
  TypeParserDestruct(&parser);
  TypeRecordDelete(type);
  if (result != NULL) {
    if (declarations->length != 1) {
      SyntaxError(syntax, "Cannot mix function definition with declaration");
    }
    VectorAppendVector(declarations, &syntax->inline_static_member_definitions);
    // Ownership of the queued inline static data member definitions now belongs
    // to `declarations`; clear the queue so later declarations don't re-emit
    // (and double-free) them.
    VectorClear(&syntax->inline_static_member_definitions);
    // result is a DeclarationListASTNode that owns `declarations`; its teardown
    // frees the vector and its contents, so don't free them here.
    AttributeListDestruct(&attributes);
    return result;
  }

  VectorAppendVector(declarations, &syntax->inline_static_member_definitions);
  VectorClear(&syntax->inline_static_member_definitions);

  // The declaration is followed by a semicolon.
  SyntaxNeedSemicolon(syntax, TC(type));

  AttributeListDestruct(&attributes);
  
  return NewDeclarationListASTNode(declarations,
                                   syntax->lex->current_token_location);
}

// Parses an external declaration (a global variable, etc.) and adds it
// to the symbol table.
static ASTNode* ParseExternalDeclarationBody(Syntax* syntax) {
  syntax->context = kParsingFileScope;
  if (syntax->current_namespace == NULL) {
    syntax->current_namespace = compiler->global_namespace;
  }
  ASTNode* result = NULL;
  if (ParseKeywordExternalDeclaration(syntax, &result)) {
    return result;
  }
  return ParseSimpleExternalDeclaration(syntax);
}

// Parses one file-scope external declaration, consuming its tokens and
// ensuring the lexer advances.  Returns a declaration list or function
// definition AST.
ASTNode* SyntaxParseExternalDeclaration(Syntax* syntax) {
  Token token_before = syntax->lex->current_token;
  SourceLocation location_before = syntax->lex->current_token_location;
  int errors_before = NumErrors();
  ASTNode* node = ParseExternalDeclarationBody(syntax);
  SyntaxEnsureProgress(syntax, token_before, location_before, errors_before,
                       TC(closebrace));
  return node;
}
