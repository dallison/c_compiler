//
//  statement_semantics.c
//  c_compiler
//
//  Created by David Allison on 11/7/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "statement_semantics.h"
#include <assert.h>
#include <stdlib.h>
#include "compiler.h"
#include "expr_evaluator.h"
#include "expr_semantics.h"

static void AnalyzeExpressionStatement(ExpressionStatementASTNode* node) {
  AnalyzeExpression(node->expr);
}

static void AnalyzeIfStatement(IfStatementASTNode* node) {
  AnalyzeExpression(node->cond);
  SemanticCheckScalarType(node->cond);
  AnalyzeStatement(node->if_part);
  AnalyzeStatement(node->else_part);
  SemanticCheckScalarType(node->cond);
}

static void AnalyzeWhileStatement(CombinedStatementASTNode* node) {
  AnalyzeExpression(node->cond);
  SemanticCheckScalarType(node->cond);
  AnalyzeStatement(node->stmt);
  SemanticCheckScalarType(node->cond);
}

static void AnalyzeDoStatement(CombinedStatementASTNode* node) {
  AnalyzeExpression(node->cond);
  SemanticCheckScalarType(node->cond);
  AnalyzeStatement(node->stmt);
  SemanticCheckScalarType(node->cond);
}

// Function to compare case values for the qsort function.  This
// is passed pointers to the elements of the array (which are
// void* pointers in our case because it's a Vector).
static int CompareCaseValue(const void* case1, const void* case2) {
  CaseLabelASTNode* node1 = *(CaseLabelASTNode**)case1;
  CaseLabelASTNode* node2 = *(CaseLabelASTNode**)case2;

  // NOTE that we don't care what the actual value returned is, as long
  // as its sign is correct.
  return (int)(node1->value - node2->value);
}

static void AnalyzeSwitchStatement(SwitchStatementASTNode* node) {
  AnalyzeExpression(node->expr);
  AnalyzeStatement(node->stmt);
  SemanticCheckScalarType(node->expr);
  if (!TypeIsIntegral(node->expr->type)) {
    SemanticError(node->expr, "Switch statements need an integer type");
    return;
  }

  // The case statement must be a compound statement.
  if (node->stmt->op != AST_OP(compound)) {
    return;
  }

  // Check for duplicate case labels and defaults in statement.
  // Also fill in the 'cases' vector in the switch statement AST node
  // and the defualt_node if present.
  CompoundStatementASTNode* body = (CompoundStatementASTNode*)node->stmt;
  size_t num_statments = body->statements->length;
  for (size_t i = 0; i < num_statments; i++) {
    ASTNode* stmt = (ASTNode*)body->statements->value.p[i];

    if (stmt->op == AST_OP(case)) {
      CaseLabelASTNode* case_node = (CaseLabelASTNode*)stmt;

      if (case_node->expr == NULL) {
        // This is a default node.
        if (node->default_node != NULL) {
          SemanticError(stmt, "Duplicate default in switch statement");
        }
        node->default_node = case_node;
        continue;
      }

      // Convert the case expression to the type of the switch controlling
      // expression.
      SemanticConvertType(case_node->expr, node->expr->type);

      // Case labels need to be constant integer expressions.
      if (!EvaluateIntegerExpression(case_node->expr, &case_node->value)) {
        SemanticError(node->expr,
                      "Case labels must be constant integral expressions");
      }

      // Calculate min, max and density.
      if (case_node->value < node->min_case_value) {
        node->min_case_value = case_node->value;
      }
      if (case_node->value > node->max_case_value) {
        node->max_case_value = case_node->value;
      }
      VectorAppend(&node->cases, stmt);
    }
  }

  // Get an idea of the case density.
  // Density is mass/volume.  Let's say that the number of cases is the
  // masss and the distance between the min and max values is the volume.
  float mass = (node->cases.length + (node->default_node != NULL ? 1 : 0));
  float volume = node->max_case_value - node->min_case_value;
  if (volume != 0) {
    node->density = mass / volume;
  }

  // Now we sort the cases into ascending order.  We do this because it's better
  // for code generation.  We can do a branch table or a binary search if the
  // values are sorted.
  qsort(node->cases.value.p, node->cases.length, sizeof(void*), CompareCaseValue);

  // Check the case labels for duplicates. Since they are sorted we only need
  // to check for two adjacent values being the same.  This is faster than doing
  // an n^2 search for each value;
  size_t num_cases = node->cases.length;
  for (size_t i = 0; i < num_cases - 1; i++) {
    int64_t case_value1 = ((CaseLabelASTNode*)(node->cases.value.p[i]))->value;
    int64_t case_value2 =
        ((CaseLabelASTNode*)(node->cases.value.p[i + 1]))->value;
    if (case_value1 == case_value2) {
      const char* filename;
      int lineno;
      int start, end;
      DecodeSourceLocation(
          ((CaseLabelASTNode*)node->cases.value.p[i])->expr->location, &filename,
          &lineno, &start, &end);
      SemanticError(((CaseLabelASTNode*)node->cases.value.p[i + 1])->expr,
                    "Duplicate case value %d; previous is at %s:%d",
                    case_value2, filename, lineno);
    }
  }
}

static void AnalyzeForStatement(ForStatementASTNode* node) {
  if (node->c1 != NULL) {
    if (node->c1->op == AST_OP(decl_list)) {
      // First "expression" might be a list of variable declaration statements.
      AnalyzeStatement(node->c1);
      DeclarationListASTNode* decls = (DeclarationListASTNode*)node->c1;
      for (size_t i = 0; i < decls->declarations->length; i++) {
        VariableDeclarationASTNode* vardecl =
            (VariableDeclarationASTNode*)decls->declarations->value.p[i];
        if (!StorageIs(vardecl->symbol->storage, STO(auto)) &&
            !StorageIs(vardecl->symbol->storage, STO(register)) &&
            vardecl->symbol->storage != STO(implicit)) {
          SemanticError((ASTNode*)vardecl,
                        "Only auto or register variables "
                        "allowed in a for statement "
                        "declaration");
        }
      }
    } else {
      AnalyzeExpression(node->c1);
    }
  }

  AnalyzeExpression(node->c2);
  if (node->c2 != NULL) {
    SemanticCheckScalarType(node->c2);
  }

  // Optional expression 3.
  AnalyzeExpression(node->c3);

  // Finally the statment.
  AnalyzeStatement(node->stmt);
}

static void AnalyzeCompoundStatement(CompoundStatementASTNode* node) {
  size_t num_statements = node->statements->length;
  for (size_t i = 0; i < num_statements; i++) {
    AnalyzeStatement((ASTNode*)node->statements->value.p[i]);
  }
}

static void AnalyzeReturnStatement(CombinedStatementASTNode* node) {
  ASTNode* return_value = node->cond;
  if (return_value != NULL && return_value->op == AST_OP(asm)) {
    // Extension: return asm("foo") is allowed
    // Set the type of the asm statement to the return type of this function.
    ASTNodeSetType(return_value, compiler->current_function->next);
    return;
  }
  AnalyzeExpression(return_value);

  // Check current function return type.
  if (TypeIsVoid(compiler->current_function->next)) {
    // C does not allow a return statement with a value in a void function.
    if (return_value != NULL) {
      SemanticError(return_value, "Cannot return a value from a void function");
    }
  } else {
    // Function returns a value.
    if (return_value == NULL) {
      SemanticError((ASTNode*)node,
                    "Must return a value from a non-void function");
    } else {
      SemanticConvertType(return_value, compiler->current_function->next);
    }
  }
}

static void AnalyzeCaseLabel(CaseLabelASTNode* node) {
  if (node->expr != NULL) {
    // A case with no expression is used for 'default'.
    AnalyzeExpression(node->expr);
  }
  // Since we don't know the type of the switch controlling expressions here
  // we delay the analysis of the case label statements to the analysis of the
  // switch statement.
}

void AnalyzeVariableDeclaration(VariableDeclarationASTNode* node) {
  AnalyzeExpression(node->initializer);
  if (node->initializer != NULL) {
    SemanticConvertType(node->initializer, node->symbol->type);
  }
}

void AnalyzeDeclarationList(DeclarationListASTNode* node) {
  size_t num_decls = node->declarations->length;
  for (size_t i = 0; i < num_decls; i++) {
    AnalyzeStatement(node->declarations->value.p[i]);
  }
}

void AnalyzeGotoStatement(CombinedStatementASTNode* node) {
  Vector* function_body = &compiler->current_function->info.function.body;
  String* label_name = ((ConstantASTNode*)node->cond)->value.string;

  // Look for label matching the goto.
  // If we find it, set the 'stmt' field of the node to point to it.
  // In C, labels are global to the whole function.
  size_t num_statements = function_body->length;
  ASTNode* label_node = NULL;
  for (size_t i = 0; i < num_statements; i++) {
    ASTNode* stmt = (ASTNode*)function_body->value.p[i];
    if (stmt->op == AST_OP(label)) {
      LabelASTNode* label = (LabelASTNode*)stmt;
      if (StringEqualString(label_name, &label->name)) {
        label_node = stmt;
        break;
      }
    }
  }
  if (label_node == NULL) {
    SemanticError((ASTNode*)node, "Undefined label %s", label_name->value);
  }
  node->stmt = label_node;
}

void AnalyzeLabel(LabelASTNode* node) {
  Vector* function_body = &compiler->current_function->info.function.body;
  String* label_name = &node->name;

  // Look for another label with same name.
  size_t num_statements = function_body->length;
  for (size_t i = 0; i < num_statements; i++) {
    ASTNode* stmt = (ASTNode*)function_body->value.p[i];
    if (stmt == (ASTNode*)node) {
      // Same statement, ignore.
      continue;
    }
    if (stmt->op == AST_OP(label)) {
      LabelASTNode* label = (LabelASTNode*)stmt;
      if (StringEqualString(label_name, &label->name)) {
        SemanticError(stmt, "Duplicate label %s", label_name->value);
        break;
      }
    }
  }
}

void AnalyzeStatement(ASTNode* node) {
  if (node == NULL) {
    return;
  }

  switch (node->op) {
    case AST_OP(decl_list):
      AnalyzeDeclarationList((DeclarationListASTNode*)node);
      break;
    case AST_OP(vardecl):
      AnalyzeVariableDeclaration((VariableDeclarationASTNode*)node);
      break;
    case AST_OP(expr):
      AnalyzeExpressionStatement((ExpressionStatementASTNode*)node);
      break;
    case AST_OP(compound):
      AnalyzeCompoundStatement((CompoundStatementASTNode*)node);
      break;
    case AST_OP(if):
      AnalyzeIfStatement((IfStatementASTNode*)node);
      break;
    case AST_OP(while):
      AnalyzeWhileStatement((CombinedStatementASTNode*)node);
      break;
    case AST_OP(do):
      AnalyzeDoStatement((CombinedStatementASTNode*)node);
      break;
    case AST_OP(switch):
      AnalyzeSwitchStatement((SwitchStatementASTNode*)node);
      break;
    case AST_OP(for):
      AnalyzeForStatement((ForStatementASTNode*)node);
      break;
    case AST_OP(return ):
      AnalyzeReturnStatement((CombinedStatementASTNode*)node);
      break;
    case AST_OP(case):
      AnalyzeCaseLabel((CaseLabelASTNode*)node);
      break;
    case AST_OP(goto):
      AnalyzeGotoStatement((CombinedStatementASTNode*)node);
      break;
    case AST_OP(label):
      AnalyzeLabel((LabelASTNode*)node);
      break;

    case AST_OP(break):
    case AST_OP(continue):
    case AST_OP(asm):
      // No analysis needed for these.
      break;

    default:
      assert(false);
  }
}
