#include "contracts.h"

#include "compiler.h"
#include "errors.h"
#include "expr_semantics.h"
#include "semantics.h"
#include "statement_semantics.h"

typedef struct {
  TypeRecord* func;
  ContractAssertion* assertion;
  bool invalid_parameter_use;
} ContractUseCheck;

static void CheckPostconditionParameterUse(ASTNode* node, void* data,
                                           int child_id, VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node->op != AST_OP(identifier)) {
    return;
  }
  ContractUseCheck* check = data;
  Symbol* symbol = ((IdentifierASTNode*)node)->symbol;
  if (symbol == NULL || symbol == check->assertion->result_binding) {
    return;
  }
  Vector* prototype = &check->func->info.function.prototype;
  for (size_t i = 0; i < prototype->length; i++) {
    Symbol* formal = prototype->value.p[i];
    if (formal != symbol || TypeIsReference(formal->type) ||
        StringEqual(&formal->name, "this")) {
      continue;
    }
    if (check->func->info.function.is_coroutine) {
      SemanticError(node,
                    "postcondition of a coroutine cannot odr-use "
                    "a non-reference parameter");
      check->invalid_parameter_use = true;
    }
    return;
  }
}

static void SetResultBindingType(TypeRecord* func,
                                 ContractAssertion* assertion) {
  if (assertion->result_binding == NULL) {
    return;
  }
  TypeRecord* return_type = func->next;
  if (return_type == NULL || TypeContainsAuto(return_type) ||
      TypeIsUnknown(return_type)) {
    SemanticError(assertion->predicate,
                  "postcondition result type could not be deduced");
    return;
  }
  if (TypeIsVoid(return_type)) {
    SemanticError(assertion->predicate,
                  "a void function cannot declare a postcondition result");
    return;
  }
  bool returns_reference = TypeIsReference(return_type);
  if (returns_reference) {
    return_type = return_type->next;
  }
  TypeRecord* object_type = TypeRecordCopy(return_type);
  object_type->qualifiers |= kQualConst;
  TypeRecord* binding_type = object_type;
  if (returns_reference || TypeIsStructOrUnion(return_type)) {
    binding_type = NewReferenceTypeRecord(kQualPlain, false);
    TypeRecordChain(binding_type, object_type);
  }
  SymbolSetType(assertion->result_binding, binding_type);
}

static TypeRecord* ContractParameterViewType(Symbol* formal) {
  TypeRecord* view = TypeRecordCloneSpine(formal->type);
  if (StringEqual(&formal->name, "this") && TypeIsPointer(view) &&
      view->next != NULL) {
    view->next->qualifiers |= kQualConst;
  } else if (TypeIsReference(view) && view->next != NULL) {
    view->next->qualifiers |= kQualConst;
  } else {
    view->qualifiers |= kQualConst;
  }
  return view;
}

static void EnterContractParameterView(TypeRecord* func, Vector* saved_types) {
  VectorInit(saved_types);
  Vector* prototype = &func->info.function.prototype;
  for (size_t i = 0; i < prototype->length; i++) {
    Symbol* formal = prototype->value.p[i];
    TypeRecord* saved = formal != NULL ? formal->type : NULL;
    VectorAppend(saved_types, saved);
    if (saved == NULL) {
      continue;
    }
    TypeRecordIncRef(saved);
    SymbolSetType(formal, ContractParameterViewType(formal));
  }
}

static void LeaveContractParameterView(TypeRecord* func,
                                       Vector* saved_types) {
  Vector* prototype = &func->info.function.prototype;
  for (size_t i = 0; i < prototype->length && i < saved_types->length; i++) {
    Symbol* formal = prototype->value.p[i];
    TypeRecord* saved = saved_types->value.p[i];
    if (formal == NULL || saved == NULL) {
      continue;
    }
    SymbolSetType(formal, saved);
    TypeRecordDecRef(saved);
  }
  VectorDestruct(saved_types);
}

void SemanticAnalyzeFunctionContracts(TypeRecord* func, ASTNode* declaration) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.contract_assertions.length == 0) {
    return;
  }
  FunctionInfo* info = &func->info.function;
  for (size_t i = 0; i < info->contract_assertions.length; i++) {
    ContractAssertion* assertion = info->contract_assertions.value.p[i];
    if (assertion->kind == kContractPostcondition) {
      SetResultBindingType(func, assertion);
      ContractUseCheck check = {
          .func = func,
          .assertion = assertion,
      };
      ASTNodeVisit(assertion->predicate, CheckPostconditionParameterUse, 0,
                   &check);
    }
    Vector saved_parameter_types = {0};
    EnterContractParameterView(func, &saved_parameter_types);
    assertion->predicate = AnalyzeExpression(assertion->predicate);
    LeaveContractParameterView(func, &saved_parameter_types);
    if (assertion->predicate == NULL) {
      SemanticError(declaration, "invalid contract assertion predicate");
      continue;
    }
    SemanticConvertType(assertion->predicate,
                        NewTypeRecordWithSize(kTypeBool, kQualPlain),
                        kConvertContextualBool);
    assertion->predicate =
        AppendCXXFullExpressionTemporaryDestructors(assertion->predicate);
  }
}

void SemanticAnalyzeContractAssert(ContractAssertASTNode* node) {
  node->predicate = AnalyzeExpression(node->predicate);
  if (node->predicate == NULL) {
    SemanticError((ASTNode*)node, "invalid contract assertion predicate");
    return;
  }
  SemanticConvertType(node->predicate,
                      NewTypeRecordWithSize(kTypeBool, kQualPlain),
                      kConvertContextualBool);
  node->predicate =
      AppendCXXFullExpressionTemporaryDestructors(node->predicate);
}
