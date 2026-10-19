//
//  expr_parser_fold.c
//  c_compiler
//
//  Fold expressions and pack indexing.
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


typedef struct {
  Syntax* syntax;
  bool concept_template_pack;
  bool other_pack;
} CXXFoldPackKinds;


static bool CXXTemplateParameterIndexIsPack(Syntax* syntax, int index);
static TemplateParameter* CXXTemplateParameterForIndex(Syntax* syntax, int index);
static void ClassifyCXXFoldPackIndex(CXXFoldPackKinds* kinds, int index);
static void ClassifyCXXFoldTemplateArgument(CXXFoldPackKinds* kinds, TemplateArgument* argument);
static void ClassifyCXXFoldPacks(ASTNode* node, void* data, int child_id, VisitorMode mode);
static bool CXXFoldMixesConceptAndOtherPacks(Syntax* syntax, ASTNode* pattern);
static void FindCXXParameterPackExpression(ASTNode* node, void* data, int child_id, VisitorMode mode);
static bool FoldOperatorOpcode(Token token, ASTOpcode* op);
static bool ParseFoldOperator(Syntax* syntax, ASTOpcode* op);
static bool FoldExpressionContainsPack(Syntax* syntax, ASTNode* node);
static ASTNode* ParseFoldPackExpression(Syntax* syntax, TokenClass followers);
static bool FoldLessStartsTemplateArgument(Syntax* syntax);
static bool TokenIsFoldOperator(Token token);
static bool FoldExpressionHasTopLevelEllipsis(Syntax* syntax);
static ASTNode* NewInvalidFoldExpression(Syntax* syntax, LexCheckpoint* checkpoint, const char* message, SourceLocation location, TokenClass followers);


// Reports whether a template-parameter index names a pack in the active
// template context.
static bool CXXTemplateParameterIndexIsPack(Syntax* syntax, int index) {
  return CurrentTemplateParameterIsPack(syntax, index);
}


// Finds the active template parameter with the given index, including
// parameters owned by the current function.
static TemplateParameter* CXXTemplateParameterForIndex(Syntax* syntax,
                                                       int index) {
  const Vector* parameters =
      syntax != NULL ? syntax->current_template_parameters : NULL;
  for (size_t i = 0; parameters != NULL && i < parameters->length; i++) {
    TemplateParameter* parameter = parameters->value.p[i];
    if (parameter != NULL && parameter->index == index) {
      return parameter;
    }
  }
  TypeRecord* function = compiler->current_function;
  if (function != NULL && TypeIsFunction(function)) {
    parameters =
        &FunctionExtraOf(&function->info.function)->template_parameters;
    for (size_t i = 0; i < parameters->length; i++) {
      TemplateParameter* parameter = parameters->value.p[i];
      if (parameter != NULL && parameter->index == index) {
        return parameter;
      }
    }
  }
  return NULL;
}


// Classifies one pack index as a concept-template pack or an ordinary pack for
// fold diagnostics.
static void ClassifyCXXFoldPackIndex(CXXFoldPackKinds* kinds, int index) {
  TemplateParameter* parameter =
      CXXTemplateParameterForIndex(kinds->syntax, index);
  if (parameter == NULL || !parameter->is_parameter_pack) {
    return;
  }
  if (parameter->kind == kTemplateParameterTemplate &&
      parameter->template_template_kind ==
          kTemplateTemplateParameterConcept) {
    kinds->concept_template_pack = true;
  } else {
    kinds->other_pack = true;
  }
}


// Recursively classifies every parameter pack referenced by a template
// argument.
static void ClassifyCXXFoldTemplateArgument(CXXFoldPackKinds* kinds,
                                            TemplateArgument* argument) {
  if (argument == NULL) {
    return;
  }
  ClassifyCXXFoldPackIndex(kinds, argument->template_parameter_index);
  if (TemplateArgumentExtraOf(argument)->template_symbol != NULL &&
      TemplateArgumentExtraOf(argument)
          ->template_symbol->flags.is_parameter_pack) {
    ClassifyCXXFoldPackIndex(kinds,
                             TemplateArgumentExtraOf(argument)
                                 ->template_symbol->template_parameter_index);
  }
  for (TypeRecord* type = argument->type; type != NULL; type = type->next) {
    ClassifyCXXFoldPackIndex(kinds, type->template_parameter_index);
  }
  for (size_t i = 0;
       argument->pack_arguments != NULL &&
       i < argument->pack_arguments->length;
       i++) {
    ClassifyCXXFoldTemplateArgument(
        kinds, argument->pack_arguments->value.p[i]);
  }
}


// AST visitor that collects the kinds of packs referenced by a prospective
// fold pattern.
static void ClassifyCXXFoldPacks(ASTNode* node, void* data, int child_id,
                                 VisitorMode mode) {
  (void)child_id;
  if (node == NULL || mode != kVisitPreChildren) {
    return;
  }
  CXXFoldPackKinds* kinds = data;
  Vector* arguments = NULL;
  switch (ASTNodeGetShape(node)) {
    case kASTShapeIdentifier: {
      IdentifierASTNode* id = (IdentifierASTNode*)node;
      if (id->symbol != NULL && id->symbol->flags.is_parameter_pack) {
        ClassifyCXXFoldPackIndex(kinds,
                                 id->symbol->template_parameter_index);
      }
      arguments = id->template_arguments;
      break;
    }
    case kASTShapeStructMember:
      arguments = ((StructMemberASTNode*)node)->template_arguments;
      break;
    case kASTShapeConstant:
      arguments = ((ConstantASTNode*)node)->template_arguments;
      break;
    default:
      break;
  }
  for (size_t i = 0; arguments != NULL && i < arguments->length; i++) {
    ClassifyCXXFoldTemplateArgument(kinds, arguments->value.p[i]);
  }
}


// Detects the unsupported case where one fold pattern mixes a concept-template
// pack with another pack kind.
static bool CXXFoldMixesConceptAndOtherPacks(Syntax* syntax,
                                             ASTNode* pattern) {
  CXXFoldPackKinds kinds = {.syntax = syntax};
  ASTNodeVisit(pattern, ClassifyCXXFoldPacks, 0, &kinds);
  return kinds.concept_template_pack && kinds.other_pack;
}


// Returns whether a template argument, its packed arguments, or its dependent
// expression references a parameter pack.
bool CXXTemplateArgumentReferencesParameterPack(
    CXXPackExpressionSearch* search, TemplateArgument* argument) {
  if (argument == NULL) {
    return false;
  }
  if (argument->references_parameter_pack) {
    return true;
  }
  if (argument->template_parameter_index >= 0 &&
      CXXTemplateParameterIndexIsPack(
          search->syntax, argument->template_parameter_index)) {
    return true;
  }
  for (size_t i = 0;
       argument->pack_arguments != NULL &&
       i < argument->pack_arguments->length; i++) {
    if (CXXTemplateArgumentReferencesParameterPack(
            search, argument->pack_arguments->value.p[i])) {
      return true;
    }
  }
  if (argument->dependent_expr != NULL) {
    ASTNodeVisit(argument->dependent_expr, FindCXXParameterPackExpression, 0,
                 search);
  }
  return search->found;
}


// AST visitor that marks a search as found when a node, type, or template
// argument refers to a parameter pack.
static void FindCXXParameterPackExpression(ASTNode* node, void* data,
                                           int child_id, VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL) {
    return;
  }
  if ((node->flags & kASTReferencesParameterPack) != 0) {
    ((CXXPackExpressionSearch*)data)->found = true;
    return;
  }
  if ((node->op == AST_OP(sizeof) || node->op == AST_OP(alignof)) &&
      TypeContainsParameterPack(((CXXPackExpressionSearch*)data)->syntax,
                                ((SizeofASTNode*)node)->type_operand)) {
    ((CXXPackExpressionSearch*)data)->found = true;
    return;
  }
  // `static_cast<T (*)()>(nullptr)...` names the pack in the cast type, not
  // in a subexpression.
  if (node->type != NULL &&
      TypeContainsParameterPack(((CXXPackExpressionSearch*)data)->syntax,
                                node->type)) {
    ((CXXPackExpressionSearch*)data)->found = true;
    return;
  }
  Vector* template_arguments = NULL;
  ASTNodeShape shape = ASTNodeGetShape(node);
  if (shape == kASTShapeCast) {
    CastASTNode* cast = (CastASTNode*)node;
    if (TypeContainsParameterPack(((CXXPackExpressionSearch*)data)->syntax,
                                  cast->cast_type)) {
      ((CXXPackExpressionSearch*)data)->found = true;
    }
    return;
  }
  if (shape == kASTShapeIdentifier) {
    IdentifierASTNode* id = (IdentifierASTNode*)node;
    if (id->symbol != NULL && id->symbol->flags.is_parameter_pack) {
      ((CXXPackExpressionSearch*)data)->found = true;
      return;
    }
    if (id->symbol != NULL &&
        TypeContainsParameterPack(((CXXPackExpressionSearch*)data)->syntax,
                                  id->symbol->type)) {
      ((CXXPackExpressionSearch*)data)->found = true;
      return;
    }
    template_arguments = id->template_arguments;
  } else if (shape == kASTShapeStructMember) {
    template_arguments =
        ((StructMemberASTNode*)node)->template_arguments;
  } else if (shape == kASTShapeConstant) {
    template_arguments = ((ConstantASTNode*)node)->template_arguments;
  } else {
    return;
  }
  CXXPackExpressionSearch* search = data;
  for (size_t i = 0;
       template_arguments != NULL && i < template_arguments->length; i++) {
    if (CXXTemplateArgumentReferencesParameterPack(
            search, template_arguments->value.p[i])) {
      search->found = true;
      return;
    }
  }
}


// Scans an expression and reports whether it contains any parameter-pack
// reference.
bool CXXExpressionContainsParameterPack(Syntax* syntax, ASTNode* node) {
  CXXPackExpressionSearch search = {.syntax = syntax, .found = false};
  ASTNodeVisit(node, FindCXXParameterPackExpression, 0, &search);
  return search.found;
}


// Consumes a trailing ellipsis, validates that its operand contains a pack,
// and marks the operand as a pack expansion.
void MarkCXXPackExpansionIfPresent(Syntax* syntax, ASTNode* actual) {
  if (!CompilerIsCXX() || !LexMatch(syntax->lex, TOK(ellipsis))) {
    return;
  }
  if (!CXXExpressionContainsParameterPack(syntax, actual)) {
    SyntaxError(syntax, "pack expansion requires a function parameter pack");
  }
  actual->flags |= kASTPackExpansion;
}


/* Map exactly the binary operators permitted as fold-operators by
 * [expr.prim.fold]. Keep this as the single source of truth for both parsing
 * and the top-level-ellipsis lookahead. In particular, <=> is not a permitted
 * fold-operator. */
static bool FoldOperatorOpcode(Token token, ASTOpcode* op) {
  ASTOpcode result = AST_OP(bad);
  switch (token) {
    case TOK(star): result = AST_OP(mult); break;
    case TOK(slash): result = AST_OP(div); break;
    case TOK(percent): result = AST_OP(mod); break;
    case TOK(plus): result = AST_OP(plus); break;
    case TOK(minus): result = AST_OP(minus); break;
    case TOK(lessless): result = AST_OP(lshift); break;
    case TOK(greatergreater): result = AST_OP(rshift); break;
    case TOK(amp): result = AST_OP(and); break;
    case TOK(caret): result = AST_OP(exor); break;
    case TOK(bar): result = AST_OP(bitor); break;
    case TOK(ampamp): result = AST_OP(logand); break;
    case TOK(barbar): result = AST_OP(logor); break;
    case TOK(equalequal): result = AST_OP(equal); break;
    case TOK(bangeq): result = AST_OP(noteq); break;
    case TOK(less): result = AST_OP(less); break;
    case TOK(greater): result = AST_OP(greater); break;
    case TOK(lesseq): result = AST_OP(lesseq); break;
    case TOK(greatereq): result = AST_OP(greatereq); break;
    case TOK(equal): result = AST_OP(assign); break;
    case TOK(pluseq): result = AST_OP(pluseq); break;
    case TOK(minuseq): result = AST_OP(minuseq); break;
    case TOK(stareq): result = AST_OP(multeq); break;
    case TOK(slasheq): result = AST_OP(diveq); break;
    case TOK(percenteq): result = AST_OP(percenteq); break;
    case TOK(lesslesseq): result = AST_OP(lshifteq); break;
    case TOK(greatergreatereq): result = AST_OP(rshifteq); break;
    case TOK(ampeq): result = AST_OP(andeq); break;
    case TOK(bareq): result = AST_OP(oreq); break;
    case TOK(careteq): result = AST_OP(exoreq); break;
    case TOK(comma): result = AST_OP(comma); break;
    case TOK(dotstar): result = AST_OP(dotstar); break;
    case TOK(arrowstar): result = AST_OP(arrowstar); break;
    default: return false;
  }
  if (op != NULL) {
    *op = result;
  }
  return true;
}


// Consumes a token when it is a permitted fold operator and returns the
// corresponding AST opcode.
static bool ParseFoldOperator(Syntax* syntax, ASTOpcode* op) {
  if (!FoldOperatorOpcode(syntax->lex->current_token, op)) {
    return false;
  }
  LexNextToken(syntax->lex);
  return true;
}


/* davecc parses .* and ->* in its postfix-expression loop. Do not let that
 * loop consume the first operator of `(pack .* ... [.* init])` or
 * `(pack ->* ... [->* init])`; it belongs to the surrounding fold grammar. */
bool MemberPointerOperatorPrecedesFoldEllipsis(Syntax* syntax) {
  if (!LexLookingAt(syntax->lex, TOK(dotstar)) &&
      !LexLookingAt(syntax->lex, TOK(arrowstar))) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  LexNextToken(syntax->lex);
  bool result = LexLookingAt(syntax->lex, TOK(ellipsis));
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return result;
}


// Checks that a parsed fold operand contains the parameter pack required by
// the fold grammar.
static bool FoldExpressionContainsPack(Syntax* syntax, ASTNode* node) {
  // A fold pattern can name its pack through a template argument rather than
  // through an ordinary identifier child, as in
  // `(is_convertible_v<Ts, U> && ...)`.  Use the complete expression-pack
  // search so those template arguments participate in fold parsing too.
  return CXXExpressionContainsParameterPack(syntax, node);
}


// Parses one fold operand as an assignment-expression and records any trailing
// pack expansion.
static ASTNode* ParseFoldPackExpression(Syntax* syntax, TokenClass followers) {
  ASTNode* expr = ParseCastExpression(syntax, followers);
  if (!FoldExpressionContainsPack(syntax, expr)) {
    ASTNodeDelete(expr);
    return NULL;
  }
  return expr;
}


// A parenthesized expression may contain a template argument pack expansion,
// as in `Trait<T, Args&&...>::value`.  That ellipsis is not a fold-expression
// ellipsis.  At a `<`, look ahead for a balanced template close before the
// surrounding `)`; a genuine `<` fold operator has no matching `>`.
static bool FoldLessStartsTemplateArgument(Syntax* syntax) {
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  int angle_depth = 0;
  int paren_depth = 0;
  int square_depth = 0;
  int brace_depth = 0;
  bool matched = false;
  while (!LexEof(syntax->lex)) {
    Token token = syntax->lex->current_token;
    if (token == TOK(rparen) && paren_depth == 0 &&
        square_depth == 0 && brace_depth == 0) {
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
    } else if (paren_depth == 0 && square_depth == 0 && brace_depth == 0) {
      if (token == TOK(less)) {
        angle_depth++;
      } else {
        angle_depth -= LexClosingAngleCount(token);
      }
    }
    LexNextToken(syntax->lex);
    if (angle_depth <= 0) {
      matched = true;
      break;
    }
  }
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return matched;
}


// Reports whether a token is one of the binary operators permitted in a fold
// expression.
static bool TokenIsFoldOperator(Token token) {
  return FoldOperatorOpcode(token, NULL);
}


// Looks ahead through the current parenthesized expression for an ellipsis at
// the fold-expression level.
static bool FoldExpressionHasTopLevelEllipsis(Syntax* syntax) {
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  bool found = false;
  int paren_depth = 0;
  int square_depth = 0;
  int brace_depth = 0;
  int angle_depth = 0;
  Token previous_top_level = TOK(bad);
  bool at_start = true;

  while (!LexEof(syntax->lex)) {
    Token token = syntax->lex->current_token;
    if (token == TOK(rparen) && paren_depth == 0 &&
        square_depth == 0 && brace_depth == 0) {
      break;
    }
    if (token == TOK(ellipsis) && paren_depth == 0 &&
        square_depth == 0 && brace_depth == 0 && angle_depth == 0) {
      if (at_start || TokenIsFoldOperator(previous_top_level)) {
        found = true;
        break;
      }
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
    } else if (token == TOK(less) && angle_depth == 0 &&
               paren_depth == 0 && square_depth == 0 && brace_depth == 0 &&
        FoldLessStartsTemplateArgument(syntax)) {
      angle_depth++;
    } else if (token == TOK(less) && angle_depth > 0 &&
               paren_depth == 0 && square_depth == 0 && brace_depth == 0) {
      angle_depth++;
    } else if (LexClosingAngleCount(token) != 0 && angle_depth > 0 &&
               paren_depth == 0 && square_depth == 0 && brace_depth == 0) {
      angle_depth -= LexClosingAngleCount(token);
      if (angle_depth < 0) {
        angle_depth = 0;
      }
    }
    if (paren_depth == 0 && square_depth == 0 && brace_depth == 0 &&
        angle_depth == 0) {
      previous_top_level = token;
      at_start = false;
    }
    LexNextToken(syntax->lex);
  }

  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return found;
}


// Diagnoses an invalid fold, recovers through the closing parenthesis, and
// returns an unknown integer placeholder.
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


// A fold-expression inside the parentheses already consumed by the primary
// parser: `(pack op ...)` , `(... op pack)`, or `(pack op ... op init)`.
// Returns NULL when the parentheses do not contain a fold, restoring the
// token so the caller can parse an ordinary expression.
ASTNode* TryParseCXXFoldExpression(Syntax* syntax,
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
    bool seed_contains_pack = FoldExpressionContainsPack(syntax, seed);
    if (!seed_contains_pack && seed != NULL &&
        ParseFoldOperator(syntax, &op) &&
        LexMatch(syntax->lex, TOK(ellipsis))) {
      // A variable-template specialization can carry the pack only in its
      // explicit template arguments, while the parsed expression has already
      // folded to a constant node.  In `(Trait<Ts, U> && ...)`, the immediate
      // ')' unambiguously identifies a unary right fold even if that constant
      // node no longer exposes the pack to the AST visitor.
      if (LexMatch(syntax->lex, TOK(rparen))) {
        pack = seed;
        seed = NULL;
        pack_on_left = true;
      } else if (!ParseFoldOperator(syntax, &second_op)) {
        ASTNode* invalid =
            NewInvalidFoldExpression(syntax, &checkpoint,
                                     "fold expression requires a second operator",
                                     location, followers);
        LexCheckpointDestruct(&checkpoint);
        return invalid;
      } else if (second_op != op) {
        ASTNode* invalid =
            NewInvalidFoldExpression(syntax, &checkpoint,
                                     "fold expression operators must match",
                                     location, followers);
        LexCheckpointDestruct(&checkpoint);
        return invalid;
      } else if ((pack = ParseFoldPackExpression(
                      syntax, followers | TC(closebra))) == NULL) {
        ASTNode* invalid =
            NewInvalidFoldExpression(syntax, &checkpoint,
                                     "fold expression requires a parameter pack",
                                     location, followers);
        LexCheckpointDestruct(&checkpoint);
        return invalid;
      } else if (!LexMatch(syntax->lex, TOK(rparen))) {
        ASTNode* invalid =
            NewInvalidFoldExpression(syntax, &checkpoint,
                                     "fold expression syntax error",
                                     location, followers);
        LexCheckpointDestruct(&checkpoint);
        return invalid;
      } else {
        pack_on_left = false;
      }
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
        if (seed == NULL || FoldExpressionContainsPack(syntax, seed)) {
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

  if (CompilerCXXAtLeast(kLanguageStandardCXX26) &&
      CXXFoldMixesConceptAndOtherPacks(syntax, pack)) {
    SyntaxError(syntax,
                "fold expression cannot expand a concept template parameter "
                "pack together with another parameter pack");
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


// Recognizes a C++26 pack-index suffix without consuming an ordinary array
// subscript.
bool LookingAtCXXPackIndexSuffix(Syntax* syntax) {
  if (!CompilerIsCXX() ||
      !CompilerCXXAtLeast(kLanguageStandardCXX26) ||
      !LexLookingAt(syntax->lex, TOK(ellipsis))) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  LexNextToken(syntax->lex);
  bool result = LexLookingAt(syntax->lex, TOK(lsquare));
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return result;
}


// Parses a pack index, requires a constant nonnegative index, and builds the
// pack-index AST node.
ASTNode* ParseCXXPackIndexExpression(Syntax* syntax, ASTNode* pack,
                                            TokenClass followers) {
  SourceLocation location = syntax->lex->current_token_location;
  LexMatch(syntax->lex, TOK(ellipsis));
  LexMatch(syntax->lex, TOK(lsquare));
  ASTNode* index =
      SyntaxParseSingleExpression(syntax, followers | TC(closebra));
  SyntaxNeedBracket(syntax, TOK(rsquare), followers);

  bool valid_pack = pack != NULL && pack->op == AST_OP(identifier) &&
                    (pack->flags & kASTParenthesized) == 0;
  if (valid_pack) {
    IdentifierASTNode* id = (IdentifierASTNode*)pack;
    valid_pack = id->symbol != NULL && id->symbol->flags.is_parameter_pack;
    if (valid_pack && id->symbol->flags.is_template_template_parameter) {
      if (!CompilerCXXAtLeast(kLanguageStandardCXX29)) {
        SyntaxError(syntax,
                    "Template-name pack indexing requires C++29");
      }
      if (LexLookingAt(syntax->lex, TOK(less))) {
        id->template_arguments =
            SyntaxParseTemplateArgumentList(syntax, followers);
      }
    }
  }
  if (!valid_pack) {
    SyntaxError(syntax,
                "pack indexing requires an unexpanded parameter pack name");
  }
  return NewBinaryASTNode(AST_OP(pack_index), NULL, location, pack, index);
}
