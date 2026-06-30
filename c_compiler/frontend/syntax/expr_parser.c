//
//  expr_parser.c
//  c_compiler
//
//  Created by David Allison on 10/30/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include <stdint.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "errors.h"
#include "expr_evaluator.h"
#include "expr_parser.h"
#include "expr_semantics.h"
#include "preprocessor.h"
#include "statement_parser.h"
#include "symbol_table.h"
#include "type.h"
#include "compiler.h"

static struct Intrinsic {
  const char* name;
  ASTOpcode opcode;
  int num_args;
} intrinsics[] = {
    {"__atomic_add_fetch", AST_OP(builtin_atomic_add_fetch), 3},
    {"__atomic_compare_exchange_n", AST_OP(builtin_atomic_compare_exchange_n), 6},
    {"__atomic_fetch_add", AST_OP(builtin_atomic_fetch_add), 3},
    {"__atomic_fetch_sub", AST_OP(builtin_atomic_fetch_sub), 3},
    {"__atomic_load_n", AST_OP(builtin_atomic_load), 2},
    {"__atomic_store_n", AST_OP(builtin_atomic_store), 3},
    {"__atomic_sub_fetch", AST_OP(builtin_atomic_sub_fetch), 3},
    {"__builtin_COLUMN", AST_OP(builtin_source_column), 0},
    {"__builtin_FILE", AST_OP(builtin_source_file), 0},
    {"__builtin_FUNCTION", AST_OP(builtin_source_function), 0},
    {"__builtin_LINE", AST_OP(builtin_source_line), 0},
    {"__builtin_PRETTY_FUNCTION", AST_OP(builtin_source_pretty_function), 0},
    {"__builtin_va_arg", AST_OP(builtin_va_arg), 2},
    {"__builtin_va_copy", AST_OP(builtin_va_copy), 2},
    {"__builtin_va_end", AST_OP(builtin_va_end), 1},
    {"__builtin_va_start", AST_OP(builtin_va_start), 2},
    {"__sync_add_and_fetch", AST_OP(builtin_atomic_add_fetch), 2},
    {"__sync_bool_compare_and_swap", AST_OP(builtin_atomic_compare_exchange_bool), 3},
    {"__sync_fetch_and_add", AST_OP(builtin_atomic_fetch_add), 2},
    {"__sync_fetch_and_sub", AST_OP(builtin_atomic_fetch_sub), 2},
    {"__sync_sub_and_fetch", AST_OP(builtin_atomic_sub_fetch), 2},
    {"__sync_synchronize", AST_OP(builtin_atomic_fence), 0},
    {"__sync_val_compare_and_swap", AST_OP(builtin_atomic_compare_exchange_val), 3},
};

// The lambda's default capture mode: none ([]), by-value ([=]) or by-reference
// ([&]).  It governs how identifiers used in the body but not named explicitly
// are implicitly captured.
typedef enum {
  kLambdaCaptureDefaultNone,
  kLambdaCaptureDefaultValue,
  kLambdaCaptureDefaultReference,
} LambdaCaptureDefault;

// One captured entity of a lambda.  Each capture becomes a member of the
// synthesized closure class.
typedef struct {
  Symbol* captured;          // Enclosing variable (or init-capture local) being captured.
  Symbol* field;             // Closure-class member created for the capture.
  ASTNode* initializer;      // Initializer expression for an init-capture, else NULL.
  bool by_reference;         // Captured by reference (stored as a pointer field).
  bool is_pack_expansion;    // Capture expands a parameter pack (e.g. [...xs]).
  bool is_init_capture;      // Init-capture ([x = expr]) introducing a new name.
} LambdaCapture;

static ASTNode* ParseAssignmentExpression(Syntax* syntax,
                                          TokenClass followers);

static int CompareIntrinsicName(const void* key, const void* element) {
  const char* name = key;
  const struct Intrinsic* intrinsic = element;
  return strcmp(name, intrinsic->name);
}

static const struct Intrinsic* GetIntrinsic(const char* name) {
  return bsearch(name, intrinsics,
                 sizeof(intrinsics) / sizeof(intrinsics[0]),
                 sizeof(intrinsics[0]), CompareIntrinsicName);
}

static Symbol* FindThisSymbol(Syntax* syntax) {
  String this_name;
  StringInit(&this_name, "this");
  Symbol* symbol = SyntaxFindSymbol(syntax, &this_name);
  StringDestruct(&this_name);
  return symbol;
}

static ASTNode* NewMemberAccessFromThis(Syntax* syntax,
                                        FullyQualifiedIdentifier* name) {
  if (name->is_qualified) {
    return NULL;
  }
  Symbol* this_symbol = FindThisSymbol(syntax);
  if (this_symbol == NULL || this_symbol->type == NULL ||
      !TypeIsStructOrUnionPointer(this_symbol->type) ||
      this_symbol->type->next == NULL ||
      this_symbol->type->next->info.struct_info == NULL) {
    return NULL;
  }

  String member_name;
  StringInit(&member_name, FullyQualifiedIdentifierLast(name));
  StructMember* member =
      FindStructMember(this_symbol->type->next->info.struct_info, &member_name);
  if (member == NULL || member->is_static) {
    StringDestruct(&member_name);
    return NULL;
  }

  ASTNode* left =
      NewIdentifierASTNode(this_symbol, syntax->lex->current_token_location);
  ASTNode* right = NewStringConstantASTNode(NewString(member_name.value), NULL,
                                            syntax->lex->current_token_location);
  StringDestruct(&member_name);
  return NewBinaryASTNode(AST_OP(arrow), NULL,
                          syntax->lex->current_token_location, left, right);
}

static ASTNode* ParseThisExpression(Syntax* syntax) {
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);
  Symbol* this_symbol = FindThisSymbol(syntax);
  if (this_symbol == NULL) {
    SyntaxError(syntax, "'this' is only valid inside a C++ member function");
    TypeRecord* type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown,
                                             kQualPlain);
    return (ASTNode*)NewIntConstantASTNode(0, type, location);
  }
  return NewIdentifierASTNode(this_symbol, location);
}

// Forward declarations.
static ASTNode* ParseCastExpression(Syntax* syntax, TokenClass followers);
static ASTNode* ParseUnaryExpression(Syntax* syntax, TokenClass followers);
static ASTNode* ParseCompoundLiteral(Syntax* syntax, TypeRecord* type);
static StructMember* FindCXXConstructorForType(TypeRecord* type);
static bool CXXNewConstructorSetHasInitializerList(StructMember* ctor);
static Vector* ParseCXXNewInitializerArguments(Syntax* syntax, Token open,
                                               TokenClass followers);

typedef struct {
  bool found;
} CXXPackExpressionSearch;

static void FindCXXParameterPackExpression(ASTNode* node, void* data,
                                           int child_id, VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(identifier)) {
    return;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  if (id->symbol != NULL && id->symbol->flags.is_parameter_pack) {
    ((CXXPackExpressionSearch*)data)->found = true;
  }
}

static bool CXXExpressionContainsParameterPack(ASTNode* node) {
  CXXPackExpressionSearch search = {0};
  ASTNodeVisit(node, FindCXXParameterPackExpression, 0, &search);
  return search.found;
}

static void MarkCXXPackExpansionIfPresent(Syntax* syntax, ASTNode* actual) {
  if (!CompilerIsCXX() || !LexMatch(syntax->lex, TOK(ellipsis))) {
    return;
  }
  if (!CXXExpressionContainsParameterPack(actual)) {
    SyntaxError(syntax, "pack expansion requires a function parameter pack");
  }
  actual->flags |= kASTPackExpansion;
}

static bool ParseFoldOperator(Syntax* syntax, ASTOpcode* op) {
  if (LexMatch(syntax->lex, TOK(star))) {
    *op = AST_OP(mult);
    return true;
  }
  if (LexMatch(syntax->lex, TOK(slash))) {
    *op = AST_OP(div);
    return true;
  }
  if (LexMatch(syntax->lex, TOK(percent))) {
    *op = AST_OP(mod);
    return true;
  }
  if (LexMatch(syntax->lex, TOK(plus))) {
    *op = AST_OP(plus);
    return true;
  }
  if (LexMatch(syntax->lex, TOK(minus))) {
    *op = AST_OP(minus);
    return true;
  }
  if (LexMatch(syntax->lex, TOK(lessless))) {
    *op = AST_OP(lshift);
    return true;
  }
  if (LexMatch(syntax->lex, TOK(greatergreater))) {
    *op = AST_OP(rshift);
    return true;
  }
  if (LexMatch(syntax->lex, TOK(amp))) {
    *op = AST_OP(and);
    return true;
  }
  if (LexMatch(syntax->lex, TOK(caret))) {
    *op = AST_OP(exor);
    return true;
  }
  if (LexMatch(syntax->lex, TOK(bar))) {
    *op = AST_OP(bitor);
    return true;
  }
  if (LexMatch(syntax->lex, TOK(ampamp))) {
    *op = AST_OP(logand);
    return true;
  }
  if (LexMatch(syntax->lex, TOK(barbar))) {
    *op = AST_OP(logor);
    return true;
  }
  return false;
}

typedef struct {
  bool found;
} FoldPackSearch;

static void FindFoldPackIdentifier(ASTNode* node, void* data, int child_id,
                                   VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(identifier)) {
    return;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  if (id->symbol != NULL && id->symbol->flags.is_parameter_pack) {
    ((FoldPackSearch*)data)->found = true;
  }
}

static bool FoldExpressionContainsPack(ASTNode* node) {
  FoldPackSearch search = {0};
  ASTNodeVisit(node, FindFoldPackIdentifier, 0, &search);
  return search.found;
}

static ASTNode* ParseFoldPackExpression(Syntax* syntax, TokenClass followers) {
  ASTNode* expr = ParseCastExpression(syntax, followers);
  if (!FoldExpressionContainsPack(expr)) {
    ASTNodeDelete(expr);
    return NULL;
  }
  return expr;
}

static bool FoldExpressionHasTopLevelEllipsis(Syntax* syntax) {
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  bool found = false;
  int paren_depth = 0;
  int square_depth = 0;
  int brace_depth = 0;

  while (!LexEof(syntax->lex)) {
    Token token = syntax->lex->current_token;
    if (token == TOK(rparen) && paren_depth == 0 &&
        square_depth == 0 && brace_depth == 0) {
      break;
    }
    if (token == TOK(ellipsis) && paren_depth == 0 &&
        square_depth == 0 && brace_depth == 0) {
      found = true;
      break;
    }
    if (token == TOK(lparen)) {
      paren_depth++;
    } else if (token == TOK(rparen)) {
      paren_depth--;
    } else if (token == TOK(lsquare)) {
      square_depth++;
    } else if (token == TOK(rsquare)) {
      square_depth--;
    } else if (token == TOK(lbrace)) {
      brace_depth++;
    } else if (token == TOK(rbrace)) {
      brace_depth--;
    }
    LexNextToken(syntax->lex);
  }

  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return found;
}

static ASTNode* NewInvalidFoldExpression(Syntax* syntax,
                                         LexCheckpoint* checkpoint,
                                         const char* message,
                                         SourceLocation location,
                                         TokenClass followers) {
  LexCheckpointRestore(syntax->lex, checkpoint);
  SyntaxError(syntax, "%s", message);
  SyntaxRecover(syntax, TC(closebra));
  SyntaxNeedBracket(syntax, TOK(rparen), followers);
  TypeRecord* type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  return NewIntConstantASTNode(0, type, location);
}

static ASTNode* TryParseCXXFoldExpression(Syntax* syntax,
                                          TokenClass followers) {
  if (!CompilerIsCXX()) {
    return NULL;
  }

  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  SourceLocation location = syntax->lex->current_token_location;
  ASTOpcode op = AST_OP(bad);
  ASTOpcode second_op = AST_OP(bad);
  ASTNode* pack = NULL;
  ASTNode* seed = NULL;
  bool pack_on_left = false;

  if (LexMatch(syntax->lex, TOK(ellipsis))) {
    if (!ParseFoldOperator(syntax, &op)) {
      ASTNode* invalid =
          NewInvalidFoldExpression(syntax, &checkpoint,
                                   "fold expression requires an operator",
                                   location, followers);
      LexCheckpointDestruct(&checkpoint);
      return invalid;
    }
    if ((pack = ParseFoldPackExpression(syntax, followers | TC(closebra))) == NULL) {
      ASTNode* invalid =
          NewInvalidFoldExpression(syntax, &checkpoint,
                                   "fold expression requires a parameter pack",
                                   location, followers);
      LexCheckpointDestruct(&checkpoint);
      return invalid;
    }
    if (!LexMatch(syntax->lex, TOK(rparen))) {
      ASTNode* invalid =
          NewInvalidFoldExpression(syntax, &checkpoint,
                                   "fold expression syntax error",
                                   location, followers);
      LexCheckpointDestruct(&checkpoint);
      return invalid;
    }
  } else {
    if (!FoldExpressionHasTopLevelEllipsis(syntax)) {
      LexCheckpointRestore(syntax->lex, &checkpoint);
      LexCheckpointDestruct(&checkpoint);
      return NULL;
    }

    seed = ParseCastExpression(syntax, followers | TC(closebra));
    bool seed_contains_pack = FoldExpressionContainsPack(seed);
    if (!seed_contains_pack && seed != NULL &&
        ParseFoldOperator(syntax, &op) &&
        LexMatch(syntax->lex, TOK(ellipsis))) {
      if (!ParseFoldOperator(syntax, &second_op)) {
        ASTNode* invalid =
            NewInvalidFoldExpression(syntax, &checkpoint,
                                     "fold expression requires a second operator",
                                     location, followers);
        LexCheckpointDestruct(&checkpoint);
        return invalid;
      }
      if (second_op != op) {
        ASTNode* invalid =
            NewInvalidFoldExpression(syntax, &checkpoint,
                                     "fold expression operators must match",
                                     location, followers);
        LexCheckpointDestruct(&checkpoint);
        return invalid;
      }
      if ((pack = ParseFoldPackExpression(syntax, followers | TC(closebra))) == NULL) {
        ASTNode* invalid =
            NewInvalidFoldExpression(syntax, &checkpoint,
                                     "fold expression requires a parameter pack",
                                     location, followers);
        LexCheckpointDestruct(&checkpoint);
        return invalid;
      }
      if (!LexMatch(syntax->lex, TOK(rparen))) {
        ASTNode* invalid =
            NewInvalidFoldExpression(syntax, &checkpoint,
                                     "fold expression syntax error",
                                     location, followers);
        LexCheckpointDestruct(&checkpoint);
        return invalid;
      }
      pack_on_left = false;
    } else {
      if (!seed_contains_pack) {
        ASTNode* invalid =
            NewInvalidFoldExpression(syntax, &checkpoint,
                                     "fold expression requires a parameter pack",
                                     location, followers);
        LexCheckpointDestruct(&checkpoint);
        return invalid;
      }
      pack = seed;
      seed = NULL;
      if (!ParseFoldOperator(syntax, &op)) {
        ASTNode* invalid =
            NewInvalidFoldExpression(syntax, &checkpoint,
                                     "fold expression requires an operator",
                                     location, followers);
        LexCheckpointDestruct(&checkpoint);
        return invalid;
      }
      if (!LexMatch(syntax->lex, TOK(ellipsis))) {
        ASTNode* invalid =
            NewInvalidFoldExpression(syntax, &checkpoint,
                                     "fold expression syntax error",
                                     location, followers);
        LexCheckpointDestruct(&checkpoint);
        return invalid;
      }
      if (LexMatch(syntax->lex, TOK(rparen))) {
        pack_on_left = true;
        seed = NULL;
      } else {
        if (!ParseFoldOperator(syntax, &second_op)) {
          ASTNode* invalid =
              NewInvalidFoldExpression(syntax, &checkpoint,
                                       "fold expression requires a second operator",
                                       location, followers);
          LexCheckpointDestruct(&checkpoint);
          return invalid;
        }
        if (second_op != op) {
          ASTNode* invalid =
              NewInvalidFoldExpression(syntax, &checkpoint,
                                       "fold expression operators must match",
                                       location, followers);
          LexCheckpointDestruct(&checkpoint);
          return invalid;
        }
        seed = ParseCastExpression(syntax, followers | TC(closebra));
        if (seed == NULL || FoldExpressionContainsPack(seed)) {
          ASTNode* invalid =
              NewInvalidFoldExpression(syntax, &checkpoint,
                                       "fold expression requires a non-pack initializer",
                                       location, followers);
          LexCheckpointDestruct(&checkpoint);
          return invalid;
        }
        if (!LexMatch(syntax->lex, TOK(rparen))) {
          ASTNode* invalid =
              NewInvalidFoldExpression(syntax, &checkpoint,
                                       "fold expression syntax error",
                                       location, followers);
          LexCheckpointDestruct(&checkpoint);
          return invalid;
        }
        pack_on_left = true;
      }
    }
  }

  ASTNode* fold =
      pack_on_left ? NewBinaryASTNode(op, NULL, location, pack, seed)
                   : NewBinaryASTNode(op, NULL, location, seed, pack);
  fold->flags |= kASTFoldExpression;
  if (pack_on_left) {
    fold->flags |= kASTFoldPackOnLeft;
  }
  LexCheckpointDestruct(&checkpoint);
  (void)followers;
  return fold;
}

static bool SymbolHasFunctionTemplateOverload(Symbol* symbol) {
  for (Symbol* candidate = symbol; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->flags.is_template && candidate->type != NULL &&
        TypeIsFunction(candidate->type)) {
      return true;
    }
  }
  return false;
}

static bool ExpressionIdentifierNeedsTemplateIdParser(Syntax* syntax) {
  if (!CompilerIsCXX()) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  bool needs_template_ids = false;
  if (LexMatch(syntax->lex, TOK(coloncolon)) &&
      !LexLookingAt(syntax->lex, TOK(identifier))) {
    goto done;
  }
  while (LexLookingAt(syntax->lex, TOK(identifier))) {
    LexNextToken(syntax->lex);
    if (LexLookingAt(syntax->lex, TOK(less))) {
      int depth = 0;
      do {
        // A `;` or brace can never appear at the top level of a
        // template-argument list, so this `<` is a less-than operator rather
        // than a template-id.  Stopping here also keeps the lookahead from
        // running to the end of an #include'd file, which on EOF frees the
        // current source out from under the checkpoint we restore below.
        if (LexLookingAt(syntax->lex, TOK(semicolon)) ||
            LexLookingAt(syntax->lex, TOK(lbrace)) ||
            LexLookingAt(syntax->lex, TOK(rbrace))) {
          break;
        }
        if (LexLookingAt(syntax->lex, TOK(less))) {
          depth++;
        } else if (LexLookingAt(syntax->lex, TOK(greater))) {
          depth--;
        }
        LexNextToken(syntax->lex);
      } while (!LexEof(syntax->lex) && depth > 0);
      if (depth == 0 && LexLookingAt(syntax->lex, TOK(coloncolon))) {
        needs_template_ids = true;
        goto done;
      }
    }
    if (!LexMatch(syntax->lex, TOK(coloncolon))) {
      break;
    }
  }

done:
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return needs_template_ids;
}

// A qualified name like `T::member` whose leading nested-name-specifier is a
// template type parameter is a dependent name: it cannot be resolved until the
// template is instantiated.  Builds a placeholder identifier carrying the
// template-parameter scope (as a placeholder type) and the trailing member
// name (in dependent_member_name), flagged so the template-body cloner can
// resolve it against the concrete type argument.  Returns NULL when `name` is
// not such a dependent qualified value name.
static ASTNode* BuildDependentQualifiedValueName(Syntax* syntax,
                                                  FullyQualifiedIdentifier* name,
                                                  SourceLocation location) {
  if (!CompilerIsCXX() || !name->is_qualified || name->absolute ||
      name->components.length != 2) {
    return NULL;
  }
  // No component may carry template arguments (e.g. `T::tmpl<...>`); that needs
  // the richer dependent-template-id handling, which this does not cover.
  for (size_t i = 0; i < name->template_arguments.length; i++) {
    if (name->template_arguments.value.p[i] != NULL) {
      return NULL;
    }
  }
  String* scope = name->components.value.p[0];
  Symbol* scope_symbol = SyntaxFindSymbol(syntax, scope);
  // The scope must name a type that is (or aliases) a template type parameter:
  // either the parameter itself, or a typedef/using-alias to it such as
  // `using traits_type = Traits;`.  In both cases the symbol's type carries the
  // template parameter index, which is what the template-body cloner uses to
  // substitute the concrete argument at instantiation time.
  if (scope_symbol == NULL || scope_symbol->type == NULL ||
      scope_symbol->type->template_parameter_index < 0) {
    return NULL;
  }
  if (!scope_symbol->flags.is_template_type_parameter &&
      !StorageIs(scope_symbol->storage, STO(typedef))) {
    return NULL;
  }
  String* member = name->components.value.p[1];
  TypeRecord* dependent_type = TypeRecordCopy(scope_symbol->type);
  if (dependent_type->dependent_member_name != NULL) {
    StringDelete(dependent_type->dependent_member_name);
  }
  dependent_type->dependent_member_name = NewString(member->value);
  Symbol* placeholder = NewSymbol(member->value, dependent_type, STO(implicit));
  placeholder->flags.invented = true;
  ASTNode* node = NewIdentifierASTNode(placeholder, location);
  node->flags |= kASTQualifiedName | kASTDependentQualifiedName;
  return node;
}

static ASTNode* ParseIdentifier(Syntax* syntax,
                                            TokenClass followers) {
  Lex* lex = syntax->lex;

  FullyQualifiedIdentifier name;
  FullyQualifiedIdentifierInit(&name);
  bool parsed = ExpressionIdentifierNeedsTemplateIdParser(syntax)
      ? SyntaxParseFullyQualifiedIdentifierWithTemplateIds(syntax, &name,
                                                          followers)
      : SyntaxParseFullyQualifiedIdentifier(syntax, &name);
  if (!parsed) {
    SyntaxError(syntax, "Expected identifier");
    FullyQualifiedIdentifierDestruct(&name);
    return (ASTNode*)NewIntConstantASTNode(
        0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
        syntax->lex->current_token_location);
  }
  
  // In preprocesor mode we have no symbols, everything is a macro name.
  if (lex->preprocessor_mode) {
    String macro_name;
    StringInit(&macro_name, FullyQualifiedIdentifierLast(&name));
    if (PreprocessorFindMacro(lex->preprocessor, &macro_name) == NULL) {
      PreprocessorWarning(lex->preprocessor, "undef",
                          "'%s' is not defined, evaluates to 0",
                          macro_name.value);
    }
    ASTNode* result = NewMacroNameASTNode(&macro_name, lex->current_token_location);
    StringDestruct(&macro_name);
    FullyQualifiedIdentifierDestruct(&name);
    return result;
  }
  
  // Find the symbol by searching all symbol tables.  It must exist.
  Symbol* symbol = SyntaxFindQualifiedSymbol(syntax, &name);
  if (symbol == NULL) {
    if (name.is_qualified) {
      ASTNode* dependent = BuildDependentQualifiedValueName(
          syntax, &name, lex->current_token_location);
      if (dependent != NULL) {
        FullyQualifiedIdentifierDestruct(&name);
        return dependent;
      }
      SyntaxError(syntax, "No such symbol \"%s\"", name.spelling.value);
      TypeRecord* type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
      symbol = NewSymbol(FullyQualifiedIdentifierLast(&name), type, STO(implicit));
      symbol->flags.invented = true;
    } else if (lex->assembler_mode) {
      // In assembler mode we have symbols but we pre-declare them
      // if they don't exist.  They are declared as variables with
      // type unsigned long.
      TypeRecord* type = NewTypeRecordWithSize(kTypeLong | kTypeUnsigned, kQualPlain);
      symbol = NewSymbol(FullyQualifiedIdentifierLast(&name), type, STO(implicit));
      symbol->flags.is_forward_declared = true;
      // Register it so it is found on later references (and owned/freed by a
      // symbol table) rather than leaked.
      SyntaxAddSymbol(syntax, symbol);
    } else {
      ASTNode* member_access = NewMemberAccessFromThis(syntax, &name);
      if (member_access != NULL) {
        FullyQualifiedIdentifierDestruct(&name);
        return member_access;
      }
      if (LexLookingAt(lex, TOK(lparen))) {
        if (!CompilerIsCXX() &&
            GetIntrinsic(FullyQualifiedIdentifierLast(&name)) == NULL) {
          // Calling an unknown function is a warning.
          SyntaxWarning(syntax, "implicit-function-declaration",
                        "Calling undeclared function %s",
                        FullyQualifiedIdentifierLast(&name));
        }

        // Declare the function so we don't get more warnings for the same
        // function.
        TypeRecord* type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown,
                                                 kQualPlain);
        TypeRecord* func_type = NewFunctionTypeRecord();
        func_type->info.function.unknown_args = true;
        TypeRecordChain(func_type, type);
        symbol = NewSymbol(FullyQualifiedIdentifierLast(&name), func_type,
                           STO(implicit));
        symbol->flags.is_forward_declared = true;
        // Declare it so repeated calls find this symbol (no duplicate warnings)
        // and so it is owned/freed by a symbol table rather than leaked.
        SyntaxAddSymbol(syntax, symbol);
      } else {
        SyntaxError(syntax, "No such symbol \"%s\"", name.spelling.value);
        TypeRecord* type =
            NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
        symbol = NewSymbol(FullyQualifiedIdentifierLast(&name), type,
                           STO(implicit));
        symbol->flags.invented = true;
        SyntaxAddSymbol(syntax, symbol);
      }
    }
  }
  Vector* template_arguments = NULL;
  if (symbol != NULL && name.template_arguments.length > 0 &&
      (TypeIsFunction(symbol->type) ||
       SymbolHasFunctionTemplateOverload(symbol))) {
    Vector* parsed_args =
        name.template_arguments.value.p[name.template_arguments.length - 1];
    template_arguments = TemplateArgumentVectorCopy(parsed_args);
  }
  if (template_arguments == NULL && symbol != NULL &&
      (symbol->flags.is_template || SymbolHasFunctionTemplateOverload(symbol)) &&
      LexLookingAt(lex, TOK(less))) {
    Vector* args = SyntaxParseTemplateArgumentList(syntax, followers);
    if (args != NULL) {
      if (TypeIsFunction(symbol->type) ||
          SymbolHasFunctionTemplateOverload(symbol)) {
        template_arguments = args;
        args = NULL;
      }
      if (args != NULL) {
        VectorDestructWithContents(args,
                                   (VectorElementDestructor)TemplateArgumentDelete,
                                   /*free_element=*/false);
      }
    }
  }
  bool is_qualified_name = name.is_qualified;
  FullyQualifiedIdentifierDestruct(&name);
  ASTNode* node = NewIdentifierASTNode(symbol, lex->current_token_location);
  if (is_qualified_name) {
    node->flags |= kASTQualifiedName;
  }
  ((IdentifierASTNode*)node)->template_arguments = template_arguments;
  return node;
}

static ASTNode* ParseIntegerConstant(Syntax* syntax,
                                                 TokenClass followers) {
  
  Lex* lex = syntax->lex;
  int64_t value = lex->number;

  // The type of an integer constant is the first type from a candidate list
  // (C11 6.4.4.1) that can represent its value.  Decimal constants only
  // consider signed types when there is no 'u' suffix; hexadecimal and octal
  // constants may also pick an unsigned type.  Capture the radix and suffix
  // flags before advancing the lexer, which overwrites them.
  const char* spelling = lex->spelling.value;
  bool octal_or_hex = spelling != NULL && spelling[0] == '0' &&
                      (spelling[1] == 'x' || spelling[1] == 'X' ||
                       spelling[1] == 'b' || spelling[1] == 'B' ||
                       (spelling[1] >= '0' && spelling[1] <= '7'));
  bool has_u = StringContainsChar(&lex->suffix, 'U');
  bool has_ll = StringContainsString(&lex->suffix, "LL");
  bool has_l = !has_ll && StringContainsChar(&lex->suffix, 'L');
  LexNextToken(lex);

  uint64_t uval = (uint64_t)value;
  // Whether an unsigned candidate may be selected when there is no 'u' suffix.
  bool allow_unsigned = octal_or_hex;

  // Candidate integer ranks, in increasing width.  The bit widths are taken
  // from the active target (e.g. int is 16 bits on the 6502 but 32 bits
  // elsewhere) so that a literal is given the narrowest standard type that can
  // actually represent it on that target.
  Type rank_spec[3] = { kTypeInt, kTypeLong, kTypeLongLong };
  int rank_bits[3] = { SizeofType(kTypeInt) * 8,
                       SizeofType(kTypeLong) * 8,
                       SizeofType(kTypeLongLong) * 8 };

  // An explicit L/LL suffix sets a minimum rank.  A 'u' suffix forbids signed
  // candidates; hex/octal constants (or a 'u' suffix) permit unsigned ones.
  int min_rank = has_ll ? 2 : (has_l ? 1 : 0);
  bool try_signed = !has_u;
  bool try_unsigned = has_u || allow_unsigned;

  Type type_specifier = kTypeLongLong | kTypeUnsigned;
  for (int r = min_rank; r < 3; r++) {
    int bits = rank_bits[r];
    uint64_t smax = (bits >= 64) ? 0x7fffffffffffffffULL
                                 : ((1ULL << (bits - 1)) - 1);
    uint64_t umax = (bits >= 64) ? 0xffffffffffffffffULL
                                 : ((1ULL << bits) - 1);
    if (try_signed && uval <= smax) {
      type_specifier = rank_spec[r];
      break;
    }
    if (try_unsigned && uval <= umax) {
      type_specifier = rank_spec[r] | kTypeUnsigned;
      break;
    }
  }

  TypeRecord* type = NewTypeRecordWithSize(type_specifier, kQualPlain);
  return NewIntConstantASTNode(value, type,
                               syntax->lex->current_token_location);
}

static ASTNode* ParseFloatingPointConstant(Syntax* syntax,
                                                 TokenClass followers) {
  Lex* lex = syntax->lex;
  double value = lex->fnumber;
  LexNextToken(lex);
  
  // Floating point numbers have a suffix: F or L, meaning
  // F: float, L: long double.
  Type type_specifier = kTypeDouble;
  if (StringContainsChar(&lex->suffix, 'F')) {
    type_specifier &= ~kTypeDouble;
    type_specifier |= kTypeFloat;
  } else if (StringContainsChar(&lex->suffix, 'L')) {
    type_specifier &= ~kTypeDouble;
    type_specifier |= kTypeLongDouble;
  }
  TypeRecord* type = NewTypeRecordWithSize(type_specifier, kQualPlain);
  return NewRealConstantASTNode(value, type,
                                syntax->lex->current_token_location);
}

static ASTNode* ParseStringLiteral(Syntax* syntax, TokenClass followers) {
  Lex* lex = syntax->lex;
  String* contents = NewString(lex->spelling.value);
  LexNextToken(lex);
  
  // Adjacent string literals are joined together.
  while (LexLookingAt(lex, TOK(string))) {
    StringAppend(contents, lex->spelling.value);
    LexNextToken(lex);
  }
  
  TypeRecord* array =
    NewBasicArrayTypeRecord(kQualPlain, (int)contents->length + 1, false);
  TypeRecord* type = NewTypeRecordWithSize(kTypeChar, kQualPlain);
  TypeRecordChain(array, type);
  TypeRecordCalculateSize(array);
  return NewStringConstantASTNode(contents, array,
                                  syntax->lex->current_token_location);
  
}

static ASTNode* ParseWideStringLiteral(Syntax* syntax,
                                               TokenClass followers) {
  Lex* lex = syntax->lex;
  String* contents = NewStringWithLength(lex->spelling.value,
                                         lex->spelling.length + 4);
  LexNextToken(lex);
  
  // Adjacent wide string literals are joined together.
  while (LexLookingAt(lex, TOK(string_wide))) {
    StringAppend(contents, lex->spelling.value);
    LexNextToken(lex);
  }
  
  TypeRecord* array =
  NewBasicArrayTypeRecord(kQualPlain, (int)contents->length + 4, false);
  TypeRecord* type = NewTypeRecordWithSize(kTypeInt, kQualPlain);
  TypeRecordChain(array, type);
  return NewWideStringConstantASTNode(contents, array,
                                 syntax->lex->current_token_location);
}

static ASTNode* ParseCharacterConstant(Syntax* syntax,
                                                   TokenClass followers) {
  Lex* lex = syntax->lex;
  int value = (int)lex->number;
  LexNextToken(lex);
  TypeRecord* type = NewTypeRecordWithSize(kTypeChar, kQualPlain);
  return NewCharConstantASTNode(value, type,
                                syntax->lex->current_token_location);
}

static ASTNode* ParseWideCharacterConstant(Syntax* syntax,
                                       TokenClass followers) {
  Lex* lex = syntax->lex;
  int value = (int)lex->number;
  LexNextToken(lex);
  TypeRecord* type = NewTypeRecordWithSize(kTypeInt, kQualPlain);
  return NewCharConstantASTNode(value, type,
                                syntax->lex->current_token_location);
}

// Parse the highest priority expression - a primary expression.
// This consists of one of:
// 1. A parenthesized expression.
// 2. An identifier
// 3. An integer or floating point constant
// 4. A string literal
// 5. A character constant
//
// We create an Abstract Syntax Tree node for it.  At this point we know
// the type of the AST node because it's either well defined (an integer
// constant for example) or can be read from the symbol table.
//
// The syntax for this is:

// Build the type used to match a _Generic controlling expression against the
// association type names.  This is the type of the controlling expression
// after lvalue conversion: array and function types decay to pointers and any
// top-level qualifiers are removed (C11 6.5.1.1).
static TypeRecord* GenericControllingType(TypeRecord* ctype) {
  if (TypeIsArray(ctype)) {
    TypeRecord* ptr = NewPointerTypeRecord(kQualPlain);
    TypeRecordChain(ptr, ctype->next);
    TypeRecordCalculateSize(ptr);
    return ptr;
  }
  if (TypeIsFunction(ctype)) {
    TypeRecord* ptr = NewPointerTypeRecord(kQualPlain);
    TypeRecordChain(ptr, ctype);
    TypeRecordCalculateSize(ptr);
    return ptr;
  }
  TypeRecord* copy = TypeRecordCopy(ctype);
  copy->qualifiers = kQualPlain;
  return copy;
}

// Canonicalize a primitive type's specifier bits for _Generic matching so that
// equivalent spellings compare equal (e.g. "long" == "signed long int").  The
// char family (char/signed char/unsigned char) stays distinct, as does
// signedness for the other integer types.
static int CanonicalPrimitive(int t) {
  if (t & kTypeChar) {
    return t & (kTypeChar | kTypeSigned | kTypeUnsigned);
  }
  if (t & (kTypeFloat | kTypeDouble | kTypeLongDouble | kTypeVoid | kTypeBool)) {
    return t & (kTypeFloat | kTypeDouble | kTypeLongDouble | kTypeVoid |
                kTypeBool);
  }
  int size = t & (kTypeShort | kTypeLong | kTypeLongLong);
  int sign = (t & kTypeUnsigned) ? kTypeUnsigned : kTypeSigned;
  return size | kTypeInt | sign;
}

// Type matching for _Generic associations.  Unlike TypeEqual this distinguishes
// distinct struct/union/enum tags (which must select different associations)
// and treats equivalent integer spellings as identical.
static bool GenericTypeMatch(TypeRecord* a, TypeRecord* b) {
  if ((a->type & kTypeUnknown) != 0 || (b->type & kTypeUnknown) != 0) {
    return true;
  }
  if (a->declarator != b->declarator) {
    return false;
  }
  switch (a->declarator) {
    case kDeclArray:
      return GenericTypeMatch(a->next, b->next) &&
             a->info.array.size.fixed == b->info.array.size.fixed;
    case kDeclPointer:
      return GenericTypeMatch(a->next, b->next);
    case kDeclFunction:
      return TypeEqual(a, b);
    case kDeclPrimitive:
    default:
      if (a->qualifiers != b->qualifiers) {
        return false;
      }
      if (TypeIsStructOrUnion(a) || TypeIsStructOrUnion(b)) {
        return TypeIsStructOrUnion(a) && TypeIsStructOrUnion(b) &&
               a->info.struct_info == b->info.struct_info;
      }
      if (TypeIsEnum(a) || TypeIsEnum(b)) {
        return TypeIsEnum(a) && TypeIsEnum(b) &&
               a->info.enum_info == b->info.enum_info;
      }
      return CanonicalPrimitive(a->type) == CanonicalPrimitive(b->type);
  }
}

// Parse a C11 _Generic selection:
//   _Generic ( assignment-expression , generic-assoc-list )
//   generic-association:
//     type-name : assignment-expression
//     default : assignment-expression
// The controlling expression is an unevaluated operand: only its type is used
// to pick the matching association.  We return the selected expression (or the
// default), which is then analyzed normally as part of the surrounding AST.
static ASTNode* ParseGenericSelection(Syntax* syntax, TokenClass followers) {
  Lex* lex = syntax->lex;
  LexNextToken(lex);  // Consume "_Generic".
  SyntaxNeedBracket(syntax, TOK(lparen), followers);

  // Determine the controlling expression's type without keeping the node: it
  // is unevaluated, so we analyze it only to obtain its type.
  ASTNode* controlling =
      SyntaxParseSingleExpression(syntax, followers | TC(exprsep));
  controlling = AnalyzeExpression(controlling);
  TypeRecord* match_type =
      controlling->type != NULL ? GenericControllingType(controlling->type)
                                : NewTypeRecordWithSize(kTypeInt, kQualPlain);

  SyntaxNeedBracket(syntax, TOK(comma), followers);

  ASTNode* selected = NULL;
  ASTNode* default_expr = NULL;
  while (!LexEof(lex)) {
    bool is_default = false;
    TypeRecord* assoc_type = NULL;
    Symbol* assoc_sym = NULL;
    if (LexMatch(lex, TOK(default))) {
      is_default = true;
    } else {
      TypeParser parser;
      TypeParserInit(&parser, lex, syntax, STO(implicit), syntax->context);
      TypeRecord* type = TypeParserParseType(&parser, true);
      assoc_sym = TypeParserParseDeclarator(&parser, type);
      assoc_type = assoc_sym != NULL ? assoc_sym->type : type;
      TypeParserDestruct(&parser);
    }
    SyntaxNeedBracket(syntax, TOK(colon), followers);
    ASTNode* expr = SyntaxParseSingleExpression(syntax, followers | TC(exprsep));

    if (is_default) {
      default_expr = expr;
    } else if (selected == NULL && assoc_type != NULL &&
               GenericTypeMatch(match_type, assoc_type)) {
      selected = expr;
    } else {
      ASTNodeDelete(expr);
    }
    if (assoc_sym != NULL) {
      SymbolDelete(assoc_sym);
    }
    if (!LexMatch(lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(syntax, TOK(rparen), followers);

  ASTNode* result = selected != NULL ? selected : default_expr;
  if (result == NULL) {
    SyntaxError(syntax, "No matching association in _Generic selection");
    result = (ASTNode*)NewIntConstantASTNode(
        0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
        syntax->lex->current_token_location);
  } else if (selected != NULL && default_expr != NULL) {
    ASTNodeDelete(default_expr);
  }
  return result;
}

// Allocate a LambdaCapture; the closure-class `field` is filled in later by
// AddLambdaCaptureFields.
static LambdaCapture* NewLambdaCapture(Symbol* captured, bool by_reference,
                                       bool is_pack_expansion,
                                       ASTNode* initializer,
                                       bool is_init_capture) {
  LambdaCapture* capture = malloc(sizeof(LambdaCapture));
  capture->captured = captured;
  capture->field = NULL;
  capture->initializer = initializer;
  capture->by_reference = by_reference;
  capture->is_pack_expansion = is_pack_expansion;
  capture->is_init_capture = is_init_capture;
  return capture;
}

// Find an existing capture of `symbol`, or NULL if it is not yet captured.
static LambdaCapture* FindLambdaCapture(Vector* captures, Symbol* symbol) {
  for (size_t i = 0; i < captures->length; i++) {
    LambdaCapture* capture = captures->value.p[i];
    if (capture->captured == symbol) {
      return capture;
    }
  }
  return NULL;
}

// Find an existing capture whose captured name matches `name` (used to reject
// duplicate captures, including init-captures that introduce a fresh symbol).
static LambdaCapture* FindLambdaCaptureName(Vector* captures, String* name) {
  for (size_t i = 0; i < captures->length; i++) {
    LambdaCapture* capture = captures->value.p[i];
    if (capture->captured != NULL &&
        StringEqualString(&capture->captured->name, name)) {
      return capture;
    }
  }
  return NULL;
}

// True when `symbol` is one of the lambda's own parameters (including the
// synthesized `this`), which must never be treated as a captured variable.
static bool LambdaFunctionOwnsSymbol(TypeRecord* func, Symbol* symbol) {
  if (func == NULL || symbol == NULL) {
    return false;
  }
  for (size_t i = 0; i < func->info.function.prototype.length; i++) {
    if (func->info.function.prototype.value.p[i] == symbol) {
      return true;
    }
  }
  return false;
}

// True when `symbol` is eligible for implicit capture: a local automatic
// variable of the enclosing scope.  Lambda parameters, invented temporaries,
// functions, statics/externs/typedefs and globals are all excluded.
static bool CanCaptureSymbol(Symbol* symbol, TypeRecord* lambda_func) {
  if (symbol == NULL || LambdaFunctionOwnsSymbol(lambda_func, symbol) ||
      symbol->flags.invented || TypeIsFunction(symbol->type) ||
      StorageIs(symbol->storage, STO(static) | STO(extern) | STO(typedef))) {
    return false;
  }
  Symbol* global = FindGlobalSymbol(&symbol->name);
  return global != symbol;
}

// Parse the `[...]` capture list, populating `captures` and reporting the
// default capture mode.  Handles the leading default ([=] / [&]), explicit
// by-value and by-reference captures, init-captures ([x = expr]) and pack
// expansions ([...xs]).
static void ParseLambdaCaptureList(Syntax* syntax, Vector* captures,
                                   LambdaCaptureDefault* capture_default,
                                   TokenClass followers) {
  *capture_default = kLambdaCaptureDefaultNone;
  if (LexMatch(syntax->lex, TOK(rsquare))) {
    return;
  }
  if (LexMatch(syntax->lex, TOK(equal))) {
    *capture_default = kLambdaCaptureDefaultValue;
    LexMatch(syntax->lex, TOK(comma));
  } else if (LexLookingAt(syntax->lex, TOK(amp))) {
    LexCheckpoint checkpoint;
    LexCheckpointSave(syntax->lex, &checkpoint);
    LexNextToken(syntax->lex);
    if (LexLookingAt(syntax->lex, TOK(rsquare)) ||
        LexLookingAt(syntax->lex, TOK(comma))) {
      *capture_default = kLambdaCaptureDefaultReference;
      LexMatch(syntax->lex, TOK(comma));
      LexCheckpointDestruct(&checkpoint);
    } else {
      LexCheckpointRestore(syntax->lex, &checkpoint);
      LexCheckpointDestruct(&checkpoint);
    }
  }

  while (!LexLookingAt(syntax->lex, TOK(rsquare)) && !LexEof(syntax->lex)) {
    bool by_reference = LexMatch(syntax->lex, TOK(amp));
    if (!LexLookingAt(syntax->lex, TOK(identifier))) {
      SyntaxError(syntax, "Expected lambda capture name");
      break;
    }
    String name;
    StringInit(&name, syntax->lex->spelling.value);
    SourceLocation name_location = syntax->lex->current_token_location;
    Symbol* symbol = SyntaxFindSymbol(syntax, &name);
    LexNextToken(syntax->lex);
    if (LexMatch(syntax->lex, TOK(equal))) {
      // Init-capture `[name = expr]`: introduce a brand-new local whose type is
      // deduced from the initializer (defaulting to int when unknown).
      ASTNode* initializer =
          ParseAssignmentExpression(syntax, followers | TC(exprsep) |
                                                TC(closebra));
      MarkCXXPackExpansionIfPresent(syntax, initializer);
      bool is_pack_expansion =
          initializer != NULL && (initializer->flags & kASTPackExpansion) != 0;
      TypeRecord* capture_type =
          initializer != NULL && initializer->type != NULL
              ? TypeRecordCopy(initializer->type)
              : NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
      Symbol* init_capture =
          NewSymbol(name.value, capture_type, STO(auto));
      init_capture->flags.is_defined = true;
      init_capture->flags.is_local = true;
      init_capture->flags.is_parameter_pack = is_pack_expansion;
      init_capture->location = name_location;
      if (FindLambdaCaptureName(captures, &name) == NULL) {
        VectorAppend(captures,
                     NewLambdaCapture(init_capture, by_reference,
                                      is_pack_expansion, initializer,
                                      /*is_init_capture=*/true));
      }
      StringDestruct(&name);
      if (!LexMatch(syntax->lex, TOK(comma))) {
        break;
      }
      continue;
    }
    bool is_pack_expansion = LexMatch(syntax->lex, TOK(ellipsis));
    if (symbol == NULL) {
      SyntaxError(syntax, "Unknown lambda capture %s", name.value);
    } else if (is_pack_expansion && !symbol->flags.is_parameter_pack) {
      SyntaxError(syntax,
                  "lambda capture pack expansion requires a function parameter pack");
    } else if (FindLambdaCapture(captures, symbol) == NULL &&
               FindLambdaCaptureName(captures, &name) == NULL) {
      VectorAppend(captures,
                   NewLambdaCapture(symbol, by_reference, is_pack_expansion,
                                    /*initializer=*/NULL,
                                    /*is_init_capture=*/false));
    }
    StringDestruct(&name);
    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(syntax, TOK(rsquare), followers);
}

// Queue the closure's operator() for instantiation/codegen alongside template
// instantiations, so its body is emitted after the enclosing context is parsed.
static void QueueLambdaCallOperatorDefinition(Symbol* symbol) {
  Vector* declarations = NewVector();
  VectorAppend(declarations,
               NewVariableDeclarationASTNode(symbol, NULL, symbol->location));
  VectorAppend(&compiler->pending_template_instantiations,
               NewDeclarationListASTNode(declarations, symbol->location));
}

// Bring the lambda's parameters into the body's local scope before parsing it.
static void AddLambdaFunctionScopeSymbols(Syntax* syntax, TypeRecord* func) {
  Vector* formals = &func->info.function.prototype;
  for (size_t i = 0; i < formals->length; i++) {
    Symbol* formal = formals->value.p[i];
    InsertLocalSymbol(syntax->local_symbol_stack, formal);
  }
}

// Parse the optional `(params)` of a lambda into `func`'s prototype.  An
// omitted parameter list is allowed and leaves the prototype empty.
static void ParseLambdaParameterList(Syntax* syntax, TypeRecord* func,
                                     TokenClass followers) {
  if (!LexMatch(syntax->lex, TOK(lparen))) {
    return;
  }
  int arg_number = 0;
  while (!LexLookingAt(syntax->lex, TOK(rparen)) && !LexEof(syntax->lex)) {
    TypeParser parser;
    TypeParserInit(&parser, syntax->lex, syntax, STO(auto), kParsingPrototype);
    TypeRecord* type = TypeParserParseType(&parser, true);
    Symbol* formal = TypeParserParseDeclarator(&parser, type);
    TypeParserDestruct(&parser);
    if (formal != NULL) {
      if (TypeContainsAuto(formal->type)) {
        SymbolSetType(formal, NewTypeRecordWithSize(kTypeInt, kQualPlain));
      }
      formal->flags.is_defined = true;
      formal->flags.is_argument = true;
      formal->value.arg_number = arg_number++;
      VectorAppend(&func->info.function.prototype, formal);
    }
    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(syntax, TOK(rparen), followers);
}

// Parse a lambda's optional noexcept-specifier.  Returns true when the lambda
// is known non-throwing (a plain `noexcept`).  A conditional `noexcept(expr)`
// operand is skipped without evaluation and conservatively reported as not
// guaranteed non-throwing, so noexcept enforcement never produces a false
// positive for a lambda whose operand we did not evaluate.
static bool SkipNoexceptSpecifier(Syntax* syntax, TokenClass followers) {
  if (!LexMatch(syntax->lex, TOK(noexcept))) {
    return false;
  }
  if (!LexMatch(syntax->lex, TOK(lparen))) {
    return true;
  }
  int depth = 1;
  while (depth > 0 && !LexEof(syntax->lex)) {
    if (LexLookingAt(syntax->lex, TOK(lparen))) {
      depth++;
    } else if (LexLookingAt(syntax->lex, TOK(rparen))) {
      depth--;
    }
    LexNextToken(syntax->lex);
  }
  if (depth != 0) {
    SyntaxRecover(syntax, followers);
  }
  return false;
}

// Parse the specifiers that follow a lambda's parameter list (`mutable`,
// `constexpr`, `noexcept`) in any order, then an optional `-> type` trailing
// return type.  Returns the trailing return type if present, otherwise
// `default_type`, and reports each specifier through its out-parameter.
static TypeRecord* ParseLambdaSpecifiersAndReturnType(Syntax* syntax,
                                                      bool* is_mutable,
                                                      bool* is_constexpr,
                                                      bool* is_noexcept,
                                                      TypeRecord* default_type,
                                                      TokenClass followers) {
  *is_mutable = false;
  *is_constexpr = false;
  *is_noexcept = false;
  bool keep_parsing = true;
  while (keep_parsing) {
    if (LexMatch(syntax->lex, TOK(mutable))) {
      *is_mutable = true;
    } else if (LexMatch(syntax->lex, TOK(constexpr))) {
      *is_constexpr = true;
    } else if (LexLookingAt(syntax->lex, TOK(noexcept))) {
      *is_noexcept = SkipNoexceptSpecifier(syntax, followers);
    } else {
      keep_parsing = false;
    }
  }

  if (!LexMatch(syntax->lex, TOK(arrow))) {
    return default_type;
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(auto), kParsingPrototype);
  TypeRecord* return_type = TypeParserParseType(&parser, true);
  Symbol* declarator = TypeParserParseDeclarator(&parser, return_type);
  if (declarator != NULL) {
    TypeRecord* parsed_type = TypeRecordCopy(declarator->type);
    SymbolDelete(declarator);
    TypeRecordDelete(return_type);
    return_type = parsed_type;
  }
  TypeParserDestruct(&parser);
  return return_type;
}

// Create the synthesized, uniquely-named closure class for a lambda and
// register it as a tag.  Capture fields are added later; for now it carries a
// single private placeholder member so it has non-zero size before captures are
// known.  Returns the tag symbol and outputs its struct type in `closure_type`.
static Symbol* NewLambdaClosureTag(Syntax* syntax, SourceLocation location,
                                   TypeRecord** closure_type) {
  String tag_name;
  StringInit(&tag_name, NULL);
  SyntaxFakeTagName(syntax, &tag_name);

  Struct* closure = NewStruct(false);
  closure->is_class = true;
  TypeRecord* type = NewTypeRecord(kTypeStruct, kQualPlain);
  type->info.struct_info = closure;
  Symbol* tag = NewSymbol(tag_name.value, type, STO(implicit));
  tag->flags.invented = true;
  tag->flags.is_defined = true;
  tag->location = location;
  closure->tag_name = &tag->name;
  closure->tag_symbol = tag;
  SyntaxAddTag(syntax, tag);
  Symbol* empty_member =
      NewSymbol("__lambda_empty", NewTypeRecordWithSize(kTypeChar, kQualPlain),
                STO(implicit));
  empty_member->flags.invented = true;
  empty_member->flags.is_defined = true;
  empty_member->location = location;
  StructMember* member = NewStructMember(empty_member);
  member->access = kAccessPrivate;
  StructAddSyntheticMember(closure, member);
  TypeRecordCalculateSize(type);
  StringDestruct(&tag_name);
  *closure_type = type;
  return tag;
}

// Build the closure class's `operator()` from the lambda's parameter list,
// specifiers and return type, and add it as a public member function.  Unless
// the lambda is `mutable`, the operator is const-qualified.  The implicit
// `this` parameter is what later lets the body reach capture fields.
static Symbol* NewLambdaCallOperator(Syntax* syntax, TypeRecord* closure_type,
                                     TypeRecord* return_type, bool is_mutable,
                                     SourceLocation location) {
  Struct* closure = closure_type->info.struct_info;
  TypeRecord* func = NewFunctionTypeRecord();
  ParseLambdaParameterList(syntax, func, TC(closebra));
  bool is_constexpr = false;
  bool is_noexcept = false;
  return_type = ParseLambdaSpecifiersAndReturnType(syntax, &is_mutable,
                                                   &is_constexpr, &is_noexcept,
                                                   return_type, TC(closebra));
  func->info.function.is_const_member = !is_mutable;
  func->info.function.is_constexpr = is_constexpr;
  func->info.function.is_noexcept = is_noexcept;
  TypeRecordChain(func, return_type);
  TypeRecordAddCXXThisParameter(func, closure, location);

  Symbol* op = NewSymbol("operator()", func, STO(implicit));
  op->location = location;
  op->flags.is_defined = true;
  op->flags.is_inline_defn = true;
  op->value.func_defn = op;
  func->info.function.symbol = op;
  func->info.function.is_inline = true;
  func->info.function.definition = true;

  StructMember* member = NewStructMember(op);
  member->is_member_function = true;
  member->access = kAccessPublic;
  StructAddSyntheticMember(closure, member);
  return op;
}

// State threaded through CollectDefaultLambdaCaptures while scanning the body
// for identifiers that the default capture mode must implicitly capture.
typedef struct {
  Vector* captures;
  TypeRecord* lambda_func;
  LambdaCaptureDefault capture_default;
} LambdaCaptureScan;

// Visitor: for a `[=]`/`[&]` lambda, append a capture for each enclosing-scope
// identifier referenced in the body that is not already captured.
static void CollectDefaultLambdaCaptures(ASTNode* node, void* data,
                                         int child_id, VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(identifier)) {
    return;
  }
  LambdaCaptureScan* scan = data;
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  if (!CanCaptureSymbol(id->symbol, scan->lambda_func) ||
      FindLambdaCapture(scan->captures, id->symbol) != NULL) {
    return;
  }
  bool by_reference =
      scan->capture_default == kLambdaCaptureDefaultReference;
  VectorAppend(scan->captures, NewLambdaCapture(id->symbol, by_reference,
                                                /*is_pack_expansion=*/false,
                                                /*initializer=*/NULL,
                                                /*is_init_capture=*/false));
}

// The closure-member type for a capture: a pointer to the captured object for
// by-reference captures, otherwise a copy of the captured (referent) type.
static TypeRecord* LambdaCaptureFieldType(LambdaCapture* capture) {
  TypeRecord* captured_type = TypeIsReference(capture->captured->type)
                                  ? capture->captured->type->next
                                  : capture->captured->type;
  if (capture->by_reference) {
    return NewPointerTo(kQualPlain, captured_type);
  }
  return TypeRecordCopy(captured_type);
}

// Add one private member to the closure class for each capture and record it on
// the capture, then recompute the closure's size now that all fields are known.
static void AddLambdaCaptureFields(TypeRecord* closure_type, Vector* captures,
                                   SourceLocation location) {
  Struct* closure = closure_type->info.struct_info;
  for (size_t i = 0; i < captures->length; i++) {
    LambdaCapture* capture = captures->value.p[i];
    Symbol* field = NewSymbol(capture->captured->name.value,
                              LambdaCaptureFieldType(capture), STO(implicit));
    field->flags.invented = true;
    field->flags.is_defined = true;
    field->flags.is_parameter_pack = capture->is_pack_expansion;
    field->location = location;
    StructMember* member = NewStructMember(field);
    member->access = kAccessPrivate;
    StructAddSyntheticMember(closure, member);
    capture->field = field;
  }
  closure_type->size = closure->size;
}

// Build the designated initializer (`.field = value`) for one capture used when
// constructing the closure object: the captured value (or its address, for
// by-reference captures), or the init-capture's initializer expression.
static ASTNode* NewLambdaCaptureInitializer(LambdaCapture* capture,
                                            SourceLocation location) {
  ASTNode* value = capture->is_init_capture
                       ? capture->initializer
                       : NewIdentifierASTNode(capture->captured, location);
  if (capture->by_reference) {
    value = NewUnaryASTNode(AST_OP(address), NULL, location, value);
  }
  if (capture->is_pack_expansion) {
    value->flags |= kASTPackExpansion;
  }
  Vector* designators = NewVector();
  VectorAppend(designators, NewStructDesignator(NewString(capture->field->name.value)));
  return NewDesignatedInitializerASTNode(
      designators, NewExpressionInitializerASTNode(value, location), location);
}

// Build the expression that reads a capture from inside the body: `this->field`
// for by-value captures, dereferenced (`*this->field`) for by-reference
// captures whose field is a pointer.  This replaces uses of the original name.
static ASTNode* NewLambdaCaptureAccess(Symbol* this_symbol,
                                       LambdaCapture* capture,
                                       SourceLocation location) {
  ASTNode* this_node = NewIdentifierASTNode(this_symbol, location);
  ASTNode* member = NewStringConstantASTNode(
      NewString(capture->field->name.value), NULL, location);
  ASTNode* access =
      NewBinaryASTNode(AST_OP(arrow), NULL, location, this_node, member);
  if (capture->by_reference) {
    ASTNode* contents = NewUnaryASTNode(AST_OP(contents), NULL, location,
                                        access);
    if (capture->is_pack_expansion) {
      contents->flags |= kASTPackExpansion;
      access->flags |= kASTPackExpansion;
    }
    return contents;
  }
  if (capture->is_pack_expansion) {
    access->flags |= kASTPackExpansion;
  }
  return access;
}

// State threaded through RewriteLambdaCaptureUses: the captures to look up and
// the closure's `this` parameter through which their fields are reached.
typedef struct {
  Vector* captures;
  Symbol* this_symbol;
} LambdaRewrite;

// Make init-capture locals visible while parsing the body, since their names do
// not exist in the enclosing scope.
static void AddLambdaInitCaptureScopeSymbols(Syntax* syntax, Vector* captures) {
  for (size_t i = 0; captures != NULL && i < captures->length; i++) {
    LambdaCapture* capture = captures->value.p[i];
    if (capture != NULL && capture->is_init_capture &&
        capture->captured != NULL) {
      InsertLocalSymbol(syntax->local_symbol_stack, capture->captured);
    }
  }
}

// Replace child `child_id` of `parent` with `replacement` while rewriting a
// lambda body's capture uses.  For call-like nodes the arguments live in a
// child vector indexed from zero while child_id 0 is the callee, so an argument
// at child_id N maps to vector slot N-1; patch that slot directly.  All other
// node shapes go through the generic child-replacement helper.
static void ReplaceChildForLambdaCapture(ASTNode* parent, int child_id,
                                         ASTNode* replacement) {
  if (parent == NULL) {
    ASTNodeDelete(replacement);
    return;
  }
  if (ASTIsCallNode(parent) && child_id > 0) {
    VectorASTNode* vector = (VectorASTNode*)parent;
    ASTNode* old = vector->children->value.p[child_id - 1];
    VectorSet(vector->children, (size_t)child_id - 1, replacement);
    replacement->parent = parent;
    replacement->child_id = child_id;
    ASTNodeDelete(old);
    return;
  }
  ASTNodeReplaceChild(parent, child_id, replacement, true);
}

// Visitor: rewrite each identifier in the body that names a captured entity
// into the corresponding `this->field` access, so the body reads captures from
// the closure object rather than the (now out-of-scope) enclosing variables.
static void RewriteLambdaCaptureUses(ASTNode* node, void* data,
                                     int child_id, VisitorMode mode) {
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(identifier)) {
    return;
  }
  LambdaRewrite* rewrite = data;
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  LambdaCapture* capture = FindLambdaCapture(rewrite->captures, id->symbol);
  if (capture == NULL || capture->field == NULL) {
    return;
  }
  ASTNode* replacement =
      NewLambdaCaptureAccess(rewrite->this_symbol, capture, node->location);
  ReplaceChildForLambdaCapture(node->parent, child_id, replacement);
}

// Parse the lambda's compound-statement body as the closure operator()'s body,
// in a fresh block scope holding the parameters and any init-capture locals,
// then queue the operator for later definition.
static ASTNode* ParseLambdaBody(Syntax* syntax, Symbol* call_operator,
                                Vector* captures, TokenClass followers) {
  ParserContext old_context = syntax->context;
  syntax->context = kParsingBlockScope;
  SyntaxOpenScope(syntax);
  AddLambdaFunctionScopeSymbols(syntax, call_operator->type);
  AddLambdaInitCaptureScopeSymbols(syntax, captures);
  ASTNode* body = SyntaxParseStatement(syntax, followers);
  SyntaxCloseScope(syntax);
  syntax->context = old_context;
  call_operator->type->info.function.body = body;
  VectorAppend(&compiler->declaration_asts, body);
  QueueLambdaCallOperatorDefinition(call_operator);
  return body;
}

// Build the braced initializer that constructs the closure object, one
// designated `.field = value` initializer per capture.
static ASTNode* NewLambdaClosureInitializer(TypeRecord* closure_type,
                                            Vector* captures,
                                            SourceLocation location) {
  Vector* initializers = NewVector();
  for (size_t i = 0; i < captures->length; i++) {
    VectorAppend(initializers,
                 NewLambdaCaptureInitializer(captures->value.p[i], location));
  }
  return NewBracedInitializerASTNode(initializers, closure_type, location);
}

// Parse a C++ lambda `[captures](params) specifiers -> ret { body }` and lower
// it to an anonymous closure class.  The steps are:
//   1. parse the capture list and default capture mode;
//   2. synthesize the closure class and its operator();
//   3. parse the body (so identifier uses are resolved against the enclosing
//      scope) and, for a default capture mode, scan it for implicit captures;
//   4. add a closure field per capture and rewrite capture uses in the body to
//      `this->field` accesses;
//   5. produce a compound literal that constructs a temporary closure object,
//      brace-initialized from the captured values.
// Returns NULL when the next token does not begin a lambda.
static ASTNode* ParseCXXLambdaExpression(Syntax* syntax,
                                         TokenClass followers) {
  if (!CompilerIsCXX() || !LexLookingAt(syntax->lex, TOK(lsquare))) {
    return NULL;
  }
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);
  Vector captures;
  VectorInit(&captures);
  LambdaCaptureDefault capture_default;
  ParseLambdaCaptureList(syntax, &captures, &capture_default, followers);

  TypeRecord* closure_type = NULL;
  NewLambdaClosureTag(syntax, location, &closure_type);
  Symbol* call_operator =
      NewLambdaCallOperator(syntax, closure_type,
                            NewTypeRecordWithSize(kTypeInt, kQualPlain),
                            /*is_mutable=*/false, location);
  ParseLambdaBody(syntax, call_operator, &captures, followers);
  if (capture_default != kLambdaCaptureDefaultNone) {
    LambdaCaptureScan scan = {&captures, call_operator->type, capture_default};
    ASTNodeVisit(call_operator->type->info.function.body,
                 CollectDefaultLambdaCaptures, 0, &scan);
  }
  AddLambdaCaptureFields(closure_type, &captures, location);
  LambdaRewrite rewrite = {
      &captures, call_operator->type->info.function.prototype.value.p[0]};
  ASTNodeVisit(call_operator->type->info.function.body,
               RewriteLambdaCaptureUses, 0, &rewrite);

  Symbol* temp = SyntaxNewTemporary(syntax, closure_type);
  temp->location = location;
  ASTNode* initializer =
      NewLambdaClosureInitializer(closure_type, &captures, location);
  ASTNode* result = NewCompoundLiteralASTNode(NewIdentifierASTNode(temp, location),
                                             location, initializer);
  VectorDestructWithContents(&captures, NULL, true);
  return result;
}

// primary-expression:
//   identifier
//   constant
//   string-literal
//   ( expression )
static ASTNode* ParsePrimaryExpression(Syntax* syntax, TokenClass followers) {
  Lex* lex = syntax->lex;

  // Check for parenthesized expression.
  // This either looks at the flag 'found_open_paren' in the Syntax
  // struct or looks for an open paren.  The syntax is slightly ambiguous
  // and the open paren can be the start of a cast expression or postfix
  // expression.
  if (syntax->found_open_paren || LexMatch(lex, TOK(lparen))) {
    syntax->found_open_paren = false;
    // GCC statement expression: ( { statements } ).  The value is that of the
    // last statement if it is an expression statement.
    if (LexLookingAt(lex, TOK(lbrace))) {
      ASTNode* compound = SyntaxParseStatement(syntax, followers | TC(closebra));
      SyntaxNeedBracket(syntax, TOK(rparen), followers);
      return NewUnaryASTNode(AST_OP(stmt_expr), NULL,
                             syntax->lex->current_token_location, compound);
    }
    ASTNode* fold = TryParseCXXFoldExpression(syntax, followers);
    if (fold != NULL) {
      return fold;
    }
    ASTNode* node = SyntaxParseExpression(syntax, followers | TC(closebra));
    SyntaxNeedBracket(syntax, TOK(rparen), followers);
    return node;
  }

  // C11 _Generic selection (lexes as an identifier).
  if (LexLookingAt(lex, TOK(identifier)) &&
      StringEqual(&lex->spelling, "_Generic")) {
    return ParseGenericSelection(syntax, followers);
  }

  // Check for identifier.  In C++ an unqualified operator-function-id (e.g.
  // `operator+`) is also a valid primary expression naming a free operator
  // function, so route a leading `operator` keyword through the same identifier
  // path (SyntaxParseFullyQualifiedIdentifier already parses the operator name).
  if (LexLookingAt(lex, TOK(identifier)) ||
      LexLookingAt(lex, TOK(coloncolon)) ||
      (CompilerIsCXX() && LexLookingAt(lex, TOK(operator)))) {
    return ParseIdentifier(syntax, followers);
  }

  if (LexLookingAt(lex, TOK(this))) {
    return ParseThisExpression(syntax);
  }

  if (LexLookingAt(lex, TOK(nullptr))) {
    LexNextToken(lex);
    TypeRecord* type = NewTypeRecordWithSize(kTypeNullPointer, kQualPlain);
    return NewIntConstantASTNode(0, type, syntax->lex->current_token_location);
  }

  if (LexLookingAt(lex, TOK(true)) || LexLookingAt(lex, TOK(false))) {
    bool value = LexLookingAt(lex, TOK(true));
    SourceLocation location = lex->current_token_location;
    LexNextToken(lex);
    return NewIntConstantASTNode(value ? 1 : 0,
                                 NewTypeRecordWithSize(kTypeBool, kQualPlain),
                                 location);
  }

  ASTNode* lambda = ParseCXXLambdaExpression(syntax, followers);
  if (lambda != NULL) {
    return lambda;
  }

  // Check for integer constant.
  if (LexLookingAt(lex, TOK(number))) {
    return ParseIntegerConstant(syntax, followers);
 }

  // Check for floating point constant.
  if (LexLookingAt(lex, TOK(fnumber))) {
    return ParseFloatingPointConstant(syntax, followers);
  }

  // Check for string literal.
  if (LexLookingAt(lex, TOK(string))) {
    return ParseStringLiteral(syntax, followers);
  }

  // Wide string literal
  if (LexLookingAt(lex, TOK(string_wide))) {
    return ParseWideStringLiteral(syntax, followers);
   }

  // Character constant.
  if (LexLookingAt(lex, TOK(charconst))) {
    return ParseCharacterConstant(syntax, followers);
  }
  
  // Wide character constant.
  if (LexLookingAt(lex, TOK(charconst_wide))) {
    return ParseWideCharacterConstant(syntax, followers);
  }

  // Invalid primary expression, error out, recover and return 0.
  SyntaxError(syntax, "Expression syntax error; primary expression expected");
  SyntaxRecover(syntax, followers);
  TypeRecord* type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  return (ASTNode*)NewIntConstantASTNode(0, type,
                                         syntax->lex->current_token_location);
}

// Check if we have a varargs intrinsic.
static ASTNode* VarargsIntrinsic(Syntax* syntax, ASTNode* left,
                               TokenClass followers) {
  if (left->op == AST_OP(identifier)) {
    IdentifierASTNode* id_node = (IdentifierASTNode*)left;
    const char* name = id_node->symbol->name.value;

    const struct Intrinsic* intrinsic = GetIntrinsic(name);
    if (intrinsic == NULL) {
      return NULL;
    }

    Vector* actuals = NewVector();
    while (!LexLookingAt(syntax->lex, TOK(rparen))) {
      ASTNode* actual;
      // Special case: __builtin_va_arg has a type as its second arg.
      if (intrinsic->opcode == AST_OP(builtin_va_arg) &&
          actuals->length == 1) {
        TypeParser parser;
        TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), kParsingBlockScope);
        TypeRecord* type = TypeParserParseType(&parser, true);
        Symbol* sym = TypeParserParseDeclarator(&parser, type);
        type = sym->type;
        actual =
            NewIntConstantASTNode(0, type, syntax->lex->current_token_location);
        SymbolDelete(sym);  // Don't need this.
        TypeParserDestruct(&parser);
      } else {
        actual = SyntaxParseSingleExpression(syntax, followers);
      }
      VectorAppend(actuals, actual);
      if (!LexMatch(syntax->lex, TOK(comma))) {
        break;
      }
    }
    SyntaxNeedBracket(syntax, TOK(rparen), followers);
    if (actuals->length != (size_t)intrinsic->num_args) {
      SyntaxError(
          syntax,
          "Wrong number of args for builtin; expected %d, got %zd",
          intrinsic->num_args, actuals->length);
    }
    return NewVectorASTNode(intrinsic->opcode, NULL,
                            syntax->lex->current_token_location, left, actuals);
  }
  return NULL;
}

// Parse an array subscript expression.
static ASTNode* ParseArraySubscript(ASTNode* left, Syntax* syntax,
                                TokenClass followers) {
  ASTNode* index = SyntaxParseExpression(syntax, followers);
  SyntaxNeedBracket(syntax, TOK(rsquare), followers);
  return NewBinaryASTNode(AST_OP(subscript), NULL,
                   syntax->lex->current_token_location, left, index);
}

// Parse a function call or varargs builtin.
static ASTNode* ParseFunctionCall(ASTNode* left, Syntax* syntax,
                                  TokenClass followers) {
  // __builtin_expect(expr, hint) is a branch-prediction hint that evaluates to
  // its first argument; the hint is ignored.
  if (left->op == AST_OP(identifier) &&
      StringEqual(&((IdentifierASTNode*)left)->symbol->name,
                  "__builtin_expect")) {
    ASTNode* value = NULL;
    while (!LexLookingAt(syntax->lex, TOK(rparen))) {
      ASTNode* arg = SyntaxParseSingleExpression(syntax, followers);
      if (value == NULL) {
        value = arg;
      } else {
        ASTNodeDelete(arg);
      }
      if (!LexMatch(syntax->lex, TOK(comma))) {
        break;
      }
    }
    SyntaxNeedBracket(syntax, TOK(rparen), followers);
    if (value == NULL) {
      value = (ASTNode*)NewIntConstantASTNode(
          0, NewTypeRecordWithSize(kTypeLong, kQualPlain),
          syntax->lex->current_token_location);
    }
    return value;
  }

  // Check for varargs intrinsic functions.
  ASTNode* varargs = VarargsIntrinsic(syntax, left, followers);
  if (varargs != NULL) {
    return varargs;
  }
  
  // Normal function call.
  Vector* actuals = NewVector();
  while (!LexLookingAt(syntax->lex, TOK(rparen))) {
    ASTNode* actual = LexMatch(syntax->lex, TOK(lbrace))
                          ? SyntaxParseBracedInitializer(syntax)
                          : SyntaxParseSingleExpression(syntax, followers);
    MarkCXXPackExpansionIfPresent(syntax, actual);
    VectorAppend(actuals, actual);
    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(syntax, TOK(rparen), followers);
  return NewVectorASTNode(AST_OP(call), NULL,
                            syntax->lex->current_token_location, left,
                            actuals);
}

static bool CXXExpressionNamesTemplateTypeParameter(ASTNode* node) {
  if (!CompilerIsCXX() || node == NULL || node->op != AST_OP(identifier)) {
    return false;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  return id->symbol != NULL && id->symbol->flags.is_template_parameter &&
         id->symbol->flags.is_template_type_parameter &&
         id->symbol->type != NULL;
}

static ASTNode* ParseCXXDependentValueInitialization(ASTNode* type_expr,
                                                     Syntax* syntax,
                                                     TokenClass followers) {
  IdentifierASTNode* id = (IdentifierASTNode*)type_expr;
  SourceLocation location = type_expr->location;
  TypeRecord* type = TypeRecordCopy(id->symbol->type);
  ASTNodeDelete(type_expr);

  ASTNode* initializer = NULL;
  if (LexLookingAt(syntax->lex, TOK(rparen))) {
    initializer = NewIntConstantASTNode(
        0, NewTypeRecordWithSize(kTypeInt, kQualPlain), location);
  } else {
    initializer = SyntaxParseSingleExpression(syntax, followers | TC(exprsep));
    if (LexMatch(syntax->lex, TOK(comma))) {
      SyntaxError(syntax,
                  "dependent function-style cast supports at most one argument");
      SyntaxRecover(syntax, TC(closebra));
    }
  }
  SyntaxNeedBracket(syntax, TOK(rparen), followers);

  ASTNode* result = NewCastASTNode(type, location, initializer);
  ((CastASTNode*)result)->kind = kCastStatic;
  ASTNodeSetType(result, type);
  return result;
}

// Whether the object expression of a member access is type-dependent (i.e.
// involves a template parameter), in which case a member named with a trailing
// template-argument list needs the `template` disambiguator under the standard.
static bool MemberAccessObjectIsDependent(ASTNode* object) {
  if (object == NULL) {
    return false;
  }
  if (object->type != NULL && TypeContainsTemplateParameter(object->type)) {
    return true;
  }
  switch (object->op) {
    case AST_OP(identifier): {
      IdentifierASTNode* id = (IdentifierASTNode*)object;
      return id->symbol != NULL && id->symbol->type != NULL &&
             TypeContainsTemplateParameter(id->symbol->type);
    }
    case AST_OP(dot):
    case AST_OP(arrow):
      return MemberAccessObjectIsDependent(((BinaryASTNode*)object)->left);
    default:
      return false;
  }
}

static ASTNode* ParseStructMember(ASTNode* left, ASTOpcode op, Syntax* syntax,
                                  TokenClass followers) {
  String* member_name;
  // Optional 'template' disambiguator in dependent member access, e.g.
  // `g.template onMessage<R>(...)` or `p->template get<0>()`.  When present the
  // member name must be a template-id, so its `<...>` is parsed as a template
  // argument list rather than a less-than comparison.
  bool saw_template_keyword =
      CompilerIsCXX() && LexMatch(syntax->lex, TOK(template));
  if (LexLookingAt(syntax->lex, TOK(identifier))) {
    member_name = NewString(syntax->lex->spelling.value);
    LexNextToken(syntax->lex);
  } else if (!saw_template_keyword && CompilerIsCXX() &&
             LexLookingAt(syntax->lex, TOK(operator))) {
    // Explicit operator / conversion call, e.g. `x.operator+(y)`,
    // `x.operator()(y)`, `p->operator int()`.  Build the same member name the
    // operator/conversion function was registered under so the access resolves
    // to it.
    String op_name;
    if (SyntaxParseMemberOperatorName(syntax, &op_name)) {
      member_name = NewString(op_name.value);
      StringDestruct(&op_name);
    } else {
      SyntaxError(syntax, "Expected operator or conversion name");
      member_name = NewString(SyntaxFakeName(syntax));
    }
  } else if (!saw_template_keyword && CompilerIsCXX() &&
             LexMatch(syntax->lex, TOK(tilde))) {
    if (LexLookingAt(syntax->lex, TOK(identifier))) {
      member_name = NewString("~");
      StringAppend(member_name, syntax->lex->spelling.value);
      LexNextToken(syntax->lex);
    } else {
      SyntaxError(syntax, "Expected destructor name");
      member_name = NewString(SyntaxFakeName(syntax));
    }
  } else {
    SyntaxError(syntax, saw_template_keyword
                            ? "Expected template member name after 'template'"
                            : "Expected struct or union member name");
    member_name = NewString(SyntaxFakeName(syntax));
  }
  bool has_template_arguments = false;
  if (saw_template_keyword) {
    // The 'template' keyword guarantees a template-id, so a following '<'
    // unambiguously opens the template argument list.
    has_template_arguments = LexLookingAt(syntax->lex, TOK(less));
  } else if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(less))) {
    LexCheckpoint checkpoint;
    LexCheckpointSave(syntax->lex, &checkpoint);
    int depth = 0;
    // True once we see a token that cannot appear at the top level of a
    // template-argument list, which means this `<` is a less-than operator
    // rather than the start of a template-id.
    bool not_a_template_id = false;
    do {
      // A `;` or a `{`/`}` brace never appears at the top level of a
      // template-argument list.  Bailing out here is not just an optimization:
      // it stops the lookahead before it can run to the end of an #include'd
      // file, which (on EOF) frees the current source out from under the
      // checkpoint we are about to restore -- a use-after-free.
      if (LexLookingAt(syntax->lex, TOK(semicolon)) ||
          LexLookingAt(syntax->lex, TOK(lbrace)) ||
          LexLookingAt(syntax->lex, TOK(rbrace))) {
        not_a_template_id = true;
        break;
      }
      if (LexLookingAt(syntax->lex, TOK(less))) {
        depth++;
      } else if (LexLookingAt(syntax->lex, TOK(greater))) {
        depth--;
      } else if (LexLookingAt(syntax->lex, TOK(greatergreater))) {
        depth -= 2;
      }
      LexNextToken(syntax->lex);
    } while (depth > 0 && !LexEof(syntax->lex));
    has_template_arguments = !not_a_template_id && depth == 0 &&
                             LexLookingAt(syntax->lex, TOK(lparen));
    LexCheckpointRestore(syntax->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
    // We resolved the `<` as a template-argument list on a type-dependent
    // object without the `template` disambiguator.  This is well-formed here
    // only thanks to a lookahead heuristic; the standard requires the keyword,
    // and other compilers reject it, so steer the user toward portable code.
    if (has_template_arguments && MemberAccessObjectIsDependent(left)) {
      SyntaxWarning(syntax, "missing-template-keyword",
                    "use 'template' keyword to treat '%s' as a dependent "
                    "template name (e.g. '%stemplate %s<...>')",
                    member_name->value,
                    op == AST_OP(arrow) ? "ptr->" : "obj.", member_name->value);
    }
  }
  Vector* template_arguments =
      has_template_arguments
          ? SyntaxParseTemplateArgumentList(syntax, followers)
          : NULL;
  ASTNode* member_node = NewStringConstantASTNode(member_name,
                                          NULL,
                                          syntax->lex->current_token_location);
  ((ConstantASTNode*)member_node)->template_arguments = template_arguments;
  return NewBinaryASTNode(op, NULL,
                          syntax->lex->current_token_location, left,
                          member_node);
}

static ASTNode* ParseCompoundLiteral(Syntax* syntax, TypeRecord* type) {
  SourceLocation location = syntax->lex->current_token_location;
  type = TypeRecordCalculateSize(type);
  Symbol* sym = SyntaxNewTemporary(syntax, type);
  ASTNode* initializer = SyntaxParseInitializer(syntax, sym, syntax->init_storage);
  return NewCompoundLiteralASTNode(NewIdentifierASTNode(sym, location),
                                   location, initializer);
}

static bool CXXPostfixExpressionNamesType(ASTNode* node) {
  if (!CompilerIsCXX() || node == NULL || node->op != AST_OP(identifier)) {
    return false;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  return id->symbol != NULL && StorageIs(id->symbol->storage, STO(typedef)) &&
         id->symbol->type != NULL && TypeIsStructOrUnion(id->symbol->type);
}

static Vector* ParseCXXBracedTemporaryActuals(Syntax* syntax, TypeRecord* type,
                                              TokenClass followers) {
  StructMember* ctor = FindCXXConstructorForType(type);
  if (CXXNewConstructorSetHasInitializerList(ctor)) {
    Vector* actuals = NewVector();
    LexNextToken(syntax->lex);
    VectorAppend(actuals, SyntaxParseBracedInitializer(syntax));
    return actuals;
  }
  return ParseCXXNewInitializerArguments(syntax, TOK(lbrace), followers);
}

static ASTNode* ParseCXXBracedTemporaryExpression(ASTNode* type_expr,
                                                  Syntax* syntax,
                                                  TokenClass followers) {
  if (!CXXPostfixExpressionNamesType(type_expr)) {
    return NULL;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)type_expr;
  TypeRecord* type = id->symbol->type;
  SourceLocation location = type_expr->location;
  if (TypeIsClassTemplatePlaceholder(type) ||
      FindCXXConstructorForType(type) != NULL) {
    Vector* actuals =
        ParseCXXBracedTemporaryActuals(syntax, type, followers);
    return NewVectorASTNode(AST_OP(call), NULL, location, type_expr, actuals);
  }
  return ParseCompoundLiteral(syntax, TypeRecordCopy(type));
}

// Parse a postfix-expression.  This is a primary expression with a postfixed
// operator.  The syntax is:

// postfix-expression:
//   primary-expression
//   postfix-expression [ expression ]
//   postfix-expression ( argument-expression-listopt )
//   postfix-expression . identifier
//   postfix-expression -> identifier
//   postfix-expression ++
//   postfix-expression --
//   ( type-name ) { initializer-list }
//   ( type-name ) { initializer-list , }

// argument-expression-list:
//   assignment-expression
//   argument-expression-list , assignment-expression

// Parse the trailing postfix operators ([], (), {}, ++, --, ., ->) of a
// postfix-expression, given an already-parsed primary `result`.  Shared by the
// ordinary postfix-expression parser and by named-cast parsing (a named cast
// such as `static_cast<T>(x)` is itself a postfix-expression and may be
// directly followed by `.member`, `[i]`, etc.).
static ASTNode* ParsePostfixOperators(Syntax* syntax, ASTNode* result,
                                      TokenClass followers) {
  while (!LexEof(syntax->lex)) {
    if (LexMatch(syntax->lex, TOK(lsquare))) {
      result = ParseArraySubscript(result, syntax, followers);
    } else if (LexMatch(syntax->lex, TOK(lparen))) {
      result = CXXExpressionNamesTemplateTypeParameter(result)
                   ? ParseCXXDependentValueInitialization(result, syntax,
                                                         followers)
                   : ParseFunctionCall(result, syntax, followers);
    } else if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(lbrace))) {
      ASTNode* braced =
          ParseCXXBracedTemporaryExpression(result, syntax, followers);
      if (braced == NULL) {
        break;
      }
      result = braced;
     } else if (LexMatch(syntax->lex, TOK(plusplus))) {
      result = NewUnaryASTNode(AST_OP(postinc), NULL,
                               syntax->lex->current_token_location, result);
    } else if (LexMatch(syntax->lex, TOK(minusminus))) {
      result = NewUnaryASTNode(AST_OP(postdec), NULL,
                               syntax->lex->current_token_location, result);
    } else if (LexMatch(syntax->lex, TOK(dot))) {
      result = ParseStructMember(result, AST_OP(dot), syntax,
                                 followers);
    } else if (LexMatch(syntax->lex, TOK(arrow))) {
      result = ParseStructMember(result, AST_OP(arrow), syntax,
                                 followers);
    } else {
      break;
    }
  }
  return result;
}

static ASTNode* ParsePostfixExpression(Syntax* syntax, TokenClass followers) {
  // A compound literal looks exactly like a cast except it is followed
  // by an initializer (in braces).  We've already consumed the ( type-name )
  // and determined it's not a cast, so we can parse the
  if (syntax->compound_literal_type != NULL && LexLookingAt(syntax->lex, TOK(lbrace))) {
    TypeRecord* type = syntax->compound_literal_type;
    syntax->compound_literal_type = NULL;
    return ParseCompoundLiteral(syntax, type);
  }
  ASTNode* result = ParsePrimaryExpression(syntax, followers);
  if (syntax->lex->assembler_mode) {
    // No postfix expressions in assembler mode.
    return result;
  }
  return ParsePostfixOperators(syntax, result, followers);
}


static ASTNode* ParsePossiblePreprocessorFunction(Syntax* syntax,
                                                  TokenClass followers) {
  
  if (StringEqual(&syntax->lex->spelling, "defined")) {
    LexNextToken(syntax->lex);
    String macro_name;
    if (LexMatch(syntax->lex, TOK(lparen))) {
      if (LexLookingAt(syntax->lex, TOK(identifier))) {
        StringInit(&macro_name, syntax->lex->spelling.value);
        LexNextToken(syntax->lex);
      } else {
        StringInit(&macro_name, NULL);
      }
      SyntaxNeedBracket(syntax, TOK(rparen), followers);
    } else if (LexLookingAt(syntax->lex, TOK(identifier))) {
      StringInit(&macro_name, syntax->lex->spelling.value);
      LexNextToken(syntax->lex);
    }
    Macro* macro =
    PreprocessorFindMacro(syntax->lex->preprocessor, &macro_name);
    TypeRecord* int_type = NewTypeRecordWithSize(kTypeInt, kQualPlain);
    return NewIntConstantASTNode(macro == NULL ? 0 : 1, int_type,
                                 syntax->lex->current_token_location);
  } else if (StringEqual(&syntax->lex->spelling, "__has_feature")) {
    LexNextToken(syntax->lex);
    if (LexMatch(syntax->lex, TOK(lparen))) {
      if (LexLookingAt(syntax->lex, TOK(identifier))) {
        // TODO: handle __has_feature?
        LexNextToken(syntax->lex);
      }
      SyntaxNeedBracket(syntax, TOK(rparen), followers);
      return NewIntConstantASTNode(0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                                   syntax->lex->current_token_location);
    }
  } else if (StringEqual(&syntax->lex->spelling, "__has_include") ||
             StringEqual(&syntax->lex->spelling, "__has_include_next")) {
    bool include_next =
    StringEqual(&syntax->lex->spelling, "__has_include_next");
    LexNextToken(syntax->lex);
    if (LexLookingAt(syntax->lex, TOK(lparen))) {
      String filename;
      bool system_include = false;
      int64_t value = 0;
      if (PreprocessorParseIncludeFilename(
                                           syntax->lex->preprocessor,
                                           &syntax->lex->line,
                                           &syntax->lex->pos,
                                           &filename, &system_include)) {
        if (include_next) {
          value = PreprocessorHasIncludeNext(syntax->lex->preprocessor,
                                             &filename, system_include);
        } else {
          value = PreprocessorHasInclude(syntax->lex->preprocessor,
                                         &filename, system_include);
        }
      }
      LexNextToken(syntax->lex);
      SyntaxNeedBracket(syntax, TOK(rparen), followers);
      return NewIntConstantASTNode(value,
                                   NewTypeRecordWithSize(kTypeInt, kQualPlain),
                                   syntax->lex->current_token_location);
    }
  }
  return NULL;
}

static ASTNode* CloneVLASize(ASTNode* node, void* data) {
  return node;
}

// Expression that is the size of an array for a VLAThis is the
// size expression of the VLA multiplied by the size of its type.
// sizeof(t) * GetSizeofVLA(t->next)
static ASTNode* GetSizeofVLA(TypeRecord* type, SourceLocation location) {
  ASTNode* size = ASTNodeClone(type->info.array.size.vla.size,
                               CloneVLASize, NULL, NULL);
  size->location = location;
  TypeRecord* t = type->next;
  do {
    ASTNode* next_size;
    if (TypeIsVLA(t)) {
      next_size = ASTNodeClone(t->info.array.size.vla.size,
                               CloneVLASize, NULL, NULL);
    } else {
      next_size = NewIntConstantASTNode(t->size,
                                   NewTypeRecordWithSize(
                                                 kTypeLong |
                                                 kTypeUnsigned,
                                                 kQualPlain), location);
    }
    size = NewBinaryASTNode(AST_OP(mult), t, location, size, next_size);
    t = t->next;
  } while (t != NULL && TypeIsArray(t));
  return size;
}

static ASTNode* ParseSizeof(Syntax* syntax, TokenClass followers) {
  SourceLocation location = syntax->lex->current_token_location;
  if (CompilerIsCXX() && LexMatch(syntax->lex, TOK(ellipsis))) {
    SyntaxNeedBracket(syntax, TOK(lparen), followers);
    if (!LexLookingAt(syntax->lex, TOK(identifier))) {
      SyntaxError(syntax, "Expected parameter pack name");
      SyntaxRecover(syntax, TC(closebra));
      SyntaxNeedBracket(syntax, TOK(rparen), followers);
      return NewSizeofPackASTNode(
          NewIntConstantASTNode(0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                                location),
          location);
    }
    Symbol* symbol = SyntaxFindSymbol(syntax, &syntax->lex->spelling);
    if (symbol == NULL || !symbol->flags.is_parameter_pack) {
      SyntaxError(syntax, "sizeof... requires a parameter pack");
      LexNextToken(syntax->lex);
      SyntaxNeedBracket(syntax, TOK(rparen), followers);
      return NewSizeofPackASTNode(
          NewIntConstantASTNode(0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                                location),
          location);
    }
    ASTNode* id = NewIdentifierASTNode(symbol, syntax->lex->current_token_location);
    LexNextToken(syntax->lex);
    SyntaxNeedBracket(syntax, TOK(rparen), followers);
    return NewSizeofPackASTNode(id, location);
  }
  bool has_brackets = LexMatch(syntax->lex, TOK(lparen));
  ASTNode* result = NULL;
  bool sizeof_type_name = false;
  if (SyntaxLookingAtType(syntax)) {
    if (!has_brackets) {
      SyntaxError(syntax,
                  "Parentheses expected around type name "
                  " in sizeof operator");
    }
    sizeof_type_name = true;
  }
  if (sizeof_type_name) {
    TypeParser parser;
    TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
    TypeRecord* type = TypeParserParseType(&parser, true);
    int size = -1;
    Symbol* sym = TypeParserParseDeclarator(&parser, type);
    if (sym != NULL) {
      if (TypeIsVLA(sym->type)) {
        // The size of a VLA is its size expression.
        result = GetSizeofVLA(sym->type,
                              syntax->lex->current_token_location);
        SymbolDelete(sym);
        TypeParserDestruct(&parser);
        goto done;
      }
      if (CompilerIsCXX() && TypeContainsTemplateParameter(sym->type)) {
        // The size depends on a template parameter, so it cannot be computed
        // until the enclosing template is instantiated.  Retain the operand
        // type so it can be substituted and re-measured then.
        result = NewSizeofASTNodeWithType(
            sym->type, syntax->lex->current_token_location);
        SymbolDelete(sym);
        TypeParserDestruct(&parser);
        goto done;
      }
      size = sym->type->size;
      SymbolDelete(sym);
    }
    TypeParserDestruct(&parser);
    assert(size != -1);
    result = NewSizeofASTNodeWithKnownSize(size,
                                           syntax->lex->current_token_location);
  } else {
    if (has_brackets) {
      syntax->found_open_paren = true;
      has_brackets = false;
    }
    ASTNode* expr = ParseUnaryExpression(syntax, followers);
    result = NewSizeofASTNodeWithExpression(expr,
                                            syntax->lex->current_token_location);
  }
done:
  if (has_brackets) {
    SyntaxNeedBracket(syntax, TOK(rparen), followers);
  }
  return result;
}

// Parses a typeid operator: `typeid ( type-id )` or `typeid ( expression )`.
// Disambiguates via SyntaxLookingAtType, mirroring the sizeof type/expression
// split.  The resulting node is rewritten into a type_info access during
// semantic analysis.
static ASTNode* ParseTypeid(Syntax* syntax, TokenClass followers) {
  SourceLocation location = syntax->lex->current_token_location;
  SyntaxNeedBracket(syntax, TOK(lparen), followers);
  ASTNode* result;
  if (SyntaxLookingAtType(syntax)) {
    TypeParser parser;
    TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                   syntax->context);
    TypeRecord* type = TypeParserParseType(&parser, true);
    Symbol* sym = TypeParserParseDeclarator(&parser, type);
    if (sym != NULL && sym->type != NULL) {
      // Retain the declarator's type (sym is intentionally not deleted, like
      // the cast-expression type-name path).
      type = sym->type;
    }
    TypeParserDestruct(&parser);
    result = NewTypeidASTNodeWithType(type, location);
  } else {
    ASTNode* expr = SyntaxParseExpression(syntax, followers | TC(closebra));
    result = NewTypeidASTNodeWithExpression(expr, location);
  }
  SyntaxNeedBracket(syntax, TOK(rparen), followers);
  return result;
}

static Symbol* FindCXXAllocationFunctionByArgCount(Symbol* first,
                                                   size_t arg_count);

static Symbol* GetImplicitCXXAllocationFunction(const char* name,
                                                TypeRecord* return_type,
                                                TypeRecord* arg_type,
                                                SourceLocation location) {
  String symbol_name;
  StringInit(&symbol_name, name);
  Symbol* symbol = FindGlobalSymbol(&symbol_name);
  StringDestruct(&symbol_name);
  Symbol* existing = FindCXXAllocationFunctionByArgCount(symbol, 1);
  if (existing != NULL) {
    TypeRecordDelete(return_type);
    TypeRecordDelete(arg_type);
    return existing;
  }

  TypeRecord* func_type = NewFunctionTypeRecord();
  TypeRecordChain(func_type, return_type);
  Symbol* formal = NewSymbol(SyntaxFakeName(&compiler->syntax), arg_type,
                             STO(auto));
  formal->flags.is_argument = true;
  formal->flags.invented = true;
  VectorAppend(&func_type->info.function.prototype, formal);

  symbol = NewSymbol(name, func_type, STO(extern));
  symbol->flags.invented = true;
  symbol->location = location;
  func_type->info.function.symbol = symbol;
  SymbolSetCXXMangledAsmName(symbol);
  if (FindGlobalSymbol(&symbol->name) == NULL) {
    bool ok = InsertGlobalSymbol(symbol);
    assert(ok);
    (void)ok;
  } else {
    Symbol* first = FindGlobalSymbol(&symbol->name);
    Symbol* tail = first;
    while (tail->overload_next != NULL) {
      tail = tail->overload_next;
    }
    symbol->namespace_ = first->namespace_;
    tail->overload_next = symbol;
    first->flags.is_overloaded = true;
    symbol->flags.is_overloaded = true;
    SymbolSetCXXMangledAsmName(first);
    SymbolSetCXXMangledAsmName(symbol);
  }
  return symbol;
}

static Symbol* FindCXXAllocationFunctionByArgCount(Symbol* first,
                                                   size_t arg_count) {
  for (Symbol* symbol = first; symbol != NULL; symbol = symbol->overload_next) {
    if (symbol->type != NULL && TypeIsFunction(symbol->type) &&
        symbol->type->info.function.prototype.length == arg_count) {
      return symbol;
    }
  }
  return NULL;
}

static Symbol* FindGlobalCXXAllocationFunction(const char* name,
                                               size_t arg_count) {
  String symbol_name;
  StringInit(&symbol_name, name);
  Symbol* first = FindGlobalSymbol(&symbol_name);
  StringDestruct(&symbol_name);
  return FindCXXAllocationFunctionByArgCount(first, arg_count);
}

static Symbol* GetImplicitCXXOperatorNew(SourceLocation location) {
  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecord* return_type = NewPointerTo(kQualPlain, void_type);
  return GetImplicitCXXAllocationFunction("operator new", return_type,
                                          NewSizeTypeRecord(), location);
}

static Symbol* GetImplicitCXXOperatorNewArray(SourceLocation location) {
  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecord* return_type = NewPointerTo(kQualPlain, void_type);
  return GetImplicitCXXAllocationFunction("operator new[]", return_type,
                                          NewSizeTypeRecord(), location);
}

static Symbol* GetImplicitCXXPlacementOperatorNew(SourceLocation location) {
  Symbol* existing = FindGlobalCXXAllocationFunction("operator new", 2);
  if (existing != NULL) {
    return existing;
  }

  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecord* return_type = NewPointerTo(kQualPlain, void_type);
  TypeRecord* func_type = NewFunctionTypeRecord();
  TypeRecordChain(func_type, return_type);

  Symbol* size_formal = NewSymbol(SyntaxFakeName(&compiler->syntax),
                                  NewSizeTypeRecord(), STO(auto));
  size_formal->flags.is_argument = true;
  size_formal->flags.invented = true;
  VectorAppend(&func_type->info.function.prototype, size_formal);

  TypeRecord* ptr_type =
      NewPointerTo(kQualPlain, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
  Symbol* ptr_formal = NewSymbol(SyntaxFakeName(&compiler->syntax), ptr_type,
                                 STO(auto));
  ptr_formal->flags.is_argument = true;
  ptr_formal->flags.invented = true;
  VectorAppend(&func_type->info.function.prototype, ptr_formal);

  Symbol* symbol = NewSymbol("operator new", func_type, STO(extern));
  symbol->flags.invented = true;
  symbol->location = location;
  func_type->info.function.symbol = symbol;
  SymbolSetCXXMangledAsmName(symbol);

  Symbol* first = FindGlobalSymbol(&symbol->name);
  if (first == NULL) {
    bool ok = InsertGlobalSymbol(symbol);
    assert(ok);
    (void)ok;
  } else {
    Symbol* tail = first;
    while (tail->overload_next != NULL) {
      tail = tail->overload_next;
    }
    symbol->namespace_ = first->namespace_;
    tail->overload_next = symbol;
    first->flags.is_overloaded = true;
    symbol->flags.is_overloaded = true;
    SymbolSetCXXMangledAsmName(first);
    SymbolSetCXXMangledAsmName(symbol);
  }
  return symbol;
}

static Symbol* GetImplicitCXXOperatorDelete(SourceLocation location) {
  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecord* arg_type =
      NewPointerTo(kQualPlain, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
  return GetImplicitCXXAllocationFunction("operator delete", void_type,
                                          arg_type, location);
}

static Symbol* GetImplicitCXXOperatorDeleteArray(SourceLocation location) {
  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecord* arg_type =
      NewPointerTo(kQualPlain, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
  return GetImplicitCXXAllocationFunction("operator delete[]", void_type,
                                          arg_type, location);
}

static Symbol* GetCXXClassAllocationFunction(TypeRecord* type,
                                             const char* name,
                                             size_t arg_count) {
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL) {
    return NULL;
  }
  StructMember* member = FindStructMemberByName(type->info.struct_info, name);
  if (member == NULL || !member->is_member_function ||
      member->symbol == NULL || !TypeIsFunction(member->symbol->type)) {
    return NULL;
  }
  return FindCXXAllocationFunctionByArgCount(member->symbol, arg_count);
}

static Symbol* GetCXXOperatorNewForType(TypeRecord* type,
                                        bool is_array,
                                        size_t arg_count,
                                        SourceLocation location) {
  Symbol* member = GetCXXClassAllocationFunction(
      type, is_array ? "operator new[]" : "operator new", arg_count);
  if (member != NULL) {
    return member;
  }
  const char* name = is_array ? "operator new[]" : "operator new";
  Symbol* global = FindGlobalCXXAllocationFunction(name, arg_count);
  if (global != NULL) {
    return global;
  }
  if (arg_count == 1) {
    return is_array ? GetImplicitCXXOperatorNewArray(location)
                    : GetImplicitCXXOperatorNew(location);
  }
  if (!is_array && arg_count == 2) {
    return GetImplicitCXXPlacementOperatorNew(location);
  }
  SyntaxError(&compiler->syntax, "No matching allocation function for placement new");
  return is_array ? GetImplicitCXXOperatorNewArray(location)
                  : GetImplicitCXXOperatorNew(location);
}

static Symbol* GetCXXOperatorDeleteForType(TypeRecord* type,
                                           bool is_array,
                                           SourceLocation location) {
  Symbol* member = GetCXXClassAllocationFunction(
      type, is_array ? "operator delete[]" : "operator delete", 1);
  if (member != NULL) {
    return member;
  }
  return is_array ? GetImplicitCXXOperatorDeleteArray(location)
                  : GetImplicitCXXOperatorDelete(location);
}

static ASTNode* NewCallASTNode(Symbol* callee, SourceLocation location,
                               Vector* actuals) {
  return NewVectorASTNode(AST_OP(call), NULL, location,
                          NewIdentifierASTNode(callee, location), actuals);
}

static StructMember* FindCXXConstructorForType(TypeRecord* type) {
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL ||
      type->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  StructMember* ctor =
      FindStructMember(type->info.struct_info, type->info.struct_info->tag_name);
  if (ctor == NULL || !ctor->is_member_function ||
      !ctor->symbol->type->info.function.is_constructor) {
    return NULL;
  }
  return ctor;
}

static bool CXXNewConstructorSetHasInitializerList(StructMember* ctor) {
  for (StructMember* candidate = ctor; candidate != NULL;
       candidate = candidate->overload_next) {
    if (!candidate->is_member_function || candidate->symbol == NULL ||
        candidate->symbol->type == NULL ||
        !TypeIsFunction(candidate->symbol->type)) {
      continue;
    }
    FunctionInfo* info = &candidate->symbol->type->info.function;
    if (!info->is_constructor) {
      continue;
    }
    for (size_t i = 0; i < info->prototype.length; i++) {
      Symbol* formal = info->prototype.value.p[i];
      TypeRecord* formal_type = formal->type;
      if (TypeIsReference(formal_type)) {
        formal_type = formal_type->next;
      }
      if (TypeIsCXXInitializerList(formal_type)) {
        return true;
      }
    }
  }
  return false;
}

static Vector* ParseCXXNewInitializerArguments(Syntax* syntax, Token open,
                                               TokenClass followers) {
  Token close = open == TOK(lparen) ? TOK(rparen) : TOK(rbrace);
  LexNextToken(syntax->lex);
  Vector* actuals = NewVector();
  while (!LexLookingAt(syntax->lex, close)) {
    ASTNode* actual = SyntaxParseSingleExpression(syntax, followers | TC(exprsep));
    MarkCXXPackExpansionIfPresent(syntax, actual);
    VectorAppend(actuals, actual);
    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(syntax, close, followers | TC(exprsep));
  return actuals;
}

static bool CXXNewHasPlacementArguments(Syntax* syntax) {
  if (!LexLookingAt(syntax->lex, TOK(lparen))) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  LexNextToken(syntax->lex);
  bool result =
      !LexLookingAt(syntax->lex, TOK(rparen)) && !SyntaxLookingAtType(syntax);
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return result;
}

static ASTNode* NewCXXConstructorCallForReceiver(TypeRecord* allocated_type,
                                                 ASTNode* receiver,
                                                 Vector* actuals,
                                                 SourceLocation location);

static bool TypeNeedsCXXCompleteObjectArgument(TypeRecord* type) {
  return TypeIsStructOrUnion(type) && type->info.struct_info != NULL &&
         StructHasVirtualBases(type->info.struct_info);
}

static void CXXPrependCompleteObjectArgument(TypeRecord* type, Vector* actuals,
                                             SourceLocation location) {
  if (actuals == NULL || !TypeNeedsCXXCompleteObjectArgument(type)) {
    return;
  }
  ASTNode* arg =
      NewIntConstantASTNode(1, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                            location);
  if (actuals->length == 0) {
    VectorAppend(actuals, arg);
  } else {
    VectorInsertBefore(actuals, 0, arg);
  }
}

static ASTNode* NewCXXConstructorCallForPointer(TypeRecord* allocated_type,
                                                Symbol* ptr,
                                                Vector* actuals,
                                                SourceLocation location) {
  ASTNode* receiver = NewUnaryASTNode(AST_OP(contents), allocated_type, location,
                                      NewIdentifierASTNode(ptr, location));
  return NewCXXConstructorCallForReceiver(allocated_type, receiver, actuals,
                                          location);
}

static ASTNode* NewCXXConstructorCallForReceiver(TypeRecord* allocated_type,
                                                 ASTNode* receiver,
                                                 Vector* actuals,
                                                 SourceLocation location) {
  CXXPrependCompleteObjectArgument(allocated_type, actuals, location);
  ASTNode* member =
      NewStringConstantASTNode(NewString(allocated_type->info.struct_info
                                             ->tag_name->value),
                               NULL, location);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, member);
  return NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals);
}

static StructMember* FindCXXDestructorForType(TypeRecord* type) {
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL ||
      type->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  String name;
  StringInit(&name, "~");
  StringAppendString(&name, type->info.struct_info->tag_name);
  StructMember* destructor = FindStructMember(type->info.struct_info, &name);
  StringDestruct(&name);
  if (destructor == NULL || !destructor->is_member_function ||
      !destructor->symbol->type->info.function.is_destructor) {
    return NULL;
  }
  return destructor;
}

static ASTNode* NewCXXDestructorCallForPointer(TypeRecord* object_type,
                                               Symbol* ptr,
                                               SourceLocation location) {
  String destructor_name;
  StringInit(&destructor_name, "~");
  StringAppendString(&destructor_name, object_type->info.struct_info->tag_name);
  ASTNode* receiver =
      NewUnaryASTNode(AST_OP(contents), object_type, location,
                      NewIdentifierASTNode(ptr, location));
  ASTNode* member =
      NewStringConstantASTNode(NewString(destructor_name.value), NULL,
                               location);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, member);
  StringDestruct(&destructor_name);
  Vector* actuals = NewVector();
  CXXPrependCompleteObjectArgument(object_type, actuals, location);
  return NewVectorASTNode(AST_OP(call), NULL, location, member_access,
                          actuals);
}

static ASTNode* NewExpressionStatement(ASTNode* expr, SourceLocation location) {
  return NewExpressionStatementASTNode(expr, location);
}

static ASTNode* NewAssign(ASTNode* left, ASTNode* right, TypeRecord* type,
                          SourceLocation location) {
  return NewBinaryASTNode(AST_OP(assign), type, location, left, right);
}

static ASTNode* NewTypedCast(TypeRecord* type, ASTNode* expr,
                             SourceLocation location) {
  ASTNode* result = NewCastASTNode(type, location, expr);
  ((CastASTNode*)result)->kind = kCastStatic;
  return result;
}

static ASTNode* NewPtrAdd(ASTNode* ptr, ASTNode* offset,
                          SourceLocation location) {
  return NewBinaryASTNode(AST_OP(plus), NULL, location, ptr, offset);
}

static ASTNode* NewPtrSub(ASTNode* ptr, ASTNode* offset,
                          SourceLocation location) {
  return NewBinaryASTNode(AST_OP(minus), NULL, location, ptr, offset);
}

static ASTNode* NewIntLiteral(int64_t value, SourceLocation location) {
  return NewIntConstantASTNode(value,
                               NewTypeRecordWithSize(kTypeInt, kQualConst),
                               location);
}

static ASTNode* NewPostIncrement(Symbol* sym, SourceLocation location) {
  return NewUnaryASTNode(AST_OP(postinc), NULL, location,
                         NewIdentifierASTNode(sym, location));
}

static ASTNode* NewArrayConstructionLoop(Syntax* syntax, TypeRecord* element_type,
                                         Symbol* ptr, Symbol* count,
                                         SourceLocation location) {
  Symbol* index =
      SyntaxNewTemporary(syntax, NewTypeRecordWithSize(kTypeInt, kQualPlain));
  Symbol* element_ptr =
      SyntaxNewTemporary(syntax, NewPointerTo(kQualPlain, element_type));
  Vector* statements = NewVector();
  VectorAppend(statements,
               NewExpressionStatement(
                   NewAssign(NewIdentifierASTNode(index, location),
                             NewIntLiteral(0, location), index->type,
                             location),
                   location));

  Vector* body_statements = NewVector();
  VectorAppend(body_statements,
               NewExpressionStatement(
                   NewAssign(NewIdentifierASTNode(element_ptr, location),
                             NewPtrAdd(NewIdentifierASTNode(ptr, location),
                                       NewIdentifierASTNode(index, location),
                                       location),
                             element_ptr->type, location),
                   location));
  VectorAppend(body_statements,
               NewExpressionStatement(
                   NewCXXConstructorCallForPointer(element_type, element_ptr,
                                                   NewVector(), location),
                   location));
  VectorAppend(body_statements,
               NewExpressionStatement(NewPostIncrement(index, location),
                                      location));
  ASTNode* body = NewCompoundStatementASTNode(body_statements, location);
  ASTNode* cond =
      NewBinaryASTNode(AST_OP(less), NULL, location,
                       NewIdentifierASTNode(index, location),
                       NewIdentifierASTNode(count, location));
  VectorAppend(statements,
               NewCombinedStatementASTNode(AST_OP(while), cond, body,
                                           location));
  return NewCompoundStatementASTNode(statements, location);
}

static ASTNode* NewArrayDestructionLoop(Syntax* syntax, TypeRecord* element_type,
                                        Symbol* ptr, Symbol* count,
                                        SourceLocation location) {
  Symbol* index =
      SyntaxNewTemporary(syntax, NewTypeRecordWithSize(kTypeInt, kQualPlain));
  Symbol* element_ptr =
      SyntaxNewTemporary(syntax, NewPointerTo(kQualPlain, element_type));
  Vector* statements = NewVector();
  VectorAppend(statements,
               NewExpressionStatement(
                   NewAssign(NewIdentifierASTNode(index, location),
                             NewIntLiteral(0, location), index->type,
                             location),
                   location));

  Vector* body_statements = NewVector();
  VectorAppend(body_statements,
               NewExpressionStatement(
                   NewAssign(NewIdentifierASTNode(element_ptr, location),
                             NewPtrAdd(NewIdentifierASTNode(ptr, location),
                                       NewIdentifierASTNode(index, location),
                                       location),
                             element_ptr->type, location),
                   location));
  VectorAppend(body_statements,
               NewExpressionStatement(
                   NewCXXDestructorCallForPointer(element_type, element_ptr,
                                                  location),
                   location));
  VectorAppend(body_statements,
               NewExpressionStatement(NewPostIncrement(index, location),
                                      location));
  ASTNode* body = NewCompoundStatementASTNode(body_statements, location);
  ASTNode* cond =
      NewBinaryASTNode(AST_OP(less), NULL, location,
                       NewIdentifierASTNode(index, location),
                       NewIdentifierASTNode(count, location));
  VectorAppend(statements,
               NewCombinedStatementASTNode(AST_OP(while), cond, body,
                                           location));
  return NewCompoundStatementASTNode(statements, location);
}

static ASTNode* IdentityCloneNodeForCXXNew(ASTNode* node, void* data) {
  (void)data;
  return node;
}

static Vector* CloneAndAnalyzeCXXNewDeductionActuals(Vector* actuals) {
  Vector* clones = NewVector();
  for (size_t i = 0; actuals != NULL && i < actuals->length; i++) {
    ASTNode* actual = actuals->value.p[i];
    if (actual == NULL) {
      continue;
    }
    ASTNode* clone =
        ASTNodeClone(actual, IdentityCloneNodeForCXXNew, NULL, NULL);
    VectorAppend(clones, AnalyzeExpression(clone));
  }
  return clones;
}

static TypeRecord* DeduceCXXNewAllocatedType(Syntax* syntax,
                                             TypeRecord* allocated_type,
                                             Vector* actuals) {
  if (!CompilerIsCXX() || syntax == NULL ||
      !TypeIsClassTemplatePlaceholder(allocated_type) || actuals == NULL) {
    return allocated_type;
  }
  Symbol* class_template = TypeClassTemplatePlaceholderOrigin(allocated_type);
  Vector* deduction_actuals = CloneAndAnalyzeCXXNewDeductionActuals(actuals);
  bool alias_rejected = false;
  TypeRecord* deduced =
      TypeDeduceClassTemplateFromPlaceholder(syntax, allocated_type,
                                             deduction_actuals,
                                             /*allow_explicit=*/true,
                                             &alias_rejected);
  VectorDeleteWithContents(deduction_actuals,
                           (VectorElementDestructor)ASTNodeDelete,
                           /*free_element=*/false);
  if (deduced != NULL &&
      !TypeClassTemplatePlaceholderAcceptsDeduced(allocated_type, deduced)) {
    SyntaxError(syntax,
                "Deduced template arguments do not match alias template");
    TypeRecordDelete(deduced);
    deduced = NULL;
  }
  if (deduced == NULL) {
    if (alias_rejected) {
      SyntaxError(syntax,
                  "Deduced template arguments do not match alias template");
    } else {
      SyntaxError(syntax, "Could not deduce template arguments for %s",
                  class_template != NULL ? class_template->name.value
                                         : "<class template>");
    }
    return allocated_type;
  }
  TypeRecordDelete(allocated_type);
  return deduced;
}

static Vector* ParseCXXNewDeductionInitializerArguments(Syntax* syntax,
                                                        TypeRecord* type,
                                                        Token open,
                                                        TokenClass followers) {
  if (open == TOK(lbrace) &&
      CXXNewConstructorSetHasInitializerList(FindCXXConstructorForType(type))) {
    Vector* actuals = NewVector();
    LexNextToken(syntax->lex);
    VectorAppend(actuals, SyntaxParseBracedInitializer(syntax));
    return actuals;
  }
  return ParseCXXNewInitializerArguments(syntax, open, followers);
}

static ASTNode* ParseCXXNewExpression(Syntax* syntax, TokenClass followers) {
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // new

  Vector* placement_actuals = NULL;
  if (CXXNewHasPlacementArguments(syntax)) {
    placement_actuals =
        ParseCXXNewInitializerArguments(syntax, TOK(lparen), followers);
  }

  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  TypeRecord* allocated_type = TypeParserParseType(&parser, true);
  Token initializer_open = TOK(bad);
  ASTNode* array_size = NULL;
  if (LexLookingAt(syntax->lex, TOK(lparen)) ||
      LexLookingAt(syntax->lex, TOK(lbrace)) ||
      LexLookingAt(syntax->lex, TOK(lsquare))) {
    initializer_open = syntax->lex->current_token;
  }
  Symbol* sym = initializer_open == TOK(bad)
      ? TypeParserParseDeclarator(&parser, allocated_type)
      : NULL;
  if (sym == NULL) {
    if (allocated_type == NULL) {
      SyntaxError(syntax, "Invalid type in new expression");
      allocated_type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
    }
  } else {
    allocated_type = sym->type;
  }
  if (initializer_open == TOK(bad) &&
      (LexLookingAt(syntax->lex, TOK(lparen)) ||
       LexLookingAt(syntax->lex, TOK(lbrace)) ||
       LexLookingAt(syntax->lex, TOK(lsquare)))) {
    initializer_open = syntax->lex->current_token;
  }
  TypeParserDestruct(&parser);
  Vector* ctor_actuals = NULL;
  if (TypeIsClassTemplatePlaceholder(allocated_type) &&
      (initializer_open == TOK(lparen) || initializer_open == TOK(lbrace))) {
    ctor_actuals = ParseCXXNewDeductionInitializerArguments(
        syntax, allocated_type, initializer_open, followers);
    allocated_type = DeduceCXXNewAllocatedType(syntax, allocated_type,
                                               ctor_actuals);
  }
  if (TypeIsClassTemplatePlaceholder(allocated_type)) {
    SyntaxError(syntax,
                "Class template argument deduction requires an initializer");
  }
  TypeRecordCalculateSize(allocated_type);
  if (TypeIsAbstractClass(allocated_type)) {
    SyntaxError(syntax, "Cannot allocate object of abstract class %s",
                allocated_type->info.struct_info->tag_name != NULL
                    ? allocated_type->info.struct_info->tag_name->value
                    : "<anonymous>");
  }

  ASTNode* scalar_initializer = NULL;
  if (initializer_open == TOK(lsquare)) {
    LexNextToken(syntax->lex);
    array_size = SyntaxParseSingleExpression(syntax, TC(closebra));
    SyntaxNeedBracket(syntax, TOK(rsquare), followers);
  } else if (ctor_actuals == NULL &&
             (initializer_open == TOK(lparen) || initializer_open == TOK(lbrace))) {
    if (FindCXXConstructorForType(allocated_type) == NULL) {
      Vector* initializers =
          ParseCXXNewInitializerArguments(syntax, initializer_open, followers);
      if (initializers->length == 1) {
        scalar_initializer = initializers->value.p[0];
        VectorDestruct(initializers);
      } else {
        SyntaxError(syntax, "new initializer for non-class type requires one expression");
        VectorDelete(initializers);
      }
    } else {
      ctor_actuals =
          ParseCXXNewInitializerArguments(syntax, initializer_open, followers);
    }
  } else if (ctor_actuals == NULL &&
             FindCXXConstructorForType(allocated_type) != NULL) {
    ctor_actuals = NewVector();
  }

  Vector* actuals = NewVector();
  ASTNode* allocation_size =
      NewSizeofASTNodeWithKnownSize(allocated_type->size, location);
  Symbol* array_count = NULL;
  if (array_size != NULL) {
    array_count = SyntaxNewTemporary(syntax, NewSizeTypeRecord());
    allocation_size =
        NewBinaryASTNode(AST_OP(plus), NULL, location,
                         NewBinaryASTNode(AST_OP(mult), NULL, location,
                                          NewIdentifierASTNode(array_count,
                                                               location),
                                          allocation_size),
                         NewSizeofASTNodeWithKnownSize(
                             NewSizeTypeRecord()->size, location));
  }
  VectorAppend(actuals, allocation_size);
  if (placement_actuals != NULL) {
    for (size_t i = 0; i < placement_actuals->length; i++) {
      VectorAppend(actuals, placement_actuals->value.p[i]);
    }
    VectorDelete(placement_actuals);
  }
  ASTNode* allocation =
      NewCallASTNode(GetCXXOperatorNewForType(allocated_type,
                                              array_size != NULL,
                                              actuals->length,
                                              location),
                     location, actuals);
  TypeRecord* result_type = NewPointerTo(kQualPlain, allocated_type);
  ASTNode* result = NewCastASTNode(result_type, location, allocation);
  ((CastASTNode*)result)->kind = kCastStatic;
  if (array_size != NULL) {
    TypeRecord* size_type = NewSizeTypeRecord();
    TypeRecord* size_ptr_type = NewPointerTo(kQualPlain, size_type);
    Symbol* header = SyntaxNewTemporary(syntax, size_ptr_type);
    Symbol* object_ptr = SyntaxNewTemporary(syntax, result_type);
    Vector* statements = NewVector();
    VectorAppend(statements,
                 NewExpressionStatement(
                     NewAssign(NewIdentifierASTNode(array_count, location),
                               array_size, array_count->type, location),
                     location));
    VectorAppend(statements,
                 NewExpressionStatement(
                     NewAssign(NewIdentifierASTNode(header, location),
                               NewTypedCast(size_ptr_type, result, location),
                               header->type, location),
                     location));
    VectorAppend(statements,
                 NewExpressionStatement(
                     NewAssign(NewUnaryASTNode(AST_OP(contents), size_type,
                                              location,
                                              NewIdentifierASTNode(header,
                                                                   location)),
                               NewIdentifierASTNode(array_count, location),
                               size_type, location),
                     location));
    VectorAppend(statements,
                 NewExpressionStatement(
                     NewAssign(NewIdentifierASTNode(object_ptr, location),
                               NewTypedCast(result_type,
                                            NewPtrAdd(
                                                NewIdentifierASTNode(header,
                                                                     location),
                                                NewIntLiteral(1, location),
                                                location),
                                            location),
                               object_ptr->type, location),
                     location));
    if (TypeIsStructOrUnion(allocated_type) &&
        FindCXXConstructorForType(allocated_type) != NULL) {
      VectorAppend(statements,
                   NewArrayConstructionLoop(syntax, allocated_type, object_ptr,
                                            array_count, location));
    }
    VectorAppend(statements,
                 NewExpressionStatement(NewIdentifierASTNode(object_ptr,
                                                             location),
                                        location));
    result = NewUnaryASTNode(AST_OP(stmt_expr), result_type, location,
                             NewCompoundStatementASTNode(statements, location));
  } else if (ctor_actuals != NULL || scalar_initializer != NULL) {
    Symbol* temp = SyntaxNewTemporary(syntax, result_type);
    ASTNode* assign =
        NewBinaryASTNode(AST_OP(assign), result_type, location,
                         NewIdentifierASTNode(temp, location), result);
    ASTNode* init = ctor_actuals != NULL
        ? NewCXXConstructorCallForPointer(allocated_type, temp, ctor_actuals,
                                          location)
        : NewAssign(NewUnaryASTNode(AST_OP(contents), allocated_type, location,
                                    NewIdentifierASTNode(temp, location)),
                    scalar_initializer, allocated_type, location);
    if (ctor_actuals == NULL && TypeContainsTemplateParameter(allocated_type)) {
      init->flags |= kASTDependentNewInitializer;
    }
    result = NewBinaryASTNode(
        AST_OP(comma), result_type, location, assign,
        NewBinaryASTNode(AST_OP(comma), result_type, location, init,
                         NewIdentifierASTNode(temp, location)));
  }
  if (sym != NULL) {
    SymbolDelete(sym);
  }
  return result;
}

ASTNode* NewCXXDeleteExpressionForPointer(Syntax* syntax, ASTNode* expr,
                                          bool is_array_delete,
                                          SourceLocation location) {
  TypeRecord* pointer_type = expr->type;
  if (pointer_type != NULL && TypeContainsTemplateParameter(pointer_type)) {
    Vector* actuals = NewVector();
    VectorAppend(actuals, expr);
    ASTNode* node = NewCallASTNode(
        is_array_delete ? GetImplicitCXXOperatorDeleteArray(location)
                        : GetImplicitCXXOperatorDelete(location),
        location, actuals);
    node->flags |= kASTDependentDelete;
    if (is_array_delete) {
      node->flags |= kASTDependentArrayDelete;
    }
    return node;
  }
  if (is_array_delete) {
    if (pointer_type == NULL || !TypeIsPointerOrArray(pointer_type)) {
      Vector* actuals = NewVector();
      VectorAppend(actuals, expr);
    return NewCallASTNode(GetImplicitCXXOperatorDeleteArray(location),
                            location, actuals);
    }
    TypeRecord* size_type = NewSizeTypeRecord();
    TypeRecord* size_ptr_type = NewPointerTo(kQualPlain, size_type);
    Symbol* object_ptr = SyntaxNewTemporary(syntax, pointer_type);
    Symbol* header = SyntaxNewTemporary(syntax, size_ptr_type);
    Symbol* count = SyntaxNewTemporary(syntax, size_type);
    Vector* statements = NewVector();
    VectorAppend(statements,
                 NewExpressionStatement(
                     NewAssign(NewIdentifierASTNode(object_ptr, location),
                               expr, object_ptr->type, location),
                     location));
    VectorAppend(statements,
                 NewExpressionStatement(
                     NewAssign(NewIdentifierASTNode(header, location),
                               NewPtrSub(NewTypedCast(size_ptr_type,
                                                      NewIdentifierASTNode(
                                                          object_ptr,
                                                          location),
                                                      location),
                                         NewIntLiteral(1, location), location),
                               header->type, location),
                     location));
    VectorAppend(statements,
                 NewExpressionStatement(
                     NewAssign(NewIdentifierASTNode(count, location),
                               NewUnaryASTNode(AST_OP(contents), size_type,
                                                location,
                                                NewIdentifierASTNode(header,
                                                                     location)),
                               count->type, location),
                     location));
    if (TypeIsStructOrUnionPointer(pointer_type) &&
        FindCXXDestructorForType(pointer_type->next) != NULL) {
      VectorAppend(statements,
                   NewArrayDestructionLoop(syntax, pointer_type->next,
                                           object_ptr, count, location));
    }
    Vector* actuals = NewVector();
    VectorAppend(actuals, NewIdentifierASTNode(header, location));
    VectorAppend(statements,
                 NewExpressionStatement(
                     NewCallASTNode(GetCXXOperatorDeleteForType(
                                        pointer_type->next, true, location),
                                    location, actuals),
                     location));
    return NewUnaryASTNode(
        AST_OP(stmt_expr), NewTypeRecordWithSize(kTypeVoid, kQualPlain),
        location, NewCompoundStatementASTNode(statements, location));
  }
  if (pointer_type == NULL || !TypeIsStructOrUnionPointer(pointer_type) ||
      FindCXXDestructorForType(pointer_type->next) == NULL) {
    Vector* actuals = NewVector();
    VectorAppend(actuals, expr);
    TypeRecord* object_type =
        TypeIsPointerOrArray(pointer_type) ? pointer_type->next : NULL;
    return NewCallASTNode(GetCXXOperatorDeleteForType(object_type, false,
                                                     location),
                          location, actuals);
  }

  Symbol* temp = SyntaxNewTemporary(syntax, pointer_type);
  ASTNode* assign =
      NewBinaryASTNode(AST_OP(assign), pointer_type, location,
                       NewIdentifierASTNode(temp, location), expr);
  ASTNode* destructor =
      NewCXXDestructorCallForPointer(pointer_type->next, temp, location);
  Vector* actuals = NewVector();
  VectorAppend(actuals, NewIdentifierASTNode(temp, location));
  ASTNode* deallocate =
      NewCallASTNode(GetCXXOperatorDeleteForType(pointer_type->next, false,
                                                 location),
                     location, actuals);
  return NewBinaryASTNode(
      AST_OP(comma), NewTypeRecordWithSize(kTypeVoid, kQualPlain), location,
      assign, NewBinaryASTNode(AST_OP(comma),
                               NewTypeRecordWithSize(kTypeVoid, kQualPlain),
                               location, destructor, deallocate));
}

static ASTNode* ParseCXXDeleteExpression(Syntax* syntax, TokenClass followers) {
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // delete
  bool is_array_delete = false;
  if (LexMatch(syntax->lex, TOK(lsquare))) {
    SyntaxNeedBracket(syntax, TOK(rsquare), followers);
    is_array_delete = true;
  }
  ASTNode* expr = ParseCastExpression(syntax, followers);
  return NewCXXDeleteExpressionForPointer(syntax, expr, is_array_delete,
                                          location);
}

static ASTNode* ParseCXXThrowExpression(Syntax* syntax, TokenClass followers) {
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // throw
  ASTNode* expr = NULL;
  if (!LexLookingAt(syntax->lex, TOK(semicolon)) &&
      !LexLookingAt(syntax->lex, TOK(rparen)) &&
      !LexLookingAt(syntax->lex, TOK(comma))) {
    expr = ParseAssignmentExpression(syntax, followers);
  }
  return NewThrowASTNode(expr, location);
}

static ASTNode* ParseCXXCoAwaitExpression(Syntax* syntax,
                                          TokenClass followers) {
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // co_await
  ASTNode* expr = ParseCastExpression(syntax, followers);
  return NewUnaryASTNode(AST_OP(co_await), NULL, location, expr);
}

static ASTNode* ParseCXXCoYieldExpression(Syntax* syntax,
                                          TokenClass followers) {
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // co_yield
  ASTNode* expr = NULL;
  if (!LexLookingAt(syntax->lex, TOK(semicolon)) &&
      !LexLookingAt(syntax->lex, TOK(rparen)) &&
      !LexLookingAt(syntax->lex, TOK(comma))) {
    expr = ParseAssignmentExpression(syntax, followers);
  } else {
    SyntaxError(syntax, "Expected expression after co_yield");
    expr = NewIntConstantASTNode(
        0, NewTypeRecordWithSize(kTypeInt, kQualPlain), location);
  }
  return NewUnaryASTNode(AST_OP(co_yield), NULL, location, expr);
}

// Parse a unary expression with syntax:
// unary-expression:
//    postfix-expression
//    ++ unary-expression
//    -- unary-expression
//    unary-operator cast-expression
//    sizeof unary-expression
//    sizeof ( type-name )

// unary-operator: one of
//    & * + - ~ !

static ASTNode* ParseUnaryExpression(Syntax* syntax, TokenClass followers) {
  if (syntax->lex->preprocessor_mode) {
    // In preprocessor mode we have a unary operator that is a the identifier
    // 'defined'.  This is followed by a possibly parenthesized macro name.
    // The result is an int constant with value 1 if the macro exists and 0
    // otherwise.
    if (LexLookingAt(syntax->lex, TOK(identifier))) {
      ASTNode* result = ParsePossiblePreprocessorFunction(syntax, followers);
      if (result != NULL) {
        return result;
      }
    }
  }

  if (LexMatch(syntax->lex, TOK(plus))) {
    ASTNode* sub = ParseCastExpression(syntax, followers);
    return NewUnaryASTNode(AST_OP(uplus), NULL,
                           syntax->lex->current_token_location, sub);
  }

  if (LexMatch(syntax->lex, TOK(minus))) {
    ASTNode* sub = ParseCastExpression(syntax, followers);
    return NewUnaryASTNode(AST_OP(uminus), NULL,
                           syntax->lex->current_token_location, sub);
  }

  if (LexMatch(syntax->lex, TOK(amp))) {
    ASTNode* sub = ParseCastExpression(syntax, followers);
    // If we are taking the address of an identifier we need to
    // set a flag so that the later phases know that this has
    // to be in memory (can't be in a register, if that is supported).
    if (sub->op == AST_OP(identifier)) {
      IdentifierASTNode* ident = (IdentifierASTNode*)sub;
      ident->symbol->flags.address_taken = true;
    }
    return NewUnaryASTNode(AST_OP(address), NULL,
                           syntax->lex->current_token_location, sub);
  }

  if (LexMatch(syntax->lex, TOK(star))) {
    ASTNode* sub = ParseCastExpression(syntax, followers);
    return NewUnaryASTNode(AST_OP(contents), NULL,
                           syntax->lex->current_token_location, sub);
  }

  if (LexMatch(syntax->lex, TOK(tilde))) {
    ASTNode* sub = ParseCastExpression(syntax, followers);
    return NewUnaryASTNode(AST_OP(onescomp), NULL,
                           syntax->lex->current_token_location, sub);
  }

  if (LexMatch(syntax->lex, TOK(bang))) {
    ASTNode* sub = ParseCastExpression(syntax, followers);
    return NewUnaryASTNode(AST_OP(not), NULL,
                           syntax->lex->current_token_location, sub);
  }

  if (LexMatch(syntax->lex, TOK(plusplus))) {
    ASTNode* sub = ParseUnaryExpression(syntax, followers);
    return NewUnaryASTNode(AST_OP(preinc), NULL,
                           syntax->lex->current_token_location, sub);
  }

  if (LexMatch(syntax->lex, TOK(minusminus))) {
    ASTNode* sub = ParseUnaryExpression(syntax, followers);
    return NewUnaryASTNode(AST_OP(predec), NULL,
                           syntax->lex->current_token_location, sub);
  }

  if (LexMatch(syntax->lex, TOK(sizeof))) {
    return ParseSizeof(syntax, followers);
  }

  if (CompilerIsCXX() && LexMatch(syntax->lex, TOK(typeid))) {
    // typeid(...) is a postfix-expression, so allow trailing postfix operators
    // such as the `.name()` member call.
    ASTNode* result = ParseTypeid(syntax, followers);
    return ParsePostfixOperators(syntax, result, followers);
  }

  if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(new))) {
    return ParseCXXNewExpression(syntax, followers);
  }

  if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(delete))) {
    return ParseCXXDeleteExpression(syntax, followers);
  }

  if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(throw))) {
    return ParseCXXThrowExpression(syntax, followers);
  }

  if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(co_await))) {
    return ParseCXXCoAwaitExpression(syntax, followers);
  }

  return ParsePostfixExpression(syntax, followers);
}

static CastKind CXXNamedCastKind(Token token) {
  switch (token) {
    case TOK(static_cast):
      return kCastStatic;
    case TOK(reinterpret_cast):
      return kCastReinterpret;
    case TOK(const_cast):
      return kCastConst;
    case TOK(dynamic_cast):
      return kCastDynamic;
    default:
      assert(false);
      return kCastCStyle;
  }
}

static bool TokenIsCXXNamedCast(Token token) {
  return token == TOK(static_cast) || token == TOK(reinterpret_cast) ||
         token == TOK(const_cast) || token == TOK(dynamic_cast);
}

static ASTNode* ParseCXXNamedCastExpression(Syntax* syntax,
                                            TokenClass followers) {
  Token cast_token = syntax->lex->current_token;
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);
  SyntaxNeedBracket(syntax, TOK(less), followers);

  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  TypeRecord* type = TypeParserParseType(&parser, false);
  Symbol* sym = NULL;
  if (type == NULL) {
    SyntaxError(syntax, "Invalid type name");
    type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  } else {
    sym = TypeParserParseDeclarator(&parser, type);
    type = sym->type;
  }
  TypeParserDestruct(&parser);

  SyntaxNeedBracket(syntax, TOK(greater), followers);
  SyntaxNeedBracket(syntax, TOK(lparen), followers);
  ASTNode* expr = SyntaxParseSingleExpression(syntax, TC(closebra));
  SyntaxNeedBracket(syntax, TOK(rparen), followers);

  ASTNode* result = NewCastASTNode(type, location, expr);
  ((CastASTNode*)result)->kind = CXXNamedCastKind(cast_token);
  if (sym != NULL) {
    SymbolDelete(sym);
  }
  // A named cast is a postfix-expression, so it may be directly followed by
  // postfix operators, e.g. `static_cast<T>(x).member` or `static_cast<T>(p)->m`.
  return ParsePostfixOperators(syntax, result, followers);
}

// Parse cast expression with syntax:
// cast-expression:
//    unary-expression
//    ( type-name ) cast-expression
//
// This also handles compound literals, which are actually postfix expressions.
static ASTNode* ParseCastExpression(Syntax* syntax, TokenClass followers) {
  if (CompilerIsCXX() && TokenIsCXXNamedCast(syntax->lex->current_token)) {
    return ParseCXXNamedCastExpression(syntax, followers);
  }
  if (!syntax->lex->preprocessor_mode && !syntax->lex->assembler_mode &&
      LexMatch(syntax->lex, TOK(lparen))) {
    // A leading __attribute__ (GCC extension) only appears in type names, so
    // treat "( __attribute__((...)) type-name )" as a cast / compound literal.
    if (SyntaxLookingAtType(syntax) ||
        LexLookingAt(syntax->lex, TOK(attribute)) ||
        SyntaxLookingAtCXXAttribute(syntax)) {
      // The leading '(' is followed by something that begins a type, so this is
      // potentially a cast or compound literal `( type-name ) ...`.  It can,
      // however, also be a parenthesized functional-cast expression such as
      // `(T(args))`, where `T(args)` is not a type-id (e.g. the arguments are
      // values rather than parameter declarations).  Tentatively parse the
      // type-name; if it does not form a well-formed `( type-name )` (i.e. the
      // type-name is not immediately followed by ')', or the parse hits an
      // error), backtrack and reparse the parenthesized construct as an
      // expression.  Valid casts always succeed here, so their behavior is
      // unchanged; only inputs that previously failed as casts are rerouted.
      LexCheckpoint checkpoint;
      LexCheckpointSave(syntax->lex, &checkpoint);
      TypeParser parser;
      volatile bool parse_completed = false;
      volatile bool is_type_name = false;
      TypeRecord* volatile type = NULL;
      Symbol* volatile sym = NULL;
      // Trap any diagnostics emitted while speculatively parsing the type-name
      // so a backtrack stays silent.  `abort_on_error` makes a SyntaxError
      // longjmp out immediately (so we stop at the first one), while the trap
      // additionally catches LexErrors (e.g. "Missing close parenthesis"), which
      // do not honor `abort_on_error`.
      volatile bool saved_trap = DiagnosticErrorTrapBegin();
      bool prev_abort_on_error = abort_on_error;
      // Save the global abort target: parsing the type-name can recurse through
      // the expression parser (e.g. an array bound) and set up its own trial,
      // which would otherwise clobber our jmp_buf.
      jmp_buf saved_abort_state;
      memcpy(saved_abort_state, error_abort_state, sizeof(error_abort_state));
      abort_on_error = true;
      if (setjmp(error_abort_state) == 0) {
        TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                       syntax->context);
        type = TypeParserParseType(&parser, false);
        if (type != NULL) {
          sym = TypeParserParseDeclarator(&parser, type);
          type = sym->type;
          // A well-formed cast / compound-literal type-name is an abstract
          // declarator ending exactly at the closing ')'.  Two shapes of the
          // parenthesized functional cast `(T(arg))` slip past a naive check and
          // must be rejected so we backtrack to expression parsing:
          //   * a value argument is (mis)parsed as a *named* declarator, e.g.
          //     `(Fahrenheit(c))` -> "Fahrenheit c"; a real type-name names
          //     nothing, so require the declarator to be abstract (invented),
          //   * a type argument is (mis)parsed as a *function* type, e.g.
          //     `(weak_ordering(strong_ordering::x))`; a bare function type is
          //     never a valid cast target, so reject it (function pointers,
          //     which are valid, are not function types).
          is_type_name = LexLookingAt(syntax->lex, TOK(rparen)) &&
                         sym != NULL && sym->flags.invented &&
                         !TypeIsFunction(type);
        }
        parse_completed = true;
      }
      abort_on_error = prev_abort_on_error;
      memcpy(error_abort_state, saved_abort_state, sizeof(error_abort_state));
      bool trapped = DiagnosticErrorTrapped();
      DiagnosticErrorTrapEnd(saved_trap);
      if (!parse_completed || trapped || !is_type_name) {
        // Not a well-formed `( type-name )`; abandon the trial parse (its
        // TypeParser is only released when it completed cleanly, since a
        // longjmp leaves it in a partial state) and reparse as an expression.
        if (parse_completed) {
          if (sym != NULL) {
            SymbolDelete(sym);
          }
          TypeParserDestruct(&parser);
        }
        LexCheckpointRestore(syntax->lex, &checkpoint);
        LexCheckpointDestruct(&checkpoint);
        syntax->found_open_paren = true;
        return ParsePostfixExpression(syntax, followers);
      }
      LexCheckpointDestruct(&checkpoint);
      SyntaxNeedBracket(syntax, TOK(rparen), followers);
      // If the (type-name) is followed by an initializer list we have
      // a compound literal.  This is actually a postfix expression so we
      // save the parsed type and move forward.  The initializer will be
      // parsed by the postfix expression parser.
      if (LexLookingAt(syntax->lex, TOK(lbrace))) {
        TypeRecord* saved_type = syntax->compound_literal_type;
        syntax->compound_literal_type = type;
        ASTNode* result = ParseCastExpression(syntax, followers);
        syntax->compound_literal_type = saved_type;
        if (sym != NULL) {
          // The compound literal will create its own symbol.
          SymbolDelete(sym);
        }
        TypeParserDestruct(&parser);
        return result;
      }
      ASTNode* expr = ParseCastExpression(syntax, followers);
      // The cast AST node will take ownership of the TypeRecord pointer.
      ASTNode* result =
          NewCastASTNode(type, syntax->lex->current_token_location, expr);
      if (sym != NULL) {
        SymbolDelete(sym);
      }
      TypeParserDestruct(&parser);
      return result;
    } else {
      // Not a type name.  We have consumed the open paren so we
      // know that it is present.  Set a flag to tell all downstream
      // parsers that we've found and consumed it.
      syntax->found_open_paren = true;
      return ParsePostfixExpression(syntax, followers);
    }
  } else {
    return ParseUnaryExpression(syntax, followers);
  }
}

static ASTNode* ParseMultiplicativeExpression(Syntax* syntax,
                                              TokenClass followers) {
  ASTNode* result = ParseCastExpression(syntax, followers);
  for (;;) {
    if (LexMatch(syntax->lex, TOK(star))) {
      ASTNode* right = ParseCastExpression(syntax, followers);
      result =
          NewBinaryASTNode(AST_OP(mult), NULL,
                           syntax->lex->current_token_location, result, right);
    } else if (LexMatch(syntax->lex, TOK(slash))) {
      ASTNode* right = ParseCastExpression(syntax, followers);
      result =
          NewBinaryASTNode(AST_OP(div), NULL,
                           syntax->lex->current_token_location, result, right);
    } else if (LexMatch(syntax->lex, TOK(percent))) {
      ASTNode* right = ParseCastExpression(syntax, followers);
      result =
          NewBinaryASTNode(AST_OP(mod), NULL,
                           syntax->lex->current_token_location, result, right);
    } else {
      break;
    }
  }
  return result;
}

static ASTNode* ParseAdditiveExpression(Syntax* syntax, TokenClass followers) {
  ASTNode* result = ParseMultiplicativeExpression(syntax, followers);
  for (;;) {
    if (LexMatch(syntax->lex, TOK(plus))) {
      ASTNode* right = ParseMultiplicativeExpression(syntax, followers);
      result =
          NewBinaryASTNode(AST_OP(plus), NULL,
                           syntax->lex->current_token_location, result, right);
    } else if (LexMatch(syntax->lex, TOK(minus))) {
      ASTNode* right = ParseMultiplicativeExpression(syntax, followers);
      result =
          NewBinaryASTNode(AST_OP(minus), NULL,
                           syntax->lex->current_token_location, result, right);
    } else {
      break;
    }
  }
  return result;
}

static ASTNode* ParseShiftExpression(Syntax* syntax, TokenClass followers) {
  ASTNode* result = ParseAdditiveExpression(syntax, followers);
  for (;;) {
    if (LexMatch(syntax->lex, TOK(lessless))) {
      ASTNode* right = ParseAdditiveExpression(syntax, followers);
      result =
          NewBinaryASTNode(AST_OP(lshift), NULL,
                           syntax->lex->current_token_location, result, right);
    } else if (LexMatch(syntax->lex, TOK(greatergreater))) {
      ASTNode* right = ParseAdditiveExpression(syntax, followers);
      result =
          NewBinaryASTNode(AST_OP(rshift), NULL,
                           syntax->lex->current_token_location, result, right);
    } else {
      break;
    }
  }
  return result;
}

// C++20 three-way comparison binds tighter than the relational operators and
// looser than the shift operators.
static ASTNode* ParseCompareExpression(Syntax* syntax, TokenClass followers) {
  ASTNode* result = ParseShiftExpression(syntax, followers);
  while (LexMatch(syntax->lex, TOK(spaceship))) {
    ASTNode* right = ParseShiftExpression(syntax, followers);
    result =
        NewBinaryASTNode(AST_OP(spaceship), NULL,
                         syntax->lex->current_token_location, result, right);
  }
  return result;
}

static ASTNode* ParseRelationalExpression(Syntax* syntax,
                                          TokenClass followers) {
  ASTNode* result = ParseCompareExpression(syntax, followers);
  for (;;) {
    if (LexMatch(syntax->lex, TOK(less))) {
      ASTNode* right = ParseCompareExpression(syntax, followers);
      result =
          NewBinaryASTNode(AST_OP(less), NULL,
                           syntax->lex->current_token_location, result, right);
    } else if (LexMatch(syntax->lex, TOK(lesseq))) {
      ASTNode* right = ParseCompareExpression(syntax, followers);
      result =
          NewBinaryASTNode(AST_OP(lesseq), NULL,
                           syntax->lex->current_token_location, result, right);
    } else if (syntax->parsing_template_argument &&
               LexLookingAt(syntax->lex, TOK(greater))) {
      break;
    } else if (LexMatch(syntax->lex, TOK(greater))) {
      ASTNode* right = ParseCompareExpression(syntax, followers);
      result =
          NewBinaryASTNode(AST_OP(greater), NULL,
                           syntax->lex->current_token_location, result, right);
    } else if (LexMatch(syntax->lex, TOK(greatereq))) {
      ASTNode* right = ParseCompareExpression(syntax, followers);
      result =
          NewBinaryASTNode(AST_OP(greatereq), NULL,
                           syntax->lex->current_token_location, result, right);
    } else {
      break;
    }
  }
  return result;
}

static ASTNode* ParseEqualityExpression(Syntax* syntax, TokenClass followers) {
  ASTNode* result = ParseRelationalExpression(syntax, followers);
  for (;;) {
    if (LexMatch(syntax->lex, TOK(equalequal))) {
      ASTNode* right = ParseRelationalExpression(syntax, followers);
      result =
          NewBinaryASTNode(AST_OP(equal), NULL,
                           syntax->lex->current_token_location, result, right);
    } else if (LexMatch(syntax->lex, TOK(bangeq))) {
      ASTNode* right = ParseRelationalExpression(syntax, followers);
      result =
          NewBinaryASTNode(AST_OP(noteq), NULL,
                           syntax->lex->current_token_location, result, right);
    } else {
      break;
    }
  }
  return result;
}

static ASTNode* ParseAndExpression(Syntax* syntax, TokenClass followers) {
  ASTNode* result = ParseEqualityExpression(syntax, followers);
  while (LexMatch(syntax->lex, TOK(amp))) {
    ASTNode* right = ParseEqualityExpression(syntax, followers);
    result = NewBinaryASTNode(
        AST_OP(and), NULL, syntax->lex->current_token_location, result, right);
  }
  return result;
}

static ASTNode* ParseExclusiveOrExpression(Syntax* syntax,
                                           TokenClass followers) {
  ASTNode* result = ParseAndExpression(syntax, followers);
  while (LexMatch(syntax->lex, TOK(caret))) {
    ASTNode* right = ParseAndExpression(syntax, followers);
    result =
        NewBinaryASTNode(AST_OP(exor), NULL,
                         syntax->lex->current_token_location, result, right);
  }
  return result;
}

static ASTNode* ParseInclusiveOrExpression(Syntax* syntax,
                                           TokenClass followers) {
  ASTNode* result = ParseExclusiveOrExpression(syntax, followers);
  while (LexMatch(syntax->lex, TOK(bar))) {
    ASTNode* right = ParseExclusiveOrExpression(syntax, followers);
    result =
        NewBinaryASTNode(AST_OP(bitor), NULL,
                         syntax->lex->current_token_location, result, right);
  }
  return result;
}

static ASTNode* ParseLogicalAndExpression(Syntax* syntax,
                                          TokenClass followers) {
  ASTNode* result = ParseInclusiveOrExpression(syntax, followers);
  while (LexMatch(syntax->lex, TOK(ampamp))) {
    ASTNode* right = ParseInclusiveOrExpression(syntax, followers);
    result =
        NewBinaryASTNode(AST_OP(logand), NULL,
                         syntax->lex->current_token_location, result, right);
  }
  return result;
}

static ASTNode* ParseLogicalOrExpression(Syntax* syntax, TokenClass followers) {
  ASTNode* result = ParseLogicalAndExpression(syntax, followers);
  while (LexMatch(syntax->lex, TOK(barbar))) {
    ASTNode* right = ParseLogicalAndExpression(syntax, followers);
    result =
        NewBinaryASTNode(AST_OP(logor), NULL,
                         syntax->lex->current_token_location, result, right);
  }
  return result;
}

static ASTNode* ParseConditionalExpression(Syntax* syntax,
                                           TokenClass followers) {
  ASTNode* result = ParseLogicalOrExpression(syntax, followers);
  if (LexMatch(syntax->lex, TOK(question))) {
    ASTNode* left = SyntaxParseExpression(syntax, followers);
    ASTNode* right = NULL;
    if (LexMatch(syntax->lex, TOK(colon))) {
      right = ParseConditionalExpression(syntax, followers);
      right =
          NewBinaryASTNode(AST_OP(colon), NULL,
                           syntax->lex->current_token_location, left, right);
    } else {
      SyntaxError(syntax, "Missing : in conditional expression");
    }
    result =
        NewBinaryASTNode(AST_OP(question), NULL,
                         syntax->lex->current_token_location, result, right);
  }
  return result;
}

static ASTOpcode AssignASTOpcode(Syntax* syntax, Token tok) {
  switch (tok) {
    case TOK(equal):
      return AST_OP(assign);
    case TOK(pluseq):
      return AST_OP(pluseq);
    case TOK(minuseq):
      return AST_OP(minuseq);
    case TOK(stareq):
      return AST_OP(multeq);
    case TOK(slasheq):
      return AST_OP(diveq);
    case TOK(percenteq):
      return AST_OP(percenteq);
    case TOK(lesslesseq):
      return AST_OP(lshifteq);
    case TOK(greatergreatereq):
      return AST_OP(rshifteq);
    case TOK(ampeq):
      return AST_OP(andeq);
    case TOK(bareq):
      return AST_OP(oreq);
    case TOK(careteq):
      return AST_OP(exoreq);
    default:
      assert(false);
      return 0;
  }
}

static ASTNode* ParseAssignmentExpression(Syntax* syntax,
                                          TokenClass followers) {
  if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(co_yield))) {
    return ParseCXXCoYieldExpression(syntax, followers);
  }

  ASTNode* result = ParseConditionalExpression(syntax, followers);
  if (syntax->lex->preprocessor_mode || syntax->lex->assembler_mode) {
    return result;
  }
  switch (syntax->lex->current_token) {
    case TOK(equal):
    case TOK(pluseq):
    case TOK(minuseq):
    case TOK(stareq):
    case TOK(slasheq):
    case TOK(percenteq):
    case TOK(lesslesseq):
    case TOK(greatergreatereq):
    case TOK(ampeq):
    case TOK(bareq):
    case TOK(careteq): {
      Token tok = syntax->lex->current_token;
      LexNextToken(syntax->lex);
      ASTNode* right = ParseAssignmentExpression(syntax, followers);
      result = NewBinaryASTNode(AssignASTOpcode(syntax, tok), NULL,
                                syntax->lex->current_token_location, result,
                                right);
      break;
    }
    default:
      break;
  }
  return result;
}

ASTNode* SyntaxParseExpression(Syntax* syntax, TokenClass followers) {
  ASTNode* result = ParseAssignmentExpression(syntax, followers | TC(exprsep));
  while (LexMatch(syntax->lex, TOK(comma))) {
    ASTNode* right = ParseAssignmentExpression(syntax, followers | TC(exprsep));
    result =
        NewBinaryASTNode(AST_OP(comma), NULL,
                         syntax->lex->current_token_location, result, right);
  }
  return result;
}

ASTNode* SyntaxParseSingleExpression(Syntax* syntax, TokenClass followers) {
  return ParseAssignmentExpression(syntax, followers);
}
