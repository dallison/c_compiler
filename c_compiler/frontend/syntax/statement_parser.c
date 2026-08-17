//
//  statement_parser.c
//  c_compiler
//
//  Created by David Allison on 10/31/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include <stdlib.h>
#include <string.h>

#include "expr_parser.h"
#include "expr_semantics.h"
#include "statement_parser.h"
#include "compiler.h"
#include "errors.h"
#include "type.h"
#include "type_inheritance.h"
#include "type_template.h"

static ASTNode* NewRangeForInitExpression(Symbol* sym, ASTNode* initializer,
                                          SourceLocation location) {
  ASTNode* decl_id = NewIdentifierASTNode(sym, location);
  decl_id->flags |= kASTNeedAddress | kASTIsDeclaration;
  return NewBinaryASTNode(AST_OP(init), sym->type, location, decl_id,
                          NewExpressionInitializerASTNode(initializer,
                                                          location));
}

static ASTNode* NewRangeForDeclaration(Symbol* sym, ASTNode* initializer,
                                       SourceLocation location) {
  sym->flags.is_local = true;
  sym->flags.is_defined = true;
  return NewVariableDeclarationASTNode(
      sym, NewRangeForInitExpression(sym, initializer, location), location);
}

static ASTNode* NewRangeForDeclarationList(Symbol* sym, ASTNode* initializer,
                                           SourceLocation location) {
  Vector* declarations = NewVector();
  VectorAppend(declarations, NewRangeForDeclaration(sym, initializer,
                                                    location));
  return NewDeclarationListASTNode(declarations, location);
}

static TypeRecord* NewRangeForAutoType(void) {
  return NewTypeRecord(kTypeAuto, kQualPlain);
}

static TypeRecord* NewRangeForAutoReferenceType(bool rvalue) {
  TypeRecord* ref = NewReferenceTypeRecord(kQualPlain, rvalue);
  TypeRecordChain(ref, NewRangeForAutoType());
  TypeRecordCalculateSize(ref);
  return ref;
}

static Symbol* NewRangeForAutoSymbol(Syntax* syntax, const char* name,
                                     SourceLocation location) {
  Symbol* sym = NewSymbol(name != NULL ? name : SyntaxFakeName(syntax),
                          NewRangeForAutoType(), STO(auto));
  sym->location = location;
  sym->flags.is_local = true;
  sym->flags.is_defined = true;
  return sym;
}

static Symbol* NewRangeForRangeSymbol(Syntax* syntax,
                                      SourceLocation location,
                                      bool rvalue,
                                      TypeRecord* concrete_type) {
  TypeRecord* type = NULL;
  if (concrete_type != NULL && !TypeIsUnknown(concrete_type) &&
      (concrete_type->type & kTypeAuto) == 0 &&
      !TypeContainsTemplateParameter(concrete_type)) {
    type = NewReferenceTypeRecord(kQualPlain, rvalue);
    TypeRecordChain(type, TypeRecordCopy(concrete_type));
    TypeRecordCalculateSize(type);
  } else {
    type = NewRangeForAutoReferenceType(rvalue);
  }
  Symbol* sym =
      NewSymbol(SyntaxFakeName(syntax), type, STO(auto));
  sym->location = location;
  sym->flags.is_local = true;
  sym->flags.is_defined = true;
  SyntaxAddSymbol(syntax, sym);
  return sym;
}

typedef struct {
  Symbol* loop_var;
  TypeRecord* structured_binding_type;
  Vector names;  // String* entries for [x, y] bindings.
  Vector attributes;  // Vector* of Attribute* entries corresponding to names.
  Vector symbols;  // Symbol* entries corresponding to names.
  int pack_index;
} RangeForBinding;

static void RangeForBindingInit(RangeForBinding* binding) {
  binding->loop_var = NULL;
  binding->structured_binding_type = NULL;
  VectorInit(&binding->names);
  VectorInit(&binding->attributes);
  VectorInit(&binding->symbols);
  binding->pack_index = -1;
}

static void RangeForBindingDestruct(RangeForBinding* binding) {
  TypeRecordDelete(binding->structured_binding_type);
  VectorDestructWithContents(&binding->names,
                             (VectorElementDestructor)StringDelete,
                             /*free_element=*/false);
  for (size_t i = 0; i < binding->attributes.length; i++) {
    Vector* attributes = binding->attributes.value.p[i];
    if (attributes != NULL) {
      AttributeListDestruct(attributes);
      VectorDelete(attributes);
    }
  }
  VectorDestruct(&binding->attributes);
  VectorDestruct(&binding->symbols);
}

static ASTNode* NewCXXDestructorCallForReceiver(TypeRecord* type,
                                                ASTNode* receiver,
                                                SourceLocation location) {
  if (!CompilerIsCXX() || receiver == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL || type->info.struct_info->tag_name == NULL) {
    return NULL;
  }

  String destructor_name;
  StringInit(&destructor_name, "~");
  StringAppendString(&destructor_name, type->info.struct_info->tag_name);
  StructMember* destructor =
      FindStructMember(type->info.struct_info, &destructor_name);
  if (destructor == NULL || !destructor->is_member_function ||
      !destructor->symbol->type->info.function.is_destructor) {
    StringDestruct(&destructor_name);
    return NULL;
  }
  if (!TypeHasNonTrivialDestructor(type)) {
    StringDestruct(&destructor_name);
    return NULL;
  }

  ASTNode* member = NewStringConstantASTNode(NewString(destructor_name.value),
                                             NULL, location);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, member);
  StringDestruct(&destructor_name);
  // A class with virtual bases has a destructor that takes a hidden
  // complete-object flag; a named local is a complete (most-derived) object, so
  // pass 1 so the destructor also tears down the virtual bases.
  Vector* actuals = NewVector();
  if (StructHasVirtualBases(type->info.struct_info)) {
    VectorAppend(actuals,
                 NewIntConstantASTNode(
                     1, NewTypeRecordWithSize(kTypeInt, kQualPlain), location));
  }
  return NewExpressionStatementASTNode(
      NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals),
      location);
}

static ASTNode* NewCXXDestructorCall(Symbol* sym, SourceLocation location) {
  if (sym == NULL || StorageIs(sym->storage, STO(static)) ||
      !TypeIsStructOrUnion(sym->type)) {
    return NULL;
  }
  return NewCXXDestructorCallForReceiver(
      sym->type, NewIdentifierASTNode(sym, location), location);
}

static ASTNode* NewCXXArrayElementDestructorCall(Symbol* sym, int64_t index,
                                                 SourceLocation location) {
  if (sym == NULL || StorageIs(sym->storage, STO(static)) ||
      !TypeIsFixedArray(sym->type) || !TypeIsStructOrUnion(sym->type->next)) {
    return NULL;
  }
  ASTNode* array = NewIdentifierASTNode(sym, location);
  ASTNode* subscript = NewBinaryASTNode(
      AST_OP(subscript), NULL, location, array,
      NewIntConstantASTNode(index, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                            location));
  return NewCXXDestructorCallForReceiver(sym->type->next, subscript, location);
}

void SyntaxAppendCXXBlockScopeDestructors(Vector* statements) {
  if (!CompilerIsCXX()) {
    return;
  }

  for (size_t i = statements->length; i > 0; i--) {
    ASTNode* stmt = statements->value.p[i - 1];
    if (stmt->op != AST_OP(decl_list)) {
      continue;
    }
    DeclarationListASTNode* decls = (DeclarationListASTNode*)stmt;
    for (size_t j = decls->declarations->length; j > 0; j--) {
      ASTNode* decl_node = decls->declarations->value.p[j - 1];
      if (decl_node->op != AST_OP(vardecl)) {
        continue;
      }
      VariableDeclarationASTNode* decl =
          (VariableDeclarationASTNode*)decl_node;
      if (TypeIsFixedArray(decl->symbol->type) &&
          TypeIsStructOrUnion(decl->symbol->type->next)) {
        int64_t length = decl->symbol->type->info.array.size.fixed;
        for (int64_t k = length; k > 0; k--) {
          ASTNode* destructor = NewCXXArrayElementDestructorCall(
              decl->symbol, k - 1, decl->base.location);
          if (destructor != NULL) {
            destructor->flags |= kASTFallthroughDestructor;
            VectorAppend(statements, destructor);
          }
        }
      } else {
        ASTNode* destructor =
            NewCXXDestructorCall(decl->symbol, decl->base.location);
        if (destructor != NULL) {
          destructor->flags |= kASTFallthroughDestructor;
          VectorAppend(statements, destructor);
        }
      }
    }
  }
}

// A compound statement is a brace-enclosed sequence of statements.
// The AST node holding this is a CompoundStatementASTNode.
// The node simply contains a vector of statements.
static ASTNode* ParseCompoundStatement(Syntax* syntax, TokenClass followers,
                                       SourceLocation location) {
  // Compound statements open a scope.
  SyntaxOpenScope(syntax);
  Vector* statements = NewVector();
  Lex* lex = syntax->lex;

  // Add label for start of statements in block
  if (compiler->debug_output) {
    VectorAppend(statements, SyntaxNewPCLabel(location));
  }
  // Parse the sequence of statements or declarations, adding them to the vector.
  bool seen_statement = false;
  while (!LexEof(lex) && !LexLookingAt(lex, TOK(rbrace))) {
    if (!LexMatch(lex, TOK(semicolon))) {
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
        stmt = SyntaxParseStatement(syntax, followers | TC(closebrace));
      }
      if (stmt != NULL) {
        VectorAppend(statements, stmt);
      }
    }
  }
   
  SyntaxAppendCXXBlockScopeDestructors(statements);

  if (compiler->debug_output) {
    // Label at end of statements in block.
    VectorAppend(statements, SyntaxNewPCLabel(location));
  }
  
  // We have not consumed the close paren yet, do so now.
  SyntaxNeedBracket(syntax, TOK(rbrace), followers);

  // Close the scope.
  SyntaxCloseScope(syntax);
  return NewCompoundStatementASTNode(statements, location);
}

// An 'if' statement can have an optional 'else' clause.
// The AST node is an IfStatementASTNode and contains:
// 1. A condition (expression)
// 2. An 'if' clause stateemnt
// 3. An optional 'else' statement clause - NULL is absent.
// --- C++ condition-declarations -------------------------------------------
//
// In C++ the controlling expression of if/while/switch may instead be a
// declaration with an initializer ("if (T x = init)").  The declared variable
// is in scope for the controlled statement(s) and its value, contextually
// converted to bool, is the condition.  These are lowered to ordinary
// constructs the rest of the compiler already handles:
//
//   if (T x = init) A else B  =>  { T x = init; if (x) A else B; }
//   switch (T x = init) BODY  =>  { T x = init; switch (x) BODY; }
//   while (T x = init) BODY   =>  for (;;) { T x = init;
//                                            if (x) {} else break; BODY; }
//
// The while form re-creates and destroys x on every iteration, matching the
// standard, and reuses the existing break/continue block-scope destructor
// handling.

// Returns the symbol declared by a condition-declaration node produced by
// SyntaxParseConditionDeclaration, or NULL if none could be extracted.
static Symbol* ConditionDeclaredSymbol(ASTNode* decl) {
  if (decl == NULL || decl->op != AST_OP(decl_list)) {
    return NULL;
  }
  DeclarationListASTNode* list = (DeclarationListASTNode*)decl;
  if (list->declarations == NULL || list->declarations->length == 0) {
    return NULL;
  }
  ASTNode* first = list->declarations->value.p[0];
  if (first->op != AST_OP(vardecl)) {
    return NULL;
  }
  return ((VariableDeclarationASTNode*)first)->symbol;
}

// Side-effect-free test for whether the condition begins with a type specifier,
// used to gate condition-declaration parsing.  Unlike SyntaxLookingAtType, this
// never parses template-ids or qualified names, which can perturb parser state
// and would corrupt parsing of an ordinary comparison condition like "a < b"
// where "a" is an unresolved dependent name.  It deliberately recognizes only
// fundamental type keywords and plain identifiers that resolve to a typedef,
// template type parameter, or non-template tag; qualified or template-id type
// conditions fall through and are parsed as expressions.
static bool ConditionStartsWithType(Syntax* syntax) {
  switch (syntax->lex->current_token) {
    case TOK(char): case TOK(char8_t): case TOK(char16_t): case TOK(char32_t):
    case TOK(int): case TOK(short):
    case TOK(long):
    case TOK(float): case TOK(double): case TOK(bool): case TOK(signed):
    case TOK(unsigned): case TOK(void): case TOK(wchar_t):
    case TOK(const): case TOK(volatile): case TOK(restrict):
    case TOK(struct): case TOK(class): case TOK(union): case TOK(enum):
      return true;
    case TOK(auto): case TOK(decltype): case TOK(typename):
      return CompilerIsCXX();
    case TOK(identifier): {
      Symbol* sym = SyntaxFindSymbol(syntax, &syntax->lex->spelling);
      if (sym != NULL) {
        if (sym->flags.is_template_type_parameter) {
          return true;
        }
        return StorageIs(sym->storage, STO(typedef)) && !sym->flags.is_template;
      }
      Symbol* tag = SyntaxFindTag(syntax, &syntax->lex->spelling);
      return (tag != NULL && !tag->flags.is_template) ||
             SyntaxCurrentClassNameStartsType(syntax);
    }
    default:
      return false;
  }
}

// Decides whether the controlling condition at the current position is a
// declaration rather than an expression.  A condition-declaration must have a
// brace-or-equal-initializer, which distinguishes "if (T x = v)" (a
// declaration) from "if (T(x))" or "if (T(x).f())" (functional-cast
// expressions).  Speculatively parses a type and declarator behind a lex
// checkpoint and an error trap, then rolls back.
static bool LooksLikeConditionDeclaration(Syntax* syntax) {
  if (!CompilerIsCXX() || !ConditionStartsWithType(syntax)) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  bool trap = DiagnosticErrorTrapBegin();

  bool is_declaration = false;
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(auto), kParsingBlockScope);
  TypeRecord* type = TypeParserParseType(&parser, true);
  if (type != NULL) {
    TypeRecordIncRef(type);
    // Only a genuine declarator follows the type in a condition-declaration.
    // If the type is immediately followed by '{', '(' or an operator, this is
    // an expression instead (a braced temporary "T{...}", a functional cast
    // "T(...)", or a comparison "T < x"), not a declaration.  The declarator
    // parser synthesizes names for abstract declarators, so we must gate on the
    // lookahead token rather than trust the parsed name.
    Token after_type = syntax->lex->current_token;
    if (after_type == TOK(identifier) || after_type == TOK(star) ||
        after_type == TOK(amp) || after_type == TOK(ampamp)) {
      Symbol* sym = TypeParserParseDeclarator(&parser, type);
      if (sym != NULL && sym->name.length > 0 &&
          (LexLookingAt(syntax->lex, TOK(equal)) ||
           LexLookingAt(syntax->lex, TOK(lbrace)))) {
        is_declaration = true;
      }
      if (sym != NULL) {
        SymbolDelete(sym);
      }
    } else if (after_type == TOK(lsquare) &&
               CompilerCXXAtLeast(kLanguageStandardCXX26)) {
      int depth = 0;
      do {
        if (LexLookingAt(syntax->lex, TOK(lsquare))) {
          depth++;
        } else if (LexLookingAt(syntax->lex, TOK(rsquare))) {
          depth--;
        }
        LexNextToken(syntax->lex);
      } while (!LexEof(syntax->lex) && depth > 0);
      is_declaration =
          depth == 0 &&
          (LexLookingAt(syntax->lex, TOK(equal)) ||
           LexLookingAt(syntax->lex, TOK(lbrace)));
    }
    TypeRecordDelete(type);
  }
  TypeParserDestruct(&parser);

  // A genuine condition-declaration ("T x = init") parses its type and
  // declarator cleanly.  If the speculative parse tripped any diagnostic (for
  // example a dependent comparison like "a < b" where SyntaxLookingAtType is
  // conservatively true and the type parser mis-reads "<" as a template-id),
  // treat it as an expression instead.
  if (DiagnosticErrorTrapped()) {
    is_declaration = false;
  }

  DiagnosticErrorTrapEnd(trap);
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return is_declaration;
}

// Parses the controlling condition of if/while/switch.  When the condition is a
// declaration, a fresh scope is opened for the declared variable, *out_decl
// receives the declaration node, and the returned condition expression
// references the declared variable.  Otherwise no scope is opened, *out_decl is
// NULL, and the condition is a plain expression parsed exactly as before (so
// expression conditions are entirely unaffected).  When *out_decl is non-NULL
// the caller must eventually call FinishConditionScope (or otherwise close the
// scope) exactly once.
static ASTNode* ParseControllingCondition(Syntax* syntax, TokenClass followers,
                                          ASTNode** out_decl) {
  *out_decl = NULL;
  if (LooksLikeConditionDeclaration(syntax)) {
    SyntaxOpenScope(syntax);
    ASTNode* decl = SyntaxParseConditionDeclaration(syntax);
    Symbol* sym = ConditionDeclaredSymbol(decl);
    if (sym == NULL && decl != NULL && decl->op == AST_OP(decl_list)) {
      DeclarationListASTNode* declarations = (DeclarationListASTNode*)decl;
      if (declarations->declarations != NULL &&
          declarations->declarations->length > 0) {
        ASTNode* first = declarations->declarations->value.p[0];
        if (first != NULL && first->op == AST_OP(structured_binding)) {
          StructuredBindingASTNode* binding =
              (StructuredBindingASTNode*)first;
          binding->condition_symbol = SyntaxNewTemporary(
              syntax, TypeRecordCopy(binding->declared_type));
          binding->condition_symbol->flags.is_local = true;
          binding->condition_symbol->flags.is_defined = true;
          binding->condition_symbol->structured_binding_pack_size = -1;
          binding->condition_symbol->location = first->location;
          sym = binding->condition_symbol;
        }
      }
    }
    *out_decl = decl;
    if (sym != NULL) {
      return NewIdentifierASTNode(sym, sym->location);
    }
    // Unreachable on well-formed input; the declaration parse already reported
    // the error.  Return a well-typed placeholder so the AST stays valid.
    return NewIntConstantASTNode(1, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                                 syntax->lex->current_token_location);
  }
  return SyntaxParseExpression(syntax, followers);
}

// Wraps a controlled statement whose condition declared a variable in a scope
// block: "{ decl; stmt; <destructors> }".  Used for if and switch, whose
// conditions are evaluated once.  When there was no declaration no scope was
// opened, so stmt is returned unchanged.
static ASTNode* FinishConditionScope(Syntax* syntax, ASTNode* decl,
                                     ASTNode* stmt, SourceLocation location) {
  if (decl == NULL) {
    return stmt;
  }
  Vector* statements = NewVector();
  VectorAppend(statements, decl);
  VectorAppend(statements, stmt);
  SyntaxAppendCXXBlockScopeDestructors(statements);
  SyntaxCloseScope(syntax);
  return NewCompoundStatementASTNode(statements, location);
}

// --- C++17 init-statements ------------------------------------------------
//
// if / switch may carry an init-statement before the condition:
// "if (init; cond)".  The init-statement (an expression-statement or a
// simple-declaration) and any names it declares are in scope for the condition
// and the controlled statements.  We lower this to a scope block wrapping the
// init-statement and the (already lowered) selection statement:
//
//   if (init; cond) A else B  =>  { init; if (cond) A else B; }
//
// combining with condition-declarations when both are present.
//
// An init-statement is present iff a ';' appears at the top level of the
// parenthesised control clause (a plain condition is never terminated by ';').
// We can't decide this by parsing the first component, because an init-
// statement may be a simple-declaration using any initializer form, including
// direct-initialization "T x(args)", which is not a valid condition-declaration
// and would be mis-parsed as an expression.  Instead we do a side-effect-free
// lexical scan for a top-level ';'.  When one is found, the text before it is
// the init-statement (a simple-declaration or an expression-statement) and the
// text after it is the real condition (which may itself be a condition-
// declaration).  When no ';' is found, behaviour is exactly as before and
// ordinary conditions are unaffected.

// Scans, without side effects, for a ';' at the top nesting level of the
// current control clause (before the ')' that closes it).  Parentheses, braces
// and brackets are tracked so ';' inside a lambda body, initializer list, etc.
// does not count.  Restores the lexer to its entry position before returning.
static bool SelectionHasInitStatement(Syntax* syntax) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX17)) {
    return false;
  }
  Lex* lex = syntax->lex;
  LexCheckpoint checkpoint;
  LexCheckpointSave(lex, &checkpoint);
  int paren = 0, brace = 0, bracket = 0;
  bool found = false;
  while (!LexEof(lex)) {
    Token t = lex->current_token;
    if (t == TOK(lparen)) {
      paren++;
    } else if (t == TOK(rparen)) {
      if (paren == 0) break;
      paren--;
    } else if (t == TOK(lbrace)) {
      brace++;
    } else if (t == TOK(rbrace)) {
      if (brace == 0) break;
      brace--;
    } else if (t == TOK(lsquare)) {
      bracket++;
    } else if (t == TOK(rsquare)) {
      if (bracket == 0) break;
      bracket--;
    } else if (t == TOK(semicolon) && paren == 0 && brace == 0 &&
               bracket == 0) {
      found = true;
      break;
    }
    LexNextToken(lex);
  }
  LexCheckpointRestore(lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return found;
}

static bool LookingAtUsingAliasDeclaration(Syntax* syntax) {
  if (!CompilerIsCXX() || !LexLookingAt(syntax->lex, TOK(using))) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  LexNextToken(syntax->lex);
  bool is_alias = LexLookingAt(syntax->lex, TOK(identifier));
  if (is_alias) {
    LexNextToken(syntax->lex);
    is_alias = LexLookingAt(syntax->lex, TOK(equal));
  }
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return is_alias;
}

static ASTNode* ParseSelectionCondition(Syntax* syntax, TokenClass followers,
                                        ASTNode** out_cond_decl,
                                        ASTNode** out_init,
                                        bool* out_init_scope) {
  *out_init = NULL;
  *out_init_scope = false;
  *out_cond_decl = NULL;
  SourceLocation location = syntax->lex->current_token_location;

  if (SelectionHasInitStatement(syntax)) {
    // Open one scope covering the init-statement, the condition and the
    // controlled statements; any names the init-statement declares are visible
    // throughout.  FinishInitScope closes it.
    SyntaxOpenScope(syntax);
    *out_init_scope = true;
    if (LexLookingAt(syntax->lex, TOK(semicolon))) {
      // A null (empty) expression-statement init-statement.
      LexMatch(syntax->lex, TOK(semicolon));
      *out_init = NewCompoundStatementASTNode(NewVector(), location);
    } else if (SyntaxLookingAtDeclaration(syntax)) {
      if (LookingAtUsingAliasDeclaration(syntax) &&
          !CompilerCXXAtLeast(kLanguageStandardCXX23)) {
        SyntaxError(syntax,
                    "alias declaration in init-statement requires C++23");
      }
      // A simple-declaration init-statement consumes its own ';'.
      *out_init = SyntaxParseLocalDeclaration(syntax);
    } else {
      ASTNode* expr = SyntaxParseExpression(syntax, followers | TC(expr));
      SyntaxNeedSemicolon(syntax, followers | TC(expr));
      *out_init = NewExpressionStatementASTNode(expr, location);
    }
  }

  // Parse the real controlling condition, which may itself be a condition-
  // declaration (opening a further nested scope closed by FinishConditionScope).
  ASTNode* decl = NULL;
  ASTNode* cond = ParseControllingCondition(syntax, followers, &decl);
  *out_cond_decl = decl;
  return cond;
}

// Wraps a selection statement carrying an init-statement in a scope block:
// "{ init; stmt; <destructors> }".  When there was no init-statement stmt is
// returned unchanged.  opened_scope records whether a scope was opened for a
// declaration init-statement so it is closed (and its destructors emitted)
// symmetrically; an expression init-statement declares nothing and needs no
// scope.
static ASTNode* FinishInitScope(Syntax* syntax, ASTNode* init,
                                bool opened_scope, ASTNode* stmt,
                                SourceLocation location) {
  if (init == NULL) {
    return stmt;
  }
  Vector* statements = NewVector();
  VectorAppend(statements, init);
  VectorAppend(statements, stmt);
  if (opened_scope) {
    SyntaxAppendCXXBlockScopeDestructors(statements);
    SyntaxCloseScope(syntax);
  }
  return NewCompoundStatementASTNode(statements, location);
}

static bool ParseIfConstevalPrefix(Syntax* syntax, bool* negated) {
  *negated = false;
  if (LexMatch(syntax->lex, TOK(consteval))) {
    return true;
  }
  if (!LexLookingAt(syntax->lex, TOK(bang))) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  LexNextToken(syntax->lex);
  if (LexMatch(syntax->lex, TOK(consteval))) {
    *negated = true;
    LexCheckpointDestruct(&checkpoint);
    return true;
  }
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return false;
}

static ASTNode* ParseIfStatement(Syntax* syntax, TokenClass followers,
                                 SourceLocation location) {
  bool is_constexpr = false;
  if (CompilerCXXAtLeast(kLanguageStandardCXX17)) {
    is_constexpr = LexMatch(syntax->lex, TOK(constexpr));
  }
  bool consteval_negated = false;
  bool is_consteval =
      !is_constexpr && CompilerIsCXX() &&
      ParseIfConstevalPrefix(syntax, &consteval_negated);
  if (is_consteval) {
    if (!CompilerCXXAtLeast(kLanguageStandardCXX23)) {
      SyntaxError(syntax, "'if consteval' requires C++23");
    }
    if (!LexLookingAt(syntax->lex, TOK(lbrace))) {
      SyntaxError(
          syntax,
          "'if consteval' substatements must be compound statements");
    }
    ASTNode* if_part = SyntaxParseStatement(syntax, followers);
    ASTNode* else_part = NULL;
    if (LexMatch(syntax->lex, TOK(else))) {
      if (!LexLookingAt(syntax->lex, TOK(lbrace))) {
        SyntaxError(
            syntax,
            "'if consteval' substatements must be compound statements");
      }
      else_part = SyntaxParseStatement(syntax, followers);
    }
    ASTNode* cond = NewIntConstantASTNode(
        1, NewTypeRecordWithSize(kTypeBool, kQualPlain), location);
    ASTNode* stmt =
        NewIfStatementASTNode(cond, if_part, else_part, false, location);
    IfStatementASTNode* if_stmt = (IfStatementASTNode*)stmt;
    if_stmt->is_consteval = true;
    if_stmt->consteval_negated = consteval_negated;
    return stmt;
  }
  SyntaxNeedBracket(syntax, TOK(lparen), followers);
  ASTNode* decl = NULL;
  ASTNode* init = NULL;
  bool init_scope = false;
  ASTNode* cond =
      ParseSelectionCondition(syntax, followers, &decl, &init, &init_scope);
  SyntaxNeedBracket(syntax, TOK(rparen), followers);
  Lex* lex = syntax->lex;

  location = lex->current_token_location;
  ASTNode* if_part = SyntaxParseStatement(syntax, followers);
  ASTNode* else_part = NULL;
  if (LexMatch(lex, TOK(else))) {
    else_part = SyntaxParseStatement(syntax, followers);
  }
  ASTNode* if_stmt =
      NewIfStatementASTNode(cond, if_part, else_part, is_constexpr, location);
  ASTNode* inner = FinishConditionScope(syntax, decl, if_stmt, location);
  return FinishInitScope(syntax, init, init_scope, inner, location);
}

// A while statement.
static ASTNode* ParseWhileStatement(Syntax* syntax, TokenClass followers,
                                    SourceLocation location) {
  SyntaxNeedBracket(syntax, TOK(lparen), followers);
  ASTNode* decl = NULL;
  ASTNode* cond = ParseControllingCondition(syntax, followers, &decl);
  SyntaxNeedBracket(syntax, TOK(rparen), followers);
  location = syntax->lex->current_token_location;

  syntax->loop_count++;
  ASTNode* stmt = SyntaxParseStatement(syntax, followers);
  syntax->loop_count--;

  if (decl == NULL) {
    return NewCombinedStatementASTNode(AST_OP(while), cond, stmt, location);
  }

  // Re-create the condition variable each iteration:
  //   for (;;) { T x = init; if (x) {} else break; body; }
  Vector* loop_body = NewVector();
  VectorAppend(loop_body, decl);
  ASTNode* guard = NewIfStatementASTNode(
      cond, NewCompoundStatementASTNode(NewVector(), location),
      NewASTNode(AST_OP(break), NULL, location), false, location);
  VectorAppend(loop_body, guard);
  VectorAppend(loop_body, stmt);
  SyntaxAppendCXXBlockScopeDestructors(loop_body);
  SyntaxCloseScope(syntax);
  ASTNode* body = NewCompoundStatementASTNode(loop_body, location);
  return NewForStatementASTNode(NULL, NULL, NULL, body, location);
}

// A do statement.
static ASTNode* ParseDoStatement(Syntax* syntax, TokenClass followers,
                                 SourceLocation location) {
  Lex* lex = syntax->lex;
  syntax->loop_count++;
  ASTNode* stmt = SyntaxParseStatement(syntax, followers);
  syntax->loop_count--;
  location = syntax->lex->current_token_location;

  if (!LexMatch(lex, TOK(while))) {
    SyntaxError(syntax, "Missing while in do loop");
  }
  SyntaxNeedBracket(syntax, TOK(lparen), followers);
  ASTNode* cond = SyntaxParseExpression(syntax, followers);
  SyntaxNeedBracket(syntax, TOK(rparen), followers);

  return NewCombinedStatementASTNode(AST_OP(do), cond, stmt, location);
}

// Switch statement.
static ASTNode* ParseSwitchStatement(Syntax* syntax, TokenClass followers,
                                     SourceLocation location) {
  SyntaxNeedBracket(syntax, TOK(lparen), followers);
  ASTNode* decl = NULL;
  ASTNode* init = NULL;
  bool init_scope = false;
  ASTNode* expr =
      ParseSelectionCondition(syntax, followers, &decl, &init, &init_scope);
  SyntaxNeedBracket(syntax, TOK(rparen), followers);

  location = syntax->lex->current_token_location;
  syntax->switch_count++;
  ASTNode* stmt = SyntaxParseStatement(syntax, followers);
  syntax->switch_count--;
  ASTNode* switch_stmt = NewSwitchStatementASTNode(expr, stmt, location);
  ASTNode* inner = FinishConditionScope(syntax, decl, switch_stmt, location);
  return FinishInitScope(syntax, init, init_scope, inner, location);
}

static String* ParseAsmString(Syntax* syntax) {
  String* text = NewString("");
  while (LexLookingAt(syntax->lex, TOK(string))) {
    StringAppend(text, syntax->lex->spelling.value);
    LexNextToken(syntax->lex);
  }
  return text;
}

static char* ParseAsmSymbolicName(Syntax* syntax) {
  if (!LexMatch(syntax->lex, TOK(lsquare))) {
    return NULL;
  }
  if (!LexLookingAt(syntax->lex, TOK(identifier))) {
    SyntaxError(syntax, "Expected asm operand name");
    return NULL;
  }
  char* name = strdup(syntax->lex->spelling.value);
  LexNextToken(syntax->lex);
  SyntaxNeedBracket(syntax, TOK(rsquare), TC(openbra));
  return name;
}

static AsmOperand* ParseAsmOperand(Syntax* syntax, bool is_output,
                                   TokenClass followers) {
  char* name = ParseAsmSymbolicName(syntax);
  if (!LexLookingAt(syntax->lex, TOK(string))) {
    SyntaxError(syntax, "Expected asm operand constraint");
    free(name);
    return NULL;
  }
  String* constraint = ParseAsmString(syntax);
  SyntaxNeedBracket(syntax, TOK(lparen), TC(openbra));
  ASTNode* expr = SyntaxParseExpression(syntax, followers | TC(closebra));
  SyntaxNeedBracket(syntax, TOK(rparen), followers);
  AsmOperand* operand =
      NewAsmOperand(constraint->value, name, expr, is_output);
  StringDelete(constraint);
  free(name);
  return operand;
}

static void ParseAsmOperandList(Syntax* syntax, Vector* operands, bool is_output,
                                TokenClass followers) {
  while (!LexLookingAt(syntax->lex, TOK(colon)) &&
         !LexLookingAt(syntax->lex, TOK(rparen))) {
    AsmOperand* operand = ParseAsmOperand(syntax, is_output, followers);
    if (operand != NULL) {
      VectorAppend(operands, operand);
    }
    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
  }
}

static void ParseAsmClobberList(Syntax* syntax, Vector* clobbers) {
  while (!LexLookingAt(syntax->lex, TOK(colon)) &&
         !LexLookingAt(syntax->lex, TOK(rparen))) {
    if (!LexLookingAt(syntax->lex, TOK(string))) {
      SyntaxError(syntax, "Expected asm clobber string");
      return;
    }
    String* clobber = ParseAsmString(syntax);
    VectorAppend(clobbers, clobber);
    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
  }
}

static void ParseAsmLabelList(Syntax* syntax, Vector* labels) {
  while (!LexLookingAt(syntax->lex, TOK(rparen))) {
    if (!LexLookingAt(syntax->lex, TOK(identifier))) {
      SyntaxError(syntax, "Expected asm goto label");
      return;
    }
    VectorAppend(labels, NewString(syntax->lex->spelling.value));
    LexNextToken(syntax->lex);
    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
  }
}

static ASTNode* ParseAsmStatement(Syntax* syntax, TokenClass followers,
                         SourceLocation location) {
  bool is_volatile = false;
  bool is_goto = false;
  bool parsed_modifier = true;
  while (parsed_modifier) {
    parsed_modifier = false;
    if (LexMatch(syntax->lex, TOK(volatile))) {
      is_volatile = true;
      parsed_modifier = true;
    } else if (LexMatch(syntax->lex, TOK(goto))) {
      is_goto = true;
      parsed_modifier = true;
    }
  }
  SyntaxNeedBracket(syntax, TOK(lparen), TC(openbra));
  String* text = ParseAsmString(syntax);
  ASTNode* node = NewAsmASTNode(text, is_volatile, location);
  AsmASTNode* asm_node = (AsmASTNode*)node;
  asm_node->is_goto = is_goto;
  if (LexMatch(syntax->lex, TOK(colon))) {
    ParseAsmOperandList(syntax, &asm_node->outputs, true,
                        followers | TC(exprsep));
    if (LexMatch(syntax->lex, TOK(colon))) {
      ParseAsmOperandList(syntax, &asm_node->inputs, false,
                          followers | TC(exprsep));
      if (LexMatch(syntax->lex, TOK(colon))) {
        ParseAsmClobberList(syntax, &asm_node->clobbers);
        if (LexMatch(syntax->lex, TOK(colon))) {
          ParseAsmLabelList(syntax, &asm_node->labels);
        }
      }
    }
  }
  SyntaxNeedBracket(syntax, TOK(rparen), TC(exprsep) | TC(decl));
  return node;
}

static bool AddRangeForVariable(Syntax* syntax, Symbol* sym) {
  if (sym == NULL) {
    return false;
  }
  // The range-for loop variable belongs to the for statement's own scope
  // (opened by ParseForStatement before this runs) and may legitimately shadow
  // a name from an enclosing scope -- e.g. `int value; for (auto value : r)`.
  // So only a clash within the current innermost scope is an error, which is
  // exactly what SyntaxAddSymbol's insert enforces; an enclosing-scope match
  // must not be rejected.  (Searching all scopes here also spuriously fired
  // during template instantiation, when a sibling member's parameter of the
  // same name was still visible in an enclosing scope.)
  if (!SyntaxAddSymbol(syntax, sym)) {
    SyntaxError(syntax, "Duplicate definition of local symbol %s",
                sym->name.value);
    return false;
  }
  sym->flags.is_local = true;
  sym->flags.is_defined = true;
  return true;
}

static ASTNode* NewRangeForMemberCall(Symbol* range_sym, const char* name,
                                      SourceLocation location) {
  ASTNode* member =
      NewStringConstantASTNode(NewString(name), NULL, location);
  ASTNode* access =
      NewBinaryASTNode(AST_OP(dot), NULL, location,
                       NewIdentifierASTNode(range_sym, location), member);
  return NewVectorASTNode(AST_OP(call), NULL, location, access, NewVector());
}

static ASTNode* NewRangeForADLCall(Syntax* syntax, Symbol* range_sym,
                                   const char* name,
                                   SourceLocation location) {
  String function_name;
  StringInit(&function_name, name);
  Symbol* function = SyntaxFindSymbol(syntax, &function_name);
  if (function == NULL) {
    TypeRecord* return_type =
        NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
    TypeRecord* function_type = NewFunctionTypeRecord();
    function_type->info.function.unknown_args = true;
    TypeRecordChain(function_type, return_type);
    function = NewSymbol(name, function_type, STO(implicit));
    function->flags.is_forward_declared = true;
    function->flags.invented = true;
    SyntaxAddSymbol(syntax, function);
  }
  StringDestruct(&function_name);
  Vector* args = NewVector();
  VectorAppend(args, NewIdentifierASTNode(range_sym, location));
  return NewVectorASTNode(AST_OP(call), NULL, location,
                          NewIdentifierASTNode(function, location), args);
}

static ASTNode* NewRangeForMemberAccess(Symbol* object, const char* name,
                                        SourceLocation location) {
  return NewBinaryASTNode(
      AST_OP(dot), NULL, location,
      NewIdentifierASTNode(object, location),
      NewStringConstantASTNode(NewString(name), NULL, location));
}

static void AppendRangeForBindingDeclarations(Syntax* syntax,
                                              RangeForBinding* binding,
                                              ASTNode* current,
                                              Vector* body_statements,
                                              SourceLocation location) {
  if (binding->loop_var != NULL) {
    VectorAppend(body_statements,
                 NewRangeForDeclarationList(binding->loop_var, current,
                                            location));
    return;
  }

  // Emit a real structured-binding declaration and let the ordinary lowering
  // apply the [dcl.struct.bind] rules: array elements, the tuple-like `get<I>`
  // protocol (std::pair and std::tuple, hence map iteration), or otherwise the
  // class's non-static data members in declaration order.
  Vector* names = NewVector();
  for (size_t i = 0; i < binding->names.length; i++) {
    String* name = binding->names.value.p[i];
    VectorAppend(names, NewString(name->value));
  }
  Vector* symbols = NewVector();
  VectorAppendVector(symbols, &binding->symbols);
  Vector* declarations = NewVector();
  VectorAppend(declarations,
               NewStructuredBindingASTNode(
                   TypeRecordCopy(binding->structured_binding_type), names,
                   symbols, binding->pack_index,
                   NewExpressionInitializerASTNode(current, location),
                   location));
  VectorAppend(body_statements,
               NewDeclarationListASTNode(declarations, location));
}

static ASTNode* NewRangeForIteratorLoop(Syntax* syntax,
                                        RangeForBinding* binding,
                                        TypeRecord* iterator_type,
                                        ASTNode* begin_init,
                                        ASTNode* end_init,
                                        ASTNode* stmt,
                                        SourceLocation location) {
  Symbol* begin = iterator_type != NULL
                      ? SyntaxNewTemporary(syntax, iterator_type)
                      : NewRangeForAutoSymbol(syntax, NULL, location);
  Symbol* end = iterator_type != NULL
                    ? SyntaxNewTemporary(syntax, TypeRecordCopy(iterator_type))
                    : NewRangeForAutoSymbol(syntax, NULL, location);
  if (iterator_type == NULL) {
    SyntaxAddSymbol(syntax, begin);
    SyntaxAddSymbol(syntax, end);
  }

  Vector* statements = NewVector();
  VectorAppend(statements, NewRangeForDeclarationList(begin, begin_init,
                                                      location));
  VectorAppend(statements, NewRangeForDeclarationList(end, end_init,
                                                      location));

  Vector* body_statements = NewVector();
  ASTNode* current =
      NewUnaryASTNode(AST_OP(contents), NULL, location,
                      NewIdentifierASTNode(begin, location));
  if (iterator_type == NULL) {
    current->flags |= kASTDeferredRangeContents;
  }
  AppendRangeForBindingDeclarations(syntax, binding, current, body_statements,
                                    location);
  VectorAppend(body_statements, stmt);
  ASTNode* body = NewCompoundStatementASTNode(body_statements, location);

  ASTNode* cond =
      NewBinaryASTNode(AST_OP(noteq), NULL, location,
                       NewIdentifierASTNode(begin, location),
                       NewIdentifierASTNode(end, location));
  ASTNode* next =
      NewUnaryASTNode(AST_OP(preinc), NULL, location,
                      NewIdentifierASTNode(begin, location));
  ASTNode* loop = NewForStatementASTNode(NULL, cond, next, body, location);
  VectorAppend(statements, loop);
  return NewCompoundStatementASTNode(statements, location);
}

static ASTNode* NewRangeForArrayLoop(Syntax* syntax,
                                     RangeForBinding* binding,
                                     Symbol* range_sym,
                                     TypeRecord* range_type,
                                     ASTNode* stmt,
                                     SourceLocation location) {
  ASTNode* begin_init = NewIdentifierASTNode(range_sym, location);
  TypeRecord* iterator_type = NewPointerTo(kQualPlain, range_type->next);
  ASTNode* end_init =
      NewBinaryASTNode(AST_OP(plus), NULL, location,
                       NewIdentifierASTNode(range_sym, location),
                       NewIntConstantASTNode(range_type->info.array.size.fixed,
                                             NewTypeRecordWithSize(kTypeInt,
                                                                   kQualPlain),
                                             location));
  return NewRangeForIteratorLoop(syntax, binding, iterator_type, begin_init,
                                 end_init, stmt, location);
}

static ASTNode* NewRangeForMemberIteratorLoop(Syntax* syntax,
                                              RangeForBinding* binding,
                                              Symbol* range_sym,
                                              ASTNode* stmt,
                                              SourceLocation location) {
  return NewRangeForIteratorLoop(
      syntax, binding, NULL,
      NewRangeForMemberCall(range_sym, "begin", location),
      NewRangeForMemberCall(range_sym, "end", location), stmt, location);
}

static ASTNode* NewRangeForADLIteratorLoop(Syntax* syntax,
                                           RangeForBinding* binding,
                                           Symbol* range_sym, ASTNode* stmt,
                                           SourceLocation location) {
  return NewRangeForIteratorLoop(
      syntax, binding, NULL,
      NewRangeForADLCall(syntax, range_sym, "begin", location),
      NewRangeForADLCall(syntax, range_sym, "end", location), stmt, location);
}

static bool RangeTypeHasMemberBeginEnd(TypeRecord* range_type) {
  if (range_type == NULL || !TypeIsStructOrUnion(range_type) ||
      range_type->info.struct_info == NULL) {
    return false;
  }
  String begin_name;
  String end_name;
  StringInit(&begin_name, "begin");
  StringInit(&end_name, "end");
  bool result =
      FindStructMember(range_type->info.struct_info, &begin_name) != NULL &&
      FindStructMember(range_type->info.struct_info, &end_name) != NULL;
  StringDestruct(&begin_name);
  StringDestruct(&end_name);
  return result;
}

// Build the deferred begin-expr / end-expr for a loop over a dependent range.
// The ADL form is built here, at the loop's point of definition, because that is
// where the unqualified lookup of `begin` and `end` has to happen; the array and
// member forms need nothing but the range itself and are synthesized on
// resolution.
bool SyntaxTypeHasRangeMemberBeginEnd(TypeRecord* range_type) {
  return RangeTypeHasMemberBeginEnd(range_type);
}

static ASTNode* NewRangeForDependentIterator(Syntax* syntax, ASTOpcode op,
                                            Symbol* range_sym,
                                            SourceLocation location);

ASTNode* SyntaxNewRangeForBoundExpr(Syntax* syntax, Symbol* range_sym,
                                    bool is_begin, SourceLocation location) {
  ASTOpcode op = is_begin ? AST_OP(range_begin) : AST_OP(range_end);
  return NewRangeForDependentIterator(syntax, op, range_sym, location);
}

static ASTNode* NewRangeForDependentIterator(Syntax* syntax, ASTOpcode op,
                                            Symbol* range_sym,
                                            SourceLocation location) {
  const char* name = op == AST_OP(range_begin) ? "begin" : "end";
  return NewBinaryASTNode(
      op, NULL, location, NewIdentifierASTNode(range_sym, location),
      NewRangeForADLCall(syntax, range_sym, name, location));
}

static ASTNode* NewRangeForDependentIteratorLoop(Syntax* syntax,
                                                RangeForBinding* binding,
                                                Symbol* range_sym,
                                                ASTNode* stmt,
                                                SourceLocation location) {
  return NewRangeForIteratorLoop(
      syntax, binding, NULL,
      NewRangeForDependentIterator(syntax, AST_OP(range_begin), range_sym,
                                   location),
      NewRangeForDependentIterator(syntax, AST_OP(range_end), range_sym,
                                   location),
      stmt, location);
}

// Puts |replacement| where the deferred |iterator| node sat, so that later
// analysis -- which rewrites a node by replacing it in its parent -- updates the
// live tree rather than the discarded one.
static ASTNode* RangeForIteratorReplacement(ASTNode* iterator,
                                            ASTNode* replacement) {
  if (iterator->parent != NULL) {
    ASTNodeReplaceChild(iterator->parent, iterator->child_id, replacement,
                        /*delete_old_child=*/false);
  }
  return replacement;
}

ASTNode* SyntaxResolveRangeForIterator(ASTNode* node) {
  BinaryASTNode* iterator = (BinaryASTNode*)node;
  bool is_begin = node->op == AST_OP(range_begin);
  ASTNode* range = iterator->left;
  Symbol* range_sym = range != NULL && range->op == AST_OP(identifier)
                          ? ((IdentifierASTNode*)range)->symbol
                          : NULL;
  TypeRecord* range_type = range_sym != NULL ? range_sym->type : NULL;
  if (range_type != NULL && TypeIsReference(range_type)) {
    range_type = range_type->next;
  }
  // The range may still be the class-template primary plus arguments (a view
  // type such as `transform_view<ref_view<int[6]>, F>` named inside another
  // template).  Its members only exist on the specialization, so materialize
  // that before asking whether the class has `begin`/`end`; otherwise the loop
  // falls through to the ADL form and no `begin` is found.
  range_type =
      TypeMaterializeClassTemplateSpecialization(&compiler->syntax, range_type);
  SourceLocation location = node->location;

  if (range_sym != NULL && range_type != NULL && TypeIsArray(range_type) &&
      !TypeIsVLA(range_type)) {
    // `__range + 0` and `__range + N`: the array decays and both bounds are
    // pointers into it, matching the array form of [stmt.ranged].
    int offset = is_begin ? 0 : range_type->info.array.size.fixed;
    return RangeForIteratorReplacement(
        node,
        NewBinaryASTNode(AST_OP(plus), NULL, location,
                         NewIdentifierASTNode(range_sym, location),
                         NewIntConstantASTNode(
                             offset, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                             location)));
  }
  if (range_sym != NULL && RangeTypeHasMemberBeginEnd(range_type)) {
    return RangeForIteratorReplacement(
        node,
        NewRangeForMemberCall(range_sym, is_begin ? "begin" : "end", location));
  }
  return RangeForIteratorReplacement(node, iterator->right);
}

static bool TryParseRangeForStructuredBinding(Syntax* syntax,
                                              RangeForBinding* binding) {
  if (!LexMatch(syntax->lex, TOK(lsquare))) {
    return false;
  }
  while (!LexEof(syntax->lex) && !LexLookingAt(syntax->lex, TOK(rsquare))) {
    bool is_pack = LexMatch(syntax->lex, TOK(ellipsis));
    if (is_pack) {
      if (!CompilerCXXAtLeast(kLanguageStandardCXX26)) {
        SyntaxError(syntax,
                    "Structured binding packs require C++26");
      }
      if (binding->pack_index >= 0) {
        SyntaxError(syntax,
                    "Structured binding declaration cannot contain multiple packs");
      } else {
        binding->pack_index = (int)binding->names.length;
      }
    }
    if (!LexLookingAt(syntax->lex, TOK(identifier))) {
      SyntaxError(syntax, "Expected structured binding name");
      break;
    }
    VectorAppend(&binding->names, NewString(syntax->lex->spelling.value));
    LexNextToken(syntax->lex);
    Vector* attributes = NewVector();
    if (SyntaxLookingAtCXXAttribute(syntax)) {
      if (!CompilerCXXAtLeast(kLanguageStandardCXX26)) {
        SyntaxError(syntax,
                    "Attributes on structured bindings require C++26");
      }
      SyntaxParseCXXAttributes(syntax, attributes);
    }
    VectorAppend(&binding->attributes, attributes);
    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(syntax, TOK(rsquare), TC(closebra));
  if (binding->pack_index >= 0 &&
      syntax->current_template_parameter_count == 0) {
    SyntaxError(
        syntax,
        "Structured binding pack can only appear in a templated context");
  }
  return true;
}

static bool TryParseRangeForBinding(Syntax* syntax,
                                    RangeForBinding* binding) {
  if (!SyntaxLookingAtType(syntax)) {
    return false;
  }

  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(auto), kParsingBlockScope);
  TypeRecord* type = TypeParserParseType(&parser, true);
  if (type == NULL) {
    TypeParserDestruct(&parser);
    return false;
  }
  TypeRecordIncRef(type);

  LexCheckpoint structured_checkpoint;
  LexCheckpointSave(syntax->lex, &structured_checkpoint);
  TypeRecord* declared_type = TypeRecordCopy(type);
  if (LexLookingAt(syntax->lex, TOK(amp)) ||
      LexLookingAt(syntax->lex, TOK(ampamp))) {
    bool rvalue = LexMatch(syntax->lex, TOK(ampamp));
    if (!rvalue) {
      LexMatch(syntax->lex, TOK(amp));
    }
    TypeRecord* reference = NewReferenceTypeRecord(kQualPlain, rvalue);
    TypeRecordChain(reference, declared_type);
    TypeRecordCalculateSize(reference);
    declared_type = reference;
  }
  if (TryParseRangeForStructuredBinding(syntax, binding)) {
    binding->structured_binding_type = declared_type;
    LexCheckpointDestruct(&structured_checkpoint);
    TypeRecordDelete(type);
    TypeParserDestruct(&parser);
    return true;
  }
  TypeRecordDelete(declared_type);
  LexCheckpointRestore(syntax->lex, &structured_checkpoint);
  LexCheckpointDestruct(&structured_checkpoint);

  binding->loop_var = TypeParserParseDeclarator(&parser, type);
  TypeRecordDelete(type);
  TypeParserDestruct(&parser);
  return binding->loop_var != NULL;
}

static void AddRangeForStructuredBindingVariables(Syntax* syntax,
                                                  RangeForBinding* binding,
                                                  SourceLocation location) {
  for (size_t i = 0; i < binding->names.length; i++) {
    String* name = binding->names.value.p[i];
    Symbol* sym = NewRangeForAutoSymbol(syntax, name->value, location);
    if (i < binding->attributes.length) {
      Vector* attributes = binding->attributes.value.p[i];
      AttributeListDestruct(&sym->attributes);
      AttributeListClone(&sym->attributes, attributes);
      SyntaxApplyDeclarationAttributes(syntax, sym);
    }
    sym->flags.is_parameter_pack = binding->pack_index == (int)i;
    sym->structured_binding_pack_size = -1;
    AddRangeForVariable(syntax, sym);
    VectorAppend(&binding->symbols, sym);
  }
}

static ASTNode* TryParseCXXRangeForStatement(Syntax* syntax,
                                             TokenClass followers,
                                             SourceLocation location) {
  if (!CompilerIsCXX()) {
    return NULL;
  }

  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);

  RangeForBinding binding;
  RangeForBindingInit(&binding);
  if (!TryParseRangeForBinding(syntax, &binding) ||
      !LexMatch(syntax->lex, TOK(colon))) {
    RangeForBindingDestruct(&binding);
    LexCheckpointRestore(syntax->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
    return NULL;
  }
  LexCheckpointDestruct(&checkpoint);

  if (binding.loop_var != NULL) {
    AddRangeForVariable(syntax, binding.loop_var);
  } else {
    AddRangeForStructuredBindingVariables(syntax, &binding, location);
  }
  ASTNode* range = SyntaxParseExpression(syntax, followers | TC(closebra));
  SyntaxNeedBracket(syntax, TOK(rparen), followers);

  TypeRecord* range_type = range != NULL ? range->type : NULL;
  if (range_type == NULL && range != NULL &&
      range->op == AST_OP(identifier)) {
    Symbol* original = ((IdentifierASTNode*)range)->symbol;
    range_type = original != NULL ? original->type : NULL;
  }
  if (range_type != NULL && TypeIsReference(range_type)) {
    range_type = range_type->next;
  }
  bool reuse_named_range =
      range != NULL && range->op == AST_OP(identifier);
  if (!reuse_named_range && range != NULL) {
    range = AnalyzeExpression(range);
  }
  bool range_is_lvalue =
      range != NULL && range->value_category == kValueCategoryLvalue;
  Symbol* range_sym =
      reuse_named_range ? ((IdentifierASTNode*)range)->symbol
                        : NewRangeForRangeSymbol(syntax, location,
                                                 !range_is_lvalue,
                                                 range != NULL ? range->type
                                                               : NULL);

  location = syntax->lex->current_token_location;
  syntax->loop_count++;
  ASTNode* stmt = SyntaxParseStatement(syntax, followers);
  syntax->loop_count--;
  ASTNode* loop = NULL;
  if (range_type != NULL && TypeIsArray(range_type) &&
      !TypeIsVLA(range_type)) {
    loop = NewRangeForArrayLoop(syntax, &binding, range_sym, range_type, stmt,
                                location);
  } else if (range_type != NULL &&
             RangeTypeHasMemberBeginEnd(
                 TypeMaterializeClassTemplateSpecialization(syntax,
                                                            range_type))) {
    loop = NewRangeForMemberIteratorLoop(syntax, &binding, range_sym, stmt,
                                         location);
  } else {
    // The type is either dependent or not yet deduced (a range named by an
    // `auto` variable whose initializer returns `auto` is only typed during
    // analysis).  Which of the three [stmt.ranged] forms applies then depends on
    // a type this parse cannot see, so emit a deferred begin/end pair carrying
    // the ADL fallback -- unqualified lookup of `begin`/`end` has to happen
    // here, at the loop's point of definition -- and choose between the array,
    // member and ADL forms once the range's type is known.
    loop = NewRangeForDependentIteratorLoop(syntax, &binding, range_sym, stmt,
                                            location);
  }
  Vector* statements = NewVector();
  if (!reuse_named_range) {
    VectorAppend(statements,
                 NewRangeForDeclarationList(range_sym, range, location));
  } else {
    ASTNodeDelete(range);
  }
  VectorAppend(statements, loop);
  ASTNode* result = NewCompoundStatementASTNode(statements, location);
  if (!reuse_named_range) {
    result->flags |= kASTRangeForInitializer;
  }
  RangeForBindingDestruct(&binding);
  return result;
}

// After a range-for init-statement has consumed its semicolon, distinguish the
// required range-declaration (`decl : range`) from an ordinary for condition
// without invoking the declaration parser speculatively.  That parser can emit
// diagnostics before its lexer checkpoint is restored; in particular,
// `index < count` was mistaken for an unterminated template-id.
static bool LookingAtCXXRangeForAfterInit(Syntax* syntax) {
  Lex* lex = syntax->lex;
  LexCheckpoint checkpoint;
  LexCheckpointSave(lex, &checkpoint);
  int paren_depth = 0;
  int square_depth = 0;
  int brace_depth = 0;
  int conditional_depth = 0;
  bool found_colon = false;
  while (!LexEof(lex)) {
    Token token = lex->current_token;
    bool top_level =
        paren_depth == 0 && square_depth == 0 && brace_depth == 0;
    if (top_level && (token == TOK(semicolon) || token == TOK(rparen))) {
      break;
    }
    if (token == TOK(lparen)) {
      paren_depth++;
    } else if (token == TOK(rparen)) {
      if (paren_depth == 0) {
        break;
      }
      paren_depth--;
    } else if (token == TOK(lsquare)) {
      square_depth++;
    } else if (token == TOK(rsquare) && square_depth > 0) {
      square_depth--;
    } else if (token == TOK(lbrace)) {
      brace_depth++;
    } else if (token == TOK(rbrace) && brace_depth > 0) {
      brace_depth--;
    } else if (top_level && token == TOK(question)) {
      conditional_depth++;
    } else if (top_level && token == TOK(colon)) {
      if (conditional_depth > 0) {
        conditional_depth--;
      } else {
        found_colon = true;
        break;
      }
    }
    LexNextToken(lex);
  }
  LexCheckpointRestore(lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return found_colon;
}

typedef struct {
  ExpansionItemKind item_kind;
  Symbol* item_symbol;
  TypeRecord* binding_type;
  Vector names;
  Vector symbols;
  Vector attributes;
  int pack_index;
  bool is_constexpr;
} ExpansionItemBinding;

static void ExpansionItemBindingInit(ExpansionItemBinding* binding) {
  binding->item_kind = kExpansionItemSimple;
  binding->item_symbol = NULL;
  binding->binding_type = NULL;
  VectorInit(&binding->names);
  VectorInit(&binding->symbols);
  VectorInit(&binding->attributes);
  binding->pack_index = -1;
  binding->is_constexpr = false;
}

static void ExpansionItemBindingDestruct(ExpansionItemBinding* binding) {
  VectorDestructWithContents(&binding->names,
                             (VectorElementDestructor)StringDelete,
                             /*free_element=*/false);
  for (size_t i = 0; i < binding->attributes.length; i++) {
    Vector* attributes = binding->attributes.value.p[i];
    AttributeListDestruct(attributes);
    VectorDelete(attributes);
  }
  VectorDestruct(&binding->attributes);
  VectorDestruct(&binding->symbols);
  TypeRecordDelete(binding->binding_type);
}

static bool ExpansionHasInitStatement(Syntax* syntax) {
  Lex* lex = syntax->lex;
  LexCheckpoint checkpoint;
  LexCheckpointSave(lex, &checkpoint);
  int paren = 1;
  int brace = 0;
  int bracket = 0;
  bool found = false;
  while (!LexEof(lex)) {
    Token t = lex->current_token;
    if (t == TOK(lparen)) {
      paren++;
    } else if (t == TOK(rparen)) {
      if (paren == 0) {
        break;
      }
      paren--;
    } else if (t == TOK(lbrace)) {
      brace++;
    } else if (t == TOK(rbrace)) {
      if (brace == 0) {
        break;
      }
      brace--;
    } else if (t == TOK(lsquare)) {
      bracket++;
    } else if (t == TOK(rsquare)) {
      if (bracket == 0) {
        break;
      }
      bracket--;
    } else if (t == TOK(colon) && paren == 1 && brace == 0 && bracket == 0) {
      break;
    } else if (t == TOK(semicolon) && paren == 1 && brace == 0 &&
               bracket == 0) {
      found = true;
      break;
    }
    LexNextToken(lex);
  }
  LexCheckpointRestore(lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return found;
}

static ASTNode* ParseExpansionInitStatement(Syntax* syntax,
                                            TokenClass followers) {
  SourceLocation location = syntax->lex->current_token_location;
  if (LexLookingAt(syntax->lex, TOK(semicolon))) {
    LexMatch(syntax->lex, TOK(semicolon));
    return NewCompoundStatementASTNode(NewVector(), location);
  }
  if (LookingAtUsingAliasDeclaration(syntax)) {
    if (!CompilerCXXAtLeast(kLanguageStandardCXX23)) {
      SyntaxError(syntax,
                  "alias declaration in init-statement requires C++23");
    }
    return SyntaxParseLocalDeclaration(syntax);
  }
  if (SyntaxLookingAtType(syntax)) {
    return SyntaxParseLocalDeclaration(syntax);
  }
  ASTNode* expr = SyntaxParseExpression(syntax, followers | TC(expr));
  SyntaxNeedSemicolon(syntax, followers | TC(expr));
  return NewExpressionStatementASTNode(expr, location);
}

static bool TryParseExpansionStructuredBinding(Syntax* syntax,
                                               ExpansionItemBinding* binding) {
  if (!LexMatch(syntax->lex, TOK(lsquare))) {
    return false;
  }
  while (!LexEof(syntax->lex) && !LexLookingAt(syntax->lex, TOK(rsquare))) {
    bool is_pack = LexMatch(syntax->lex, TOK(ellipsis));
    if (is_pack) {
      if (!CompilerCXXAtLeast(kLanguageStandardCXX26)) {
        SyntaxError(syntax, "Structured binding packs require C++26");
      }
      if (binding->pack_index >= 0) {
        SyntaxError(syntax,
                    "Structured binding declaration cannot contain multiple packs");
      } else {
        binding->pack_index = (int)binding->names.length;
      }
    }
    if (!LexLookingAt(syntax->lex, TOK(identifier))) {
      SyntaxError(syntax, "Expected structured binding name");
      break;
    }
    VectorAppend(&binding->names, NewString(syntax->lex->spelling.value));
    LexNextToken(syntax->lex);
    Vector* attributes = NewVector();
    if (SyntaxLookingAtCXXAttribute(syntax)) {
      if (!CompilerCXXAtLeast(kLanguageStandardCXX26)) {
        SyntaxError(syntax,
                    "Attributes on structured bindings require C++26");
      }
      SyntaxParseCXXAttributes(syntax, attributes);
    }
    VectorAppend(&binding->attributes, attributes);
    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(syntax, TOK(rsquare), TC(closebra));
  if (binding->pack_index >= 0 &&
      syntax->current_template_parameter_count == 0) {
    SyntaxError(
        syntax,
        "Structured binding pack can only appear in a templated context");
  }
  return true;
}

static bool TryParseExpansionItemDeclaration(Syntax* syntax,
                                             ExpansionItemBinding* binding) {
  if (!SyntaxLookingAtType(syntax)) {
    SyntaxError(syntax, "Expected expansion statement item declaration");
    return false;
  }

  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(auto), kParsingBlockScope);
  parser.allow_constexpr_decl_specifier = true;
  TypeRecord* type = TypeParserParseType(&parser, true);
  if (type == NULL) {
    TypeParserDestruct(&parser);
    SyntaxError(syntax, "Expected expansion statement item declaration");
    return false;
  }
  TypeRecordIncRef(type);
  binding->is_constexpr = parser.is_constexpr;

  LexCheckpoint structured_checkpoint;
  LexCheckpointSave(syntax->lex, &structured_checkpoint);
  TypeRecord* declared_type = TypeRecordCopy(type);
  if (LexLookingAt(syntax->lex, TOK(amp)) ||
      LexLookingAt(syntax->lex, TOK(ampamp))) {
    bool rvalue = LexMatch(syntax->lex, TOK(ampamp));
    if (!rvalue) {
      LexMatch(syntax->lex, TOK(amp));
    }
    TypeRecord* reference = NewReferenceTypeRecord(kQualPlain, rvalue);
    TypeRecordChain(reference, declared_type);
    TypeRecordCalculateSize(reference);
    declared_type = reference;
  }
  if (TryParseExpansionStructuredBinding(syntax, binding)) {
    binding->item_kind = kExpansionItemStructuredBinding;
    binding->binding_type = declared_type;
    LexCheckpointDestruct(&structured_checkpoint);
    TypeRecordDelete(type);
    TypeParserDestruct(&parser);
    return true;
  }
  TypeRecordDelete(declared_type);
  LexCheckpointRestore(syntax->lex, &structured_checkpoint);
  LexCheckpointDestruct(&structured_checkpoint);

  binding->item_symbol = TypeParserParseDeclarator(&parser, type);
  TypeRecordDelete(type);
  TypeParserDestruct(&parser);
  if (binding->item_symbol == NULL) {
    SyntaxError(syntax, "Expected expansion statement item declaration");
    return false;
  }
  binding->item_kind = kExpansionItemSimple;
  binding->item_symbol->flags.is_local = true;
  binding->item_symbol->flags.is_defined = true;
  binding->item_symbol->flags.is_constexpr = binding->is_constexpr;
  if (binding->is_constexpr && binding->item_symbol->type != NULL) {
    binding->item_symbol->type->qualifiers |= kQualConst;
  }
  return true;
}

static void AddExpansionStructuredBindingVariables(Syntax* syntax,
                                                   ExpansionItemBinding* binding,
                                                   SourceLocation location) {
  for (size_t i = 0; i < binding->names.length; i++) {
    String* name = binding->names.value.p[i];
    Symbol* sym = NewRangeForAutoSymbol(syntax, name->value, location);
    if (i < binding->attributes.length) {
      Vector* attributes = binding->attributes.value.p[i];
      AttributeListDestruct(&sym->attributes);
      AttributeListClone(&sym->attributes, attributes);
      SyntaxApplyDeclarationAttributes(syntax, sym);
    }
    sym->flags.is_parameter_pack = binding->pack_index == (int)i;
    sym->flags.is_constexpr = binding->is_constexpr;
    if (binding->is_constexpr && sym->type != NULL) {
      sym->type->qualifiers |= kQualConst;
    }
    sym->structured_binding_pack_size = -1;
    AddRangeForVariable(syntax, sym);
    VectorAppend(&binding->symbols, sym);
  }
}

static ASTNode* ParseExpansionStatement(Syntax* syntax, TokenClass followers,
                                        SourceLocation location) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX26)) {
    SyntaxError(syntax, "Expansion statements require C++26");
  }
  SyntaxNeedBracket(syntax, TOK(lparen), followers);

  SyntaxOpenScope(syntax);
  ASTNode* init_stmt = NULL;
  if (ExpansionHasInitStatement(syntax)) {
    init_stmt = ParseExpansionInitStatement(syntax, followers);
  }

  ExpansionItemBinding binding;
  ExpansionItemBindingInit(&binding);
  if (!TryParseExpansionItemDeclaration(syntax, &binding)) {
    ExpansionItemBindingDestruct(&binding);
    SyntaxCloseScope(syntax);
    return NewExpansionStatementASTNode(
        init_stmt, kExpansionItemSimple, NULL, NULL, NULL, NULL, -1,
        kExpansionInitializerExpression, NULL, NULL, location);
  }
  if (binding.item_kind == kExpansionItemSimple) {
    AddRangeForVariable(syntax, binding.item_symbol);
  } else {
    AddExpansionStructuredBindingVariables(syntax, &binding, location);
  }

  if (!LexMatch(syntax->lex, TOK(colon))) {
    SyntaxError(syntax, "Expected ':' after expansion statement item");
  }

  ExpansionInitializerKind init_kind = kExpansionInitializerExpression;
  ASTNode* initializer = NULL;
  if (LexLookingAt(syntax->lex, TOK(lbrace))) {
    init_kind = kExpansionInitializerInitList;
    LexNextToken(syntax->lex);
    initializer = SyntaxParseBracedInitializer(syntax);
  } else {
    initializer = SyntaxParseExpression(syntax, followers);
  }
  SyntaxNeedBracket(syntax, TOK(rparen), followers);

  syntax->loop_count++;
  if (!LexLookingAt(syntax->lex, TOK(lbrace))) {
    SyntaxError(syntax,
                "Expansion statement body must be a compound statement");
    SyntaxRecover(syntax, followers | TC(closebra));
  }
  ASTNode* stmt = SyntaxParseStatement(syntax, followers);
  syntax->loop_count--;
  SyntaxCloseScope(syntax);

  Vector* binding_names = NULL;
  Vector* binding_symbols = NULL;
  if (binding.item_kind == kExpansionItemStructuredBinding) {
    binding_names = NewVector();
    binding_symbols = NewVector();
    for (size_t i = 0; i < binding.names.length; i++) {
      VectorAppend(binding_names, binding.names.value.p[i]);
    }
    for (size_t i = 0; i < binding.symbols.length; i++) {
      VectorAppend(binding_symbols, binding.symbols.value.p[i]);
    }
    VectorInit(&binding.names);
    VectorInit(&binding.symbols);
  }

  ASTNode* result = NewExpansionStatementASTNode(
      init_stmt, binding.item_kind, binding.item_symbol, binding.binding_type,
      binding_names, binding_symbols, binding.pack_index, init_kind,
      initializer, stmt, location);
  binding.item_symbol = NULL;
  binding.binding_type = NULL;
  ExpansionItemBindingDestruct(&binding);
  return result;
}

// For statement.
static ASTNode* ParseForStatement(Syntax* syntax, TokenClass followers,
                                  SourceLocation location) {
  SyntaxNeedBracket(syntax, TOK(lparen), followers);
  Lex* lex = syntax->lex;
  ASTNode* c1 = NULL;
  ASTNode* c2 = NULL;
  ASTNode* c3 = NULL;

  SyntaxOpenScope(syntax);
  ASTNode* range_for = TryParseCXXRangeForStatement(syntax, followers,
                                                   location);
  if (range_for != NULL) {
    SyntaxCloseScope(syntax);
    return range_for;
  }
  if (!LexLookingAt(lex, TOK(semicolon))) {
    bool alias_init = LookingAtUsingAliasDeclaration(syntax);
    if (alias_init) {
      if (!CompilerCXXAtLeast(kLanguageStandardCXX23)) {
        SyntaxError(syntax,
                    "alias declaration in init-statement requires C++23");
      }
      c1 = SyntaxParseLocalDeclaration(syntax);
    } else if (SyntaxLookingAtType(syntax)) {
      c1 = SyntaxParseLocalDeclaration(syntax);
    } else {
      c1 = SyntaxParseExpression(syntax, followers);
      SyntaxNeedSemicolon(syntax, followers | TC(expr));
    }
  } else {
    SyntaxNeedSemicolon(syntax, followers | TC(expr));
  }
  if (CompilerCXXAtLeast(kLanguageStandardCXX20) &&
      LookingAtCXXRangeForAfterInit(syntax)) {
    range_for = TryParseCXXRangeForStatement(syntax, followers, location);
    if (range_for != NULL) {
      return FinishInitScope(syntax, c1, true, range_for, location);
    }
  }
  if (!LexLookingAt(lex, TOK(semicolon))) {
    c2 = SyntaxParseExpression(syntax, followers);
  }
  SyntaxNeedSemicolon(syntax, followers | TC(closebra));
  if (!LexLookingAt(lex, TOK(rparen))) {
    c3 = SyntaxParseExpression(syntax, followers);
  }
  SyntaxNeedBracket(syntax, TOK(rparen), followers);

  location = syntax->lex->current_token_location;
  syntax->loop_count++;
  ASTNode* stmt = SyntaxParseStatement(syntax, followers);
  syntax->loop_count--;
  SyntaxCloseScope(syntax);
  return NewForStatementASTNode(c1, c2, c3, stmt, location);
}

static ASTNode* ParseBreakStatement(Syntax* syntax, TokenClass followers,
                                    SourceLocation location) {
  if (syntax->loop_count == 0 && syntax->switch_count == 0) {
    SyntaxError(syntax, "break outside loop or switch");
  }
  return NewASTNode(AST_OP(break), NULL, location);
}

static ASTNode* ParseContinueStatement(Syntax* syntax, TokenClass followers,
                                       SourceLocation location) {
  if (syntax->loop_count == 0) {
    SyntaxError(syntax, "continue outside loop");
  }
  return NewASTNode(AST_OP(continue), NULL, location);
}

static bool LanguageAllowsLabelAtEndOfCompoundStatement(void) {
  return CompilerCAtLeast(kLanguageStandardC23) ||
         CompilerCXXAtLeast(kLanguageStandardCXX23);
}

static void DiagnoseLabelAtEndOfCompoundStatement(Syntax* syntax) {
  SyntaxError(
      syntax,
      CompilerIsCXX()
          ? "label at end of compound statement requires C++23"
          : "label at end of compound statement requires C23");
}

static ASTNode* ParseStatementAfterLabel(Syntax* syntax,
                                         TokenClass followers) {
  if (CompilerCAtLeast(kLanguageStandardC23) &&
      SyntaxLookingAtDeclaration(syntax)) {
    return SyntaxParseLocalDeclaration(syntax);
  }
  return SyntaxParseStatement(syntax, followers);
}

static ASTNode* ParseCaseStatement(Syntax* syntax, TokenClass followers,
                                   SourceLocation location) {
  if (syntax->switch_count == 0) {
    SyntaxError(syntax, "case outside switch");
  }
  ASTNode* expr = SyntaxParseExpression(syntax, followers);
  if (!LexMatch(syntax->lex, TOK(colon))) {
    SyntaxError(syntax, "Missing colon after case");
  }
  // We don't want to create a deep tree of case statements for the common
  // code sequence:
  // case a:
  // case b:
  // ...
  // case c:
  //
  ASTNode* stmt = NULL;
  if (!LexLookingAt(syntax->lex, TOK(case)) &&
      !LexLookingAt(syntax->lex, TOK(default)) &&
      !LexLookingAt(syntax->lex, TOK(rbrace))) {
    stmt = ParseStatementAfterLabel(syntax, followers);
  } else if (LexLookingAt(syntax->lex, TOK(rbrace)) &&
             !LanguageAllowsLabelAtEndOfCompoundStatement()) {
    DiagnoseLabelAtEndOfCompoundStatement(syntax);
  }
  return NewCaseLabelASTNode(expr, stmt, location);
}

static ASTNode* ParseDefaultStatement(Syntax* syntax, TokenClass followers,
                                      SourceLocation location) {
  if (syntax->switch_count == 0) {
    SyntaxError(syntax, "default outside switch");
  }
  if (!LexMatch(syntax->lex, TOK(colon))) {
    SyntaxError(syntax, "Missing colon after default");
  }
  ASTNode* stmt = NULL;
  if (LexLookingAt(syntax->lex, TOK(rbrace))) {
    if (!LanguageAllowsLabelAtEndOfCompoundStatement()) {
      DiagnoseLabelAtEndOfCompoundStatement(syntax);
    }
  } else {
    stmt = ParseStatementAfterLabel(syntax, followers);
  }
  return NewCaseLabelASTNode(NULL, stmt, location);
}

static ASTNode* ParseReturnStatement(Syntax* syntax, TokenClass followers,
                                     SourceLocation location) {
  ASTNode* expr = NULL;
  if (!LexLookingAt(syntax->lex, TOK(semicolon))) {
    if (LexMatch(syntax->lex, TOK(asm))) {
      // Extension: Allow return asm("foo")
      // This is to allow us to use assembly language as a function
      // return value.  The assembly code must set up the return
      // value correctly as we don't interpreter it in any way.
      // For example:
      //    return asm("mv a0, s0\n")
      // will move the RISC-V frame pointer into the return value
      // and then return from the function.
      expr = ParseAsmStatement(syntax, followers, location);
    } else if (CompilerIsCXX() && LexMatch(syntax->lex, TOK(lbrace))) {
      // `return {};` / `return {a, b};` returns a braced-init-list, which
      // value-initializes (or aggregate/list-initializes) the returned object.
      // Semantic analysis lowers it to a temporary of the function's return
      // type.
      expr = SyntaxParseBracedInitializer(syntax);
    } else {
      expr = SyntaxParseExpression(syntax, followers);
    }
  }
  return NewCombinedStatementASTNode(AST_OP(return), expr, NULL,
                                     location);
}

static ASTNode* ParseCoReturnStatement(Syntax* syntax, TokenClass followers,
                                       SourceLocation location) {
  ASTNode* expr = NULL;
  if (!LexLookingAt(syntax->lex, TOK(semicolon))) {
    expr = SyntaxParseExpression(syntax, followers);
  }
  return NewCombinedStatementASTNode(AST_OP(co_return), expr, NULL, location);
}

static ASTNode* ParseGotoStatement(Syntax* syntax, TokenClass followers,
                                   SourceLocation location) {
  if (!LexLookingAt(syntax->lex, TOK(identifier))) {
    SyntaxError(syntax, "Label expected after goto");
    return NULL;
  } else {
    String* label_name = NewString(syntax->lex->spelling.value);
    LexNextToken(syntax->lex);
    return NewGotoStatementASTNode(label_name, location);
  }
}

static Symbol* ParseCatchDeclaration(Syntax* syntax, TokenClass followers) {
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(auto), kParsingBlockScope);
  TypeRecord* type = TypeParserParseType(&parser, true);
  Symbol* symbol = TypeParserParseDeclarator(&parser, type);
  TypeParserDestruct(&parser);
  if (symbol == NULL) {
    SyntaxError(syntax, "Expected catch declaration");
    SyntaxRecover(syntax, followers);
    return NULL;
  }
  symbol->flags.is_local = true;
  symbol->flags.is_defined = true;
  return symbol;
}

static ASTNode* ParseCatchHandler(Syntax* syntax, TokenClass followers) {
  SourceLocation location = syntax->lex->current_token_location;
  SyntaxNeedBracket(syntax, TOK(lparen), followers);

  bool is_catch_all = false;
  Symbol* symbol = NULL;
  if (LexMatch(syntax->lex, TOK(ellipsis))) {
    is_catch_all = true;
  } else {
    symbol = ParseCatchDeclaration(syntax, followers | TC(closebra));
  }
  SyntaxNeedBracket(syntax, TOK(rparen), followers);

  if (!LexLookingAt(syntax->lex, TOK(lbrace))) {
    SyntaxError(syntax, "catch handler requires a compound statement");
  }

  SyntaxOpenScope(syntax);
  if (symbol != NULL && !SyntaxAddSymbol(syntax, symbol)) {
    SyntaxError(syntax, "Duplicate definition of local symbol %s",
                symbol->name.value);
  }
  ASTNode* stmt = SyntaxParseStatement(syntax, followers);
  SyntaxCloseScope(syntax);
  return NewCatchASTNode(symbol, is_catch_all, stmt, location);
}

static ASTNode* ParseTryStatement(Syntax* syntax, TokenClass followers,
                                  SourceLocation location) {
  if (!LexLookingAt(syntax->lex, TOK(lbrace))) {
    SyntaxError(syntax, "try statement requires a compound statement");
  }
  ASTNode* try_stmt = SyntaxParseStatement(syntax, followers);
  Vector* catches = NewVector();
  while (LexMatch(syntax->lex, TOK(catch))) {
    VectorAppend(catches, ParseCatchHandler(syntax, followers));
  }
  if (catches->length == 0) {
    SyntaxError(syntax, "try statement requires at least one catch handler");
  }
  return NewTryASTNode(try_stmt, catches, location);
}

static ASTNode* ParseContractAssertStatement(Syntax* syntax,
                                             TokenClass followers,
                                             SourceLocation location) {
  Vector attributes = {0};
  VectorInit(&attributes);
  SyntaxParseCXXAttributes(syntax, &attributes);
  SyntaxNeedBracket(syntax, TOK(lparen), followers | TC(closebra));
  ASTNode* predicate =
      SyntaxParseSingleExpression(syntax, followers | TC(closebra));
  SyntaxNeedBracket(syntax, TOK(rparen), followers | TC(semicolon));
  if (predicate == NULL) {
    AttributeListDestruct(&attributes);
    return NULL;
  }
  return NewContractAssertASTNode(predicate, &attributes, location);
}

// Table of statement parsers.
struct StatementParser {
  Token token;
  ASTNode* (*parser)(Syntax* syntax, TokenClass followers,
                     SourceLocation location);
  bool need_semicolon;
} statement_parsers[] = {
  {TOK(semicolon), NULL, false},
  {TOK(lbrace), ParseCompoundStatement, false},
  {TOK(if), ParseIfStatement, false},
  {TOK(while), ParseWhileStatement, false},
  {TOK(do), ParseDoStatement, true},
  {TOK(for), ParseForStatement, false},
  {TOK(switch), ParseSwitchStatement, false},
  {TOK(break), ParseBreakStatement, true},
  {TOK(continue), ParseContinueStatement, true},
  {TOK(case), ParseCaseStatement, false},
  {TOK(default), ParseDefaultStatement, false},
  {TOK(return), ParseReturnStatement, true},
  {TOK(co_return), ParseCoReturnStatement, true},
  {TOK(goto), ParseGotoStatement, true},
  {TOK(try), ParseTryStatement, false},
  {TOK(contract_assert), ParseContractAssertStatement, true},
  {TOK(asm), ParseAsmStatement, true},
};

#define NUM_STATEMENT_PARSERS \
  (sizeof(statement_parsers) / sizeof(statement_parsers[0]))

// Parse a single statement, returning an AST node or NULL.
ASTNode* SyntaxParseStatement(Syntax* syntax, TokenClass followers) {
  followers |= TC(stmt);
  Lex* lex = syntax->lex;
  ASTNode* stmt = NULL;
  bool need_semicolon = true;
  SourceLocation location = syntax->lex->current_token_location;
  if (SyntaxLookingAtCXXAttribute(syntax)) {
    Vector attrs = {0};
    VectorInit(&attrs);
    SyntaxParseCXXAttributes(syntax, &attrs);
    AttributeListDestruct(&attrs);
  }

  if (CompilerIsCXX() && LexLookingAt(lex, TOK(template))) {
    LexCheckpoint template_checkpoint;
    LexCheckpointSave(lex, &template_checkpoint);
    LexNextToken(lex);
    if (LexLookingAt(lex, TOK(for))) {
      LexNextToken(lex);
      LexCheckpointDestruct(&template_checkpoint);
      stmt = ParseExpansionStatement(syntax, followers, location);
      if (stmt != NULL) {
        stmt->flags |= kASTStatementStart;
      }
      return stmt;
    }
    LexCheckpointRestore(lex, &template_checkpoint);
    LexCheckpointDestruct(&template_checkpoint);
  }

  if (CompilerCXXAtLeast(kLanguageStandardCXX26) &&
      LexLookingAt(lex, TOK(consteval))) {
    LexCheckpoint cp;
    LexCheckpointSave(lex, &cp);
    LexNextToken(lex);
    if (LexLookingAt(lex, TOK(lbrace))) {
      LexNextToken(lex);
      LexCheckpointDestruct(&cp);
      ASTNode* body = ParseCompoundStatement(syntax, followers, location);
      stmt = NewConstevalBlockASTNode(body, location);
      if (stmt != NULL) stmt->flags |= kASTStatementStart;
      return stmt;
    }
    LexCheckpointRestore(lex, &cp);
    LexCheckpointDestruct(&cp);
  }

  bool found = false;
  for (size_t i = 0; i < NUM_STATEMENT_PARSERS; i++) {
    struct StatementParser* parser = &statement_parsers[i];
    if (syntax->lex->current_token == parser->token) {
      LexNextToken(lex);
      if (parser->parser != NULL) {
        stmt = (*parser->parser)(syntax, followers, location);
      }
      need_semicolon = parser->need_semicolon;
      found = true;
      break;
    }
  }
  if (!found) {
    // Expression or label statement.
    if (LexLookingAt(lex, TOK(identifier))) {
      // This will skip forward to the next non-space or non-comment
      // but will not change the current token.
      LexSkipSpacesAndComments(lex);
      if (lex->line.value[lex->pos] == ':' &&
          lex->line.value[lex->pos + 1] != ':') {
        String label_name;
        StringInit(&label_name, lex->spelling.value);
        LexNextToken(lex);  // Consume label name.
        LexNextToken(lex);  // Consume colon.
        ASTNode* label_stmt = NULL;
        if (LexLookingAt(lex, TOK(rbrace))) {
          if (!LanguageAllowsLabelAtEndOfCompoundStatement()) {
            DiagnoseLabelAtEndOfCompoundStatement(syntax);
          }
        } else {
          label_stmt = ParseStatementAfterLabel(syntax, followers);
        }
        stmt = NewLabelASTNode(label_name.value, label_stmt, false,
                               syntax->lex->current_token_location);
        need_semicolon = false;
        StringDestruct(&label_name);
      } else {
        ASTNode* expr = SyntaxParseExpression(syntax, followers);
        stmt = NewExpressionStatementASTNode(expr, location);
      }
    } else {
      ASTNode* expr = SyntaxParseExpression(syntax, followers);
      stmt = NewExpressionStatementASTNode(expr, location);
    }
    
  }

  if (need_semicolon) {
    SyntaxNeedSemicolon(syntax, followers);
  }
  if (stmt != NULL) {
    // This is the start of a statement.
    stmt->flags |= kASTStatementStart;
  }
  return stmt;
}
