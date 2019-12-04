//
//  syntax.c
//  c_compiler
//
//  Created by David Allison on 10/28/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include <stdlib.h>

#include <assert.h>
#include "expr_evaluator.h"
#include "expr_parser.h"
#include "expr_semantics.h"
#include "statement_parser.h"
#include "symbol_table.h"
#include "syntax.h"
#include "type.h"

void SyntaxInit(Syntax* syntax, Lex* lex) {
  syntax->ast = NULL;
  syntax->lex = lex;
  syntax->local_symbol_stack = NULL;
  syntax->local_tag_stack = NULL;
  syntax->fake_name_index = 1;
  syntax->found_open_paren = false;
  syntax->loop_count = 0;
  syntax->switch_count = 0;
  VectorInit(&syntax->all_local_symbols);
}

void SyntaxDestruct(Syntax* syntax) {
  // Delete all the local symbols.
  size_t num_symbols = syntax->all_local_symbols.length;
  for (size_t i = 0; i < num_symbols; i++) {
    SymbolDelete((Symbol*)VectorGet(&syntax->all_local_symbols, i));
  }
  VectorDestruct(&syntax->all_local_symbols);
  
  ASTNodeDelete(syntax->ast);
}

Symbol* SyntaxFindSymbol(Syntax* syntax, String* name) {
  LocalSymbolTable* scope = syntax->local_symbol_stack;
  Symbol* symbol = FindLocalSymbol(scope, name);
  if (symbol != NULL) {
    return symbol;
  }
  return FindGlobalSymbol(name);
}

bool SyntaxAddSymbol(Syntax* syntax, Symbol* symbol) {
  if (syntax->local_symbol_stack == NULL) {
    return InsertGlobalSymbol(symbol);
  }
  bool ok = InsertLocalSymbol(syntax->local_symbol_stack, symbol);
  if (ok) {
    VectorAppend(&syntax->all_local_symbols, symbol);
  }
  return ok;
}

Symbol* SyntaxFindTag(Syntax* syntax, String* name) {
  LocalSymbolTable* scope = syntax->local_tag_stack;
  Symbol* symbol = FindLocalSymbol(scope, name);
  if (symbol != NULL) {
    return symbol;
  }
  return FindGlobalTag(name);
}

Symbol* SyntaxFindTopScopeTag(Syntax* syntax, String* name) {
  LocalSymbolTable* scope = syntax->local_tag_stack;
  if (scope != NULL) {
    Symbol* symbol = FindSymbol(scope->table, name);
    if (symbol != NULL) {
      return symbol;
    }
  }
  return FindGlobalTag(name);
}

bool SyntaxAddTag(Syntax* syntax, Symbol* symbol) {
  if (syntax->local_tag_stack == NULL) {
    return InsertGlobalTag(symbol);
  }
  bool ok = InsertLocalSymbol(syntax->local_tag_stack, symbol);
  if (ok) {
    VectorAppend(&syntax->all_local_symbols, symbol);
  }
  return ok;
}

void SyntaxError(Syntax* syntax, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  VLexError(syntax->lex, format, ap);
  va_end(ap);
}

void SyntaxWarning(Syntax* syntax, const char* warn, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  VLexWarning(syntax->lex, warn, format, ap);
  va_end(ap);
}

void SyntaxNeedSemicolon(Syntax* syntax) {
  if (!LexMatch(syntax->lex, TOK(semicolon))) {
    SyntaxError(syntax, "Expected semicolon");
  }
}

const char* SyntaxFakeName(Syntax* syntax) {
  snprintf(syntax->fake_name_buffer, sizeof(syntax->fake_name_buffer),
           "__invented__%d", syntax->fake_name_index);
  syntax->fake_name_index++;
  return syntax->fake_name_buffer;
}

Storage SyntaxParseStorage(Syntax* syntax) {
  switch (syntax->lex->current_token) {
    case TOK(extern):
      LexNextToken(syntax->lex);
      return STO(extern);
    case TOK(typedef):
      LexNextToken(syntax->lex);
      return STO(typedef);
    case TOK(auto):
      LexNextToken(syntax->lex);
      return STO(auto);
    case TOK(static):
      LexNextToken(syntax->lex);
      return STO(static);
    case TOK(register):
      LexNextToken(syntax->lex);
      return STO(register);
    case TOK(thread):
      LexNextToken(syntax->lex);
      return STO(thread);
   default:
      return STO(implicit);
  }
}

static ASTNode* ParseBracedInitializer(Syntax* syntax);

static void ParseDesignatedInitializer(Syntax* syntax,
                                           Vector* initializers) {
  Vector* designators = NewVector();
  SourceLocation location = syntax->lex->current_token_location;
  
  // Collect all designators.  Each is either an array or struct designator.
  // The array designator is a constant expression enclosed in square
  // brackets. The struct designator is a dot followed by a struct member
  // name.
  while (LexLookingAt(syntax->lex, TOK(lsquare)) ||
         LexLookingAt(syntax->lex, TOK(dot))) {
    if (LexMatch(syntax->lex, TOK(lsquare))) {
      // Array designator.
      ASTNode* index_expr = SyntaxParseExpression(syntax, TC(semicolon));
      AnalyzeExpression(index_expr);
      int64_t value;
      bool ok = EvaluateIntegerExpression(index_expr, &value);
      if (!ok) {
        SyntaxError(syntax,
              "Need constant expression inside [] in designated initializer");
        value = 0;
      }
      SyntaxNeedBracket(syntax, TOK(rsquare), TC(closebra));
      VectorAppend(designators, NewArrayDesignator(NULL, (int)value));
    } else if (LexMatch(syntax->lex, TOK(dot))) {
      // Struct designator.  The dot is followed by a struct member name.
      String* member_name;
      if (LexLookingAt(syntax->lex, TOK(identifier))) {
        member_name = NewString(syntax->lex->spelling.value);
        LexNextToken(syntax->lex);
      } else {
        SyntaxError(syntax,
                    "Need member name after . in designated initializer");
        member_name = NewString(SyntaxFakeName(syntax));
      }
      VectorAppend(designators, NewStructDesignator(member_name));
    }
  }
  // This is followed by an = sign and an initializer.
  if (!LexMatch(syntax->lex, TOK(equal))) {
    SyntaxError(syntax, "Expected = in designated initializer");
    SyntaxRecover(syntax, TC(exprsep));
  } else {
    ASTNode* init;
    // The initialization expression is either a brace-enclosed
    // initializer or a single expression.
    if (LexMatch(syntax->lex, TOK(lbrace))) {
      init = ParseBracedInitializer(syntax);
    } else {
      init = NewExpressionInitializerASTNode(
                                             SyntaxParseSingleExpression(syntax, TC(exprsep)), location);
    }
    VectorAppend(initializers, NewDesignatedInitializerASTNode(
                                                               designators, init, location));
  }

}

// Parse a brace-enclosed initializer.
static ASTNode* ParseBracedInitializer(Syntax* syntax) {
  Vector* initializers = NewVector();
  SourceLocation location = syntax->lex->current_token_location;
  while (!LexLookingAt(syntax->lex, TOK(rbrace))) {
    if (LexMatch(syntax->lex, TOK(lbrace))) {
      // Braced initializer.
      VectorAppend(initializers, ParseBracedInitializer(syntax));
    } else if (LexLookingAt(syntax->lex, TOK(lsquare)) ||
               LexLookingAt(syntax->lex, TOK(dot))) {
      // Designated initializer.
      ParseDesignatedInitializer(syntax, initializers);
    } else {
      // Not a designated initializer, expression.
      ASTNode* expr = SyntaxParseSingleExpression(syntax, TC(exprsep));
      VectorAppend(initializers,
                   NewExpressionInitializerASTNode(expr, location));
    }
    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(syntax, TOK(rbrace), TC(closebra));
  return NewBracedInitializerASTNode(initializers, location);
}

// Parses a symbol initializer.
static ASTNode* ParseInitializer(Syntax* syntax, Symbol* sym, Storage storage) {
  if (StorageIs(storage, STO(extern))) {
    SyntaxWarning(syntax, "extern-with-init", "extern with initializer");
  }
  if (StorageIs(storage, STO(typedef))) {
    SyntaxError(syntax, "typdefs can't have initializers");
  }
  if (!StorageIs(storage, STO(extern))) {
    sym->is_defined = true;
  }
  if (LexMatch(syntax->lex, TOK(lbrace))) {
    // Braced initializer.
    return ParseBracedInitializer(syntax);
  }

  SourceLocation location = syntax->lex->current_token_location;
  return NewExpressionInitializerASTNode(
      SyntaxParseExpression(syntax, TC(semicolon) | TC(stmt)), location);
}

// Parse attributes and return a bitmask containing them.
void ParseAttribute(Syntax* syntax, Vector* attrs) {
  String attribute_list;
  StringInit(&attribute_list, NULL);
  LexReadAttributes(syntax->lex, &attribute_list);
  StringSplit(&attribute_list, ',', attrs);
  SyntaxNeedBracket(syntax, TOK(rparen), 0);
}

// For an old-style C function we have the types for the arguments specified
// in the definition, before the body.  The previous formal arguments need
// to be resolved by the new formals with types.
static void ResolveOldStyleFormalArgument(Syntax* syntax, TypeRecord* func, Symbol* formal) {
  Vector* formals = &func->info.function.prototype;
  for (size_t i = 0; i < formals->length; i++) {
    Symbol* prev_formal = (Symbol*)formals->value.p[i];
    if (StringEqualString(&prev_formal->name, &formal->name)) {
      formals->value.p[i] = formal;
      formal->is_argument = true;
      formal->value.arg_number = prev_formal->value.arg_number;
      SymbolDelete(prev_formal);
      return;
    }
  }
  SyntaxError(syntax, "No such function parameter %s", formal->name.value);
  SymbolDelete(formal);
}

static ASTNode* DeclareOrDefineFunction(Syntax* syntax,
                                        Vector* declarations,
                             Symbol* sym, Symbol* old_sym) {
  if (sym->type->info.function.old_style) {
    // Old style functions have the types of their formal
    // arguments specified before the open brace for their
    // body.
    while (!LexLookingAt(syntax->lex, TOK(lbrace))) {
      TypeParser arg_parser;
      TypeParserInit(&arg_parser, syntax->lex, syntax, STO(auto));
      TypeRecord* arg_type = TypeParserParseType(&arg_parser);
      if (arg_type != NULL) {
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
        SyntaxNeedSemicolon(syntax);
      }
    }
    // We need a function body after the argument declarations.
    if (!LexLookingAt(syntax->lex, TOK(lbrace))) {
      SyntaxError(syntax, "Function body expected");
    }
  }
  
  
  if (LexMatch(syntax->lex, TOK(lbrace))) {
    // Open a scope and add the formal arguments as symbols.
    SyntaxOpenScope(syntax);
    
    if (old_sym != NULL) {
      // We have a declaration that we are now defining.  The
      // names of the formal parameters might have been changed so
      // we need to replace their names with the current new ones.
      Vector* decl_prototype = &old_sym->type->info.function.prototype;
      Vector* defn_prototype = &sym->type->info.function.prototype;
      size_t num_decl_formals = decl_prototype->length;
      size_t num_defn_formals = defn_prototype->length;
      for (size_t i = 0; i < num_decl_formals && i < num_defn_formals;
           i++) {
        Symbol* decl_formal = decl_prototype->value.p[i];
        Symbol* defn_formal = defn_prototype->value.p[i];
        StringSet(&decl_formal->name, defn_formal->name.value);
      }
      // We now refer to the previously defined symbol rather than this new
      // one.
      SymbolDelete(sym);
      sym = old_sym;
    }
    
    Vector* prototype = &sym->type->info.function.prototype;
    size_t num_formals = prototype->length;
    for (size_t i = 0; i < num_formals; i++) {
      InsertLocalSymbol(syntax->local_symbol_stack,
                        (Symbol*)prototype->value.p[i]);
    }
    
    sym->is_defined = true;
    sym->type->info.function.definition = true;
    
    // Parse the function body.
    while (syntax->lex->current_token != TOK(rbrace)) {
      ASTNode* stmt = SyntaxParseStatement(syntax, TC(semicolon));
      if (stmt != NULL) {
        VectorAppend(&sym->type->info.function.body, stmt);
      }
    }
    SyntaxNeedBracket(syntax, TOK(rbrace), TC(decl));
    SyntaxCloseScope(syntax);
    ASTNode* decl = NewVariableDeclarationASTNode(sym, NULL,
                                                  syntax->lex->current_token_location);
    VectorAppend(declarations, decl);
    
    return NewDeclarationListASTNode(declarations,
                                     syntax->lex->current_token_location);
  }
  return NULL;
}

static bool IsPowerOf2OrZero(int32_t v) {
  return (v & (v - 1)) == 0;
}

// Only static variables can be thread local.  This checks for invalid
// symbol storage or type.
static void CheckThreadLocal(Syntax* syntax, Symbol* symbol) {
  if (StorageIs(symbol->storage, STO(thread))) {
    bool error = TypeIsFunction(symbol->type);
    error |= !StorageIs(symbol->storage, STO(static) | STO(extern));
    if (error) {
      SyntaxError(syntax, "Illegal use of __thread");
    }
  }
}

// A declaration specifier is a set of:
// 1. storage specifier
// 2. type specifier
// 3. function specifier (inline).
// This collects them into the output variables.
static void ParseDeclarationSpecifier(Syntax* syntax, Storage* storage, bool* is_inline,
                                      TypeRecord** type, Vector* attributes) {
  PartialTypeSpecifier type_specifier = {
    .type = kTypeImplicit,
    .quals = kQualPlain,
    .type_record = NULL,
    .error = false
  };

  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit));
  
  while (!LexEof(syntax->lex)) {
    Storage s = SyntaxParseStorage(syntax);
    if (s != STO(implicit)) {
      Storage new = s;
      Storage old = *storage;
    
      // Check for duplicate storage.
      if ((new & old) != 0) {
        SyntaxWarning(syntax, "dup-storage", "Duplicate storage specifier");
      }
    
      // Remove __thread from mask and check for multiple bits set.
      if (!IsPowerOf2OrZero((new | old) & ~STO(thread))) {
        SyntaxError(syntax,
                    "Multiple incompatible storage specifiers");
      }
      *storage |= s;
    } else if (LexMatch(syntax->lex, TOK(inline))) {
      if (*is_inline) {
        SyntaxWarning(syntax, "dup-inline", "Duplicate 'inline' specifier");
      }
      *is_inline = true;
    } else if (SyntaxLookingAtType(syntax)) {
      type_specifier = TypeParserParseAndCombineTypes(&parser, &type_specifier);
    } else if (LexMatch(syntax->lex, TOK(attribute))) {
      ParseAttribute(syntax, attributes);
    } else {
      *type = TypeParserBuildTypeRecord(&parser, &type_specifier);
      return;
    }
  }
}

// Parses an external declaration (a global variable, etc.) and adds it
// to the symbol table.
ASTNode* SyntaxParseExternalDeclaration(Syntax* syntax) {
  Vector* declarations = NewVector();
  Vector attributes;
  VectorInit(&attributes);
  
  // Parse common __attribute__ syntax.
  while (LexMatch(syntax->lex, TOK(attribute))) {
    ParseAttribute(syntax, &attributes);
  }

  Storage storage = STO(implicit);
  bool is_inline = false;
  TypeRecord* type = NULL;
  ParseDeclarationSpecifier(syntax, &storage, &is_inline, &type, &attributes);

  if (StorageIs(storage, STO(auto)) || StorageIs(storage, STO(register))) {
    SyntaxError(syntax, "Illegal global storage specified: %s",
                StorageIs(storage, STO(register)) ? "register" : "auto");
    storage = STO(implicit);
  }

  // We just treat inline as static for now, but the rules
  // for C99 inlining say we must treat 'extern inline' as a definition.
  // However, Mac OS seems to use 'extern inline' in header files (ctype.h
  // for example has "extern inline int isascii(...)" and I don't know how
  // this can work.
  if (is_inline) {
    storage = STO(static);
  }

  // Parse common __attribute__ syntax.
  while (LexMatch(syntax->lex, TOK(attribute))) {
    ParseAttribute(syntax, &attributes);
  }

  // Create a type parser for the declarators.
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, storage);

  // Now we get a sequence of declarations, separated by commas.
  while (!LexEof(syntax->lex)) {
    if (LexLookingAt(syntax->lex, TOK(semicolon))) {
      // We don't need to have a variable declaration.
      break;
    }

    Symbol* sym = TypeParserParseDeclarator(&parser, type);
    // The old_sym refers to a previous declaration if found.
    Symbol* old_sym = NULL;
    if (sym != NULL) {
      old_sym = FindGlobalSymbol(&sym->name);
      bool ok = true;
      if (old_sym != NULL) {
        // We have this symbol already.  If it's a declaration then it's
        // OK to declare (and define) it now.  If it's a definition then
        // this must be a declaration.
        if (old_sym->is_defined) {
          if (StorageIs(storage, STO(extern))) {
            // This might be a declaration, only if there is no initializer
            if (LexLookingAt(parser.lex, TOK(equal))) {
              // This is 'extern int foo = xxx', a definition
              SyntaxError(syntax, "Duplicate definition of symbol %s",
                          sym->name.value);
              ok = false;
            }
          } else {
            // This is a declaration of a previously known definition.
            SyntaxError(syntax, "Duplicate definition of symbol %s",
                        sym->name.value);
            ok = false;
          }
        } else {
          // Old sym is a declaration.
        }
        
        if (!TypeEqual(sym->type, old_sym->type)) {
          SyntaxError(parser.syntax, "Symbol %s redeclared with different type",
                      sym->name.value);
          ok = false;
        } else {
          // Symbol declaration is the same type as the definition, make sure
          // the linkage matches.
          if (old_sym->storage != sym->storage) {
            SyntaxError(syntax, "Symbol %s redeclared with different linkage",
                        sym->name.value);
            ok = false;
          }
        }
      } else {
        // This is the first declaration of this symbol, add to the symbol
        // table.
        bool ok = InsertGlobalSymbol(sym);
        assert(ok);
      }
    }

    // If we don't have a symbol in the type declarator we look for a comma and
    // continue parsing.
    if (sym == NULL) {
      if (!LexMatch(syntax->lex, TOK(comma))) {
        break;
      }
      TypeParserReset(&parser);
      continue;
    }

    // Parse common __attribute__ syntax.
    while (LexMatch(syntax->lex, TOK(attribute))) {
      ParseAttribute(syntax, &attributes);
    }
    
    VectorCopy(&sym->attributes, &attributes);
    VectorClear(&attributes);
  
    // Declaring or defining a function?
    if (sym->type->declarator == kDeclFunction) {
      ASTNode *result = DeclareOrDefineFunction(syntax, declarations, sym, old_sym);
      if (result != NULL) {
        return result;
      }
    }
    
    if (old_sym != NULL) {
      // We now refer to the previously defined symbol rather than this new
      // one.
      SymbolDelete(sym);
      sym = old_sym;
    }

    // Check for "asm" after declaration
    if (LexMatch(syntax->lex, TOK(asm))) {
      SyntaxNeedBracket(syntax, TOK(lparen), TC(openbra));
      // TODO: actually do something with this?
      // TODO: gcc asm syntax?
      while (LexLookingAt(syntax->lex, TOK(string))) {
        LexNextToken(syntax->lex);
      }
      SyntaxNeedBracket(syntax, TOK(rparen), TC(exprsep) | TC(decl));
    }

    // Any initializer?
    ASTNode* initializer = NULL;
    if (LexMatch(syntax->lex, TOK(equal))) {
      initializer = ParseInitializer(syntax, sym, storage);
    }

    ASTNode* decl = NewVariableDeclarationASTNode(
        sym, initializer, syntax->lex->current_token_location);
    VectorAppend(declarations, decl);

    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
    TypeParserReset(&parser);
  }

  // The declaration is followed by a semicolon.
  SyntaxNeedSemicolon(syntax);

  VectorDestruct(&attributes);
  
  return NewDeclarationListASTNode(declarations,
                                   syntax->lex->current_token_location);
}

// We have consumed the open brace, skip tokens until we find the
// matching close brace (or EOF).
static void SkipFunctionBody(Syntax* syntax) {
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

// Parses a local symbol declaration or definition.  This occurs inside a
// function.  The symbol is added to the local scope (top symbol table in the
// local symbol stack).
ASTNode* SyntaxParseLocalDeclaration(Syntax* syntax) {
  Vector* declarations = NewVector();

  Storage storage = STO(implicit);
  bool is_inline = false;
  TypeRecord* type = NULL;
  Vector attributes;
  VectorInit(&attributes);
  
  ParseDeclarationSpecifier(syntax, &storage, &is_inline, &type, &attributes);

  if (is_inline) {
    SyntaxError(syntax, "inline is not allowed here");
  }
  
  // Parse the type specifier (char, unsigned int, etc.)
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, storage);

  // Now we get a sequence of declarations, separated by commas.
  while (!LexEof(syntax->lex)) {
    if (LexLookingAt(syntax->lex, TOK(semicolon))) {
      // We don't need to have a variable declaration.
      break;
    }
    Symbol* sym = TypeParserParseDeclarator(&parser, type);
    if (sym != NULL) {
      Symbol* old_sym =
          FindTopLocalSymbol(parser.syntax->local_symbol_stack, &sym->name);
      bool ok = true;
      if (old_sym != NULL) {
        // We have this symbol already.  If it's a declaration then it's
        // OK to declare (and define) it now.  If it's a definition then
        // this must be a declaration.
        if (old_sym->is_defined) {
          if (StorageIs(storage, STO(extern))) {
            // This might be a declaration, only if there is no initializer
            if (LexLookingAt(parser.lex, TOK(equal))) {
              SyntaxError(syntax,
                          "Extern variable can't have an initializer: %s",
                          sym->name.value);
              ok = false;
            }
          } else {
            // This is a declaration of a previously known definition.
            SyntaxError(syntax, "Duplicate defintition of local symbol %s",
                        sym->name.value);
            ok = false;
          }
        } else {
          // Old sym is a declaration.
        }

        if (!TypeEqual(sym->type, old_sym->type)) {
          SyntaxError(syntax, "Symbol %s redeclared with different type",
                      sym->name.value);
          ok = false;
        } else {
          // Symbol declaration is the same type as the definition, make sure
          // the linkage matches.
          if (old_sym->storage != sym->storage) {
            SyntaxError(syntax, "Symbol %s redeclared with different linkage",
                        sym->name.value);
            ok = false;
          }
        }

        if (ok) {
          // We now refer to the previously defined symbol rather than this new
          // one.
          SymbolDelete(sym);
          sym = old_sym;
        }
      } else {
        // This is the first declaration of this symbol, add to the symbol
        // table.
        bool ok = SyntaxAddSymbol(parser.syntax, sym);
        assert(ok);
      }
    }

    // If we don't have a symbol in the type declarator we look for a comma and
    // continue parsring.
    if (sym == NULL) {
      if (!LexMatch(syntax->lex, TOK(comma))) {
        break;
      }
      TypeParserReset(&parser);
      continue;
    }

    if (!StorageIs(sym->storage, STO(extern))) {
      // This is a local variable.
      sym->is_local = true;
    }

    // Symbol takes ownerhip of attribute strings.
    VectorCopy(&sym->attributes, &attributes);
    VectorClear(&attributes);
    
    // Check for __thread violations.
    CheckThreadLocal(syntax, sym);

    // Declaring or defining a function?
    if (sym->type->declarator == kDeclFunction) {
      if (LexMatch(syntax->lex, TOK(lbrace))) {
        // C does not supported nested functions.
        SyntaxError(parser.syntax, "Function definition not allowed here");
        // Skip function body so that we can attempt to recover.
        SkipFunctionBody(parser.syntax);
        // Continue on to check for comma, which probably won't exist
        // then we will exit the loop.
      }
    } else {
      // Any initializer?
      ASTNode* initializer = NULL;
      if (LexMatch(syntax->lex, TOK(equal))) {
        initializer = ParseInitializer(syntax, sym, storage);

        ASTNode* decl_id =
            NewIdentifierASTNode(sym, syntax->lex->current_token_location);
        decl_id->flags |= kASTNeedAddress;

        // Create an assignment expression to initialize the variable.
        initializer = NewBinaryASTNode(AST_OP(init), sym->type,
                                       syntax->lex->current_token_location,
                                       decl_id, initializer);

        // Set flags to help semantic analyzer.
        decl_id->flags |= kASTIsDeclaration;
      }

      ASTNode* decl = NewVariableDeclarationASTNode(
          sym, initializer, syntax->lex->current_token_location);
      VectorAppend(declarations, decl);
    }

    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
    TypeParserReset(&parser);
  }

  // The declaration is followed by a semicolon.
  SyntaxNeedSemicolon(syntax);

  VectorDestruct(&attributes);
  
  return NewDeclarationListASTNode(declarations,
                                   syntax->lex->current_token_location);
}

void SyntaxNeedBracket(Syntax* syntax, Token bracket, TokenClass followers) {
  if (!LexMatch(syntax->lex, bracket)) {
    SyntaxError(syntax, "Missing %s", TokenName(bracket));
    SyntaxRecover(syntax, followers);
  }
}

void SyntaxRecover(Syntax* syntax, TokenClass tc) {
  while (!LexEof(syntax->lex)) {
    TokenClass c = ClassifyToken(syntax->lex->current_token);
    if ((c & tc) != 0) {
      break;
    }
    LexNextToken(syntax->lex);
  }
}

bool SyntaxLookingAtType(Syntax* syntax) {
  switch (syntax->lex->current_token) {
    case TOK(char):
    case TOK(int):
    case TOK(short):
    case TOK(long):
    case TOK(float):
    case TOK(double):
    case TOK(struct):
    case TOK(union):
    case TOK(enum):
    case TOK(bool):
    case TOK(signed):
    case TOK(unsigned):
    case TOK(const):
    case TOK(volatile):
    case TOK(restrict):
    case TOK(void):
      return true;
    case TOK(identifier): {
      Symbol* sym = SyntaxFindSymbol(syntax, &syntax->lex->spelling);
      if (sym == NULL) {
        return false;
      }
      if (StorageIs(sym->storage , STO(typedef))) {
        return true;
      }
      return false;
    }
    default:
      return false;
  }
}

bool SyntaxLookingAtDeclaration(Syntax* syntax) {
  switch (syntax->lex->current_token) {
    case TOK(extern):
    case TOK(static):
    case TOK(auto):
    case TOK(register):
    case TOK(typedef):
      return true;
    default:
      return SyntaxLookingAtType(syntax);
  }
}

// Opens a new scope by pushing a new symbol table into the local symbol stack
// and local tag stack.
void SyntaxOpenScope(Syntax* syntax) {
  LocalSymbolTable* table = NewLocalSymbolTable();
  table->prev = syntax->local_symbol_stack;
  syntax->local_symbol_stack = table;

  LocalSymbolTable* tag_table = NewLocalSymbolTable();
  tag_table->prev = syntax->local_tag_stack;
  syntax->local_tag_stack = tag_table;
}

// Closes a local variable scope.  This does not free the symbols contained
// in the symbol table at the discarded scope.  These are held in the
// all_local_symbols vector in the Syntax object and are kept until the
// compilation of the function is complete.
void SyntaxCloseScope(Syntax* syntax) {
  LocalSymbolTable* prev = syntax->local_symbol_stack->prev;
  LocalSymbolTableDelete(syntax->local_symbol_stack);
  syntax->local_symbol_stack = prev;

  prev = syntax->local_tag_stack->prev;
  LocalSymbolTableDelete(syntax->local_tag_stack);
  syntax->local_tag_stack = prev;
}

Symbol* SyntaxNewTemporary(Syntax* syntax, struct TypeRecord* type) {
  Symbol* sym = NewSymbol(SyntaxFakeName(syntax), type, STO(implicit));
  sym->is_temp = true;
  SyntaxAddSymbol(syntax, sym);
  return sym;
}

TokenClass ClassifyToken(Token tok) {
  switch (tok) {
    case TOK(bad):
    case TOK(eof):
      return TC(stmt);

    case TOK(number):
    case TOK(string):
    case TOK(string_wide):
    case TOK(charconst):
    case TOK(charconst_wide):
    case TOK(fnumber):
    case TOK(caret):
    case TOK(ellipsis):
    case TOK(false):
    case TOK(sizeof):
    case TOK(tilde):
      return TC(expr);

    case TOK(identifier):
      return TC(expr) | TC(stmt);

    case TOK(amp):
    case TOK(ampeq):
    case TOK(arrow):
    case TOK(bang):
    case TOK(bar):
    case TOK(careteq):
    case TOK(dot):
    case TOK(comma):
    case TOK(equalequal):
    case TOK(greater):
      return TC(exprsep);


    case TOK(equal):
      return TC(stmt) | TC(exprsep);

    case TOK(auto):
    case TOK(break):
    case TOK(case):
    case TOK(continue):
    case TOK(default):
    case TOK(do):
    case TOK(for):
    case TOK(if):
    case TOK(goto):
    case TOK(return ):
    case TOK(switch):
    case TOK(while):
    case TOK(asm):
    case TOK(attribute):
    case TOK(hash):
      return TC(stmt);

    case TOK(semicolon):
      return TC(stmt) | TC(semicolon);

    case TOK(extern):
    case TOK(inline):
    case TOK(register):
    case TOK(static):
    case TOK(typedef):
      return TC(stmt) | TC(decl);

    case TOK(bool):
    case TOK(char):
    case TOK(complex):
    case TOK(const):
    case TOK(double):
    case TOK(else):
    case TOK(float):
    case TOK(enum):
    case TOK(imaginary):
    case TOK(int):
    case TOK(long):
    case TOK(restrict):
    case TOK(short):
    case TOK(signed):
    case TOK(union):
    case TOK(unsigned):
    case TOK(void):
    case TOK(volatile):
    case TOK(wchar_t):
    case TOK(struct):
      return TC(type) | TC(stmt) | TC(decl);

    case TOK(colon):
      return TC(stmt) | TC(exprsep);

    case TOK(greatereq):
    case TOK(less):
    case TOK(lesseq):
    case TOK(ampamp):
    case TOK(barbar):
    case TOK(lessless):
    case TOK(lesslesseq):
    case TOK(minus):
    case TOK(minuseq):
    case TOK(minusminus):
    case TOK(bangeq):
    case TOK(bareq):
    case TOK(percent):
    case TOK(percenteq):
    case TOK(plus):
    case TOK(pluseq):
    case TOK(plusplus):
    case TOK(question):
    case TOK(greatergreater):
    case TOK(slash):
    case TOK(slasheq):
    case TOK(star):
    case TOK(stareq):
      return TC(exprsep);

    case TOK(lbrace):
      return TC(stmt);
    case TOK(lparen):
    case TOK(lsquare):
      return TC(exprsep) | TC(openbra);
    case TOK(rbrace):
      return TC(closebrace);
    case TOK(rparen):
    case TOK(rsquare):
      return TC(closebra);

    default:
      return 0;
  }
}
