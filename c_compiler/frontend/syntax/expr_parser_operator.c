//
//  expr_parser_operator.c
//  c_compiler
//
//  Binary, assignment, conditional, and pointer-to-member operators.
//

#include <stdint.h>

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "concepts.h"
#include "errors.h"
#include "expr_evaluator.h"
#include "expr_parser.h"
#include "expr_parser_internal.h"
#include "expr_semantics.h"
#include "preprocessor.h"
#include "reflection.h"
#include "statement_parser.h"
#include "symbol_table.h"
#include "std_header_suggestion.h"
#include "typo_correction.h"
#include "type.h"
#include "type_class_internal.h"
#include "type_internal.h"
#include "type_member.h"
#include "compiler.h"
#include "type_traits_semantics.h"


static bool LookingAtCompleteQualifiedName(Lex* lex);
static StructMember* PointerToMemberOverload(StructMember* member);
static StructMember* PointerToMemberTemplateSpecialization(StructMember* member, Vector* args);
static BinaryPrecedence BinaryOperatorPrecedence(Syntax* syntax, Token token, ASTOpcode* op);
static ASTNode* ParseBinaryOperand(Syntax* syntax, TokenClass followers, bool constraint);
static ASTNode* ParseBinaryExpression(Syntax* syntax, TokenClass followers, BinaryPrecedence min_precedence);
static ASTOpcode AssignASTOpcode(Syntax* syntax, Token tok);

// Skips a balanced `<...>` after a name component (`Q<int>::Get<2>`).
// Returns false when the brackets do not close before the statement ends.
static bool SkipTemplateArgumentTokens(Lex* lex) {
  if (!LexLookingAt(lex, TOK(less))) {
    return true;
  }
  int depth = 0;
  int paren_depth = 0;
  do {
    if (LexEof(lex) || LexLookingAt(lex, TOK(semicolon)) ||
        LexLookingAt(lex, TOK(lbrace)) || LexLookingAt(lex, TOK(rbrace))) {
      return false;
    }
    Token token = lex->current_token;
    if (token == TOK(lparen)) {
      paren_depth++;
    } else if (token == TOK(rparen)) {
      paren_depth--;
    } else if (paren_depth == 0) {
      if (token == TOK(less)) {
        depth++;
      } else {
        depth -= LexClosingAngleCount(token);
      }
    }
    LexNextToken(lex);
  } while (depth > 0);
  return depth == 0;
}

// True when the lexer is at `A::B` or `ns::A::B` (at least one `::`), and the
// name does not end on a dangling `::`.  Components may be template-ids.
// Used so a speculative qualified-name parse does not diagnose
// `Expected identifier after '::'`.
static bool LookingAtCompleteQualifiedName(Lex* lex) {
  if (!LexLookingAt(lex, TOK(identifier))) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(lex, &checkpoint);
  LexNextToken(lex);
  int parts = 1;
  bool dangling = !SkipTemplateArgumentTokens(lex);
  while (!dangling && LexLookingAt(lex, TOK(coloncolon))) {
    LexNextToken(lex);
    if (LexLookingAt(lex, TOK(operator))) {
      // `A::operator()` / `A::operator+`: an operator-function-id ends the
      // name.
      parts++;
      break;
    }
    if (!LexLookingAt(lex, TOK(identifier))) {
      dangling = true;
      break;
    }
    LexNextToken(lex);
    parts++;
    dangling = !SkipTemplateArgumentTokens(lex);
  }
  LexCheckpointRestore(lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return !dangling && parts >= 2;
}


// `&ns::Class::method` names one member of an overload set.  The non-template
// function is the one whose type can be a pointer-to-member without a
// template argument list (`&time_zone::next_transition`).
static StructMember* PointerToMemberOverload(StructMember* member) {
  StructMember* templ = NULL;
  for (StructMember* cand = member; cand != NULL; cand = cand->overload_next) {
    if (cand->symbol == NULL || cand->symbol->type == NULL || cand->is_static) {
      continue;
    }
    if (!cand->is_member_function && !TypeIsFunction(cand->symbol->type)) {
      return cand;
    }
    if (!cand->symbol->flags.is_template) {
      return cand;
    }
    if (templ == NULL) {
      templ = cand;
    }
  }
  return templ != NULL ? templ : member;
}

// `&S::Get<1>` names the specialization of a non-static member function
// template for those template arguments.  Returns NULL when no member template
// accepts them.
static StructMember* PointerToMemberTemplateSpecialization(StructMember* member,
                                                           Vector* args) {
  for (StructMember* cand = member; cand != NULL; cand = cand->overload_next) {
    if (cand->symbol == NULL || cand->symbol->type == NULL ||
        cand->is_static || !cand->symbol->flags.is_template ||
        !TypeIsFunction(cand->symbol->type)) {
      continue;
    }
    Vector* explicit_args = TemplateArgumentVectorCopy(args);
    Symbol* instantiated = TypeInstantiateFunctionTemplate(
        &compiler->syntax, cand->symbol, explicit_args);
    VectorDeleteWithContents(explicit_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    if (instantiated == NULL || instantiated == cand->symbol ||
        instantiated->flags.is_template || instantiated->type == NULL ||
        TypeContainsTemplateParameter(instantiated->type)) {
      continue;
    }
    StructMember* specialization = NewStructMember(instantiated);
    specialization->is_member_function = true;
    specialization->access = cand->access;
    return specialization;
  }
  return NULL;
}


// Tentatively parses &qualified-name as a pointer-to-member expression and
// restores the token on failure.
ASTNode* ParsePointerToMember(Syntax* syntax, TokenClass followers) {
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  if (!LookingAtCompleteQualifiedName(syntax->lex)) {
    LexCheckpointDestruct(&checkpoint);
    return NULL;
  }
  FullyQualifiedIdentifier name;
  FullyQualifiedIdentifierInit(&name);
  if (!SyntaxParseFullyQualifiedIdentifierWithTemplateIds(syntax, &name,
                                                          followers) ||
      !name.is_qualified || name.components.length < 2) {
    FullyQualifiedIdentifierDestruct(&name);
    LexCheckpointRestore(syntax->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
    return NULL;
  }
  Symbol* class_sym = SyntaxFindQualifiedPrefixSymbol(
      syntax, &name, name.components.length - 1);
  Struct* class_info = NULL;
  if (class_sym != NULL && class_sym->type != NULL &&
      TypeIsStructOrUnion(class_sym->type) &&
      class_sym->type->info.struct_info != NULL) {
    class_info = class_sym->type->info.struct_info;
  }
  String member_name;
  StringInit(&member_name, FullyQualifiedIdentifierLast(&name));
  Vector* member_args =
      name.template_arguments.length == name.components.length
          ? TemplateArgumentVectorCopy(VectorLast(&name.template_arguments))
          : NULL;
  FullyQualifiedIdentifierDestruct(&name);
  if (class_info == NULL) {
    StringDestruct(&member_name);
    if (member_args != NULL) {
      VectorDeleteWithContents(member_args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    LexCheckpointRestore(syntax->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
    return NULL;
  }
  StructMember* member = FindStructMember(class_info, &member_name);
  StringDestruct(&member_name);
  if (member_args != NULL) {
    member = PointerToMemberTemplateSpecialization(member, member_args);
    VectorDeleteWithContents(member_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  } else {
    member = PointerToMemberOverload(member);
  }
  if (member == NULL || member->symbol == NULL || member->symbol->type == NULL ||
      member->is_static) {
    LexCheckpointRestore(syntax->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
    return NULL;
  }
  // Forming a pointer-to-member (&Class::field) counts as a use of the data
  // member for -Wunused-private-field.
  member->symbol->flags.used = true;
  if (member->is_member_function &&
      FunctionHasExplicitObjectParameter(member->symbol->type)) {
    member->symbol->flags.address_taken = true;
    SourceLocation location = syntax->lex->current_token_location;
    ASTNode* function = NewIdentifierASTNode(member->symbol, location);
    LexCheckpointDestruct(&checkpoint);
    return NewUnaryASTNode(AST_OP(address), NULL, location, function);
  }
  TypeRecord* member_type = TypeMemberPointerPointeeFromMember(member);
  TypeRecord* mptr = NewMemberPointerTypeRecord(class_info, kQualPlain);
  TypeRecordChain(mptr, member_type);
  TypeRecordCalculateSize(mptr);
  SourceLocation location = syntax->lex->current_token_location;
  StructMemberASTNode* member_node =
      (StructMemberASTNode*)NewStructMemberASTNode(member, location);
  if (member->symbol->flags.name_independent_lookup_ambiguous) {
    member_node->base.flags |= kASTNameIndependentLookupAmbiguous;
  }
  ASTNode* result =
      NewUnaryASTNode(AST_OP(member_ptr), NULL, location, (ASTNode*)member_node);
  ASTNodeSetType(result, mptr);
  (void)followers;
  LexCheckpointDestruct(&checkpoint);
  return result;
}
// The operator a token introduces, or none if it introduces no binary operator
// here.  Inside a template-argument list a top-level '>' closes the list and
// '>>' closes two nested ones, so neither is an operator there; one that is
// genuinely wanted has to be parenthesized, which clears the flag (see
// SyntaxNeedTemplateClose).  '>>=' is never a binary operator, so the same
// handoff happens for it without a case of its own.
static BinaryPrecedence BinaryOperatorPrecedence(Syntax* syntax, Token token,
                                                 ASTOpcode* op) {
  switch (token) {
    case TOK(star):
      *op = AST_OP(mult);
      return kBinaryPrecedenceMultiplicative;
    case TOK(slash):
      *op = AST_OP(div);
      return kBinaryPrecedenceMultiplicative;
    case TOK(percent):
      *op = AST_OP(mod);
      return kBinaryPrecedenceMultiplicative;
    case TOK(plus):
      *op = AST_OP(plus);
      return kBinaryPrecedenceAdditive;
    case TOK(minus):
      *op = AST_OP(minus);
      return kBinaryPrecedenceAdditive;
    case TOK(lessless):
      *op = AST_OP(lshift);
      return kBinaryPrecedenceShift;
    case TOK(greatergreater):
      if (syntax->parsing_template_argument) {
        return kBinaryPrecedenceNone;
      }
      *op = AST_OP(rshift);
      return kBinaryPrecedenceShift;
    // C++20 three-way comparison binds tighter than the relational operators
    // and looser than the shift operators.
    case TOK(spaceship):
      *op = AST_OP(spaceship);
      return kBinaryPrecedenceCompare;
    case TOK(less):
      *op = AST_OP(less);
      return kBinaryPrecedenceRelational;
    case TOK(lesseq):
      *op = AST_OP(lesseq);
      return kBinaryPrecedenceRelational;
    case TOK(greater):
      if (syntax->parsing_template_argument) {
        return kBinaryPrecedenceNone;
      }
      *op = AST_OP(greater);
      return kBinaryPrecedenceRelational;
    case TOK(greatereq):
      *op = AST_OP(greatereq);
      return kBinaryPrecedenceRelational;
    case TOK(equalequal):
      *op = AST_OP(equal);
      return kBinaryPrecedenceEquality;
    case TOK(bangeq):
      *op = AST_OP(noteq);
      return kBinaryPrecedenceEquality;
    case TOK(amp):
      *op = AST_OP(and);
      return kBinaryPrecedenceAnd;
    case TOK(caret):
      *op = AST_OP(exor);
      return kBinaryPrecedenceExclusiveOr;
    case TOK(bar):
      *op = AST_OP(bitor);
      return kBinaryPrecedenceInclusiveOr;
    case TOK(ampamp):
      *op = AST_OP(logand);
      return kBinaryPrecedenceLogicalAnd;
    case TOK(barbar):
      *op = AST_OP(logor);
      return kBinaryPrecedenceLogicalOr;
    default:
      return kBinaryPrecedenceNone;
  }
}


// Parse an expression made of binary operators binding at least as tightly as
// `min_precedence`.  Every one of them is left associative, so the right
// operand takes only the operators that bind more tightly, and the recursion
// that collects them is bounded by the number of precedence levels rather than
// by the length or nesting of the expression.
// The operands of a requires-clause are primary expressions, so a leading
// `(` opens a parenthesized expression, never a cast: in
// `requires (Trait<F>::value) S(F f)` the constructor name `S` is not the
// operand of a cast to `Trait<F>::value`.
static ASTNode* ParseBinaryOperand(Syntax* syntax, TokenClass followers,
                                   bool constraint) {
  if (constraint && LexMatch(syntax->lex, TOK(lparen))) {
    syntax->found_open_paren = true;
    return ParsePostfixExpression(syntax, followers);
  }
  return ParseCastExpression(syntax, followers);
}


// Parses binary operators with precedence climbing, stopping before looser
// operators.
ASTNode* ParseBinaryExpressionOperands(Syntax* syntax,
                                              TokenClass followers,
                                              BinaryPrecedence min_precedence,
                                              bool constraint) {
  ASTNode* result = ParseBinaryOperand(syntax, followers, constraint);
  for (;;) {
    ASTOpcode op = 0;
    BinaryPrecedence precedence =
        BinaryOperatorPrecedence(syntax, syntax->lex->current_token, &op);
    if (precedence == kBinaryPrecedenceNone || precedence < min_precedence) {
      return result;
    }
    LexNextToken(syntax->lex);
    ASTNode* right = ParseBinaryExpressionOperands(
        syntax, followers, precedence + 1, constraint);
    result = NewBinaryASTNode(op, NULL, syntax->lex->current_token_location,
                              result, right);
  }
}


// Parses a complete non-assignment binary expression.
static ASTNode* ParseBinaryExpression(Syntax* syntax, TokenClass followers,
                                      BinaryPrecedence min_precedence) {
  return ParseBinaryExpressionOperands(syntax, followers, min_precedence,
                                       false);
}


// Parses the ternary conditional operator after its binary-expression
// condition.
ASTNode* ParseConditionalExpression(Syntax* syntax,
                                           TokenClass followers) {
  ASTNode* result =
      ParseBinaryExpression(syntax, followers, kBinaryPrecedenceLogicalOr);
  if (LexMatch(syntax->lex, TOK(question))) {
    ASTNode* left = SyntaxParseExpression(syntax, followers);
    ASTNode* right = NULL;
    if (LexMatch(syntax->lex, TOK(colon))) {
      right = ParseConditionalExpression(syntax, followers);
    } else {
      // `cond ? a :: b` is a common typo for `cond ? a : b`.  Consume `::` as
      // `:` when it is still in the token stream; otherwise parse a leftover
      // literal-like expression as the false arm so the `?:` tree stays
      // well-formed.  Identifiers are also statements, so they are left for
      // declaration/statement recovery rather than swallowed as an arm.
      SyntaxError(syntax, "Missing : in conditional expression");
      if (LexMatch(syntax->lex, TOK(coloncolon))) {
        right = ParseConditionalExpression(syntax, followers);
      } else {
        TokenClass classes = ClassifyToken(syntax->lex->current_token);
        if ((classes & TC(expr)) != 0 &&
            (classes & (TC(stmt) | TC(decl) | TC(type))) == 0) {
          right = ParseConditionalExpression(syntax, followers);
        }
      }
    }
    right =
        NewBinaryASTNode(AST_OP(colon), NULL,
                         syntax->lex->current_token_location, left, right);
    result =
        NewBinaryASTNode(AST_OP(question), NULL,
                         syntax->lex->current_token_location, result, right);
  }
  return result;
}
// Maps an assignment token to its assignment-expression AST opcode.
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


// Parses an assignment or co_yield expression, recursing for right-associative
// assignment.
ASTNode* ParseAssignmentExpression(Syntax* syntax,
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
      // In C++, the right-hand side of a simple assignment may be a
      // braced-init-list (`x = {}`, `x = {1, 2}`).  A braced-init-list is not
      // an expression, so parse it explicitly here; semantic analysis lowers it
      // to a temporary of the left-hand side's type.  Only `=` accepts it as a
      // complete right-hand side, so anything trailing the braces (e.g.
      // `x = {} + 1`) remains and is correctly rejected as a syntax error.
      ASTNode* right =
          (CompilerIsCXX() && tok == TOK(equal) &&
           LexMatch(syntax->lex, TOK(lbrace)))
              ? SyntaxParseBracedInitializer(syntax)
              : ParseAssignmentExpression(syntax, followers);
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
