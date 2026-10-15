//
//  expr_semantics_primary.c
//  c_compiler
//
//  Logical, throw, and coroutine expression semantics.
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

static void EnsureThrowCopyConstructor(ThrowASTNode* node);
static bool IsCXX23MoveEligibleThrowOperand(ASTNode* expression);

// Contract: Analyzes operands (with `CXXFoldIsSameVExpression`), may replace
// an unreachable arm with a bool constant, inserts contextual bool conversions,
// and sets logical result type. Mutates `node->left`/`node->right` in place.
void AnalyzeLogicalOperator(BinaryASTNode* node) {
  TypeRecord* bool_type = NewTypeRecordWithSize(kTypeBool, kQualPlain);

  node->left = CXXFoldIsSameVExpression(node->left);
  node->left = AnalyzeExpression(node->left);
  if (!ExpressionIsTemplateDependent(node->left)) {
    int64_t left_truth = 0;
    if (SemanticEvaluateRuntimeTruth(node->left, &left_truth)) {
      if (node->base.op == AST_OP(logand) && left_truth == 0) {
        ASTNode* discarded =
            NewIntConstantASTNode(0, bool_type, node->base.location);
        ASTNodeReplaceChild((ASTNode*)node, 1, discarded, true);
        ASTNodeSetType((ASTNode*)node, NewLogicalResultType());
        return;
      }
      if (node->base.op == AST_OP(logor) && left_truth != 0) {
        ASTNode* discarded =
            NewIntConstantASTNode(1, bool_type, node->base.location);
        ASTNodeReplaceChild((ASTNode*)node, 1, discarded, true);
        ASTNodeSetType((ASTNode*)node, NewLogicalResultType());
        return;
      }
    }
  }
  node->right = CXXFoldIsSameVExpression(node->right);
  node->right = AnalyzeExpression(node->right);
  if (ExpressionIsTemplateDependent(node->left) ||
      ExpressionIsTemplateDependent(node->right)) {
    ASTNodeSetType((ASTNode*)node, bool_type);
    return;
  }
  SemanticConvertType(node->left, bool_type, kConvertContextualBool);
  SemanticConvertType(node->right, bool_type, kConvertContextualBool);
  ASTNodeSetType((ASTNode*)node, node->left->type);
  SemanticCheckScalarType(node->left);
  SemanticCheckScalarType(node->right);

  ASTNodeSetType((ASTNode*)node, NewLogicalResultType());
}

// Contract: May synthesize defaulted copy ctor body on thrown class type; mutates type metadata only.
static void EnsureThrowCopyConstructor(ThrowASTNode* node) {
  if (node == NULL || node->expr == NULL ||
      !TypeIsStructOrUnion(node->expr->type) ||
      node->expr->value_category == kValueCategoryPrvalue) {
    return;
  }
  StructMember* constructor = FindConvertingConstructorCandidate(
      node->expr->type, node->expr, false, true);
  Symbol* symbol = constructor != NULL ? constructor->symbol : NULL;
  if (symbol == NULL || symbol->type == NULL ||
      !symbol->type->info.function.is_defaulted ||
      symbol->type->info.function.body != NULL) {
    return;
  }
  TypeParser parser;
  TypeParserInit(&parser, compiler->syntax.lex, &compiler->syntax,
                 STO(implicit), compiler->syntax.context);
  SynthesizeDefaultedMemberFunctionBody(&parser, symbol);
  TypeParserDestruct(&parser);
}

// Contract: Read-only predicate for C++23 throw-as-xvalue on local/parameter objects.
static bool IsCXX23MoveEligibleThrowOperand(ASTNode* expression) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX23) || expression == NULL ||
      expression->op != AST_OP(identifier)) {
    return false;
  }
  Symbol* sym = ((IdentifierASTNode*)expression)->symbol;
  if (sym == NULL || (!sym->flags.is_local && !sym->flags.is_argument) ||
      sym->flags.is_temp || StorageIs(sym->storage, STO(static))) {
    return false;
  }
  TypeRecord* object_type = sym->type;
  if (object_type == NULL) {
    return false;
  }
  if (TypeIsReference(object_type)) {
    if (object_type->declarator != kDeclRValueReference) {
      return false;
    }
    object_type = object_type->next;
  }
  return object_type != NULL && !TypeIsConst(object_type) &&
         !TypeIsVolatile(object_type);
}

// Contract: Analyzes operand when present; may set xvalue on C++23 move-eligible
// locals and synthesize defaulted copy ctor body on the thrown type. Result void.
void AnalyzeThrowExpression(ThrowASTNode* node) {
  if (!CompilerExceptionsEnabled()) {
    SemanticError((ASTNode*)node,
                  "cannot use 'throw' with exception handling disabled "
                  "(-fno-exceptions)");
  }
  if (node->expr == NULL) {
    if (!SemanticInCatchHandler()) {
      SemanticError((ASTNode*)node,
                    "throw without operand is only valid in a catch handler");
    }
  } else {
    node->expr = AnalyzeExpression(node->expr);
    if (IsCXX23MoveEligibleThrowOperand(node->expr)) {
      node->expr->value_category = kValueCategoryXvalue;
    }
    if (TypeIsVoid(node->expr->type) || TypeIsFunction(node->expr->type)) {
      SemanticError(node->expr, "Cannot throw expression of this type");
    }
    EnsureThrowCopyConstructor(node);
  }
  // A throw in a 'noexcept' function is well-formed: per [except.spec], if the
  // exception escapes the function at runtime std::terminate is called.  That
  // runtime guard is inserted during codegen, so nothing is enforced here.
  ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
}

// Contract: Sets `kASTNeedAddress` on `node` and `address_taken` on the
// identifier symbol; may materialize a template member definition for functions.
void SetNeedAddress(ASTNode* node) {
  node->flags |= kASTNeedAddress;
  if (node->op == AST_OP(identifier)) {
    IdentifierASTNode* idnode = (IdentifierASTNode*)node;
    idnode->symbol->flags.address_taken = true;
    if (CompilerIsCXX() && TypeIsFunction(node->type)) {
      TypeEnsureTemplateMemberFunctionDefinition(&compiler->syntax,
                                                 idnode->symbol);
    }
  }
}
