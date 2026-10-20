//
//  syntax_contracts.c
//  c_compiler
//
//  C++26 contract specifiers (pre, post) and their redeclaration rules.
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

// True when the current token starts a C++26 contract specifier (`pre` or
// `post`), setting `*kind` accordingly.
static bool LookingAtContractSpecifier(Syntax* syntax,
                                       ContractAssertionKind* kind) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX26) ||
      !LexLookingAt(syntax->lex, TOK(identifier))) {
    return false;
  }
  if (strcmp(syntax->lex->spelling.value, "pre") == 0) {
    *kind = kContractPrecondition;
    return true;
  }
  if (strcmp(syntax->lex->spelling.value, "post") == 0) {
    *kind = kContractPostcondition;
    return true;
  }
  return false;
}

typedef struct {
  Symbol* formal;
  bool used;
} ContractFormalUse;

// Visitor callback: sets `use->used` when the predicate references `use->formal`.
static void FindContractFormalUse(ASTNode* node, void* data, int child_id,
                                  VisitorMode mode) {
  (void)child_id;
  ContractFormalUse* use = data;
  if (mode == kVisitPreChildren && node->op == AST_OP(identifier) &&
      ((IdentifierASTNode*)node)->symbol == use->formal) {
    use->used = true;
  }
}

// Emits an error for each non-const, non-reference parameter of `func` that
// `predicate` references in a postcondition.
static void DiagnosePostconditionParameterConst(Syntax* syntax,
                                                TypeRecord* func,
                                                ASTNode* predicate) {
  Vector* formals = &func->info.function.prototype;
  for (size_t i = 0; i < formals->length; i++) {
    Symbol* formal = formals->value.p[i];
    if (formal == NULL || formal->name.length == 0 ||
        StringEqual(&formal->name, "this") ||
        TypeIsReference(formal->type) || TypeIsConst(formal->type)) {
      continue;
    }
    ContractFormalUse use = {
        .formal = formal,
    };
    ASTNodeVisit(predicate, FindContractFormalUse, 0, &use);
    if (use.used) {
      SyntaxError(syntax,
                  "non-reference parameter '%s' used by a postcondition "
                  "must have const type",
                  formal->name.value);
    }
  }
}

// Parses an optional postcondition result binding `name:`, or NULL when the
// next tokens do not form that syntax (lexer position restored on failure).
static Symbol* ParseContractResultBinding(Syntax* syntax) {
  if (!LexLookingAt(syntax->lex, TOK(identifier))) {
    return NULL;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  String name = {0};
  StringInit(&name, syntax->lex->spelling.value);
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);

  Vector attributes = {0};
  VectorInit(&attributes);
  SyntaxParseCXXAttributes(syntax, &attributes);
  if (!LexMatch(syntax->lex, TOK(colon))) {
    AttributeListDestruct(&attributes);
    StringDestruct(&name);
    LexCheckpointRestore(syntax->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
    return NULL;
  }
  LexCheckpointDestruct(&checkpoint);

  TypeRecord* unknown =
      NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualConst);
  Symbol* binding = NewSymbol(name.value, unknown, STO(auto));
  binding->location = location;
  binding->flags.is_local = true;
  binding->flags.is_block_scope = true;
  SymbolAppendAttributes(binding, &attributes);
  VectorDestruct(&attributes);
  StringDestruct(&name);
  return binding;
}

// Consumes leading contract specifiers on `func`, parsing predicates into
// `func->info.function.contract_assertions` and opening scope for formals.
void SyntaxParseFunctionContracts(Syntax* syntax, TypeRecord* func,
                                  Struct* member_owner,
                                  bool add_implicit_this) {
  if (func == NULL || !TypeIsFunction(func)) {
    return;
  }
  ContractAssertionKind kind;
  if (LookingAtContractSpecifier(syntax, &kind) &&
      add_implicit_this && member_owner != NULL &&
      !func->info.function.has_explicit_object_parameter &&
      !FunctionHasImplicitThisParameter(func)) {
    TypeRecordAddCXXThisParameter(func, member_owner,
                                  syntax->lex->current_token_location);
  }
  while (LookingAtContractSpecifier(syntax, &kind)) {
    SourceLocation location = syntax->lex->current_token_location;
    LexNextToken(syntax->lex);

    Vector attributes = {0};
    VectorInit(&attributes);
    SyntaxParseCXXAttributes(syntax, &attributes);
    SyntaxNeedBracket(syntax, TOK(lparen), TC(closebra));

    SyntaxOpenScope(syntax);
    if (member_owner != NULL) {
      SyntaxInsertClassMembersForConstraint(syntax, member_owner);
    }
    for (size_t i = 0; i < func->info.function.prototype.length; i++) {
      Symbol* formal = func->info.function.prototype.value.p[i];
      if (formal != NULL && formal->name.length > 0) {
        InsertLocalSymbol(syntax->local_symbol_stack, formal);
      }
    }

    Symbol* result_binding = NULL;
    if (kind == kContractPostcondition) {
      result_binding = ParseContractResultBinding(syntax);
      if (result_binding != NULL &&
          !InsertLocalSymbol(syntax->local_symbol_stack, result_binding)) {
        SyntaxError(syntax,
                    "postcondition result name '%s' conflicts with a parameter",
                    result_binding->name.value);
      }
    }
    ASTNode* predicate =
        SyntaxParseSingleExpression(syntax, TC(closebra));
    SyntaxCloseScope(syntax);
    SyntaxNeedBracket(syntax, TOK(rparen), TC(closebra));

    if (predicate == NULL) {
      SymbolDelete(result_binding);
      AttributeListDestruct(&attributes);
      continue;
    }
    if (kind == kContractPostcondition) {
      DiagnosePostconditionParameterConst(syntax, func, predicate);
    }
    VectorAppend(
        &FunctionMutableExtra(&func->info.function)->contract_assertions,
        NewContractAssertion(kind, predicate, result_binding, &attributes,
                             location));
  }
}

// Reports errors when `func` has contract assertions but is virtual, deleted,
// or otherwise not permitted to carry them under the active language mode.
void SyntaxDiagnoseInvalidFunctionContracts(Syntax* syntax, TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func) ||
      FunctionExtraOf(&func->info.function)->contract_assertions.length == 0) {
    return;
  }
  FunctionInfo* info = &func->info.function;
  if ((info->is_virtual || info->is_override) &&
      !CompilerCXXAtLeast(kLanguageStandardCXX29)) {
    SyntaxError(syntax, "virtual functions cannot have contract assertions");
  }
  if (info->is_deleted || info->is_explicitly_deleted) {
    SyntaxError(syntax, "deleted functions cannot have contract assertions");
  }
}

typedef struct {
  uint64_t value;
  ContractAssertion* assertion;
} ContractExpressionHash;

// Mixes `length` bytes from `bytes` into contract fingerprint `hash`.
static void HashContractBytes(ContractExpressionHash* hash,
                              const void* bytes, size_t length) {
  const unsigned char* p = bytes;
  for (size_t i = 0; i < length; i++) {
    hash->value ^= p[i];
    hash->value *= UINT64_C(1099511628211);
  }
}

// Mixes the contents of `string` into `hash` when it is non-NULL.
static void HashContractString(ContractExpressionHash* hash,
                               const String* string) {
  if (string != NULL) {
    HashContractBytes(hash, string->value, string->length);
  }
}

// Visitor callback: contributes node shape, constants, and stable symbol
// identities to the contract expression hash in `data`.
static void HashContractExpressionNode(ASTNode* node, void* data,
                                       int child_id, VisitorMode mode) {
  if (mode != kVisitPreChildren) {
    return;
  }
  ContractExpressionHash* hash = data;
  HashContractBytes(hash, &node->op, sizeof(node->op));
  HashContractBytes(hash, &child_id, sizeof(child_id));
  if (node->op == AST_OP(identifier)) {
    Symbol* symbol = ((IdentifierASTNode*)node)->symbol;
    if (symbol == hash->assertion->result_binding) {
      const unsigned char result_marker = 0xf1;
      HashContractBytes(hash, &result_marker, sizeof(result_marker));
    } else if (symbol != NULL && symbol->flags.is_argument) {
      HashContractBytes(hash, &symbol->value.arg_number,
                        sizeof(symbol->value.arg_number));
    } else if (symbol != NULL) {
      HashContractString(hash, &symbol->name);
    }
  } else if (ASTNodeIsIntConstant(node)) {
    ConstantASTNode* constant = (ConstantASTNode*)node;
    HashContractBytes(hash, &constant->value.ivalue,
                      sizeof(constant->value.ivalue));
  } else if (node->op == AST_OP(fnumber)) {
    ConstantASTNode* constant = (ConstantASTNode*)node;
    HashContractBytes(hash, &constant->value.fvalue,
                      sizeof(constant->value.fvalue));
  } else if (node->op == AST_OP(string) ||
             node->op == AST_OP(string_wide)) {
    HashContractString(hash, ((ConstantASTNode*)node)->value.string);
  } else if (node->op == AST_OP(structmember)) {
    StructMember* member = ((StructMemberASTNode*)node)->member;
    if (member != NULL && member->symbol != NULL) {
      HashContractString(hash, &member->symbol->name);
    }
  }
}

// Returns a hash fingerprint of `assertion`'s predicate for redeclaration
// comparison, treating the result binding distinctly from other identifiers.
static uint64_t ContractExpressionFingerprint(ContractAssertion* assertion) {
  ContractExpressionHash hash = {
      .value = UINT64_C(1469598103934665603),
      .assertion = assertion,
  };
  ASTNodeVisit(assertion->predicate, HashContractExpressionNode, 0, &hash);
  return hash.value;
}

// When a redeclaration omits contract specifiers, checks that loosened
// parameter const-qualification still satisfies old postconditions.
static void DiagnoseOmittedPostconditionParameterConst(
    Syntax* syntax, TypeRecord* old_func, TypeRecord* new_func) {
  Vector* old_formals = &old_func->info.function.prototype;
  Vector* new_formals = &new_func->info.function.prototype;
  const Vector* assertions =
      &FunctionExtraOf(&old_func->info.function)->contract_assertions;
  for (size_t i = 0; i < old_formals->length && i < new_formals->length; i++) {
    Symbol* old_formal = old_formals->value.p[i];
    Symbol* new_formal = new_formals->value.p[i];
    if (old_formal == NULL || new_formal == NULL ||
        old_formal->name.length == 0 ||
        StringEqual(&old_formal->name, "this") ||
        TypeIsReference(new_formal->type) ||
        TypeIsConst(new_formal->type)) {
      continue;
    }
    for (size_t j = 0; j < assertions->length; j++) {
      ContractAssertion* assertion = assertions->value.p[j];
      if (assertion->kind != kContractPostcondition) {
        continue;
      }
      ContractFormalUse use = {
          .formal = old_formal,
      };
      ASTNodeVisit(assertion->predicate, FindContractFormalUse, 0, &use);
      if (use.used) {
        SyntaxError(syntax,
                    "non-reference parameter '%s' used by a postcondition "
                    "must have const type",
                    new_formal->name.value);
        break;
      }
    }
  }
}

// Validates and merges contract assertions and `indeterminate` parameter
// attributes from `old_sym` onto `new_sym` during function redeclaration.
void MergeCXXContractAssertions(Syntax* syntax, Symbol* old_sym,
                                Symbol* new_sym) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX26) || old_sym == NULL ||
      new_sym == NULL || !TypeIsFunction(old_sym->type) ||
      !TypeIsFunction(new_sym->type)) {
    return;
  }
  Vector* old_parameters = &old_sym->type->info.function.prototype;
  Vector* new_parameters = &new_sym->type->info.function.prototype;
  for (size_t i = 0;
       i < old_parameters->length && i < new_parameters->length; i++) {
    Symbol* old_parameter = old_parameters->value.p[i];
    Symbol* new_parameter = new_parameters->value.p[i];
    if (old_parameter == NULL || new_parameter == NULL) {
      continue;
    }
    bool first_is_indeterminate =
        SymbolHasAttribute(old_parameter, "indeterminate");
    bool redeclaration_is_indeterminate =
        SymbolHasAttribute(new_parameter, "indeterminate");
    if (!first_is_indeterminate && redeclaration_is_indeterminate) {
      SyntaxError(syntax,
                  "'indeterminate' on a parameter must appear on the first "
                  "declaration of '%s'",
                  new_sym->name.value);
    } else if (first_is_indeterminate &&
               !redeclaration_is_indeterminate) {
      SymbolAddAttribute(new_parameter, NewAttribute("indeterminate"));
    }
  }
  const Vector* old_assertions =
      &FunctionExtraOf(&old_sym->type->info.function)->contract_assertions;
  const Vector* new_assertions =
      &FunctionExtraOf(&new_sym->type->info.function)->contract_assertions;
  // A redeclaration may omit the function-contract-specifier-seq.  When it
  // supplies one, however, it must correspond to the sequence on the reachable
  // first declaration.
  if (new_assertions->length == 0) {
    DiagnoseOmittedPostconditionParameterConst(
        syntax, old_sym->type, new_sym->type);
    TypeRecordCopyContractAssertions(new_sym->type, old_sym->type);
    return;
  }
  if (old_assertions->length != new_assertions->length) {
    SyntaxError(syntax,
                "redeclarations of '%s' have different contract assertions",
                new_sym->name.value);
    return;
  }
  for (size_t i = 0; i < old_assertions->length; i++) {
    ContractAssertion* old_assertion = old_assertions->value.p[i];
    ContractAssertion* new_assertion = new_assertions->value.p[i];
    if (old_assertion->kind != new_assertion->kind ||
        (old_assertion->result_binding != NULL) !=
            (new_assertion->result_binding != NULL) ||
        ContractExpressionFingerprint(old_assertion) !=
            ContractExpressionFingerprint(new_assertion)) {
      SyntaxError(syntax,
                  "redeclarations of '%s' have non-corresponding "
                  "contract assertions",
                  new_sym->name.value);
      return;
    }
  }
}
