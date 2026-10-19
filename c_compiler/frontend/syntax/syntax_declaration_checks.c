//
//  syntax_declaration_checks.c
//  c_compiler
//
//  Semantic checks on declarations: C23 auto and constexpr objects, main(),
//  and C++ linkage of namespace-scope variables.
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

// True in C23 mode when `type` uses inferred `auto` for a variable declaration.
bool IsC23InferredAutoType(TypeRecord* type) {
  return !CompilerIsCXX() && CompilerCAtLeast(kLanguageStandardC23) &&
         type != NULL && TypeContainsAuto(type);
}

typedef struct {
  Symbol* symbol;
  bool found;
} C23AutoSelfReference;

// Sets `search->found` when an identifier in the initializer refers to
// `search->symbol`.
static void FindC23AutoSelfReference(ASTNode* node, void* data, int child_id,
                                    VisitorMode mode) {
  (void)child_id;
  C23AutoSelfReference* search = data;
  if (mode == kVisitPreChildren && node != NULL &&
      node->op == AST_OP(identifier) &&
      ((IdentifierASTNode*)node)->symbol == search->symbol) {
    search->found = true;
  }
}

// Emits diagnostics when `sym` uses C23 inferred `auto` on an invalid
// declarator or function type.
void ValidateC23AutoDeclarator(Syntax* syntax, Symbol* sym) {
  if (sym == NULL || !IsC23InferredAutoType(sym->type)) {
    return;
  }
  if (sym->type->declarator != kDeclPrimitive ||
      (sym->type->type & kTypeAuto) == 0) {
    SyntaxError(syntax,
                "C23 inferred auto requires a simple object declarator");
  }
  if (TypeIsFunction(sym->type)) {
    SyntaxError(syntax, "C23 auto cannot infer a function type");
  }
}

// Emits diagnostics when a C23 inferred `auto` declaration lacks a suitable
// initializer or the initializer refers to the object being declared.
void ValidateC23AutoInitializer(Syntax* syntax, Symbol* sym,
                                ASTNode* initializer) {
  if (sym == NULL || !IsC23InferredAutoType(sym->type)) {
    return;
  }
  if (initializer == NULL) {
    SyntaxError(syntax, "C23 inferred auto requires an initializer");
    return;
  }
  ASTNode* deduction_initializer = initializer;
  if (deduction_initializer->op == AST_OP(init)) {
    deduction_initializer = ((BinaryASTNode*)deduction_initializer)->right;
  }
  if (deduction_initializer == NULL ||
      deduction_initializer->op != AST_OP(expr_init)) {
    SyntaxError(syntax,
                "C23 inferred auto requires an assignment-expression initializer");
  }
  C23AutoSelfReference search = {.symbol = sym, .found = false};
  ASTNodeVisit(deduction_initializer, FindC23AutoSelfReference, 0, &search);
  if (search.found) {
    SyntaxError(syntax,
                "C23 inferred auto initializer cannot refer to itself");
  }
}

// True when any record on `type`'s spine is a variably modified type.
static bool C23ConstexprTypeContainsVLA(TypeRecord* type) {
  for (TypeRecord* current = type; current != NULL; current = current->next) {
    if (TypeIsVLA(current)) {
      return true;
    }
  }
  return false;
}

// True when `type` is permitted for a C23 `constexpr` object: no VLA,
// atomic/volatile/restrict qualifiers, and allowed non-static member types.
static bool C23ConstexprObjectTypeIsAllowed(TypeRecord* type) {
  if (type == NULL || C23ConstexprTypeContainsVLA(type) ||
      (GetQualifiers(type) & (kQualAtomic | kQualVolatile | kQualRestrict)) !=
          0) {
    return false;
  }
  if (TypeIsPointer(type)) {
    return true;
  }
  if (TypeIsArray(type)) {
    return C23ConstexprObjectTypeIsAllowed(type->next);
  }
  if (TypeIsStructOrUnion(type) && type->info.struct_info != NULL) {
    Struct* aggregate = type->info.struct_info;
    for (size_t i = 0; i < aggregate->members.length; i++) {
      StructMember* member = aggregate->members.value.p[i];
      if (member != NULL && member->symbol != NULL &&
          !member->is_static && !member->is_member_function &&
          !C23ConstexprObjectTypeIsAllowed(member->symbol->type)) {
        return false;
      }
    }
  }
  return true;
}

// True when `initializer` denotes a null pointer constant, including through
// casts and designated-init wrappers.
static bool C23InitializerIsNullPointer(ASTNode* initializer) {
  if (initializer == NULL) {
    return false;
  }
  if (initializer->op == AST_OP(expr_init)) {
    return C23InitializerIsNullPointer(
        ((ExpressionInitializerASTNode*)initializer)->expr);
  }
  if (initializer->op == AST_OP(designated_init)) {
    return C23InitializerIsNullPointer(
        ((DesignatedInitializerASTNode*)initializer)->init);
  }
  if (initializer->op == AST_OP(cast)) {
    return C23InitializerIsNullPointer(((CastASTNode*)initializer)->expr);
  }
  return TypeIsNullPointer(initializer->type) ||
         (initializer->op == AST_OP(number) &&
          ((ConstantASTNode*)initializer)->value.ivalue == 0);
}

// True when `type` contains a pointer subobject in a non-static data member.
static bool C23ConstexprTypeHasPointerSubobject(TypeRecord* type) {
  if (type == NULL) {
    return false;
  }
  if (TypeIsPointer(type)) {
    return true;
  }
  if (TypeIsArray(type)) {
    return C23ConstexprTypeHasPointerSubobject(type->next);
  }
  if (TypeIsStructOrUnion(type) && type->info.struct_info != NULL) {
    Struct* aggregate = type->info.struct_info;
    for (size_t i = 0; i < aggregate->members.length; i++) {
      StructMember* member = aggregate->members.value.p[i];
      if (member != NULL && member->symbol != NULL &&
          !member->is_static && !member->is_member_function &&
          C23ConstexprTypeHasPointerSubobject(member->symbol->type)) {
        return true;
      }
    }
  }
  return false;
}

// True when every pointer subobject of `type` is initialized to null in
// `initializer`, including aggregate and designated initializers.
static bool C23ConstexprPointerSubobjectsAreNull(TypeRecord* type,
                                                 ASTNode* initializer) {
  if (!C23ConstexprTypeHasPointerSubobject(type)) {
    return true;
  }
  if (initializer != NULL && initializer->op == AST_OP(designated_init)) {
    DesignatedInitializerASTNode* designated =
        (DesignatedInitializerASTNode*)initializer;
    TypeRecord* selected = type;
    for (size_t i = 0;
         selected != NULL && designated->designators != NULL &&
         i < designated->designators->length; i++) {
      Designator* designator = designated->designators->value.p[i];
      if (designator->designator_type == kDesignatorArray &&
          TypeIsArray(selected)) {
        selected = selected->next;
      } else if (designator->designator_type == kDesignatorStruct &&
                 TypeIsStructOrUnion(selected)) {
        StructMember* member =
            designator->is_resolved_member
                ? designator->value.struct_member
                : FindStructMemberByName(
                      selected->info.struct_info,
                      designator->value.struct_member_name->value);
        selected =
            member != NULL && member->symbol != NULL ? member->symbol->type
                                                      : NULL;
      } else {
        selected = NULL;
      }
    }
    return selected != NULL &&
           C23ConstexprPointerSubobjectsAreNull(selected, designated->init);
  }
  if (TypeIsPointer(type)) {
    return C23InitializerIsNullPointer(initializer);
  }
  if (initializer != NULL && initializer->op == AST_OP(init)) {
    initializer = ((BinaryASTNode*)initializer)->right;
  }
  if (initializer != NULL && initializer->op == AST_OP(expr_init)) {
    initializer = ((ExpressionInitializerASTNode*)initializer)->expr;
  }
  if (initializer == NULL || initializer->op != AST_OP(braced_init)) {
    return false;
  }
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)initializer;
  if (TypeIsArray(type)) {
    for (size_t i = 0; i < braced->initializers->length; i++) {
      if (!C23ConstexprPointerSubobjectsAreNull(
              type->next, braced->initializers->value.p[i])) {
        return false;
      }
    }
    return true;
  }
  Struct* aggregate = type->info.struct_info;
  size_t initializer_index = 0;
  for (size_t i = 0; i < aggregate->members.length; i++) {
    StructMember* member = aggregate->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function) {
      continue;
    }
    ASTNode* member_initializer =
        initializer_index < braced->initializers->length
            ? braced->initializers->value.p[initializer_index]
            : NULL;
    if (member_initializer == NULL) {
      // Omitted aggregate members are zero-initialized.
      initializer_index++;
      continue;
    }
    if (member_initializer->op == AST_OP(designated_init)) {
      if (!C23ConstexprPointerSubobjectsAreNull(type, member_initializer)) {
        return false;
      }
    } else if (!C23ConstexprPointerSubobjectsAreNull(member->symbol->type,
                                                     member_initializer)) {
      return false;
    }
    initializer_index++;
    if (aggregate->is_union) {
      break;
    }
  }
  return true;
}

// True when the integer constant in `initializer` fits exactly in integral
// `type`; returns true when the initializer is not a foldable integer ICE.
static bool C23ConstexprIntegerConstantIsRepresentable(TypeRecord* type,
                                                       ASTNode* initializer) {
  if (type == NULL || !TypeIsIntegral(type) || initializer == NULL) {
    return true;
  }
  if (initializer->op == AST_OP(init)) {
    initializer = ((BinaryASTNode*)initializer)->right;
  }
  if (initializer != NULL && initializer->op == AST_OP(expr_init)) {
    initializer = ((ExpressionInitializerASTNode*)initializer)->expr;
  }
  if (initializer != NULL && initializer->op == AST_OP(braced_init)) {
    BracedInitializerASTNode* braced = (BracedInitializerASTNode*)initializer;
    if (braced->initializers->length != 1) {
      return true;
    }
    initializer = braced->initializers->value.p[0];
    if (initializer != NULL && initializer->op == AST_OP(expr_init)) {
      initializer = ((ExpressionInitializerASTNode*)initializer)->expr;
    }
  }
  if (initializer == NULL) {
    return true;
  }
  initializer = AnalyzeExpression(initializer);
  int64_t value = 0;
  if (!EvaluateIntegerExpression(initializer, &value)) {
    return true;
  }
  if (TypeIsBool(type)) {
    return value == 0 || value == 1;
  }
  int bits = TypeIsBitInt(type) ? type->bit_width : type->size * 8;
  if (bits <= 0 || bits >= 64) {
    return !TypeIsUnsigned(type) || value >= 0;
  }
  if (TypeIsUnsigned(type)) {
    uint64_t maximum = (UINT64_C(1) << bits) - 1;
    return value >= 0 && (uint64_t)value <= maximum;
  }
  int64_t minimum = -(INT64_C(1) << (bits - 1));
  int64_t maximum = (INT64_C(1) << (bits - 1)) - 1;
  return value >= minimum && value <= maximum;
}

// True when a `float` constexpr initializer's value is exactly representable
// as `float`; returns true for non-float types or non-foldable constants.
static bool C23ConstexprFloatingConstantIsRepresentable(TypeRecord* type,
                                                        ASTNode* initializer) {
  if (type == NULL || !TypeIsFloatingPoint(type) || initializer == NULL ||
      !TypeIsFloat(type)) {
    return true;
  }
  if (initializer->op == AST_OP(init)) {
    initializer = ((BinaryASTNode*)initializer)->right;
  }
  if (initializer != NULL && initializer->op == AST_OP(expr_init)) {
    initializer = ((ExpressionInitializerASTNode*)initializer)->expr;
  }
  if (initializer == NULL) {
    return true;
  }
  initializer = AnalyzeExpression(initializer);
  double value = 0;
  if (TypeIsIntegral(initializer->type)) {
    int64_t integer_value = 0;
    if (!EvaluateIntegerExpression(initializer, &integer_value)) {
      return true;
    }
    value = (double)integer_value;
  } else if (!EvaluateFloatingPointExpression(initializer, &value)) {
    return true;
  }
  float narrowed = (float)value;
  return (double)narrowed == value;
}

// Emits diagnostics when a C23 `constexpr` object `sym` has a disallowed
// type or an initializer that violates constexpr object constraints.
void ValidateC23ConstexprObject(Syntax* syntax, Symbol* sym,
                                ASTNode* initializer) {
  if (CompilerIsCXX() || !CompilerCAtLeast(kLanguageStandardC23) ||
      sym == NULL || !sym->flags.is_constexpr || TypeIsFunction(sym->type)) {
    return;
  }
  if (!C23ConstexprObjectTypeIsAllowed(sym->type)) {
    SyntaxError(
        syntax,
        "C constexpr object cannot have variably modified, atomic, volatile, "
        "or restrict-qualified type");
  }
  if (initializer != NULL &&
      !C23ConstexprPointerSubobjectsAreNull(sym->type, initializer)) {
    SyntaxError(syntax,
                "C constexpr pointer subobjects must be initialized to null");
  }
  if (!C23ConstexprIntegerConstantIsRepresentable(sym->type, initializer)) {
    SyntaxError(
        syntax,
        "C constexpr initializer is not exactly representable in its type");
  }
  if (!C23ConstexprFloatingConstantIsRepresentable(sym->type, initializer)) {
    SyntaxError(
        syntax,
        "C constexpr initializer is not exactly representable in its type");
  }
}

// True when `sym` names `main` at global scope (including a non-member
// function), as opposed to a class member named `main`.
static bool IsGlobalMainSymbol(Symbol* sym) {
  if (sym == NULL || !StringEqual(&sym->name, "main") ||
      sym->namespace_ != NULL) {
    return false;
  }
  return !TypeIsFunction(sym->type) ||
         sym->type->info.function.cxx_member_owner == NULL;
}

// Applies explicit module C++ linkage from `syntax->explicit_cxx_linkage` to
// `sym`, clearing module privacy and setting internal or external linkage.
void ApplyExplicitCXXLinkageAttachment(Syntax* syntax, Symbol* sym,
                                       Storage storage) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX20) ||
      !syntax->explicit_cxx_linkage || sym == NULL) {
    return;
  }
  SymbolSetModuleIdentity(sym, NULL, NULL, SymbolImportSourceModule(sym));
  sym->flags.is_module_private = false;
  sym->cxx_linkage = StorageIs(storage, STO(static)) ? kCXXLinkageInternal
                                                      : kCXXLinkageExternal;
}

// Emits C++26 diagnostics when `sym` is an ill-formed global `main`
// declaration (linkage, storage, constexpr, modules, or template).
void ValidateCXX26MainDeclaration(Syntax* syntax, Symbol* sym,
                                  Storage storage) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX26) || sym == NULL ||
      !StringEqual(&sym->name, "main")) {
    return;
  }
  if (sym->flags.is_c_linkage) {
    SyntaxError(syntax, "an entity named 'main' cannot have C language linkage");
    return;
  }
  if (!IsGlobalMainSymbol(sym)) {
    return;
  }
  if (!TypeIsFunction(sym->type)) {
    SyntaxError(syntax, "a variable named 'main' cannot belong to global scope");
    return;
  }
  FunctionInfo* info = &sym->type->info.function;
  if (sym->flags.is_template) {
    SyntaxError(syntax, "'main' cannot be a function template");
  }
  if (StorageIs(storage, STO(static))) {
    SyntaxError(syntax, "'main' cannot be declared static");
  }
  if (info->is_inline) {
    SyntaxError(syntax, "'main' cannot be declared inline");
  }
  if (info->is_constexpr) {
    SyntaxError(syntax, "'main' cannot be declared constexpr");
  }
  if (info->is_consteval) {
    SyntaxError(syntax, "'main' cannot be declared consteval");
  }
  if (SymbolHasOwningModule(sym)) {
    SyntaxError(syntax, "'main' cannot be attached to a named module");
  }
}

// Emits a diagnostic when global `main` is defined as deleted under C++26.
void ValidateCXX26DeletedMain(Syntax* syntax, Symbol* sym) {
  if (CompilerCXXAtLeast(kLanguageStandardCXX26) &&
      IsGlobalMainSymbol(sym) && TypeIsFunction(sym->type) &&
      sym->type->info.function.is_deleted) {
    SyntaxError(syntax, "'main' cannot be defined as deleted");
  }
}

// [basic.link]/3.2: the first declaration of a namespace-scope variable of
// const-qualified type that is neither extern, inline, exported, nor a template
// has internal linkage.
bool CXXConstVariableHasInternalLinkage(Syntax* syntax,
                                        TypeParser* parser, Symbol* sym,
                                        Symbol* old_sym) {
  if (!CompilerIsCXX() || old_sym != NULL || TypeIsFunction(sym->type) ||
      sym->storage != STO(implicit) || parser->is_inline ||
      parser->cxx_member_definition != NULL ||
      parser->cxx_qualified_definition_namespace != NULL ||
      sym->flags.is_exported || sym->flags.is_c_linkage ||
      sym->flags.is_template || syntax->parsing_template_declaration ||
      syntax->parsing_template_specialization) {
    return false;
  }
  TypeRecord* object = sym->type;
  while (TypeIsArray(object) && object->next != NULL) {
    object = object->next;
  }
  return TypeIsConst(object) && !TypeIsVolatile(object);
}

// [basic.link]/4: a namespace-scope function or variable declared in an
// unnamed namespace has internal linkage, even when declared `extern`
// (abseil's `extern const uint64_t kPower10MantissaHighTable[];` ahead of its
// definition).  Its name is mangled with the translation unit's own
// unnamed-namespace ordinal, so an external symbol collides with another
// TU's unrelated `(anonymous)::GetPageSize()`.
bool CXXDeclarationInUnnamedNamespace(Syntax* syntax,
                                      TypeParser* parser, Symbol* sym,
                                      Storage storage) {
  if (!CompilerIsCXX() || sym == NULL || sym->type == NULL ||
      (storage != STO(implicit) && storage != STO(extern)) ||
      sym->storage != storage ||
      parser->cxx_member_definition != NULL ||
      parser->cxx_qualified_definition_namespace != NULL ||
      sym->flags.is_c_linkage ||
      (TypeIsFunction(sym->type) &&
       sym->type->info.function.cxx_member_owner != NULL)) {
    return false;
  }
  for (Namespace* ns = syntax->current_namespace; ns != NULL;
       ns = ns->parent) {
    if (ns->is_anonymous) {
      return true;
    }
  }
  return false;
}

// Apple SDK headers spell asm labels as Mach-O symbols, so
// __DARWIN_ALIAS(regcomp) is asm("_regcomp"); the Mach-O writer adds that
// underscore itself.  DaveCC's own headers and runtime use the ELF name.
bool AsmLabelIsMachOSymbolName(Syntax* syntax) {
  if (compiler == NULL || !compiler->native_object) {
    return false;
  }
  Source* source = syntax->lex->source;
  return source != NULL && source->is_system_header &&
         source->filename.value != NULL &&
         strstr(source->filename.value, ".sdk/") != NULL;
}
