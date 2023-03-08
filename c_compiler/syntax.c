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
#include "errors.h"
#include "compiler.h"

jmp_buf error_abort_state;       // Where to abort to.
bool abort_on_error;

static int next_pc_label_id = 0;
static String next_pc_label;

void SyntaxInit(Syntax* syntax, Lex* lex) {
  syntax->ast = NULL;
  syntax->lex = lex;
  syntax->local_symbol_stack = NULL;
  syntax->local_tag_stack = NULL;
  syntax->fake_name_index = 1;
  syntax->found_open_paren = false;
  syntax->compound_literal_type = NULL;
  syntax->loop_count = 0;
  syntax->switch_count = 0;
  VectorInit(&syntax->all_local_symbols);
  VectorInit(&syntax->local_statics);
  VectorInit(&syntax->all_symbols);
  syntax->context = kParsingFileScope;
}


void SyntaxDestruct(Syntax* syntax) {
  VectorDestructWithContents(&syntax->all_local_symbols,
                             (VectorElementDestructor)SymbolDestruct, /*free_element=*/true);
  VectorDestruct(&syntax->local_statics);
  ASTNodeDelete(syntax->ast);
}

void SyntaxResetForNewDeclaration(Syntax* syntax) {
  VectorCopy(&syntax->all_symbols, &syntax->all_local_symbols);
  
  VectorDestruct(&syntax->all_local_symbols);
  VectorDestruct(&syntax->local_statics);
  ASTNodeDelete(syntax->ast);
  
  syntax->ast = NULL;
  syntax->local_symbol_stack = NULL;
  syntax->local_tag_stack = NULL;
  syntax->found_open_paren = false;
  syntax->compound_literal_type = NULL;
  syntax->loop_count = 0;
  syntax->switch_count = 0;
  VectorInit(&syntax->all_local_symbols);
  VectorInit(&syntax->local_statics);
}

ASTNode* SyntaxNewPCLabel(SourceLocation location) {
  StringInit(&next_pc_label, NULL);
  StringPrintf(&next_pc_label, ".PC.%d", next_pc_label_id++);
  ASTNode* node = NewLabelASTNode(next_pc_label.value, NULL, true, location);
  StringDestruct(&next_pc_label);
  return node;
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
    Symbol* symbol = FindSymbol(&scope->table, name);
    if (symbol != NULL) {
      return symbol;
    }
    return NULL;
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
  if (abort_on_error) {
    longjmp(error_abort_state, 1);
  }
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

void SyntaxNeedSemicolon(Syntax* syntax, TokenClass followers) {
  if (!LexMatch(syntax->lex, TOK(semicolon))) {
    SyntaxError(syntax, "Expected semicolon");
    SyntaxRecover(syntax, followers | TC(semicolon));
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

static bool IsDefinition(TypeParser* parser, Symbol* sym, Storage storage) {
  if (StorageIs(storage, STO(extern))) {
    return false;
  }
  if (sym->flags.is_argument) {
    return true;
  }
  return LexLookingAt(parser->lex, TOK(equal)) ||
      LexLookingAt(parser->lex, TOK(lbrace));
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
      index_expr = AnalyzeExpression(index_expr);
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
  return NewBracedInitializerASTNode(initializers, NULL, location);
}

// Parses a symbol initializer.
ASTNode* SyntaxParseInitializer(Syntax* syntax, Symbol* sym, Storage storage) {
  if (StorageIs(storage, STO(extern))) {
    SyntaxWarning(syntax, "extern-with-init", "extern with initializer");
  }
  if (StorageIs(storage, STO(typedef))) {
    SyntaxError(syntax, "typdefs can't have initializers");
  }
  if (!StorageIs(storage, STO(extern))) {
    sym->flags.is_defined = true;
  }
  if (LexMatch(syntax->lex, TOK(lbrace))) {
    // Braced initializer.
    return ParseBracedInitializer(syntax);
  }

  SourceLocation location = syntax->lex->current_token_location;
  return NewExpressionInitializerASTNode(
      SyntaxParseSingleExpression(syntax, TC(semicolon) | TC(stmt)), location);
}

// Parse attributes and return a bitmask containing them.
void SyntaxParseAttribute(Syntax* syntax, Vector* attrs) {
  String attribute_list = {0};
  LexReadAttributes(syntax->lex, &attribute_list);
  StringSplit(&attribute_list, ',', attrs);
  SyntaxNeedBracket(syntax, TOK(rparen), 0);
  StringDestruct(&attribute_list);
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
      formal->flags.is_argument = true;
      formal->value.arg_number = prev_formal->value.arg_number;
      SymbolDelete(prev_formal);
      return;
    }
  }
  SyntaxError(syntax, "No such function parameter %s", formal->name.value);
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
    }
    // We need a function body after the argument declarations.
    if (!LexLookingAt(syntax->lex, TOK(lbrace))) {
      SyntaxError(syntax, "Function body expected");
    }
  }
  
  
  if (LexMatch(syntax->lex, TOK(lbrace))) {
    if (old_sym != NULL && TypeIsFunction(old_sym->type)) {
      old_sym->value.func_defn = sym;
    }
    ParserContext old_context = syntax->context;
    syntax->context = kParsingBlockScope;
    
    sym->flags.is_defined = true;
    sym->type->info.function.definition = true;
    
    // Parse the function body.
    Vector* body = NewVector();
    if (compiler->debug_output) {
      VectorAppend(body, SyntaxNewPCLabel(syntax->lex->current_token_location));
    }
    
    while (syntax->lex->current_token != TOK(rbrace) &&
           syntax->lex->current_token != TOK(eof)) {
      ASTNode* stmt;
      if (SyntaxLookingAtDeclaration(syntax)) {
        // Declaration.
        stmt = SyntaxParseLocalDeclaration(syntax);
      } else {
        stmt = SyntaxParseStatement(syntax,TC(semicolon));
      }
      if (stmt != NULL) {
        VectorAppend(body, stmt);
      }
    }
    syntax->context = old_context;
    
    if (compiler->debug_output) {
      VectorAppend(body, SyntaxNewPCLabel(syntax->lex->current_token_location));
    }
    sym->type->info.function.body =
        NewCompoundStatementASTNode(body, syntax->lex->current_token_location);
    SyntaxNeedBracket(syntax, TOK(rbrace), TC(decl));
    ASTNode* decl = NewVariableDeclarationASTNode(sym, NULL,
                                                  syntax->lex->current_token_location);
    VectorAppend(declarations, decl);
    
    return NewDeclarationListASTNode(declarations,
                                     syntax->lex->current_token_location);
  } else {
    // Function is a declaration.  If the old symbol was inline, this is
    // an inline definition.
    if (old_sym != NULL && old_sym->type->info.function.is_inline) {
      old_sym->flags.is_inline_defn = true;
    }
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
                                      TypeRecord** type, Vector* attributes, ParserContext context) {
  PartialTypeSpecifier type_specifier = {
    .type = kTypeImplicit,
    .quals = kQualPlain,
    .type_record = NULL,
    .error = false
  };

  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), context);
  
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
    } else if (SyntaxLookingAtType(syntax) &&
               (type_specifier.type & (kTypeStruct | kTypeUnion | kTypeEnum)) == 0) {
      type_specifier = TypeParserParseAndCombineTypes(&parser, &type_specifier);
    } else if (LexMatch(syntax->lex, TOK(attribute))) {
      SyntaxParseAttribute(syntax, attributes);
    } else {
      *type = TypeParserBuildTypeRecord(&parser, &type_specifier);
      return;
    }
  }
}

static ASTNode* ParseExternalDeclarationList(TypeParser* parser,
                                         TypeRecord* type,
                                         Storage storage,
                                         Vector* attributes,
                                         Vector* declarations) {
  Syntax* syntax = parser->syntax;
  while (!LexEof(parser->syntax->lex)) {
    if (LexLookingAt(syntax->lex, TOK(semicolon))) {
      // We don't need to have a variable declaration.
      break;
    }

    Symbol* sym = TypeParserParseDeclarator(parser, type);
    // The old_sym refers to a previous declaration if found.
    Symbol* old_sym = NULL;
    if (sym != NULL) {
      old_sym = FindGlobalSymbol(&sym->name);
      bool ok = true;
      if (old_sym != NULL) {
        // We have this symbol already.  If it's a declaration then it's
        // OK to declare (and define) it now.  If it's a definition then
        // this must be a declaration.
        if (old_sym->flags.is_defined) {
          if (StorageIs(storage, STO(extern))) {
            // This might be a declaration, only if there is no initializer
            if (LexLookingAt(parser->lex, TOK(equal))) {
              // This is 'extern int foo = xxx', a definition
              SyntaxError(syntax, "Duplicate definition of symbol %s",
                          sym->name.value);
              ok = false;
            }
          } else {
            if (IsDefinition(parser, old_sym, storage)) {
              // This is a declaration of a previously known definition.
              SyntaxError(syntax, "Duplicate definition of symbol %s",
                          sym->name.value);
              ok = false;
            }
          }
        } else {
          // Old sym is a declaration.
          if (IsDefinition(parser, sym,  storage)) {
            old_sym->flags.is_defined = true;
          } else if (!StorageIs(storage, STO(extern))) {
            old_sym->flags.is_tentative_decl = true;
          }
        }
        if (!TypeEqual(sym->type, old_sym->type)) {
          SyntaxError(syntax, "Symbol %s redeclared with different type",
                      sym->name.value);
          TypeErrorDetails(syntax->lex->current_token_location,
                           sym->type, old_sym->type);
          const char* filename;
          int lineno, start, end;
          DecodeSourceLocation(old_sym->location, &filename, &lineno, &start, &end);
          ReportNote(filename, lineno, "Previously declared here");
          ok = false;
        } else {
          // Symbol declaration is the same type as the definition, make sure
          // the linkage matches.
          Storage old_storage = old_sym->storage & ~STO(extern);
          Storage new_storage = sym->storage & ~STO(extern);

          if (old_storage != new_storage) {
            SyntaxError(syntax, "Symbol %s redeclared with different linkage",
                        sym->name.value);
            ok = false;
          }
          sym->flags.is_defined = true;
        }
      } else {
        // This is the first declaration of this symbol, add to the symbol
        // table.
        bool ok = InsertGlobalSymbol(sym);
        assert(ok);
        if (IsDefinition(parser, sym, storage)) {
          sym->flags.is_defined = true;
        } else if (!StorageIs(storage, STO(extern))) {
          sym->flags.is_tentative_decl = true;
        }
      }
    }

    // If we don't have a symbol in the type declarator we look for a comma and
    // continue parsing.
    if (sym == NULL) {
      if (!LexMatch(syntax->lex, TOK(comma))) {
        break;
      }
      TypeParserReset(parser);
      continue;
    }

    // Parse common __attribute__ syntax.
    while (LexMatch(syntax->lex, TOK(attribute))) {
      SyntaxParseAttribute(syntax, attributes);
    }
    
    VectorCopy(&sym->attributes, attributes);
    VectorClear(attributes);
  
    // Declaring or defining a function?
    if (TypeIsFunction(sym->type)) {
      ASTNode *result = DeclareOrDefineFunction(syntax, declarations, sym, old_sym);
      if (result != NULL) {
        return result;
      }
    } else if (parser->is_inline) {
      SyntaxError(syntax, "inline can only be applied to functions");
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
      syntax->init_storage = storage;
      initializer = SyntaxParseInitializer(syntax, sym, storage);
    }
    ASTNode* decl = NewVariableDeclarationASTNode(
        sym, initializer, syntax->lex->current_token_location);
    VectorAppend(declarations, decl);

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


// Parses an external declaration (a global variable, etc.) and adds it
// to the symbol table.
ASTNode* SyntaxParseExternalDeclaration(Syntax* syntax) {
  Vector* declarations = NewVector();
  Vector attributes = {0};
  syntax->context = kParsingFileScope;
  
  // Parse common __attribute__ syntax.
  while (LexMatch(syntax->lex, TOK(attribute))) {
    SyntaxParseAttribute(syntax, &attributes);
  }

  Storage storage = STO(implicit);
  bool is_inline = false;
  TypeRecord* type = NULL;
  ParseDeclarationSpecifier(syntax, &storage, &is_inline, &type, &attributes, kParsingFileScope);

  if (StorageIs(storage, STO(auto)|STO(register))) {
    SyntaxError(syntax, "Illegal global storage specified: %s",
                StorageIs(storage, STO(register)) ? "register" : "auto");
    storage = STO(implicit);
  }

  // Parse common __attribute__ syntax.
  while (LexMatch(syntax->lex, TOK(attribute))) {
    SyntaxParseAttribute(syntax, &attributes);
  }

  // Create a type parser for the declarators.
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, storage, kParsingFileScope);
  parser.is_inline = is_inline;
 
  // Open a scope (local symbol stack) for the symbols declared in the
  // type declaration list.
  SyntaxOpenScope(syntax);

  // Now we get a sequence of declarations, separated by commas.
  ASTNode* result = ParseExternalDeclarationList(&parser,
                                                 type, storage,
                                                 &attributes,
                                                 declarations);
  SyntaxCloseScope(syntax);
  if (result != NULL) {
    if (declarations->length != 1) {
      SyntaxError(syntax, "Cannot mix function definition with declaration");
      for (size_t i = 0; i < declarations->length; i++) {
        ASTNodeDelete(declarations->value.p[i]);
      }
      VectorDelete(declarations);
    }
    VectorDestruct(&attributes);
    return result;
  }

  // The declaration is followed by a semicolon.
  SyntaxNeedSemicolon(syntax, TC(type));

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

static void ParseLocalDeclarationList(TypeParser* parser,
                                      TypeRecord* type, Storage storage,
                                      Vector* attributes, Vector* declarations) {
  Syntax* syntax = parser->syntax;
  while (!LexEof(syntax->lex)) {
    if (LexLookingAt(syntax->lex, TOK(semicolon))) {
      // We don't need to have a variable declaration.
      break;
    }
    Symbol* sym = TypeParserParseDeclarator(parser, type);
    if (sym != NULL) {
      Symbol* old_sym =
          FindTopLocalSymbol(syntax->local_symbol_stack, &sym->name);
      bool ok = true;
      if (old_sym != NULL) {
        // We have this symbol already.  If it's a declaration then it's
        // OK to declare (and define) it now.  If it's a definition then
        // this must be a declaration.
        if (old_sym->flags.is_defined) {
          if (StorageIs(storage, STO(extern))) {
            // This might be a declaration, only if there is no initializer
            if (LexLookingAt(parser->lex, TOK(equal))) {
              SyntaxError(syntax,
                          "Extern variable can't have an initializer: %s",
                          sym->name.value);
              ok = false;
            }
          } else {
            if (IsDefinition(parser, old_sym, storage)) {
              // This is a declaration of a previously known definition.
              SyntaxError(syntax, "Duplicate definition of local symbol %s",
                          sym->name.value);
              ok = false;
            }
          }
        } else {
          // Old sym is a declaration.
          if (IsDefinition(parser, sym, storage)) {
            // This is a definition so the original symbol is now defined.
            old_sym->flags.is_defined = true;
          } else if (!StorageIs(storage, STO(extern))) {
            old_sym->flags.is_tentative_decl = true;
          }
        }

        if (!TypeEqual(sym->type, old_sym->type)) {
          SyntaxError(syntax, "Symbol %s redeclared with different type",
                      sym->name.value);
          TypeErrorDetails(syntax->lex->current_token_location,
                                    sym->type, old_sym->type);
           const char* filename;
           int lineno, start, end;
           DecodeSourceLocation(old_sym->location, &filename, &lineno, &start, &end);
           ReportNote(filename, lineno, "Previously declared here");          ok = false;
        } else {
          // Symbol declaration is the same type as the definition, make sure
          // the linkage matches.
          Storage old_storage = old_sym->storage & ~STO(extern);
          Storage new_storage = sym->storage & ~STO(extern);

          if (old_storage != new_storage) {
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
        bool ok = SyntaxAddSymbol(syntax, sym);
        assert(ok);
      }
    }

    // If we don't have a symbol in the type declarator we look for a comma and
    // continue parsring.
    if (sym == NULL) {
      if (!LexMatch(syntax->lex, TOK(comma))) {
        break;
      }
      TypeParserReset(parser);
      continue;
    }

    if (!StorageIs(sym->storage, STO(extern))) {
      // This is a local variable.
      sym->flags.is_local = true;
    }

    // Symbol takes ownerhip of attribute strings.
    VectorCopy(&sym->attributes, attributes);
    VectorClear(attributes);
    
    // Check for __thread violations.
    CheckThreadLocal(syntax, sym);

    // Declaring or defining a function?
    if (TypeIsFunction(sym->type)) {
      if (LexMatch(syntax->lex, TOK(lbrace))) {
        // C does not supported nested functions.
        SyntaxError(syntax, "Function definition not allowed here");
        // Skip function body so that we can attempt to recover.
        SkipFunctionBody(syntax);
        // Continue on to check for comma, which probably won't exist
        // then we will exit the loop.
      }
    } else {
      sym->flags.is_defined = true;
      if (parser->is_inline) {
         SyntaxError(syntax, "inline can only be applied to functions");
      }
      // Any initializer?
      ASTNode* initializer = NULL;
      if (LexMatch(syntax->lex, TOK(equal))) {
        syntax->init_storage = storage;
        initializer = SyntaxParseInitializer(syntax, sym, storage);

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
      if (TypeIsConst(sym->type)) {
        SemanticAnalyzeVariableDefinition(syntax,
                                        (VariableDeclarationASTNode*)decl);
      }
      
      if (StorageIs(storage, STO(static))) {
        VectorAppend(&syntax->local_statics, decl);
      }
    }

    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
    TypeParserReset(parser);
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
  Vector attributes = {0};
  syntax->context = kParsingBlockScope;
  
  ParseDeclarationSpecifier(syntax, &storage, &is_inline, &type, &attributes, kParsingBlockScope);

  if (is_inline) {
    SyntaxError(syntax, "inline is not allowed here");
  }
  
  // Parse the type specifier (char, unsigned int, etc.)
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, storage, kParsingBlockScope);

  // Now we get a sequence of declarations, separated by commas.
  ParseLocalDeclarationList(&parser, type, storage, &attributes, declarations);

  // The declaration is followed by a semicolon.
  SyntaxNeedSemicolon(syntax, TC(type));

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
  sym->flags.is_temp = true;
  sym->flags.invented = true;
  SyntaxAddSymbol(syntax, sym);
  return sym;
}

TokenClass ClassifyToken(Token tok) {
  switch (tok) {
    case TOK(bad):
    case TOK(eof):
      return 0;

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
