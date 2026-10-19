//
//  syntax_function_decl.c
//  c_compiler
//
//  Function declarations and definitions: declaring or defining a function
//  symbol, default arguments, and = default / = delete.
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

// Deep-clone an initializer so that the same value can be used for each index
// of a GCC range designator.
ASTNode* CloneInitializer(ASTNode* init) {
  if (init->op == AST_OP(expr_init)) {
    ExpressionInitializerASTNode* e = (ExpressionInitializerASTNode*)init;
    return NewExpressionInitializerASTNode(
        ASTNodeClone(e->expr, IdentityCloneNode, NULL, NULL), init->location);
  }
  return ASTNodeClone(init, IdentityCloneNode, NULL, NULL);
}

// Deep-clones a C++ default-argument initializer AST node.
static ASTNode* CloneCXXDefaultArgument(ASTNode* arg) {
  return ASTNodeClone(arg, IdentityCloneNode, NULL, NULL);
}

// Copies default-argument initializers between matching formals of `old_sym`
// and `new_sym` on redeclaration; diagnoses duplicate defaults on the same
// parameter.
void MergeCXXDefaultArguments(Syntax* syntax, Symbol* old_sym,
                              Symbol* new_sym) {
  if (!CompilerIsCXX() || old_sym == NULL || new_sym == NULL ||
      old_sym->type == NULL || new_sym->type == NULL ||
      !TypeIsFunction(old_sym->type) || !TypeIsFunction(new_sym->type) ||
      old_sym->type->info.function.prototype.length !=
          new_sym->type->info.function.prototype.length) {
    return;
  }
  for (size_t i = 0; i < old_sym->type->info.function.prototype.length; i++) {
    Symbol* old_formal = old_sym->type->info.function.prototype.value.p[i];
    Symbol* new_formal = new_sym->type->info.function.prototype.value.p[i];
    if (old_formal == NULL || new_formal == NULL) {
      continue;
    }
    if (SymbolExtraOf(old_formal)->default_argument != NULL &&
        SymbolExtraOf(new_formal)->default_argument != NULL) {
      SyntaxError(syntax, "default argument already specified");
      continue;
    }
    if (SymbolExtraOf(old_formal)->default_argument == NULL &&
        SymbolExtraOf(new_formal)->default_argument != NULL) {
      SymbolMutableExtra(old_formal)->default_argument =
          CloneCXXDefaultArgument(SymbolExtraOf(new_formal)->default_argument);
    } else if (SymbolExtraOf(old_formal)->default_argument != NULL &&
               SymbolExtraOf(new_formal)->default_argument == NULL) {
      SymbolMutableExtra(new_formal)->default_argument =
          CloneCXXDefaultArgument(SymbolExtraOf(old_formal)->default_argument);
    }
  }
}

// After `= delete`, optionally parses C++26 `( "reason" )` into
// `func->info.function.deleted_reason`, consuming the closing `)`.
void SyntaxParseCXXDeletedFunctionReason(Syntax* syntax, TypeRecord* func) {
  if (!LexMatch(syntax->lex, TOK(lparen))) {
    return;
  }
  if (!CompilerCXXAtLeast(kLanguageStandardCXX26)) {
    SyntaxError(syntax, "deleted function reasons require C++26");
  }
  if (!LexLookingAtStringLiteral(syntax->lex)) {
    SyntaxError(syntax,
                "deleted function reason must be an unevaluated string");
    if (!LexLookingAt(syntax->lex, TOK(rparen)) &&
        !LexLookingAt(syntax->lex, TOK(semicolon))) {
      LexNextToken(syntax->lex);
    }
  } else {
    if (LexValidateUnevaluatedString(
            syntax->lex, "deleted function reason",
            /*allow_user_defined_suffix=*/false)) {
      if (FunctionExtraOf(&func->info.function)->deleted_reason != NULL) {
        StringDelete(FunctionExtraOf(&func->info.function)->deleted_reason);
      }
      FunctionMutableExtra(&func->info.function)->deleted_reason =
          NewStringWithLength(syntax->lex->spelling.value,
                              syntax->lex->spelling.length);
    }
    LexNextToken(syntax->lex);
  }
  SyntaxNeedBracket(syntax, TOK(rparen), TC(decl));
}

// When `=` follows the declarator, sets `= default` or `= delete` on `func` and
// parses an optional deleted reason.  `is_first_declaration` controls implicit
// inline on default and contract checks.
void ParseCXXDefaultDeleteFunctionSpecifier(Syntax* syntax,
                                            TypeRecord* func,
                                            bool is_first_declaration) {
  if (!CompilerIsCXX() || func == NULL || !TypeIsFunction(func) ||
      !LexMatch(syntax->lex, TOK(equal))) {
    return;
  }
  if (LexMatch(syntax->lex, TOK(default))) {
    func->info.function.is_defaulted = true;
    func->info.function.is_explicitly_defaulted = true;
    func->info.function.is_constexpr_eligible = true;
    // Only a function defaulted on its first declaration is implicitly inline.
    // An out-of-line `Test::~Test() = default` keeps the linkage of the
    // declaration; marking it inline here drops the strong definition.
    if (is_first_declaration) {
      func->info.function.is_inline = true;
    }
    if (is_first_declaration &&
        FunctionExtraOf(&func->info.function)->contract_assertions.length !=
            0) {
      SyntaxError(syntax,
                  "a function defaulted on its first declaration cannot "
                  "have contract assertions");
    }
    return;
  }
  if (LexMatch(syntax->lex, TOK(delete))) {
    func->info.function.is_deleted = true;
    func->info.function.is_explicitly_deleted = true;
    SyntaxParseCXXDeletedFunctionReason(syntax, func);
    SyntaxDiagnoseInvalidFunctionContracts(syntax, func);
    return;
  }
  SyntaxError(syntax, "function specifier must be '= default' or '= delete'");
}

// Parses the parameter declarations of an old-style (K&R) function
// definition, which sit between the declarator and the body's `{`, and
// resolves each against the identifier list of `sym`'s prototype.
static void ParseOldStyleParameterDeclarations(Syntax* syntax, Symbol* sym) {
  // Stop at end of input as well: the recovery below cannot move past it, so
  // an unparsable argument declaration would repeat forever.
  while (!LexLookingAt(syntax->lex, TOK(lbrace)) && !LexEof(syntax->lex)) {
    Token token_before = syntax->lex->current_token;
    SourceLocation location_before = syntax->lex->current_token_location;
    int errors_before = NumErrors();
    TypeParser arg_parser;
    TypeParserInit(&arg_parser, syntax->lex, syntax, STO(auto), kParsingBlockScope);
    TypeRecord* arg_type = TypeParserParseType(&arg_parser, true);
    if (arg_type == NULL) {
      SyntaxError(syntax, "Type expected");
      SyntaxRecover(syntax, TC(semicolon));
    } else {
      while (!LexLookingAt(syntax->lex, TOK(semicolon))) {
        Symbol* formal = TypeParserParseDeclarator(&arg_parser, arg_type);
        if (formal != NULL) {
          // Resolve the formal argument in the function prototype.
          ResolveOldStyleFormalArgument(syntax, sym->type, formal);
        }
        if (!LexMatch(syntax->lex, TOK(comma))) {
          break;
        }
      }
    }
    SyntaxNeedSemicolon(syntax, TC(openbra));
    TypeParserDestruct(&arg_parser);
    // Waiting for '{', so any other stuck token (including ')') must move.
    SyntaxEnsureProgress(syntax, token_before, location_before, errors_before,
                         TC(stmt));
  }
  // We need a function body after the argument declarations.
  if (!LexLookingAt(syntax->lex, TOK(lbrace))) {
    SyntaxError(syntax, "Function body expected");
  }
}

// Parses a constructor's mem-initializer-list (`: base(x), member{y}`) into
// `cxx_initializers`.  The initializers are parsed in the constructor's
// function scope so they can name its parameters.  Does nothing when no
// list follows.
static void ParseFunctionConstructorInitializers(
    Syntax* syntax, Symbol* sym, CXXConstructorInitList* cxx_initializers) {
  bool constructor_scope_open =
      CompilerIsCXX() && TypeIsFunction(sym->type) &&
      sym->type->info.function.is_constructor;
  ParserContext constructor_old_context = syntax->context;
  TypeRecord* constructor_old_function = compiler->current_function;
  if (constructor_scope_open) {
    syntax->context = kParsingBlockScope;
    compiler->current_function = sym->type;
    SyntaxOpenScope(syntax);
    AddFunctionScopeSymbols(syntax, sym->type);
  }
  SyntaxParseCXXConstructorInitializerList(syntax, sym->type,
                                           cxx_initializers);
  if (constructor_scope_open) {
    SyntaxCloseScope(syntax);
    compiler->current_function = constructor_old_function;
    syntax->context = constructor_old_context;
  }
}

// Synthesizes the body of a defaulted comparison operator or defaulted
// postfix operator `sym` (which may turn out deleted), and copies its deleted
// state to the previous declaration `old_sym`.
static void SynthesizeDefaultedOperatorDefinition(Syntax* syntax, Symbol* sym,
                                                  Symbol* old_sym) {
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                 kParsingFileScope);
  SynthesizeDefaultedMemberFunctionBody(&parser, sym);
  TypeParserDestruct(&parser);
  if (old_sym != NULL && TypeIsFunction(old_sym->type)) {
    old_sym->type->info.function.is_deleted =
        sym->type->info.function.is_deleted;
    old_sym->type->info.function.is_implicitly_deleted =
        sym->type->info.function.is_implicitly_deleted;
  }
}

// Builds the body of a defaulted special member function `sym`: the
// constructor preamble (base and member initialization), memberwise
// assignment and `return *this`, and member and base destruction, as applies.
static void BuildDefaultedSpecialMemberBody(
    Syntax* syntax, Symbol* sym, CXXConstructorInitList* cxx_initializers) {
  ParserContext old_context = syntax->context;
  syntax->context = kParsingBlockScope;
  SyntaxOpenScope(syntax);
  AddFunctionScopeSymbols(syntax, sym->type);
  sym->flags.is_defined = true;
  sym->flags.is_inline_defn = true;
  // `inline Test::~Test() = default;` in a header is defined in every
  // includer.
  if (CompilerIsCXX() && sym->type->info.function.is_inline &&
      !StorageIs(sym->storage, STO(static))) {
    sym->flags.is_weak = true;
  }
  sym->type->info.function.definition = true;
  sym->type->info.function.is_user_provided = true;
  Vector* body = NewVector();
  SyntaxInsertCXXConstructorPreamble(syntax, sym->type, body,
                                     cxx_initializers, sym->location);
  AppendCXXDefaultedMemberwiseAssignments(sym->type, body, sym->location);
  AppendCXXDefaultedAssignmentReturnThis(sym->type, body, sym->location);
  SyntaxAppendCXXMemberDestructorCalls(syntax, sym->type, body, sym->location);
  AppendCXXBaseDestructorCalls(syntax, sym->type, body, sym->location);
  SyntaxCloseScope(syntax);
  syntax->context = old_context;
  sym->type->info.function.body =
      NewCompoundStatementASTNode(body, sym->location);
  VectorAppend(&compiler->declaration_asts, sym->type->info.function.body);
}

// Defines the `= default` function `sym` (previously declared as `old_sym`,
// if any) by synthesizing its body, and consumes the `;`.  Returns the
// declaration list holding the definition.
static ASTNode* DefineDefaultedFunction(
    Syntax* syntax, Vector* declarations, Symbol* sym, Symbol* old_sym,
    CXXConstructorInitList* cxx_initializers) {
  if (old_sym != NULL && TypeIsFunction(old_sym->type)) {
    old_sym->value.func_defn = sym;
    old_sym->type->info.function.is_defaulted = true;
  }
  bool defaulted_comparison =
      sym->type->info.function.cxx_member_owner != NULL &&
      (StringEqual(&sym->name, "operator==") ||
       StringEqual(&sym->name, "operator<=>"));
  bool defaulted_postfix =
      CXXFunctionIsDefaultedPostfixOperator(sym);
  if (defaulted_comparison || defaulted_postfix) {
    SynthesizeDefaultedOperatorDefinition(syntax, sym, old_sym);
  } else {
    BuildDefaultedSpecialMemberBody(syntax, sym, cxx_initializers);
  }
  SyntaxNeedSemicolon(syntax, TC(decl));
  // An out-of-line `= default` is a definition.  The caller treats a
  // non-null result whose declaration vector is empty as a mix of a
  // definition and a declaration, and the diagnostic lands on the next
  // token (the function after the semicolon).  Record the definition the
  // same way a braced body does.
  VectorAppend(declarations,
               NewVariableDeclarationASTNode(sym, NULL, sym->location));
  return NewDeclarationListASTNode(declarations, sym->location);
}

// Carries properties of the previous declaration `old_sym` over to the
// definition `sym` (weak, C++ inline, constexpr/consteval agreement) and
// makes `old_sym` refer to the definition.
static void InheritDeclarationIntoDefinition(Syntax* syntax, Symbol* sym,
                                             Symbol* old_sym) {
  if (old_sym != NULL && TypeIsFunction(old_sym->type)) {
    old_sym->value.func_defn = sym;
    CheckMatchingConstexprConsteval(syntax, old_sym->type, sym->type);
    if (old_sym->flags.is_weak) {
      sym->flags.is_weak = true;
    }
    if (CompilerIsCXX() && old_sym->type->info.function.is_inline) {
      sym->type->info.function.is_inline = true;
    }
  }
}

// Marks `sym` (and its previous declaration `old_sym`) as defined by a
// user-provided body, and applies the C++ / C inline-definition linkage
// rules.
static void MarkFunctionDefinedWithBody(Symbol* sym, Symbol* old_sym) {
  sym->flags.is_defined = true;
  sym->type->info.function.definition = true;
  // A function body is user-provided.  An out-of-line copy constructor must
  // not memberwise-copy every field: members absent from the
  // mem-initializer-list are default-initialized, and a deleted member copy
  // (`once_flag`) is not invoked.
  sym->type->info.function.is_user_provided = true;
  if (old_sym != NULL && old_sym->type != NULL &&
      TypeIsFunction(old_sym->type)) {
    old_sym->type->info.function.is_user_provided = true;
  }
  if (CompilerIsCXX() && sym->type->info.function.is_inline) {
    sym->flags.is_inline_defn = true;
    if (!StorageIs(sym->storage, STO(static))) {
      sym->flags.is_weak = true;
    }
    if (old_sym != NULL && TypeIsFunction(old_sym->type)) {
      old_sym->type->info.function.is_inline = true;
    }
  } else if (sym->type->info.function.is_inline &&
             (StorageIs(sym->storage, STO(static)) ||
              (old_sym != NULL &&
               StorageIs(old_sym->storage, STO(static))))) {
    // C11 6.7.4p7 inline-definition rules apply only to external linkage;
    // a `static inline` function is an ordinary internal definition.
    sym->flags.is_inline_defn = true;
  }
}

// Parses the declarations and statements of a function body up to (not
// including) its closing `}`, appending them to `body`.  Consteval blocks are
// evaluated as soon as they are parsed.
static void ParseFunctionBodyStatements(Syntax* syntax, Vector* body) {
  bool seen_statement = false;
  while (syntax->lex->current_token != TOK(rbrace) &&
         syntax->lex->current_token != TOK(eof)) {
    ASTNode* stmt;
    if (SyntaxLookingAtDeclaration(syntax)) {
      if (seen_statement) {
        SyntaxWarning(syntax, "declaration-after-statement",
                      "declaration after statement");
      }
      // Declaration.
      stmt = SyntaxParseLocalDeclaration(syntax);
    } else {
      seen_statement = true;
      stmt = SyntaxParseStatement(syntax,TC(semicolon));
    }
    if (stmt != NULL) {
      VectorAppend(body, stmt);
      if (stmt->op == AST_OP(consteval_block) &&
          syntax->parsing_consteval_block_depth == 0) {
        SemanticAnalyzeFunctionConstevalBlockDuringParse(
            (ConstevalBlockASTNode*)stmt, body, body->length);
      }
    }
  }
}

// Parses the body of function definition `sym` (the `{` has been consumed)
// in its own scope, in the namespace of its declaration, and completes it:
// constructor preamble, block-scope local destruction, and member and base
// destruction.  Returns the declaration list holding the definition.
static ASTNode* DefineFunctionWithBody(Syntax* syntax, Vector* declarations,
                                       Symbol* sym, Symbol* old_sym,
                                       CXXConstructorInitList* cxx_initializers,
                                       int definition_errors_before) {
  if (sym->type->info.function.old_style) {
    SyntaxWarning(syntax, "old-style-definition",
                  "old-style function definition");
  }
  InheritDeclarationIntoDefinition(syntax, sym, old_sym);
  ParserContext old_context = syntax->context;
  syntax->context = kParsingBlockScope;
  // `void ns::f() { ... }` is lexically outside `ns`, but unqualified lookup
  // in the body still starts in `ns` (the namespace of the declaration).
  Namespace* saved_namespace = syntax->current_namespace;
  if (old_sym != NULL && old_sym->namespace_ != NULL) {
    syntax->current_namespace = old_sym->namespace_;
  }
  SyntaxOpenScope(syntax);
  AddFunctionScopeSymbols(syntax, sym->type);
  MarkFunctionDefinedWithBody(sym, old_sym);

  // Parse the function body.
  TypeRecord* old_current_function = compiler->current_function;
  compiler->current_function = sym->type;
  Vector* body = NewVector();
  if (compiler->debug_output) {
    VectorAppend(body, SyntaxNewPCLabel(syntax->lex->current_token_location));
  }
  ParseFunctionBodyStatements(syntax, body);
  SyntaxInsertCXXConstructorPreamble(syntax, sym->type, body,
                                     cxx_initializers, sym->location);
  // Destroy the function body's own block-scope locals on fall-through.  A
  // nested compound gets these appended by ParseCompoundStatement, but the
  // outermost function-body block is assembled here by hand, so append them
  // explicitly -- before the class member/base destructors so that, in a
  // destructor body, locals are destroyed before the enclosing object's
  // members and bases.  Coroutines are skipped: their locals live in the
  // coroutine frame and are destroyed by the lowered frame-cleanup logic.
  if (sym->type == NULL || !TypeIsFunction(sym->type) ||
      !sym->type->info.function.is_coroutine) {
    SyntaxAppendCXXBlockScopeDestructors(body);
  }
  SyntaxAppendCXXMemberDestructorCalls(syntax, sym->type, body, sym->location);
  AppendCXXBaseDestructorCalls(syntax, sym->type, body, sym->location);
  SyntaxCloseScope(syntax);
  syntax->current_namespace = saved_namespace;
  syntax->context = old_context;
  compiler->current_function = old_current_function;

  if (compiler->debug_output) {
    VectorAppend(body, SyntaxNewPCLabel(syntax->lex->current_token_location));
  }
  sym->type->info.function.body =
      NewCompoundStatementASTNode(body, syntax->lex->current_token_location);
  sym->type->info.function.body_had_parse_errors =
      NumErrors() != definition_errors_before;
  // The body is hung off the function type, not reachable from the
  // declaration AST, so register it as a teardown root of its own.
  VectorAppend(&compiler->declaration_asts, sym->type->info.function.body);
  SyntaxNeedBracket(syntax, TOK(rbrace), TC(decl));
  ASTNode* decl = NewVariableDeclarationASTNode(sym, NULL,
                                                syntax->lex->current_token_location);
  VectorAppend(declarations, decl);
  return NewDeclarationListASTNode(declarations,
                                   syntax->lex->current_token_location);
}

// Completes the declaration of function `sym`, whose declarator has been
// parsed and which redeclares `old_sym` (or NULL).  Parses what follows the
// declarator: old-style parameter declarations, a constructor's
// mem-initializer-list, and a body, or handles `= default` / `= delete`.
// Returns the declaration list (adopting `declarations`) for a definition,
// or NULL for a declaration only.
ASTNode* DeclareOrDefineFunction(Syntax* syntax,
                                 Vector* declarations,
                                 Symbol* sym, Symbol* old_sym) {
  int definition_errors_before = NumErrors();
  if (sym->type->info.function.old_style) {
    // Old style functions have the types of their formal
    // arguments specified before the open brace for their
    // body.
    ParseOldStyleParameterDeclarations(syntax, sym);
  }

  CXXConstructorInitList cxx_initializers;
  SyntaxCXXConstructorInitListInit(&cxx_initializers);
  ParseFunctionConstructorInitializers(syntax, sym, &cxx_initializers);

  ASTNode* result = NULL;
  if (sym->type->info.function.is_defaulted) {
    result = DefineDefaultedFunction(syntax, declarations, sym, old_sym,
                                     &cxx_initializers);
  } else if (sym->type->info.function.is_deleted) {
    if (old_sym != NULL && TypeIsFunction(old_sym->type)) {
      old_sym->type->info.function.is_deleted = true;
    }
  } else if (LexMatch(syntax->lex, TOK(lbrace))) {
    result = DefineFunctionWithBody(syntax, declarations, sym, old_sym,
                                    &cxx_initializers,
                                    definition_errors_before);
  } else {
    // Function is a declaration.  If the old symbol was inline, this is
    // an inline definition in C. C++ emits inline definitions directly.
    if (!CompilerIsCXX() && old_sym != NULL &&
        old_sym->type->info.function.is_inline) {
      old_sym->flags.is_inline_defn = true;
    }
  }
  SyntaxCXXConstructorInitListDestruct(&cxx_initializers);
  return result;
}

// We have consumed the open brace, skip tokens until we find the
// matching close brace (or EOF).
void SkipFunctionBody(Syntax* syntax) {
  Lex* lex = syntax->lex;
  int brace_count = 1;
  while (brace_count > 0 && !LexEof(lex)) {
    if (LexLookingAt(lex, TOK(lbrace))) {
      brace_count++;
    } else if (LexLookingAt(lex, TOK(rbrace))) {
      brace_count--;
    }
    LexNextToken(lex);
  }
}
