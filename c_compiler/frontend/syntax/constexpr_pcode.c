//
//  constexpr_pcode.c
//  c_compiler
//

#include "constexpr_pcode.h"
#include <string.h>
#include "ast.h"
#include "p_code_reg_alloc.h"
#include "p_code_vm.h"
#include "type.h"

static Symbol* PCodeConstexprFunctionDefinition(Symbol* symbol) {
  if (symbol == NULL || symbol->type == NULL || !TypeIsFunction(symbol->type)) {
    return NULL;
  }
  if (symbol->type->info.function.body != NULL) {
    return symbol;
  }
  if (symbol->value.func_defn != NULL &&
      symbol->value.func_defn->type != NULL &&
      TypeIsFunction(symbol->value.func_defn->type)) {
    return symbol->value.func_defn;
  }
  return NULL;
}

static Symbol* PCodeConstexprCallSymbol(ASTNode* node) {
  if (node == NULL || node->op != AST_OP(call)) {
    return NULL;
  }
  VectorASTNode* call = (VectorASTNode*)node;
  if (call->left == NULL || call->left->op != AST_OP(identifier)) {
    return NULL;
  }
  return ((IdentifierASTNode*)call->left)->symbol;
}

static ASTNode* SingleReturnExpression(TypeRecord* func) {
  if (func == NULL || func->info.function.body == NULL ||
      func->info.function.body->op != AST_OP(compound)) {
    return NULL;
  }
  CompoundStatementASTNode* compound =
      (CompoundStatementASTNode*)func->info.function.body;
  if (compound->statements == NULL || compound->statements->length != 1) {
    return NULL;
  }
  ASTNode* stmt = compound->statements->value.p[0];
  if (stmt == NULL || stmt->op != AST_OP(return)) {
    return NULL;
  }
  return ((CombinedStatementASTNode*)stmt)->cond;
}

static bool ValidateFunction(TypeRecord* func, const char** reason) {
  if (func == NULL || !TypeIsFunction(func)) {
    *reason = "callee is not a function";
    return false;
  }
  if (!func->info.function.is_constexpr) {
    *reason = "callee is not constexpr";
    return false;
  }
  if (func->info.function.body == NULL) {
    *reason = "callee has no body";
    return false;
  }
  if (func->info.function.varargs) {
    *reason = "varargs constexpr functions are not pcode-eligible";
    return false;
  }
  if (func->info.function.is_virtual) {
    *reason = "virtual constexpr functions are not pcode-eligible";
    return false;
  }
  if (func->info.function.is_constructor || func->info.function.is_destructor) {
    *reason = "constructor/destructor constexpr calls use object evaluation";
    return false;
  }
  return true;
}

bool ConstexprPCodeValidateCall(ASTNode* node, const char** reason) {
  static const char* ok = "ok";
  if (reason == NULL) {
    reason = &ok;
  }
  *reason = ok;
  Symbol* callee = PCodeConstexprFunctionDefinition(
      PCodeConstexprCallSymbol(node));
  if (callee == NULL) {
    *reason = "call is not a direct constexpr function definition";
    return false;
  }
  return ValidateFunction(callee->type, reason);
}

static PCodeVMStatus ConstexprEscape(PCodeVM* vm, int32_t code, void* data) {
  (void)vm;
  (void)data;
  return code == 4 ? kPCodeVMStatusHalted : kPCodeVMStatusUndefinedEscape;
}

static bool RunIntegerConstantProgram(int64_t value, int64_t* result) {
  uint32_t program[4];
  uint64_t raw_value = (uint64_t)value;
  program[0] = 0xc0000000 | (PCODE_OP(movxc) << 24) |
               (PCODE_INT_RETURN_REG << 16);
  program[1] = (uint32_t)(raw_value & 0xffffffffu);
  program[2] = (uint32_t)(raw_value >> 32);
  program[3] = (uint32_t)(PCODE_OP(esc) << 24 | 4);

  PCodeVM vm;
  if (!PCodeVMInitWithStack(&vm, P_CODE_VM_DEFAULT_STACK_SIZE)) {
    return false;
  }
  PCodeVMSetEscapeHandler(&vm, ConstexprEscape, NULL);
  PCodeVMSetEntry(&vm, (uint64_t)&program);
  vm.max_steps = 16;
  PCodeVMStatus status = PCodeVMRun(&vm);
  if (status == kPCodeVMStatusHalted) {
    *result = vm.iregs[PCODE_INT_RETURN_REG];
  }
  PCodeVMDestruct(&vm);
  return status == kPCodeVMStatusHalted;
}

static bool RunFloatingConstantProgram(double value, double* result) {
  uint32_t program[4];
  union {
    double f;
    uint64_t u;
  } raw_value;
  raw_value.f = value;
  program[0] = 0xc0000000 | (PCODE_OP(movdc) << 24) |
               (PCODE_DOUBLE_RETURN_REG << 16);
  program[1] = (uint32_t)(raw_value.u & 0xffffffffu);
  program[2] = (uint32_t)(raw_value.u >> 32);
  program[3] = (uint32_t)(PCODE_OP(esc) << 24 | 4);

  PCodeVM vm;
  if (!PCodeVMInitWithStack(&vm, P_CODE_VM_DEFAULT_STACK_SIZE)) {
    return false;
  }
  PCodeVMSetEscapeHandler(&vm, ConstexprEscape, NULL);
  PCodeVMSetEntry(&vm, (uint64_t)&program);
  vm.max_steps = 16;
  PCodeVMStatus status = PCodeVMRun(&vm);
  if (status == kPCodeVMStatusHalted) {
    *result = vm.dregs[PCODE_DOUBLE_RETURN_REG];
  }
  PCodeVMDestruct(&vm);
  return status == kPCodeVMStatusHalted;
}

bool ConstexprPCodeEvaluateCallAsInteger(ConstEvalContext* ctx, ASTNode* node,
                                         int64_t* result) {
  (void)ctx;
  const char* reason = NULL;
  if (!ConstexprPCodeValidateCall(node, &reason)) {
    return false;
  }
  Symbol* callee = PCodeConstexprFunctionDefinition(
      PCodeConstexprCallSymbol(node));
  ASTNode* return_expr = SingleReturnExpression(callee->type);
  if (return_expr == NULL || !ASTNodeIsIntConstant(return_expr)) {
    return false;
  }
  return RunIntegerConstantProgram(ASTNodeConstantValue(return_expr), result);
}

bool ConstexprPCodeEvaluateCallAsFloating(ConstEvalContext* ctx, ASTNode* node,
                                          double* result) {
  (void)ctx;
  const char* reason = NULL;
  if (!ConstexprPCodeValidateCall(node, &reason)) {
    return false;
  }
  Symbol* callee = PCodeConstexprFunctionDefinition(
      PCodeConstexprCallSymbol(node));
  ASTNode* return_expr = SingleReturnExpression(callee->type);
  if (return_expr == NULL) {
    return false;
  }
  if (return_expr->op == AST_OP(fnumber)) {
    return RunFloatingConstantProgram(
        ((ConstantASTNode*)return_expr)->value.fvalue, result);
  }
  if (ASTNodeIsIntConstant(return_expr)) {
    return RunFloatingConstantProgram((double)ASTNodeConstantValue(return_expr),
                                     result);
  }
  return false;
}

bool ConstexprPCodeEvaluateCallAsObject(ConstEvalContext* ctx, ASTNode* node) {
  (void)ctx;
  (void)node;
  return false;
}

bool ConstexprPCodeEvaluateObjectConstantForSymbol(Symbol* symbol,
                                                   ASTNode* initializer) {
  (void)symbol;
  (void)initializer;
  return false;
}
