//
//  statement_codegen.c
//  c_compiler
//
//  Created by David Allison on 11/21/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "statement_codegen.h"
#include <assert.h>
#include "compiler.h"
#include "expr_codegen.h"

static void GenerateDeclarationList(Generator* gen,
                                    DeclarationListASTNode* node) {
  size_t num_decls = node->declarations->length;
  for (size_t i = 0; i < num_decls; i++) {
    GenerateStatement(gen, node->declarations->value[i]);
  }
}

static void GenerateVariableDeclaration(Generator* gen,
                                        VariableDeclarationASTNode* node) {
  if (node->initializer != NULL) {
    GenerateExpression(gen, node->initializer);
  }
}

static void GenerateExpressionStatement(Generator* gen,
                                        ExpressionStatementASTNode* node) {
  GenerateExpression(gen, node->expr);
}

static void GenerateCompoundStatement(Generator* gen,
                                      CompoundStatementASTNode* node) {
  size_t num_statements = node->statements->length;
  for (size_t i = 0; i < num_statements; i++) {
    GenerateStatement(gen, (ASTNode*)node->statements->value[i]);
  }
}

static void GenerateIfStatement(Generator* gen, IfStatementASTNode* node) {
  // If the condition is a constant we can omit the expression,
  // comparison and the statement as appropriate.
  if (ASTNodeIsIntConstant(node->cond)) {
    ConstantASTNode* c = (ConstantASTNode*)node->cond;
    if (c->value.ivalue != 0) {
      GenerateStatement(gen, node->if_part);
    } else {
      if (node->else_part != NULL) {
        GenerateStatement(gen, node->else_part);
      }
    }
    return;
  }
  
  // cond
  IRNode* cond = GenerateExpression(gen, node->cond);
  IRNode* else_label = NewIR(IR_OP(label));

  // bfalse cond, else_label
  GeneratorEmit(gen, NewIR2(IR_OP(bfalse), cond, else_label));

  // if_part
  GenerateStatement(gen, node->if_part);
  if (node->else_part != NULL) {
    IRNode* end = NewIR(IR_OP(label));

    // bra end
    GeneratorEmit(gen, NewIR1(IR_OP(bra), end));

    // else_label:
    GeneratorEmit(gen, else_label);

    // else_part
    GenerateStatement(gen, node->else_part);

    // end:
    GeneratorEmit(gen, end);
  } else {
    // else_label:
    GeneratorEmit(gen, else_label);
  }
}

static void GenerateWhileStatement(Generator* gen,
                                   CombinedStatementASTNode* node) {
  // Check for constant loop condition.
  ConstantASTNode* const_cond = NULL;
  if (ASTNodeIsIntConstant(node->cond)) {
    const_cond = (ConstantASTNode*)node->cond;
    if (const_cond->value.ivalue == 0) {
      // This is while(false), omit the whole statement.
      return;
    }
  }
  IRNode* old_break = gen->break_label;
  IRNode* old_continue = gen->continue_label;

  gen->break_label = NewIR(IR_OP(label));
  gen->continue_label = NewIR(IR_OP(label));

  // continue_label:
  GeneratorEmit(gen, gen->continue_label);

  // If we have a constant condition at this point it is non-zero
  // so this is a forever loop.
  if (const_cond == NULL) {
    IRNode* cond = GenerateExpression(gen, node->cond);

    // bfalse cond, break_label
    GeneratorEmit(gen, NewIR2(IR_OP(bfalse), cond, gen->break_label));
  }
  
  // stmt
  GenerateStatement(gen, node->stmt);

  // bra continue_label
  GeneratorEmit(gen, NewIR1(IR_OP(bra), gen->continue_label));

  // break_label:
  GeneratorEmit(gen, gen->break_label);

  gen->break_label = old_break;
  gen->continue_label = old_continue;
}

static void GenerateDoStatement(Generator* gen,
                                CombinedStatementASTNode* node) {
  IRNode* old_break = gen->break_label;
  IRNode* old_continue = gen->continue_label;

  gen->break_label = NewIR(IR_OP(label));
  gen->continue_label = NewIR(IR_OP(label));

  // loop_label:
  IRNode* loop_label = GeneratorEmit(gen, NewIR(IR_OP(label)));
 
  // stmt
  GenerateStatement(gen, node->stmt);

  // continue_label:
  GeneratorEmit(gen, gen->continue_label);

  if (ASTNodeIsIntConstant(node->cond)) {
    ConstantASTNode* c = (ConstantASTNode*)node->cond;
    // do ... while(constant);
    if (c->value.ivalue != 0) {
      // do .. while(true) - always loop.
      GeneratorEmit(gen, NewIR1(IR_OP(bra), loop_label));
    } else {
      // do ... while(false) - no loop back
    }
  } else {
    // cond
    IRNode* cond = GenerateExpression(gen, node->cond);
    
    // btrue cond, loop_label
    GeneratorEmit(gen, NewIR2(IR_OP(btrue), cond, loop_label));
  }

  // break_label:
  GeneratorEmit(gen, gen->break_label);

  gen->break_label = old_break;
  gen->continue_label = old_continue;
}

// Generate IR for a dense switch statement.  This is generated as a branch
// table.  This is used when the case values are in a range that are suitable
// for coding as a branch table; that is, they have a decent density.
static void GenerateDenseSwitch(Generator* gen, SwitchStatementASTNode* node) {
  IRNode* old_break = gen->break_label;

  gen->break_label = NewIR(IR_OP(label));

  IRNode* expr = GenerateExpression(gen, node->expr);

  // Set default label as either the break label or the defined default label.
  IRNode* default_label =
      node->default_node != NULL ? node->default_node->label : gen->break_label;

  IRNode* min =
      GeneratorGetIntConstant(gen, node->expr->type, node->min_case_value);
  IRNode* max =
      GeneratorGetIntConstant(gen, node->expr->type, node->max_case_value);

  // Compare expr to min and branch to default if less.
  IRNode* cmplo = GeneratorEmit(gen, NewIR2(IR_OP(cmplti), expr, min));
  GeneratorEmit(gen, NewIR2(IR_OP(btrue), cmplo, default_label));

  // Compare expr to max and branch to default if greater.

  IRNode* cmphi = GeneratorEmit(gen, NewIR2(IR_OP(cmpgti), expr, max));
  GeneratorEmit(gen, NewIR2(IR_OP(btrue), cmphi, default_label));

  // Subtract min from expr to get branch offset.
  IRNode* zeroed = GeneratorEmit(gen, NewIR2(IR_OP(subi), expr, min));

  // Computed branch via jump table.
  GeneratorEmit(gen, NewIR1(IR_OP(cbra), zeroed));

  // Generate dense branch table with branches to default filling in empty slots
  // and branches to the case labels for those with case values.
  int64_t next_value = node->min_case_value;
  for (size_t i = 0; i < node->cases.length; i++) {
    CaseLabelASTNode* case_node = node->cases.value[i];
    if (next_value != case_node->value) {
      // Fill gap in branch table with branches to the default label.
      do {
        GeneratorEmit(gen, NewIR1(IR_OP(bra), default_label));
        next_value++;
      } while (next_value != case_node->value);
    }
    GeneratorEmit(gen, NewIR1(IR_OP(bra), case_node->label));
    next_value++;
  }

  // Switch statement body.  This includes all the case labels and default (if
  // present). These will emit their labels when they are generated.
  GenerateStatement(gen, node->stmt);

  // break_label:
  GeneratorEmit(gen, gen->break_label);

  gen->break_label = old_break;
}

static void GenerateBinaryCaseSearch(Generator* gen,
                                     SwitchStatementASTNode* node,
                                     IRNode* expr,
                                     IRNode* default_label,
                                     size_t start,
                                     size_t end) {
  size_t length = end - start;
  if (length > 15) {
    // More than 15 cases, split search into lower and upper halfs.
    IRNode* lower_half = NewIR(IR_OP(label));
    size_t mid = start + length/2;
    CaseLabelASTNode* mid_case_node = node->cases.value[mid];
    IRNode* compare =
          GeneratorEmit(gen, NewIR2(IR_OP(cmplti), expr,
                              GeneratorGetIntConstant(gen, node->expr->type,
                                                      mid_case_node->value)));
    GeneratorEmit(gen, NewIR2(IR_OP(btrue), compare, lower_half));
    
    // Search upper half
    GenerateBinaryCaseSearch(gen, node, expr, default_label, mid, end);
    
    // Search lower half.
    GeneratorEmit(gen, lower_half);
    GenerateBinaryCaseSearch(gen, node, expr, default_label, start, mid);
    return;
  }
  // 15 cases or less, use linear search.
  for (size_t i = start; i < end; i++) {
    CaseLabelASTNode* case_node = node->cases.value[i];
    IRNode* compare =
    GeneratorEmit(gen, NewIR2(IR_OP(cmpeqi), expr,
                              GeneratorGetIntConstant(gen, node->expr->type,
                                                      case_node->value)));
    GeneratorEmit(gen, NewIR2(IR_OP(btrue), compare, case_node->label));
  }
  GeneratorEmit(gen, NewIR1(IR_OP(bra), default_label));
}

// Generate IR for a switch statement using a sparse comparison coding.  This
// compares each case value in turn and branches to the appropriate label.
static void GenerateSparseSwitch(Generator* gen, SwitchStatementASTNode* node) {
  IRNode* old_break = gen->break_label;

  gen->break_label = NewIR(IR_OP(label));

  IRNode* expr = GenerateExpression(gen, node->expr);

  // Set default label as either the break label or the defined default label.
  IRNode* default_label =
      node->default_node != NULL ? node->default_node->label : gen->break_label;

  // Generate sequence of comparisons using a binary search through the
  // cases.
  GenerateBinaryCaseSearch(gen, node, expr,
                          default_label, 0, node->cases.length);
  
  // Switch statement body.  This includes all the case labels and default (if
  // present). These will emit their labels when they are generated.
  GenerateStatement(gen, node->stmt);

  // break_label:
  GeneratorEmit(gen, gen->break_label);

  gen->break_label = old_break;
}

// A constant switch is generated as a branch to a label inside
// the switch body.  What we really want is to simply generate
// the code for the located case label but that is inside a
// statement that is difficult to traverse.  So instead we
// generate a single branch to a label and rely on the
// basic block analysis to remove any unreachable code.
static void GenerateConstantSwitch(Generator* gen,
                                   SwitchStatementASTNode* node) {
  ConstantASTNode* value_node = (ConstantASTNode*)node->expr;
  int64_t value = value_node->value.ivalue;
  
  // Look for the case statement that matches the value and create
  // a label for it.
  CaseLabelASTNode* found_case = NULL;
  for (size_t i = 0; i < node->cases.length; i++) {
    CaseLabelASTNode* case_node = (CaseLabelASTNode*)node->cases.value[i];
    if (case_node->value == value) {
      found_case = case_node;
      break;
    }
  }
  
  if (found_case != NULL) {
    IRNode* label = NewIR(IR_OP(label));
    found_case->label = label;
    // Generate an unconditional branch to the label.
    GeneratorEmit(gen, NewIR1(IR_OP(bra), label));
  } else {
    // No case found, look for default.
    if (node->default_node == NULL) {
      // No default, no statement is possible.
      return;
    }
    IRNode* label = NewIR(IR_OP(label));
    node->default_node->label = label;
    // Generate an unconditional branch to the label.
    GeneratorEmit(gen, NewIR1(IR_OP(bra), label));
  }
  
  // There will be one label defined in the switch statement
  // body.  We now generate the code for all the cases and
  // anything that is unreachable will be eliminated later.
  IRNode* old_break = gen->break_label;
  
  gen->break_label = NewIR(IR_OP(label));

  GenerateStatement(gen, node->stmt);
  
  // break_label:
  GeneratorEmit(gen, gen->break_label);
  
  gen->break_label = old_break;
}
  
// Switch statement IR generation.
static void GenerateSwitchStatement(Generator* gen,
                                    SwitchStatementASTNode* node) {
  if (ASTNodeIsIntConstant(node->expr)) {
    // Constant switch expression.  Only generate code for the case
    // that matches.
    GenerateConstantSwitch(gen, node);
    return;
  }

  // Create case labels.
  for (size_t i = 0; i < node->cases.length; i++) {
    IRNode* label = NewIR(IR_OP(label));
    CaseLabelASTNode* case_node = (CaseLabelASTNode*)node->cases.value[i];
    case_node->label = label;
  }

  // Default label.
  if (node->default_node != NULL) {
    IRNode* label = NewIR(IR_OP(label));
    CaseLabelASTNode* default_node = (CaseLabelASTNode*)node->default_node;
    default_node->label = label;
  }

  // Use density calculated from semantic analysis to determine what
  // type of switch to generate.
  if (node->density > 0.5) {
    GenerateDenseSwitch(gen, node);
  } else {
    GenerateSparseSwitch(gen, node);
  }
}

static void GenerateForStatement(Generator* gen, ForStatementASTNode* node) {
  IRNode* old_break = gen->break_label;
  IRNode* old_continue = gen->continue_label;

  gen->break_label = NewIR(IR_OP(label));
  gen->continue_label = NewIR(IR_OP(label));
  IRNode* loop_label = NewIR(IR_OP(label));

  // Initial expression (e1).
  if (node->c1 != NULL) {
    if (node->c1->op == AST_OP(decl_list)) {
      GenerateStatement(gen, node->c1);
    } else {
      GenerateExpression(gen, node->c1);
    }
  }

  // loop_label:
  GeneratorEmit(gen, loop_label);

  // Condition (e2).
  if (node->c2 != NULL) {
    if (ASTNodeIsIntConstant(node->c2)) {
      ConstantASTNode* c = (ConstantASTNode*)node->c2;
      if (c->value.ivalue == 0) {
        // Condition is false, omit whole statement as it will never
        // be executed
        gen->break_label = old_break;
        gen->continue_label = old_continue;
        return;
      } else {
        // Condition is always true, omit expression and bfalse.
      }
    } else {
      IRNode* cond = GenerateExpression(gen, node->c2);

      // bfalse cond, break_label
      GeneratorEmit(gen, NewIR2(IR_OP(bfalse), cond, gen->break_label));
    }
  }

  // stmt
  GenerateStatement(gen, node->stmt);

  // Continue label.
  GeneratorEmit(gen, gen->continue_label);

  if (node->c3 != NULL) {
    GenerateExpression(gen, node->c3);
  }

  // bra loop_label
  GeneratorEmit(gen, NewIR1(IR_OP(bra), loop_label));

  // break_label:
  GeneratorEmit(gen, gen->break_label);

  gen->break_label = old_break;
  gen->continue_label = old_continue;
}

static void GenerateReturnStatement(Generator* gen,
                                    CombinedStatementASTNode* node) {
  if (node->cond != NULL) {
    if (node->cond->op == AST_OP(asm)) {
      // Extension: return asm(..)
      GenerateStatement(gen, node->cond);
    } else {
      IRNode* expr = GenerateExpression(gen, node->cond);
      if (TypeIsStructOrUnion(node->cond->type)) {
        // Returning a struct, copy result to return value.
        GeneratorEmit(gen, NewIR3(IR_OP(memcpy), gen->struct_return_value, expr,
                                  GeneratorGetIntConstant(
                                      gen, NULL, node->cond->type->size)));
      } else {
        IROpcode result;
        if (TypeIsIntegral(node->cond->type)) {
          result = IR_OP(resulti);
        } else if (TypeIsFloat(node->cond->type)) {
          result = IR_OP(resultf);
        } else if (TypeIsDouble(node->cond->type)) {
          result = IR_OP(resultd);
        } else {
          result = IR_OP(resulta);
        }
        GeneratorEmit(gen, NewIR1(result, expr));
      }
    }
  }

  // We don't explictly do the return here because the code
  // sequence can be large (restoring saved registers, etc).
  // So instead, we branch to the first return in the function.
  IRNode* return_label = GeneratorGetReturnLabel(gen);
  GeneratorEmit(gen, NewIR1(IR_OP(bra), return_label));
}

static void GenerateCaseLabel(Generator* gen, CaseLabelASTNode* node) {
  if (node->label == NULL) {
    return;
  }
  GeneratorEmit(gen, node->label);
}

// The semantic analyzer sets the 'stmt' field of the CombinedStatementASTNode
// to the AST node of the referenced label.  This may be before or after the
// goto statement has been generated.  If the label node has an IRNode already
// assigned we generate a branch to it, otherwise we create one for it.
static void GenerateGotoStatement(Generator* gen,
                                  CombinedStatementASTNode* node) {
  LabelASTNode* label_node = (LabelASTNode*)node->stmt;
  if (label_node->label == NULL) {
    label_node->label = NewIR(IR_OP(label));
  }
  GeneratorEmit(gen, NewIR1(IR_OP(bra), label_node->label));
}

static void GenerateLabel(Generator* gen, LabelASTNode* node) {
  // If the label has not already been generated (by the goto) generate
  // one now.
  if (node->label == NULL) {
    node->label = NewIR(IR_OP(label));
  }

  // Emit label.
  GeneratorEmit(gen, node->label);
}

static void GenerateBreak(Generator* gen, ASTNode* node) {
  GeneratorEmit(gen, NewIR1(IR_OP(bra), gen->break_label));
}

static void GenerateContinue(Generator* gen, ASTNode* node) {
  GeneratorEmit(gen, NewIR1(IR_OP(bra), gen->continue_label));
}

// Assembly language IR node.  This refers to a string literal.
static void GenerateAsm(Generator* gen, AsmASTNode* node) {
  int literal_id = CompilerAddStringLiteral(node->text);

  GeneratorEmit(
      gen, NewIR1(IR_OP(asm), GeneratorGetIntConstant(gen, NULL, literal_id)));
}

void GenerateStatement(Generator* gen, ASTNode* node) {
  if (node == NULL) {
    return;
  }

  // Emit location instructions if the statement will generate code.
  if (compiler->debug_output) {
    bool emit_loc = true;
    switch (node->op) {
      case AST_OP(decl_list):
        emit_loc = false;
        break;
      case AST_OP(vardecl): {
        // If the variable declaration doesn't have an initializer there is
        // no location,
        VariableDeclarationASTNode* decl = (VariableDeclarationASTNode*)node;
        if (decl->initializer == NULL) {
          emit_loc = false;
        }
        break;
      }
      case AST_OP(compound):
        // A compound statement contains statements, so there is no code for
        // itself.
        emit_loc = false;
        break;
      case AST_OP(label):
        emit_loc = false;
        break;
      default:
        break;
    }

    if (emit_loc) {
      GeneratorEmit(gen, NewIRLocation(node->location));
    }
  }

  switch (node->op) {
  case AST_OP(decl_list):
    GenerateDeclarationList(gen, (DeclarationListASTNode*)node);
    break;
  case AST_OP(vardecl):
    GenerateVariableDeclaration(gen, (VariableDeclarationASTNode*)node);
    break;
  case AST_OP(expr):
    GenerateExpressionStatement(gen, (ExpressionStatementASTNode*)node);
    break;
  case AST_OP(compound):
    GenerateCompoundStatement(gen, (CompoundStatementASTNode*)node);
    break;
  case AST_OP(if):
    GenerateIfStatement(gen, (IfStatementASTNode*)node);
    break;
  case AST_OP(while):
    GenerateWhileStatement(gen, (CombinedStatementASTNode*)node);
    break;
  case AST_OP(do):
    GenerateDoStatement(gen, (CombinedStatementASTNode*)node);
    break;
  case AST_OP(switch):
    GenerateSwitchStatement(gen, (SwitchStatementASTNode*)node);
    break;
  case AST_OP(for):
    GenerateForStatement(gen, (ForStatementASTNode*)node);
    break;
  case AST_OP(return ):
    GenerateReturnStatement(gen, (CombinedStatementASTNode*)node);
    break;
  case AST_OP(case):
    GenerateCaseLabel(gen, (CaseLabelASTNode*)node);
    break;
  case AST_OP(goto):
    GenerateGotoStatement(gen, (CombinedStatementASTNode*)node);
    break;
  case AST_OP(label):
    GenerateLabel(gen, (LabelASTNode*)node);
    break;
  case AST_OP(break):
    GenerateBreak(gen, node);
    break;
  case AST_OP(continue):
    GenerateContinue(gen, node);
    break;
  case AST_OP(asm):
    GenerateAsm(gen, (AsmASTNode*)node);
    break;
  default:
    assert(false);
  }
}
