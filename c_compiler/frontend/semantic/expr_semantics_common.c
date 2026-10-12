//
//  expr_semantics_common.c
//  c_compiler
//
//  Shared value-category and semantic-analysis helpers.
//

#include "expr_semantics.h"
#include "expr_semantics_internal.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "concepts.h"
#include "contracts.h"
#include "constexpr.h"
#include "expr_evaluator.h"
#include "init_semantics.h"
#include "statement_semantics.h"
#include "compiler.h"
#include "errors.h"
#include "symbol_table.h"
#include "typo_correction.h"
#include "rtti.h"
#include "reflection.h"
#include "reflection_meta_synthesis.h"
#include "reflection_meta_traits.h"
#include "reflection_semantics.h"
#include "type_traits_semantics.h"
#include "type_compare.h"
#include "member_pointer.h"
#include "statement_parser.h"
#include "type_parse.h"
#include "type_class_internal.h"
#include "type_inheritance.h"
#include "type_special_member.h"
#include "type_template.h"

static bool ASTNodeIsXValue(ASTNode* node);

// Contract: Read-only predicate for `ASTNodeAny`; does not mutate `node`.
bool ExpressionRequiresASTConstexpr(ASTNode* node, void* data) {
  (void)data;
  return node != NULL &&
         (((node->flags & kASTRequiresASTConstexpr) != 0) ||
          (node->op == AST_OP(identifier) &&
           ((IdentifierASTNode*)node)->symbol != NULL &&
           ((IdentifierASTNode*)node)->symbol->requires_ast_constexpr));
}

// This is the semantic analyzer for expressions.  It propagates type
// information from the leaves of the AST (Abstract Syntax Tree) up
// to the root of the tree.  The types of the leaf nodes are known
// before we do the analysis because they come from constants or
// symbols with known types.  Various operators modify the types
// as they propagate upwards in the tree.
//
// This also performs type conversion using the rules of the language.
// It also is responsible for checking that the semantic rules of the
// language are being followed.

// Contract: Read-only value-category flag test.
bool ASTNodeIsLValue(ASTNode* node) {
  return node != NULL && node->value_category == kValueCategoryLvalue;
}

// Contract: Read-only value-category flag test.
static bool ASTNodeIsXValue(ASTNode* node) {
  return node != NULL && node->value_category == kValueCategoryXvalue;
}

// Contract: Read-only; true for lvalue or xvalue.
bool ASTNodeIsGLValue(ASTNode* node) {
  return ASTNodeIsLValue(node) || ASTNodeIsXValue(node);
}

// A user-defined conversion (via a converting constructor) involves a standard
// conversion of the argument to the constructor's parameter.  While ranking
// that inner standard conversion we must not recursively consider yet another
// user-defined conversion (at most one is permitted), so this guard suppresses
// the user-defined-conversion search in OverloadConversionRank.
bool g_suppress_user_defined_conversion_rank = false;

// Contract: Saves and clears the global UDC-ranking suppress flag; pair with
// `SemanticResumeUserDefinedConversionRank` around nested conversion ranking.
bool SemanticSuspendUserDefinedConversionRank(void) {
  bool saved = g_suppress_user_defined_conversion_rank;
  g_suppress_user_defined_conversion_rank = false;
  return saved;
}

// Contract: Restores `g_suppress_user_defined_conversion_rank` from a prior
// `SemanticSuspendUserDefinedConversionRank` save value.
void SemanticResumeUserDefinedConversionRank(bool saved) {
  g_suppress_user_defined_conversion_rank = saved;
}
