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
#include "statement_parser.h"
#include "compiler.h"
#include "type.h"
#include "type_inheritance.h"

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

static Symbol* NewRangeForAutoSymbol(Syntax* syntax, const char* name,
                                     SourceLocation location) {
  Symbol* sym = NewSymbol(name != NULL ? name : SyntaxFakeName(syntax),
                          NewRangeForAutoType(), STO(auto));
  sym->location = location;
  sym->flags.is_local = true;
  sym->flags.is_defined = true;
  return sym;
}

typedef struct {
  Symbol* loop_var;
  Vector names;  // String* entries for [x, y] bindings.
  Vector symbols;  // Symbol* entries corresponding to names.
} RangeForBinding;

static void RangeForBindingInit(RangeForBinding* binding) {
  binding->loop_var = NULL;
  VectorInit(&binding->names);
  VectorInit(&binding->symbols);
}

static void RangeForBindingDestruct(RangeForBinding* binding) {
  VectorDestructWithContents(&binding->names,
                             (VectorElementDestructor)StringDelete,
                             /*free_element=*/false);
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
            VectorAppend(statements, destructor);
          }
        }
      } else {
        ASTNode* destructor =
            NewCXXDestructorCall(decl->symbol, decl->base.location);
        if (destructor != NULL) {
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
static ASTNode* ParseIfStatement(Syntax* syntax, TokenClass followers,
                                 SourceLocation location) {
  bool is_constexpr = false;
  if (CompilerCXXAtLeast(kLanguageStandardCXX17)) {
    is_constexpr = LexMatch(syntax->lex, TOK(constexpr));
  }
  SyntaxNeedBracket(syntax, TOK(lparen), followers);
  ASTNode* cond = SyntaxParseExpression(syntax, followers);
  SyntaxNeedBracket(syntax, TOK(rparen), followers);
  Lex* lex = syntax->lex;

  location = lex->current_token_location;
  ASTNode* if_part = SyntaxParseStatement(syntax, followers);
  ASTNode* else_part = NULL;
  if (LexMatch(lex, TOK(else))) {
    else_part = SyntaxParseStatement(syntax, followers);
  }
  return NewIfStatementASTNode(cond, if_part, else_part, is_constexpr,
                               location);
}

// A while statement.
static ASTNode* ParseWhileStatement(Syntax* syntax, TokenClass followers,
                                    SourceLocation location) {
  SyntaxNeedBracket(syntax, TOK(lparen), followers);
  ASTNode* cond = SyntaxParseExpression(syntax, followers);
  SyntaxNeedBracket(syntax, TOK(rparen), followers);
  location = syntax->lex->current_token_location;

  syntax->loop_count++;
  ASTNode* stmt = SyntaxParseStatement(syntax, followers);
  syntax->loop_count--;
  return NewCombinedStatementASTNode(AST_OP(while), cond, stmt, location);
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
  ASTNode* expr = SyntaxParseExpression(syntax, followers);
  SyntaxNeedBracket(syntax, TOK(rparen), followers);

  location = syntax->lex->current_token_location;
  syntax->switch_count++;
  ASTNode* stmt = SyntaxParseStatement(syntax, followers);
  syntax->switch_count--;
  return NewSwitchStatementASTNode(expr, stmt, location);
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

  Symbol* item = NewRangeForAutoSymbol(syntax, NULL, location);
  SyntaxAddSymbol(syntax, item);
  VectorAppend(body_statements, NewRangeForDeclarationList(item, current,
                                                           location));
  for (size_t i = 0; i < binding->symbols.length; i++) {
    Symbol* sym = binding->symbols.value.p[i];
    VectorAppend(body_statements,
                 NewRangeForDeclarationList(
                     sym,
                     NewRangeForMemberAccess(item, sym->name.value, location),
                     location));
  }
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

static bool TryParseRangeForStructuredBinding(Syntax* syntax,
                                              RangeForBinding* binding) {
  if (!LexMatch(syntax->lex, TOK(lsquare))) {
    return false;
  }
  while (!LexEof(syntax->lex) && !LexLookingAt(syntax->lex, TOK(rsquare))) {
    if (!LexLookingAt(syntax->lex, TOK(identifier))) {
      SyntaxError(syntax, "Expected structured binding name");
      break;
    }
    VectorAppend(&binding->names, NewString(syntax->lex->spelling.value));
    LexNextToken(syntax->lex);
    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(syntax, TOK(rsquare), TC(closebra));
  return true;
}

static bool TryParseRangeForBinding(Syntax* syntax,
                                    RangeForBinding* binding) {
  if (TryParseRangeForStructuredBinding(syntax, binding)) {
    return true;
  }
  if (!SyntaxLookingAtType(syntax)) {
    return false;
  }

  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(auto), kParsingBlockScope);
  TypeRecord* type = TypeParserParseType(&parser, true);
  TypeRecordIncRef(type);
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

  Symbol* range_sym = NULL;
  TypeRecord* range_type = NULL;
  if (range != NULL && range->op == AST_OP(identifier)) {
    range_sym = ((IdentifierASTNode*)range)->symbol;
    range_type = range_sym != NULL ? range_sym->type : NULL;
  }
  bool valid_named_range = range_type != NULL;
  if (!valid_named_range) {
    SyntaxError(syntax,
                "range-based for currently supports named ranges");
  }
  if (range != NULL) {
    ASTNodeDelete(range);
  }

  location = syntax->lex->current_token_location;
  syntax->loop_count++;
  ASTNode* stmt = SyntaxParseStatement(syntax, followers);
  syntax->loop_count--;
  ASTNode* result = NULL;
  if (!valid_named_range) {
    result = NewCompoundStatementASTNode(NewVector(), location);
  } else if (TypeIsArray(range_type) && !TypeIsVLA(range_type)) {
    result = NewRangeForArrayLoop(syntax, &binding, range_sym, range_type, stmt,
                                  location);
  } else if (TypeIsStructOrUnion(range_type)) {
    result = NewRangeForMemberIteratorLoop(syntax, &binding, range_sym, stmt,
                                           location);
  } else {
    SyntaxError(syntax,
                "range-based for supports fixed arrays or member begin/end ranges");
    result = NewCompoundStatementASTNode(NewVector(), location);
  }
  RangeForBindingDestruct(&binding);
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
    if (SyntaxLookingAtType(syntax)) {
      c1 = SyntaxParseLocalDeclaration(syntax);
    } else {
      c1 = SyntaxParseExpression(syntax, followers);
      SyntaxNeedSemicolon(syntax, followers | TC(expr));
    }
  } else {
    SyntaxNeedSemicolon(syntax, followers | TC(expr));
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
      !LexLookingAt(syntax->lex, TOK(default))) {
    stmt = SyntaxParseStatement(syntax, followers);
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
  ASTNode* stmt = SyntaxParseStatement(syntax, followers);
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
        ASTNode* label_stmt = SyntaxParseStatement(syntax, followers);
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
