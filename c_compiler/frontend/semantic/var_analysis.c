//
//  var_analysis.c
//  c_compiler_library
//
//  Created by David Allison on 5/25/20.
//  Copyright © 2020 David Allison. All rights reserved.
//
#include <stdbool.h>

#include "var_analysis.h"

typedef struct {
  int loop_count;
  int arg_count;
} VarAnalyzer;

static void AnalyzeVars(ASTNode* node, void* data, int child_id, VisitorMode mode) {
  VarAnalyzer* a = data;
  switch (node->op) {
    case AST_OP(identifier): {
      IdentifierASTNode* id = (IdentifierASTNode*)node;
      Symbol* symbol = id->symbol;
      symbol->usage_info.used_as_arg += a->arg_count > 0;
      symbol->usage_info.reads++;
      
      // Loop counts accumulate so nested loops count for more.
      symbol->usage_info.used_in_loop += a->loop_count;
      break;
    }
      

    case AST_OP(call):
      // All children except first are used arguments.
      if (child_id != 0) {
        if (mode == kVisitPreChildren) {
          a->arg_count++;
        } else {
          a->arg_count--;
        }
      break;
      }
      
    case AST_OP(while):
    case AST_OP(do):
      if (mode == kVisitPreChildren) {
        a->loop_count++;
      } else {
        a->loop_count--;
      }
      break;
      
    case AST_OP(for):
      // Second and fourth children are used on every iteration.
      if (mode == kVisitPreChildren) {
        if (child_id == 1 || child_id == 3) {
          a->loop_count++;
        }
      } else {
        if (child_id == 1 || child_id == 3) {
          a->loop_count--;
        }
      }
      break;

    default:
      break;
  }
}

void AnalyzeVariables(ASTNode* node) {
  VarAnalyzer a = {0};
  ASTNodeVisit(node, AnalyzeVars, 0, &a);
}
