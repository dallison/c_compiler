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
    GenerateStatement(gen, node->declarations->value.p[i]);
  }
}

static ASTNode* FindEnclosingLoop(ASTNode* stmt) {
  while (stmt != NULL) {
    switch (stmt->op) {
      case AST_OP(for): {
        ForStatementASTNode* f = (ForStatementASTNode*)stmt;
        return f->stmt;
      }
      case AST_OP(while):
      case AST_OP(do): {
        CombinedStatementASTNode* c = (CombinedStatementASTNode*)stmt;
        return c->stmt;
      }
      default:
        break;
    }
    stmt = stmt->parent;
  }
  return NULL;
}

static ASTNode* FindEnclosingLoopOrSwitch(ASTNode* stmt) {
  while (stmt != NULL) {
    switch (stmt->op) {
       case AST_OP(for): {
        ForStatementASTNode* f = (ForStatementASTNode*)stmt;
        return f->stmt;
      }
      case AST_OP(while):
      case AST_OP(do): {
        CombinedStatementASTNode* c = (CombinedStatementASTNode*)stmt;
        return c->stmt;
      }
      case AST_OP(switch): {
        SwitchStatementASTNode *s = (SwitchStatementASTNode*)stmt;
        return s->stmt;
      }
      default:
        break;
    }
    stmt = stmt->parent;
  }
  return NULL;
}

static IRNode* ContainsVLA(DeclarationListASTNode* decl_list) {
  for (size_t j = 0; j < decl_list->declarations->length; j++) {
     VariableDeclarationASTNode* decl = decl_list->declarations->value.p[j];
     if (TypeIsVLA(decl->base.type)) {
       return decl->saved_sp;
     }
  }
  return NULL;
}

static IRNode* FindTopVLAForJump(ASTNode* jump, ASTNode* dest) {
  ASTNode* node = jump;
  IRNode* top_vla = NULL;
  while (node != dest) {
    if (node->parent != NULL && node->parent->op == AST_OP(compound)) {
      CompoundStatementASTNode* c = (CompoundStatementASTNode*)node->parent;
      bool found_vla = false;
      for (size_t i = 0; !found_vla && i < c->statements->length; i++) {
        ASTNode* stmt = c->statements->value.p[i];
        if (stmt == node) {
          break;
        }
        if (stmt->op == AST_OP(decl_list)) {
          DeclarationListASTNode* decl_list = (DeclarationListASTNode*)stmt;
          IRNode* vla = ContainsVLA(decl_list);
          if (vla != NULL) {
            top_vla = vla;
            found_vla = true;
          }
        }
      }
    }
    node = node->parent;
  }
  return top_vla;
}

IRNode* GenerateVLASize(Generator* gen, TypeRecord* type) {
  IRNode* size_expr = GenerateExpression(gen,
                                         type->info.array.size.vla.size);
  IRNode* sub_size;
  if (TypeIsVLA(type->next)) {
    sub_size = GenerateVLASize(gen, type->next);
  } else {
    sub_size = GeneratorGetIntConstant(gen,
                                       NULL,
                                       type->next->size);
  }
  IRNode* result = GeneratorEmit(gen, NewIR2(IR_OP(muli), size_expr, sub_size));
  type->info.array.size.vla.codegen_info = result;
  return result;
}

// VLA definition.  The size of the array is an expression held in the
// array info in the type.  This needs to be multiplied by the array's
// subtype's size, which might also be a VLA.
// Also writes the saved SP address into the decl node's saved_sp.
static IRNode* GenerateVLADefinition(Generator* gen, TypeRecord* type,
                                     VariableDeclarationASTNode* decl) {
  // Size of array.
  IRNode* size = GenerateVLASize(gen, type);
  
  // Saved stack pointer.
  IRNode* saved_sp = GeneratorEmit(gen, NewIR(IR_OP(tmp)));
  IRSetType(saved_sp, NewPointerTo(kQualPlain, NewTypeRecordWithSize(kTypeVoid, kQualPlain)));
  GeneratorEmit(gen, NewIR1(IR_OP(savesp), saved_sp));
  decl->saved_sp = saved_sp;
  
  // Address of VLA (current stack pointer after decrement).
  IRNode* array_addr = GeneratorEmit(gen, NewIR(IR_OP(tmp)));
  IRSetType(array_addr, NewTypeRecordWithSize(kTypeInt, kQualPlain));
  
  // Make space for aligned array on stack.
  IRNode* aligned = GeneratorEmit(gen, NewIR2(IR_OP(aligni),
                                             size,
                                             GeneratorGetIntConstant(gen,
                                                                     NULL,
                                                                     compiler->target->stack_alignment)));
  GeneratorEmit(gen, NewIR1(IR_OP(decsp), aligned));
  GeneratorEmit(gen, NewIR1(IR_OP(savesp), array_addr));
  return array_addr;
}

static void GenerateVariableDeclaration(Generator* gen,
                                        VariableDeclarationASTNode* node) {
  if (TypeIsVLA(node->symbol->type)) {
    // Variable Length Array.
    IRNode* addr = GenerateVLADefinition(gen, node->symbol->type, node);
    node->symbol->value.other = addr;
  }
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
    ASTNode* stmt = node->statements->value.p[i];
    GenerateStatement(gen, stmt);
  }
  
  // Restore stack pointer to value saved before topmost VLA was allocated.
  for (size_t i = 0; i < num_statements; i++) {
    ASTNode* stmt = node->statements->value.p[i];
    if (stmt->op == AST_OP(decl_list)) {
      DeclarationListASTNode* decl_list = (DeclarationListASTNode*)stmt;
      IRNode* vla = ContainsVLA(decl_list);
      if (vla != NULL) {
        GeneratorEmit(gen, NewIR1(IR_OP(restoresp), vla));
        break;
      }
    }
  }
}

// Is the statement just a branch (possibly enclosed in a compound).
// Return the branch if it is, NULL otherwise;
static ASTNode* CheckSingleBranch(ASTNode* stmt, ASTOpcode opcode) {
  if (stmt == NULL) {
    return NULL;
  }
  if (OptLevel0()) {
    return NULL;
  }
  if (stmt->op == AST_OP(compound)) {
    CompoundStatementASTNode* c = (CompoundStatementASTNode*)stmt;
    if (c->statements->length >= 1) {
      stmt = c->statements->value.p[0];
    }
  }
  if (stmt->op == opcode) {
    return stmt;
  }
  
  return NULL;
}


// Does the statement subtree contain a label, case or default?  Such labels
// are valid goto/switch targets, so a branch that looks dead (e.g. the body of
// `if (0)`) is actually reachable and its code must still be generated.
static bool StatementContainsLabel(ASTNode* node) {
  if (node == NULL) {
    return false;
  }
  switch (node->op) {
    case AST_OP(label):
    case AST_OP(case):
      return true;
    case AST_OP(compound): {
      CompoundStatementASTNode* c = (CompoundStatementASTNode*)node;
      for (size_t i = 0; i < c->statements->length; i++) {
        if (StatementContainsLabel(c->statements->value.p[i])) {
          return true;
        }
      }
      return false;
    }
    case AST_OP(if): {
      IfStatementASTNode* n = (IfStatementASTNode*)node;
      return StatementContainsLabel(n->if_part) ||
             StatementContainsLabel(n->else_part);
    }
    case AST_OP(while):
    case AST_OP(do):
      return StatementContainsLabel(((CombinedStatementASTNode*)node)->stmt);
    case AST_OP(for):
      return StatementContainsLabel(((ForStatementASTNode*)node)->stmt);
    case AST_OP(switch):
      return StatementContainsLabel(((SwitchStatementASTNode*)node)->stmt);
    default:
      return false;
  }
}

static void GenerateIfStatement(Generator* gen, IfStatementASTNode* node) {
  // If the condition is a constant we can omit the expression,
  // comparison and the statement as appropriate -- but only when the dead arm
  // contains no labels.  A label inside the dead arm is a valid goto/switch
  // target, so that code is reachable and must still be generated (otherwise
  // the label is dropped while branches to it survive, dangling the target).
  if (OptLevel1() && ASTNodeIsIntConstant(node->cond) &&
      !StatementContainsLabel(node->if_part) &&
      !StatementContainsLabel(node->else_part)) {
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
  IRNode* end_label = NULL;
  if (node->else_part != NULL) {
     end_label = NewIR(IR_OP(label));
  }
  
  // Optimize branches over single break, continue, goto
  // instructions.
  // We want to avoid a condition branch over a branch.
  //
  // Say we have (as is common):
  // if (cond) {
  //   break;
  // }
  // The naive way to do this is:
  // cond
  // bfalse end_label
  // bra break_label
  // end_label:
  //
  // But it's more efficient to generate:
  // cond
  // btrue break_label
  //
  // Likewise for continue and goto.
  ASTNode* break_stmt = CheckSingleBranch(node->if_part, AST_OP(break));
  ASTNode* continue_stmt = CheckSingleBranch(node->if_part, AST_OP(continue));
  ASTNode* goto_stmt = CheckSingleBranch(node->if_part, AST_OP(goto));
  if (break_stmt != NULL) {
    // if (cond) { break; } -> if(cond) goto break_label;
    // btrue cond, break_label
    GeneratorEmit(gen, NewIR2(IR_OP(btrue), cond, gen->break_label));
  } else if (continue_stmt != NULL) {
      // if (cond) { continue; } -> if(cond) goto continue_label;
      // btrue cond, continue_label
      GeneratorEmit(gen, NewIR2(IR_OP(btrue), cond, gen->continue_label));
  } else if (goto_stmt != NULL) {
    GotoStatementASTNode* go = (GotoStatementASTNode*)goto_stmt;
    LabelASTNode* label_node = (LabelASTNode*)go->label;
    if (label_node->label == NULL) {
      label_node->label = NewIR(IR_OP(label));
    }
    GeneratorEmit(gen, NewIR2(IR_OP(btrue), cond, label_node->label));
  } else {
    // Regular if (cond) stmt;
    // bfalse cond, else_label
    GeneratorEmit(gen, NewIR2(IR_OP(bfalse), cond, else_label));

    // if_part
    GenerateStatement(gen, node->if_part);
    
    if (end_label != NULL) {
      // bra end_label
      GeneratorEmit(gen, NewIR1(IR_OP(bra), end_label));
    }
  }

  if (node->else_part != NULL) {
    // else_label:
    GeneratorEmit(gen, else_label);

    // else_part
    GenerateStatement(gen, node->else_part);

    // end:
    GeneratorEmit(gen, end_label);
  } else {
    // else_label:
    GeneratorEmit(gen, else_label);
  }
}

// Unless we are generating code for size:
// Most processors predict that a conditional branch will be
// taken.  A while loop has a conditional branch at the start
// that will be be predicted as taken but will not be taken
// on every loop iteration.  It is better to convert the
// while loop into:
//
// if (cond) {
//   do {
//   ...
//   } while (cond);
// }
static void GenerateWhileStatement(Generator* gen,
                                   CombinedStatementASTNode* node) {
  // Check for constant loop condition.
  ConstantASTNode* const_cond = NULL;
  if (OptLevel1() && ASTNodeIsIntConstant(node->cond)) {
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
 
  switch (compiler->code_preference) {
    case kCodeForSize: {
      // Preference is for size.  Generate the traditional loop:
      // continue_label:
      // bfalse cond, break_label
      // ...
      // bra continue_label
      // break_label:
      GeneratorEmit(gen, gen->continue_label);
      IRNode* cond = GenerateExpression(gen, node->cond);

      // bfalse cond, break_label
      GeneratorEmit(gen, NewIR2(IR_OP(bfalse), cond, gen->break_label));

      // stmt
      GenerateStatement(gen, node->stmt);
      GeneratorEmit(gen, NewIR1(IR_OP(bra), gen->continue_label));
      break;
    }
    case kCodeForSpeed: {
      // If we have a constant condition at this point it is non-zero
      // so this is a forever loop.
      if (const_cond == NULL) {
        IRNode* cond = GenerateExpression(gen, node->cond);

        // bfalse cond, break_label
        GeneratorEmit(gen, NewIR2(IR_OP(bfalse), cond, gen->break_label));
      }
     
      // continue_label:
      GeneratorEmit(gen, gen->continue_label);

      // stmt
      GenerateStatement(gen, node->stmt);

      if (const_cond == NULL) {
        // if (cond) goto continue_label.
        IRNode* cond = GenerateExpression(gen, node->cond);
        GeneratorEmit(gen, NewIR2(IR_OP(btrue), cond, gen->continue_label));
      } else {
        // bra continue_label
        GeneratorEmit(gen, NewIR1(IR_OP(bra), gen->continue_label));
      }
      break;
    }
  }
  
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

  if (OptLevel1() && ASTNodeIsIntConstant(node->cond)) {
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

  if (!node->all_cases_covered) {
    // If all cases are positive we can generate a negative comparison and
    // branch to default if true.  We can then use an unsigned comparison
    // for the values, which is faster on some processors.
    if (TypeIsSigned(node->expr->type) && node->all_cases_positive) {
      IRNode* cmp0 = GeneratorEmit(gen, NewIR2(IR_OP(cmplti), expr,
                                               GeneratorGetIntConstant(gen, node->expr->type, 0)));
      GeneratorEmit(gen, NewIR2(IR_OP(btrue), cmp0, default_label));
      
      // Use an unsigned comparison by resetting the node's type.
      TypeRecord* unsigned_type = TypeRecordCopy(expr->type);
      unsigned_type->type |= kTypeUnsigned;
      IRSetType(expr, unsigned_type);
      expr->flags |= kIRFakeUnsigned;     // This isn't really unsigned.
    }
    // Compare expr to min and branch to default if less.
    IRNode* cmplo = GeneratorEmit(gen, NewIR2(IR_OP(cmplti), expr, min));
    GeneratorEmit(gen, NewIR2(IR_OP(btrue), cmplo, default_label));

    // Compare expr to max and branch to default if greater.
    IRNode* cmphi = GeneratorEmit(gen, NewIR2(IR_OP(cmpgti), expr, max));
    GeneratorEmit(gen, NewIR2(IR_OP(btrue), cmphi, default_label));
  }
  
  // Subtract min from expr to get branch offset, unless min is zero (no
  // point in subtracting zero).
  IRNode* zeroed = expr;
  if (node->min_case_value != 0) {
    zeroed = GeneratorEmit(gen, NewIR2(IR_OP(subi), expr, min));
  }

  // Computed branch via jump table.
  GeneratorEmit(gen, NewIR1(IR_OP(cbra), zeroed));

  // Generate dense branch table with branches to default filling in empty slots
  // and branches to the case labels for those with case values.
  int64_t next_value = node->min_case_value;
  for (size_t i = 0; i < node->cases.length; i++) {
    CaseLabelASTNode* case_node = node->cases.value.p[i];
    if (next_value != case_node->value) {
      // Fill gap in branch table with branches to the default label.
      do {
        IRNode* bra =GeneratorEmit(gen, NewIR1(IR_OP(bra), default_label));
        bra->flags |= kIRJumpTableBranch;
        next_value++;
      } while (next_value != case_node->value);
    }
    IRNode* bra = GeneratorEmit(gen, NewIR1(IR_OP(bra), case_node->label));
    bra->flags |= kIRJumpTableBranch;
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
    CaseLabelASTNode* mid_case_node = node->cases.value.p[mid];
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
    CaseLabelASTNode* case_node = node->cases.value.p[i];
    IRNode* compare =
    GeneratorEmit(gen, NewIR2(IR_OP(cmpeqi), expr,
                              GeneratorGetIntConstant(gen, node->expr->type,
                                                      case_node->value)));
    GeneratorEmit(gen, NewIR2(IR_OP(btrue), compare, case_node->label));
  }
  if (!node->all_cases_covered) {
    GeneratorEmit(gen, NewIR1(IR_OP(bra), default_label));
  }
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
    CaseLabelASTNode* case_node = (CaseLabelASTNode*)node->cases.value.p[i];
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
  if (OptLevel1() && ASTNodeIsIntConstant(node->expr)) {
    // Constant switch expression.  Only generate code for the case
    // that matches.
    GenerateConstantSwitch(gen, node);
    return;
  }

  // Create case labels.
  for (size_t i = 0; i < node->cases.length; i++) {
    IRNode* label = NewIR(IR_OP(label));
    CaseLabelASTNode* case_node = (CaseLabelASTNode*)node->cases.value.p[i];
    case_node->label = label;
  }

  // Default label.
  if (node->default_node != NULL) {
    IRNode* label = NewIR(IR_OP(label));
    CaseLabelASTNode* default_node = (CaseLabelASTNode*)node->default_node;
    default_node->label = label;
  }

  // A dense switch is only good if the comparisons to set it up do
  // not exceed the advantage of the jump table.
  int min_dense_cases = 4;     // TODO: configure this per target.
  if (!node->all_cases_covered) {
    min_dense_cases += 2;     // Two extra comparisons.
    if (TypeIsSigned(node->expr->type) && node->all_cases_positive) {
      min_dense_cases += 1;     // One extra comparison.
    }
  }
  // Use density calculated from semantic analysis to determine what
  // type of switch to generate.
  //
  // The x86_64 back-end does not yet implement the computed-branch jump table
  // (its instructions are variable length, so the fixed-stride inline jump
  // table used by the RISC-V/AArch64 back-ends does not apply).  Always use a
  // sparse comparison search there instead.
  bool target_has_jump_table =
      !StringEqual(&compiler->target->name, "x86-64");
  if (target_has_jump_table && node->cases.length > min_dense_cases &&
      node->density > 0.5) {
    GenerateDenseSwitch(gen, node);
  } else {
    GenerateSparseSwitch(gen, node);
  }
}

// Like a while loop, convert it into a tail condition loop with
// an enclosing if statement.
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

  // Check for constant condition.
  bool constant_condition = false;
  if (node->c2 != NULL) {
    if (OptLevel1() && ASTNodeIsIntConstant(node->c2)) {
      ConstantASTNode* c = (ConstantASTNode*)node->c2;
      if (c->value.ivalue == 0) {
        // Condition is false, omit whole statement as it will never
        // be executed
        gen->break_label = old_break;
        gen->continue_label = old_continue;
        return;
      }
      constant_condition = true;
    }
  }
  
  switch (compiler->code_preference) {
    case kCodeForSize:
      // loop_label:
      GeneratorEmit(gen, loop_label);
      
      if (node->c2 != NULL && !constant_condition) {
        IRNode* cond = GenerateExpression(gen, node->c2);
        // bfalse cond, break_label
        GeneratorEmit(gen, NewIR2(IR_OP(bfalse), cond, gen->break_label));
      }
    
      // stmt
      GenerateStatement(gen, node->stmt);

      // Continue label.
      GeneratorEmit(gen, gen->continue_label);

      if (node->c3 != NULL) {
        GenerateExpression(gen, node->c3);
      }
      GeneratorEmit(gen, NewIR1(IR_OP(bra), loop_label));
      break;
      
    case kCodeForSpeed:
      // if (!cond) goto break_label
      if (node->c2 != NULL && !constant_condition) {
        IRNode* cond = GenerateExpression(gen, node->c2);
        // bfalse cond, break_label
        GeneratorEmit(gen, NewIR2(IR_OP(bfalse), cond, gen->break_label));
      }
      
      // loop_label:
      GeneratorEmit(gen, loop_label);


      // stmt
      GenerateStatement(gen, node->stmt);

      // Continue label.
      GeneratorEmit(gen, gen->continue_label);

      if (node->c3 != NULL) {
        GenerateExpression(gen, node->c3);
      }

      if (node->c2 != NULL && !constant_condition) {
        IRNode* cond = GenerateExpression(gen, node->c2);
        // btrue cond, loop_label
        GeneratorEmit(gen, NewIR2(IR_OP(btrue), cond, loop_label));
      } else {
        // bra loop_label
        GeneratorEmit(gen, NewIR1(IR_OP(bra), loop_label));
      }
      break;
    
  }
  
  // break_label:
  GeneratorEmit(gen, gen->break_label);

  gen->break_label = old_break;
  gen->continue_label = old_continue;
}

static void GenerateReturnStatement(Generator* gen,
                                    CombinedStatementASTNode* node) {
  IRNode* nrvo_expr = NULL;
  if (node->cond != NULL) {
    if (node->cond->op == AST_OP(asm)) {
      // Extension: return asm(..)
      GenerateStatement(gen, node->cond);
    } else {
      IRNode* expr = GenerateExpression(gen, node->cond);
      if (TypeIsStructOrUnion(node->cond->type)) {
        if ((node->cond->flags & kASTRvoCall) != 0) {
          // An RVO call is passed the structresult directly from the
          // current function so there's no need to copy the result.
        } else if ((node->cond->flags & kASTNrvoMarker) != 0) {
          // Named RVO, nothing to do.
          nrvo_expr = expr;
          // GeneratorEmit(gen, NewIR1(IR_OP(nrvoval), nrvo_expr));
        } else {
          // Returning a struct, copy result to return value.
          expr = GeneratorEmit(gen, NewIR1(IR_OP(addressof), expr));
          CheckForVarUse(expr, node->cond);
          IRNode* result = GeneratorEmit(gen, NewIR3(IR_OP(memcpy), gen->struct_return_value, expr,
                                    GeneratorGetIntConstant(
                                        gen, NULL, node->cond->type->size)));
          CheckForVarDef(result, &node->base);
        }
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
  IRNode* branch = GeneratorEmit(gen, NewIR1(IR_OP(bra), return_label));
  
  // Help out the lower code generators.
  branch->flags |= kIRReturnJump;
  if (nrvo_expr != NULL) {
    branch->flags |= kIRNrvoMarker;
  }
}

static void GenerateCaseLabel(Generator* gen, CaseLabelASTNode* node) {
  if (node->label == NULL) {
    return;
  }
  GeneratorEmit(gen, node->label);
  GenerateStatement(gen, node->stmt);
}

// The semantic analyzer sets the 'stmt' field of the CombinedStatementASTNode
// to the AST node of the referenced label.  This may be before or after the
// goto statement has been generated.  If the label node has an IRNode already
// assigned we generate a branch to it, otherwise we create one for it.
static void GenerateGotoStatement(Generator* gen,
                                  GotoStatementASTNode* node) {
  LabelASTNode* label_node = (LabelASTNode*)node->label;
  if (label_node->label == NULL) {
    label_node->label = NewIR(IR_OP(label));
  }
  assert(node->lca != NULL);      // Need a Lowest Common Ancestor set.
  IRNode* top_vla = FindTopVLAForJump(&node->base, node->lca);
  if (top_vla != NULL) {
    GeneratorEmit(gen, NewIR1(IR_OP(restoresp), top_vla));
  }
  GeneratorEmit(gen, NewIR1(IR_OP(bra), label_node->label));
}

static void GenerateLabel(Generator* gen, LabelASTNode* node) {
  // If the label has not already been generated (by the goto) generate
  // one now.
  if (node->label == NULL) {
    if (node->named) {
      node->label = NewIRNamedLabel(node->name.value);
    } else {
      node->label = NewIR(IR_OP(label));
    }
  }

  // Emit label.
  GeneratorEmit(gen, node->label);
  
  // Emit label statement.
  GenerateStatement(gen, node->stmt);
}

static void GenerateBreak(Generator* gen, ASTNode* node) {
  ASTNode* loop_or_switch = FindEnclosingLoopOrSwitch(node);
  assert(loop_or_switch != NULL);
  IRNode* top_vla = FindTopVLAForJump(node, loop_or_switch);
  if (top_vla != NULL) {
    GeneratorEmit(gen, NewIR1(IR_OP(restoresp), top_vla));
  }
  GeneratorEmit(gen, NewIR1(IR_OP(bra), gen->break_label));
}

static void GenerateContinue(Generator* gen, ASTNode* node) {
  ASTNode* loop = FindEnclosingLoop(node);
  assert(loop != NULL);
  IRNode* top_vla = FindTopVLAForJump(node, loop);
  if (top_vla != NULL) {
    GeneratorEmit(gen, NewIR1(IR_OP(restoresp), top_vla));
  }
  GeneratorEmit(gen, NewIR1(IR_OP(bra), gen->continue_label));
}

// Assembly language IR node.  This refers to a string literal.
static void GenerateAsm(Generator* gen, AsmASTNode* node) {
  int literal_id = CompilerAddStringLiteral(node->text, false);

  GeneratorEmit(
      gen, NewIR1(IR_OP(asm), GeneratorGetIntConstant(gen, NULL, literal_id)));
}

void GenerateStatement(Generator* gen, ASTNode* node) {
  if (node == NULL) {
    return;
  }

  IRSetLocation(node->location);
  
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
    GenerateGotoStatement(gen, (GotoStatementASTNode*)node);
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
