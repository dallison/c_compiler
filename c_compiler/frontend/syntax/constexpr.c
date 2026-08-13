//
//  constexpr.c
//  c_compiler
//
//  Created by David Allison on 11/3/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "constexpr.h"
#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "constexpr_pcode.h"
#include "errors.h"
#include "expr_semantics.h"
#include "semantics.h"
#include "type.h"
#include "type_internal.h"
#include "type_template.h"

typedef struct ConstexprBinding ConstexprBinding;

static void ReportConstexprPCodeFailure(ASTNode* node, bool always) {
  if (always ||
      (compiler->current_function == NULL &&
       compiler->constant_evaluation_required_depth > 0)) {
    SemanticError(node, "constexpr pcode evaluation failed: %s",
                  ConstexprPCodeFailureReason());
  }
}

struct ConstexprValue {
  bool is_object;
  bool is_address;
  bool is_floating;
  int64_t ivalue;
  double fvalue;
  ConstexprObject* object;
  ConstexprBinding* address_binding;
  ConstexprValue* address_slot;
  ConstexprObject* address_object;
  size_t address_index;
};

struct ConstexprBinding {
  Symbol* symbol;
  bool is_address;
  bool is_floating;
  int64_t ivalue;
  double fvalue;
  ConstexprObject* object;
  ConstexprBinding* address_binding;
  ConstexprValue* address_slot;
  ConstexprObject* address_object;
  size_t address_index;
};

struct ConstexprObject {
  TypeRecord* type;
  Vector slots;  // ConstexprValue*
  StructMember* active_union_member;
};

#define CONSTEXPR_MAX_CALL_DEPTH 512
#define CONSTEXPR_MAX_STEPS 1000000

static bool ConstexprDereferenceAddress(ConstexprValue address,
                                        ConstexprValue* result);
static ConstexprValue* ConstexprObjectSlot(ConstexprObject* object,
                                           size_t index);

typedef enum {
  kConstexprStmtInvalid,
  kConstexprStmtNormal,
  kConstexprStmtReturn,
  kConstexprStmtBreak,
  kConstexprStmtContinue,
} ConstexprStatementResult;

void ConstEvalContextInit(ConstEvalContext* ctx) {
  VectorInit(&ctx->bindings);
  VectorInit(&ctx->objects);
  ctx->call_depth = 0;
  ctx->steps = 0;
  ctx->max_steps = CONSTEXPR_MAX_STEPS;
}

void ConstEvalContextDestruct(ConstEvalContext* ctx) {
  for (size_t i = 0; i < ctx->bindings.length; i++) {
    free(ctx->bindings.value.p[i]);
  }
  VectorDestruct(&ctx->bindings);
  for (size_t i = 0; i < ctx->objects.length; i++) {
    ConstexprObject* object = ctx->objects.value.p[i];
    for (size_t j = 0; j < object->slots.length; j++) {
      free(object->slots.value.p[j]);
    }
    VectorDestruct(&object->slots);
    free(object);
  }
  VectorDestruct(&ctx->objects);
}

bool ConstEvalStep(ConstEvalContext* ctx) {
  ctx->steps++;
  return ctx->steps <= ctx->max_steps;
}

static ConstexprBinding* FindConstexprBinding(ConstEvalContext* ctx,
                                              Symbol* symbol) {
  if (symbol == NULL) {
    return NULL;
  }
  for (size_t i = ctx->bindings.length; i > 0; i--) {
    ConstexprBinding* binding = ctx->bindings.value.p[i - 1];
    if (binding->symbol == symbol) {
      return binding;
    }
  }
  // A function-template body can retain the primary template's formal symbols
  // while the instantiated function type owns cloned formals.  They denote the
  // same argument slot even though their Symbol pointers differ.  Resolve that
  // slot in the innermost active call frame by argument number and name.
  if (symbol->flags.is_argument) {
    for (size_t i = ctx->bindings.length; i > 0; i--) {
      ConstexprBinding* binding = ctx->bindings.value.p[i - 1];
      if (binding->symbol != NULL && binding->symbol->flags.is_argument &&
          binding->symbol->value.arg_number == symbol->value.arg_number &&
          StringEqual(&binding->symbol->name, symbol->name.value)) {
        return binding;
      }
    }
  }
  return NULL;
}

bool ConstexprHasBinding(ConstEvalContext* ctx, Symbol* symbol) {
  return ctx != NULL && FindConstexprBinding(ctx, symbol) != NULL;
}

static void PushConstexprBinding(ConstEvalContext* ctx, Symbol* symbol,
                                 ConstexprValue value) {
  ConstexprBinding* binding = malloc(sizeof(ConstexprBinding));
  binding->symbol = symbol;
  binding->is_address = value.is_address;
  binding->is_floating = value.is_floating;
  binding->ivalue = value.ivalue;
  binding->fvalue = value.fvalue;
  binding->object = value.object;
  binding->address_binding = value.address_binding;
  binding->address_slot = value.address_slot;
  binding->address_object = value.address_object;
  binding->address_index = value.address_index;
  VectorAppend(&ctx->bindings, binding);
}

bool ConstexprValueAsInteger(ConstexprValue value, int64_t* result) {
  if (value.is_address) {
    ConstexprValue dereferenced;
    return ConstexprDereferenceAddress(value, &dereferenced) &&
           ConstexprValueAsInteger(dereferenced, result);
  }
  if (value.is_object) {
    return false;
  }
  *result = value.is_floating ? (int64_t)value.fvalue : value.ivalue;
  return true;
}

bool ConstexprValueAsFloating(ConstexprValue value, double* result) {
  if (value.is_address) {
    ConstexprValue dereferenced;
    return ConstexprDereferenceAddress(value, &dereferenced) &&
           ConstexprValueAsFloating(dereferenced, result);
  }
  if (value.is_object) {
    return false;
  }
  *result = value.is_floating ? value.fvalue : (double)value.ivalue;
  return true;
}

bool ConstexprBindingAsInteger(ConstEvalContext* ctx, Symbol* symbol,
                               int64_t* result) {
  ConstexprBinding* binding = FindConstexprBinding(ctx, symbol);
  if (binding == NULL || binding->object != NULL) {
    return false;
  }
  if (binding->is_address) {
    if (binding->address_object != NULL) {
      ConstexprValue* value = ConstexprObjectSlot(binding->address_object,
                                                  binding->address_index);
      return value != NULL && ConstexprValueAsInteger(*value, result);
    }
    ConstexprValue* value = binding->address_slot;
    if (value != NULL) {
      return ConstexprValueAsInteger(*value, result);
    }
    if (binding->address_binding != NULL &&
        binding->address_binding->object == NULL &&
        !binding->address_binding->is_address) {
      *result = binding->address_binding->is_floating
          ? (int64_t)binding->address_binding->fvalue
          : binding->address_binding->ivalue;
      return true;
    }
    return false;
  }
  *result = binding->is_floating ? (int64_t)binding->fvalue : binding->ivalue;
  return true;
}

bool ConstexprBindingAsFloating(ConstEvalContext* ctx, Symbol* symbol,
                                double* result) {
  ConstexprBinding* binding = FindConstexprBinding(ctx, symbol);
  if (binding == NULL || binding->object != NULL) {
    return false;
  }
  if (binding->is_address) {
    if (binding->address_object != NULL) {
      ConstexprValue* value = ConstexprObjectSlot(binding->address_object,
                                                  binding->address_index);
      return value != NULL && ConstexprValueAsFloating(*value, result);
    }
    ConstexprValue* value = binding->address_slot;
    if (value != NULL) {
      return ConstexprValueAsFloating(*value, result);
    }
    if (binding->address_binding != NULL &&
        binding->address_binding->object == NULL &&
        !binding->address_binding->is_address) {
      *result = binding->address_binding->is_floating
          ? binding->address_binding->fvalue
          : (double)binding->address_binding->ivalue;
      return true;
    }
    return false;
  }
  *result = binding->is_floating ? binding->fvalue : (double)binding->ivalue;
  return true;
}

static ConstexprValue* NewConstexprValueSlot(void) {
  ConstexprValue* slot = malloc(sizeof(ConstexprValue));
  *slot = (ConstexprValue){0};
  return slot;
}

static ConstexprObject* NewConstexprObject(ConstEvalContext* ctx,
                                           TypeRecord* type,
                                           size_t slot_count) {
  ConstexprObject* object = malloc(sizeof(ConstexprObject));
  object->type = type;
  object->active_union_member = NULL;
  VectorInit(&object->slots);
  for (size_t i = 0; i < slot_count; i++) {
    VectorAppend(&object->slots, NewConstexprValueSlot());
  }
  if (ctx != NULL) {
    VectorAppend(&ctx->objects, object);
  }
  return object;
}

static ConstexprObject* CloneConstexprObject(ConstEvalContext* ctx,
                                             ConstexprObject* object) {
  if (object == NULL) {
    return NULL;
  }
  ConstexprObject* clone =
      NewConstexprObject(ctx, object->type, object->slots.length);
  clone->active_union_member = object->active_union_member;
  for (size_t i = 0; i < object->slots.length; i++) {
    ConstexprValue* from = object->slots.value.p[i];
    ConstexprValue* to = clone->slots.value.p[i];
    *to = *from;
    if (from->is_object) {
      to->object = CloneConstexprObject(ctx, from->object);
      if (to->object == NULL) {
        return NULL;
      }
    }
  }
  return clone;
}

static size_t ConstexprObjectSlotCount(TypeRecord* type) {
  if (type != NULL && TypeIsFixedArray(type)) {
    return type->info.array.size.fixed;
  }
  if (type != NULL && TypeIsStructOrUnion(type) &&
      type->info.struct_info != NULL) {
    return type->info.struct_info->is_union ? 1
                                            : type->info.struct_info->members.length;
  }
  return 0;
}

static ConstexprValue* ConstexprObjectSlot(ConstexprObject* object,
                                           size_t index) {
  if (object == NULL || index >= object->slots.length) {
    return NULL;
  }
  return object->slots.value.p[index];
}

static ConstexprObject* ConstexprStringObject(ConstEvalContext* ctx,
                                              ASTNode* node,
                                              TypeRecord* array_type) {
  if (node == NULL || node->op != AST_OP(string) || array_type == NULL ||
      !TypeIsFixedArray(array_type) || !TypeIsCharFamily(array_type->next)) {
    return NULL;
  }
  ConstantASTNode* literal = (ConstantASTNode*)node;
  if (literal->value.string == NULL) {
    return NULL;
  }
  size_t length = literal->value.string->length;
  size_t slot_count = ConstexprObjectSlotCount(array_type);
  if (slot_count < length + 1) {
    slot_count = length + 1;
  }
  ConstexprObject* object =
      NewConstexprObject(ctx, array_type, slot_count);
  for (size_t i = 0; i < object->slots.length; i++) {
    ConstexprValue* slot = ConstexprObjectSlot(object, i);
    slot->ivalue =
        i < length ? (unsigned char)literal->value.string->value[i] : 0;
    slot->fvalue = (double)slot->ivalue;
  }
  return object;
}

static bool ConstexprObjectIsUnion(ConstexprObject* object) {
  return object != NULL && TypeIsStructOrUnion(object->type) &&
         object->type->info.struct_info != NULL &&
         object->type->info.struct_info->is_union;
}

static size_t ConstexprMemberSlotIndex(ConstexprObject* object,
                                       StructMember* member) {
  return ConstexprObjectIsUnion(object) ? 0 : member->index;
}

static bool ConstexprUnionMemberActive(ConstexprObject* object,
                                       StructMember* member) {
  return !ConstexprObjectIsUnion(object) ||
         object->active_union_member == member;
}

static void ConstexprActivateUnionMember(ConstexprObject* object,
                                         StructMember* member) {
  if (ConstexprObjectIsUnion(object)) {
    object->active_union_member = member;
  }
}

static bool ConstexprNullAddress(ConstexprValue* result) {
  *result = (ConstexprValue){.is_address = true};
  return true;
}

static bool ConstexprDereferenceAddress(ConstexprValue address,
                                        ConstexprValue* result) {
  if (!address.is_address) {
    return false;
  }
  if (address.address_object != NULL) {
    ConstexprValue* slot =
        ConstexprObjectSlot(address.address_object, address.address_index);
    if (slot == NULL) {
      return false;
    }
    *result = *slot;
    return true;
  }
  if (address.address_slot != NULL) {
    *result = *address.address_slot;
    return true;
  }
  if (address.address_binding != NULL) {
    ConstexprBinding* binding = address.address_binding;
    *result = (ConstexprValue){
        .is_object = binding->object != NULL,
        .is_address = binding->is_address,
        .is_floating = binding->is_floating,
        .ivalue = binding->ivalue,
        .fvalue = binding->fvalue,
        .object = binding->object,
        .address_binding = binding->address_binding,
        .address_slot = binding->address_slot,
        .address_object = binding->address_object,
        .address_index = binding->address_index,
    };
    return true;
  }
  return false;
}

static bool ConstexprAddressEqual(ConstexprValue left, ConstexprValue right) {
  return left.is_address && right.is_address &&
         left.address_binding == right.address_binding &&
         left.address_slot == right.address_slot &&
         left.address_object == right.address_object &&
         left.address_index == right.address_index;
}

static bool EvaluateConstexprAddressValue(ConstEvalContext* ctx, ASTNode* node,
                                          ConstexprValue* result);

static bool EvaluateConstexprLValue(ConstEvalContext* ctx, ASTNode* node,
                                    ConstexprBinding** binding) {
  if (node == NULL || node->op != AST_OP(identifier)) {
    if (node != NULL && node->op == AST_OP(contents)) {
      UnaryASTNode* contents = (UnaryASTNode*)node;
      ConstexprValue address;
      if (EvaluateConstexprAddressValue(ctx, contents->sub, &address) &&
          address.address_binding != NULL) {
        *binding = address.address_binding;
        return true;
      }
    }
    return false;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  if (id->symbol == NULL || TypeIsConst(id->symbol->type)) {
    return false;
  }
  *binding = FindConstexprBinding(ctx, id->symbol);
  if (*binding != NULL && (*binding)->is_address) {
    if ((*binding)->address_binding != NULL) {
      *binding = (*binding)->address_binding;
      return true;
    }
    return false;
  }
  return *binding != NULL;
}

static bool StoreConstexprBinding(ConstEvalContext* ctx,
                                  ConstexprBinding* binding, TypeRecord* type,
                                  ConstexprValue value) {
  if (binding == NULL) {
    return false;
  }
  if (value.is_address) {
    binding->is_address = true;
    binding->address_binding = value.address_binding;
    binding->address_slot = value.address_slot;
    binding->address_object = value.address_object;
    binding->address_index = value.address_index;
    binding->object = NULL;
    binding->is_floating = false;
    binding->ivalue = 0;
    binding->fvalue = 0;
    return true;
  }
  binding->is_address = false;
  binding->address_binding = NULL;
  binding->address_slot = NULL;
  binding->address_object = NULL;
  binding->address_index = 0;
  if (value.is_object) {
    binding->object = CloneConstexprObject(ctx, value.object);
    binding->is_floating = false;
    binding->ivalue = 0;
    binding->fvalue = 0;
    return binding->object != NULL;
  }
  if (type != NULL && TypeIsFloatingPoint(type)) {
    double fvalue;
    ConstexprValueAsFloating(value, &fvalue);
    binding->object = NULL;
    binding->is_floating = true;
    binding->fvalue = fvalue;
    binding->ivalue = (int64_t)fvalue;
  } else {
    int64_t ivalue;
    ConstexprValueAsInteger(value, &ivalue);
    binding->object = NULL;
    binding->is_floating = false;
    binding->ivalue = ivalue;
    binding->fvalue = (double)ivalue;
  }
  return true;
}

static bool StoreConstexprSlot(ConstEvalContext* ctx, ConstexprValue* slot,
                               TypeRecord* type, ConstexprValue value) {
  if (slot == NULL) {
    return false;
  }
  if (value.is_address) {
    slot->is_object = false;
    slot->is_address = true;
    slot->is_floating = false;
    slot->ivalue = 0;
    slot->fvalue = 0;
    slot->object = NULL;
    slot->address_binding = value.address_binding;
    slot->address_slot = value.address_slot;
    slot->address_object = value.address_object;
    slot->address_index = value.address_index;
    return true;
  }
  slot->is_address = false;
  slot->address_binding = NULL;
  slot->address_slot = NULL;
  slot->address_object = NULL;
  slot->address_index = 0;
  if (value.is_object) {
    slot->is_object = true;
    slot->is_address = false;
    slot->is_floating = false;
    slot->ivalue = 0;
    slot->fvalue = 0;
    slot->object = CloneConstexprObject(ctx, value.object);
    return slot->object != NULL;
  }
  slot->is_object = false;
  slot->object = NULL;
  if (type != NULL && TypeIsFloatingPoint(type)) {
    double fvalue;
    ConstexprValueAsFloating(value, &fvalue);
    slot->is_floating = true;
    slot->fvalue = fvalue;
    slot->ivalue = (int64_t)fvalue;
  } else {
    int64_t ivalue;
    ConstexprValueAsInteger(value, &ivalue);
    slot->is_floating = false;
    slot->ivalue = ivalue;
    slot->fvalue = (double)ivalue;
  }
  return true;
}

static void PopConstexprBindings(ConstEvalContext* ctx, size_t mark) {
  while (ctx->bindings.length > mark) {
    ConstexprBinding* binding =
        ctx->bindings.value.p[ctx->bindings.length - 1];
    free(binding);
    ctx->bindings.length--;
  }
}

static bool EvaluateConstexprValue(ConstEvalContext* ctx, ASTNode* node,
                                   TypeRecord* type,
                                   ConstexprValue* result);
bool EvaluateConstexprCall(ConstEvalContext* ctx, ASTNode* node,
                                  ConstexprValue* result);
static bool EvaluateConstexprConstructorCall(ConstEvalContext* ctx,
                                             ASTNode* node);
static bool EvaluateConstexprDestructorCall(ConstEvalContext* ctx,
                                            ASTNode* node);
static bool EvaluateConstexprObjectExpressionInitializer(ConstEvalContext* ctx,
                                                         TypeRecord* type,
                                                         ASTNode* initializer,
                                                         ConstexprValue* result);
static bool EvaluateConstexprConstructorCallForObject(ConstEvalContext* ctx,
                                                      ASTNode* node,
                                                      ConstexprObject* object);
static bool BindConstexprConstructorObjectActuals(ConstEvalContext* ctx,
                                                  Symbol* function,
                                                  ConstexprObject* object,
                                                  Vector* actuals);
static Symbol* ConstexprCallSymbol(ASTNode* node);
ASTNode* ConstexprInitializerExpression(ASTNode* initializer);
static ASTNode* ConstexprAggregateInitializerExpression(ASTNode* initializer);
static ConstexprStatementResult EvaluateConstexprStatement(
    ConstEvalContext* ctx, ASTNode* stmt, TypeRecord* return_type,
    ConstexprValue* result);
bool EvaluateConstexprMutation(ConstEvalContext* ctx, ASTNode* node,
                                      TypeRecord* type,
                                      ConstexprValue* result);
static bool EvaluateConstexprInitializer(ConstEvalContext* ctx,
                                         TypeRecord* type,
                                         ASTNode* initializer,
                                         ConstexprValue* result);
static bool EvaluateConstexprConvertingConstruction(ConstEvalContext* ctx,
                                                    ASTNode* from,
                                                    TypeRecord* to,
                                                    bool allow_explicit,
                                                    ConstexprValue* result);
static TypeRecord* ConstexprPlainObjectType(TypeRecord* type);
static void ConstexprReleasePlainObjectType(TypeRecord* requested,
                                            TypeRecord* plain);
bool EvaluateConstexprObjectAccess(ConstEvalContext* ctx,
                                          ASTNode* node,
                                          ConstexprValue* result);
static bool EvaluateConstexprObjectLValue(ConstEvalContext* ctx,
                                          ASTNode* node,
                                          ConstexprValue** slot,
                                          bool allow_object);
static bool EvaluateConstexprObjectAddress(ConstEvalContext* ctx,
                                           ASTNode* node,
                                           ConstexprObject** object);
static bool EvaluateConstexprAddressValue(ConstEvalContext* ctx, ASTNode* node,
                                          ConstexprValue* result);
static bool ConstexprDereferenceAddress(ConstexprValue address,
                                        ConstexprValue* result);

static TypeRecord* ConstexprObjectSlotType(TypeRecord* type,
                                           size_t slot_index) {
  if (type == NULL) {
    return NULL;
  }
  if (TypeIsFixedArray(type)) {
    return type->next;
  }
  if (TypeIsStructOrUnion(type) && type->info.struct_info != NULL &&
      slot_index < type->info.struct_info->members.length) {
    StructMember* member = type->info.struct_info->members.value.p[slot_index];
    return member != NULL && member->symbol != NULL ? member->symbol->type
                                                    : NULL;
  }
  return NULL;
}

static bool ConstexprEvaluateObjectConstantForSymbolAST(Symbol* symbol,
                                                        ASTNode* initializer) {
  if (symbol == NULL || symbol->type == NULL || initializer == NULL ||
      (!TypeIsFixedArray(symbol->type) && !TypeIsStructOrUnion(symbol->type))) {
    return false;
  }
  ConstEvalContext ctx;
  ConstEvalContextInit(&ctx);
  ConstexprValue object_value = {0};
  ASTNode* expr = ConstexprInitializerExpression(initializer);
  bool ok = false;
  if (expr != NULL && expr->op == AST_OP(call)) {
    ok = EvaluateConstexprCall(&ctx, expr, &object_value) &&
         object_value.is_object && object_value.object != NULL;
    if (!ok) {
      object_value.is_object = true;
      object_value.object = NewConstexprObject(
          &ctx, symbol->type, ConstexprObjectSlotCount(symbol->type));
      size_t mark = ctx.bindings.length;
      PushConstexprBinding(&ctx, symbol, object_value);
      ok = EvaluateConstexprConstructorCall(&ctx, expr);
      PopConstexprBindings(&ctx, mark);
    }
  } else {
    ok = EvaluateConstexprInitializer(&ctx, symbol->type, initializer,
                                      &object_value);
  }

  if (ok && object_value.object != NULL) {
    symbol->value.other = CloneConstexprObject(NULL, object_value.object);
    symbol->flags.value_set = symbol->value.other != NULL;
    ok = symbol->flags.value_set;
  }
  ConstEvalContextDestruct(&ctx);
  return ok;
}

bool ConstexprEvaluateObjectConstantForSymbol(Symbol* symbol,
                                              ASTNode* initializer) {
  if (symbol == NULL || initializer == NULL) {
    return false;
  }
  ConstexprEvalMode mode = compiler->constexpr_eval_mode;
  bool pcode_attempted = mode != kConstexprEvalAST;
  bool pcode_ok =
      pcode_attempted &&
      ConstexprPCodeEvaluateObjectConstantForSymbol(symbol, initializer);
  if (mode == kConstexprEvalPCode) {
    if (!pcode_ok) {
      ReportConstexprPCodeFailure(initializer, false);
    }
    return pcode_ok;
  }
  if ((mode == kConstexprEvalAuto || mode == kConstexprEvalAudit) &&
      pcode_ok) {
    return true;
  }
  ConstexprObject* pcode_object =
      pcode_ok ? (ConstexprObject*)symbol->value.other : NULL;
  if (pcode_ok) {
    symbol->flags.value_set = false;
    symbol->value.other = NULL;
  }
  if (mode == kConstexprEvalAudit) {
    compiler->constexpr_eval_mode = kConstexprEvalAST;
  }
  bool ast_ok =
      ConstexprEvaluateObjectConstantForSymbolAST(symbol, initializer);
  compiler->constexpr_eval_mode = mode;
  if (pcode_object != NULL) {
    ConstexprPCodeDeleteObject(pcode_object);
  }
  return ast_ok;
}

static ASTNode* ConstexprValueInitializer(ConstexprValue* value,
                                          TypeRecord* type,
                                          SourceLocation location);

static ASTNode* ConstexprObjectInitializer(ConstexprObject* object,
                                           SourceLocation location) {
  if (object == NULL || object->type == NULL) {
    return NULL;
  }
  Vector* initializers = NewVector();
  if (TypeIsFixedArray(object->type)) {
    for (size_t i = 0; i < object->slots.length; i++) {
      ASTNode* init = ConstexprValueInitializer(
          object->slots.value.p[i], object->type->next, location);
      if (init != NULL) {
        VectorAppend(initializers, init);
      }
    }
  } else if (TypeIsStructOrUnion(object->type) &&
             object->type->info.struct_info != NULL) {
    Struct* str = object->type->info.struct_info;
    for (size_t i = 0; i < str->members.length; i++) {
      StructMember* member = str->members.value.p[i];
      if (member == NULL || member->symbol == NULL || member->is_static ||
          member->is_member_function ||
          StorageIs(member->symbol->storage, STO(typedef)) ||
          ConstexprMemberSlotIndex(object, member) >= object->slots.length ||
          (str->is_union && !ConstexprUnionMemberActive(object, member))) {
        continue;
      }
      ASTNode* init = ConstexprValueInitializer(
          object->slots.value.p[ConstexprMemberSlotIndex(object, member)],
          member->symbol->type, location);
      if (init != NULL) {
        VectorAppend(initializers, init);
      }
      if (str->is_union) {
        break;
      }
    }
  }
  return NewBracedInitializerASTNode(initializers, object->type, location);
}

static ASTNode* ConstexprValueInitializer(ConstexprValue* value,
                                          TypeRecord* type,
                                          SourceLocation location) {
  if (value == NULL || type == NULL) {
    return NULL;
  }
  if (value->is_object) {
    return ConstexprObjectInitializer(value->object, location);
  }
  ASTNode* expr = NULL;
  if (TypeIsPointer(type) && value->is_address &&
      value->address_object != NULL &&
      TypeIsFixedArray(value->address_object->type) &&
      TypeIsCharFamily(value->address_object->type->next)) {
    String* contents = NewString("");
    for (size_t i = value->address_index;
         i < value->address_object->slots.length; i++) {
      ConstexprValue* character = value->address_object->slots.value.p[i];
      if (character == NULL || character->ivalue == 0) {
        break;
      }
      StringAppendChar(contents, (char)character->ivalue);
    }
    expr = NewStringConstantASTNode(
        contents, TypeRecordCopy(value->address_object->type), location);
  } else if (TypeIsFloatingPoint(type)) {
    expr = NewRealConstantASTNode(value->fvalue, TypeRecordCopy(type),
                                  location);
  } else {
    expr = NewIntConstantASTNode(value->ivalue, TypeRecordCopy(type),
                                 location);
  }
  return NewExpressionInitializerASTNode(expr, location);
}

ASTNode* ConstexprObjectInitializerForSymbol(Symbol* symbol,
                                             SourceLocation location) {
  if (symbol == NULL || symbol->type == NULL || !symbol->flags.value_set ||
      (!TypeIsFixedArray(symbol->type) && !TypeIsStructOrUnion(symbol->type)) ||
      symbol->value.other == NULL) {
    return NULL;
  }
  return ConstexprObjectInitializer((ConstexprObject*)symbol->value.other,
                                    location);
}

static bool EvaluateConstexprValue(ConstEvalContext* ctx, ASTNode* node,
                                   TypeRecord* type,
                                   ConstexprValue* result) {
  if (node != NULL) {
    switch (node->op) {
      case AST_OP(assign):
      case AST_OP(pluseq):
      case AST_OP(minuseq):
      case AST_OP(multeq):
      case AST_OP(diveq):
      case AST_OP(percenteq):
      case AST_OP(andeq):
      case AST_OP(oreq):
      case AST_OP(exoreq):
      case AST_OP(lshifteq):
      case AST_OP(rshifteq):
      case AST_OP(rshifteql):
      case AST_OP(rshifteqa):
      case AST_OP(preinc):
      case AST_OP(predec):
      case AST_OP(postinc):
      case AST_OP(postdec):
      case AST_OP(comma):
        return EvaluateConstexprMutation(ctx, node, type, result);
      default:
        break;
    }
  }
  if (type != NULL && (TypeIsPointer(type) || TypeIsReference(type))) {
    return EvaluateConstexprAddressValue(ctx, node, result);
  }
  if (node != NULL && node->op == AST_OP(contents)) {
    return EvaluateConstexprAddressValue(ctx, node, result);
  }
  if (type != NULL && (TypeIsFixedArray(type) || TypeIsStructOrUnion(type))) {
    if (node != NULL && node->type != NULL && TypeIsStructOrUnion(type) &&
        TypeIsStructOrUnion(node->type) && !TypeEqual(node->type, type)) {
      TypeRecord* plain_type = ConstexprPlainObjectType(type);
      bool converted = EvaluateConstexprConvertingConstruction(
          ctx, node, plain_type, /*allow_explicit=*/false, result);
      ConstexprReleasePlainObjectType(type, plain_type);
      if (converted) {
        return true;
      }
    }
    return EvaluateConstexprInitializer(ctx, type, node, result);
  }
  if (type != NULL && TypeIsFloatingPoint(type)) {
    double value;
    if (!EvaluateFloatingPointExpressionInContext(ctx, node, &value)) {
      return false;
    }
    result->is_object = false;
    result->is_address = false;
    result->object = NULL;
    result->address_binding = NULL;
    result->address_slot = NULL;
    result->is_floating = true;
    result->fvalue = value;
    result->ivalue = (int64_t)value;
    return true;
  }
  int64_t value;
  if (!EvaluateIntegerExpressionInContext(ctx, node, &value)) {
    return false;
  }
  result->is_object = false;
  result->is_address = false;
  result->object = NULL;
  result->address_binding = NULL;
  result->address_slot = NULL;
  result->is_floating = false;
  result->ivalue = value;
  result->fvalue = (double)value;
  return true;
}

bool ConstexprMaterializeClassArgument(ConstEvalContext* ctx, ASTNode* arg,
                                       TypeRecord* object_type,
                                       ConstexprObject** object) {
  if (ctx == NULL || arg == NULL || object_type == NULL || object == NULL) {
    return false;
  }
  TypeRecord* plain_type = ConstexprPlainObjectType(object_type);
  ConstexprValue value = {0};
  bool ok = EvaluateConstexprValue(ctx, arg, plain_type, &value) &&
            value.is_object && value.object != NULL;
  ConstexprReleasePlainObjectType(object_type, plain_type);
  if (!ok) {
    return false;
  }
  *object = value.object;
  return true;
}

static bool EvaluateConstexprBinaryMutation(ConstEvalContext* ctx,
                                            BinaryASTNode* node,
                                            TypeRecord* type,
                                            ConstexprValue* result) {
  if (node->base.op == AST_OP(comma)) {
    // Recognize the materialized-temporary idiom `(temp.Ctor(args), temp)` (or
    // the reverse) used for class-type prvalue temporaries (e.g. a converting
    // constructor or functional cast).  The constructor mutates `temp` through
    // its `this` binding, so the temporary must be created and bound before the
    // constructor runs; delegate to the object-initializer evaluator which does
    // exactly that.  (The generic comma path below would instead evaluate the
    // constructor call with `temp` still unbound and fail.)
    IdentifierASTNode* temporary = NULL;
    if (node->left != NULL && node->left->op == AST_OP(call) &&
        node->right != NULL && node->right->op == AST_OP(identifier)) {
      temporary = (IdentifierASTNode*)node->right;
    } else if (node->left != NULL && node->left->op == AST_OP(identifier) &&
               node->right != NULL && node->right->op == AST_OP(call)) {
      temporary = (IdentifierASTNode*)node->left;
    }
    if (temporary != NULL && temporary->symbol != NULL &&
        temporary->symbol->type != NULL &&
        (TypeIsStructOrUnion(temporary->symbol->type) ||
         TypeIsFixedArray(temporary->symbol->type))) {
      return EvaluateConstexprObjectExpressionInitializer(
          ctx, temporary->symbol->type, (ASTNode*)node, result);
    }
    ConstexprValue ignored;
    if (node->left != NULL && node->left->type != NULL &&
        !TypeIsVoid(node->left->type) &&
        !EvaluateConstexprValue(ctx, node->left, node->left->type, &ignored)) {
      return false;
    }
    return EvaluateConstexprValue(ctx, node->right, type, result);
  }

  ConstexprBinding* binding = NULL;
  ConstexprValue* slot = NULL;
  bool is_assignment = node->base.op == AST_OP(assign);
  TypeRecord* left_type = node->left->type;
  if (TypeIsReference(left_type)) {
    left_type = left_type->next;
  }
  if (node->left->op == AST_OP(subscript) && left_type != NULL &&
      left_type->declarator == kDeclArray) {
    left_type =
        left_type->next != NULL ? left_type->next : node->right->type;
  }
  if (!EvaluateConstexprLValue(ctx, node->left, &binding) &&
      !EvaluateConstexprObjectLValue(ctx, node->left, &slot, is_assignment)) {
    return false;
  }

  ConstexprValue right;
  if (!EvaluateConstexprValue(ctx, node->right, left_type, &right)) {
    return false;
  }

  if (node->base.op == AST_OP(assign)) {
    bool stored = binding != NULL
        ? StoreConstexprBinding(ctx, binding, left_type, right)
        : StoreConstexprSlot(ctx, slot, left_type, right);
    if (!stored) {
      return false;
    }
    *result = right;
    return true;
  }

  ConstexprValue current = binding != NULL
      ? (ConstexprValue){
            .is_floating = binding->is_floating,
            .ivalue = binding->ivalue,
            .fvalue = binding->fvalue,
        }
      : *slot;

  ConstexprValue next = current;
  if (left_type != NULL && TypeIsFloatingPoint(left_type)) {
    double left;
    double right_value;
    ConstexprValueAsFloating(current, &left);
    ConstexprValueAsFloating(right, &right_value);
    next.is_floating = true;
    switch (node->base.op) {
      case AST_OP(pluseq):
        next.fvalue = left + right_value;
        break;
      case AST_OP(minuseq):
        next.fvalue = left - right_value;
        break;
      case AST_OP(multeq):
        next.fvalue = left * right_value;
        break;
      case AST_OP(diveq):
        if (right_value == 0) {
          return false;
        }
        next.fvalue = left / right_value;
        break;
      default:
        return false;
    }
    next.ivalue = (int64_t)next.fvalue;
  } else {
    int64_t left;
    int64_t right_value;
    ConstexprValueAsInteger(current, &left);
    ConstexprValueAsInteger(right, &right_value);
    next.is_floating = false;
    switch (node->base.op) {
      case AST_OP(pluseq):
        next.ivalue = left + right_value;
        break;
      case AST_OP(minuseq):
        next.ivalue = left - right_value;
        break;
      case AST_OP(multeq):
        next.ivalue = left * right_value;
        break;
      case AST_OP(diveq):
        if (right_value == 0) {
          return false;
        }
        next.ivalue = left / right_value;
        break;
      case AST_OP(percenteq):
        if (right_value == 0) {
          return false;
        }
        next.ivalue = left % right_value;
        break;
      case AST_OP(andeq):
        next.ivalue = left & right_value;
        break;
      case AST_OP(oreq):
        next.ivalue = left | right_value;
        break;
      case AST_OP(exoreq):
        next.ivalue = left ^ right_value;
        break;
      case AST_OP(lshifteq):
        next.ivalue = left << right_value;
        break;
      case AST_OP(rshifteq):
      case AST_OP(rshifteqa):
        next.ivalue = left >> right_value;
        break;
      case AST_OP(rshifteql):
        next.ivalue = (uint64_t)left >> right_value;
        break;
      default:
        return false;
    }
    next.fvalue = (double)next.ivalue;
  }

  bool stored = binding != NULL
      ? StoreConstexprBinding(ctx, binding, left_type, next)
      : StoreConstexprSlot(ctx, slot, left_type, next);
  if (!stored) {
    return false;
  }
  *result = next;
  return true;
}

static bool EvaluateConstexprIncrement(ConstEvalContext* ctx,
                                       UnaryASTNode* node,
                                       ConstexprValue* result) {
  ConstexprBinding* binding = NULL;
  ConstexprValue* slot = NULL;
  if (!EvaluateConstexprLValue(ctx, node->sub, &binding) &&
      !EvaluateConstexprObjectLValue(ctx, node->sub, &slot, false)) {
    return false;
  }
  ConstexprValue old_value = binding != NULL
      ? (ConstexprValue){
            .is_floating = binding->is_floating,
            .ivalue = binding->ivalue,
            .fvalue = binding->fvalue,
        }
      : *slot;
  ConstexprValue new_value = old_value;
  bool increment = node->base.op == AST_OP(preinc) ||
                   node->base.op == AST_OP(postinc);
  if (node->sub->type != NULL && TypeIsFloatingPoint(node->sub->type)) {
    double value;
    ConstexprValueAsFloating(old_value, &value);
    new_value.is_floating = true;
    new_value.fvalue = increment ? value + 1 : value - 1;
    new_value.ivalue = (int64_t)new_value.fvalue;
  } else {
    int64_t value;
    ConstexprValueAsInteger(old_value, &value);
    new_value.is_floating = false;
    new_value.ivalue = increment ? value + 1 : value - 1;
    new_value.fvalue = (double)new_value.ivalue;
  }
  bool stored = binding != NULL
      ? StoreConstexprBinding(ctx, binding, node->sub->type, new_value)
      : StoreConstexprSlot(ctx, slot, node->sub->type, new_value);
  if (!stored) {
    return false;
  }
  *result = (node->base.op == AST_OP(postinc) ||
             node->base.op == AST_OP(postdec))
                ? old_value
                : new_value;
  return true;
}

bool EvaluateConstexprMutation(ConstEvalContext* ctx, ASTNode* node,
                                      TypeRecord* type,
                                      ConstexprValue* result) {
  if (node == NULL || !ConstEvalStep(ctx)) {
    return false;
  }
  switch (node->op) {
    case AST_OP(assign):
    case AST_OP(pluseq):
    case AST_OP(minuseq):
    case AST_OP(multeq):
    case AST_OP(diveq):
    case AST_OP(percenteq):
    case AST_OP(andeq):
    case AST_OP(oreq):
    case AST_OP(exoreq):
    case AST_OP(lshifteq):
    case AST_OP(rshifteq):
    case AST_OP(rshifteql):
    case AST_OP(rshifteqa):
    case AST_OP(comma):
      return EvaluateConstexprBinaryMutation(ctx, (BinaryASTNode*)node, type,
                                             result);
    case AST_OP(preinc):
    case AST_OP(predec):
    case AST_OP(postinc):
    case AST_OP(postdec):
      return EvaluateConstexprIncrement(ctx, (UnaryASTNode*)node, result);
    default:
      return false;
  }
}

bool ConstexprEvaluateMutationAsInteger(ConstEvalContext* ctx, ASTNode* node,
                                        TypeRecord* type, int64_t* result) {
  ConstexprValue value;
  return EvaluateConstexprMutation(ctx, node, type, &value) &&
         ConstexprValueAsInteger(value, result);
}

bool ConstexprEvaluateMutationAsFloating(ConstEvalContext* ctx, ASTNode* node,
                                         TypeRecord* type, double* result) {
  ConstexprValue value;
  return EvaluateConstexprMutation(ctx, node, type, &value) &&
         ConstexprValueAsFloating(value, result);
}

static StructMember* ConstexprDesignatorMember(TypeRecord* type,
                                               Designator* designator) {
  if (type == NULL || designator == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL ||
      designator->designator_type != kDesignatorStruct) {
    return NULL;
  }
  if (designator->is_resolved_member) {
    return designator->value.struct_member;
  }
  if (designator->value.struct_member_name == NULL) {
    return NULL;
  }
  return FindStructMember(type->info.struct_info,
                          designator->value.struct_member_name);
}

static bool ConstexprDesignatorSlotIndex(TypeRecord* type, Designator* designator,
                                         size_t* slot_index) {
  if (type == NULL || designator == NULL || slot_index == NULL) {
    return false;
  }
  if (designator->designator_type == kDesignatorArray) {
    if (!TypeIsFixedArray(type) || designator->value.array_index < 0) {
      return false;
    }
    *slot_index = (size_t)designator->value.array_index;
    return true;
  }
  if (designator->designator_type == kDesignatorBase) {
    return false;
  }
  StructMember* member = ConstexprDesignatorMember(type, designator);
  if (member == NULL) {
    return false;
  }
  *slot_index = type->info.struct_info->is_union
      ? 0
      : member->index;
  return true;
}

static bool EvaluateConstexprDesignatedInitializer(ConstEvalContext* ctx,
                                                   TypeRecord* type,
                                                   ConstexprObject* object,
                                                   Vector* designators,
                                                   size_t designator_index,
                                                   ASTNode* initializer) {
  if (type == NULL || object == NULL || designators == NULL ||
      designator_index >= designators->length) {
    return false;
  }

  size_t slot_index;
  Designator* designator = designators->value.p[designator_index];
  if (!ConstexprDesignatorSlotIndex(
          type, designator, &slot_index)) {
    return false;
  }
  ConstexprValue* slot = ConstexprObjectSlot(object, slot_index);
  TypeRecord* slot_type = ConstexprObjectSlotType(type, slot_index);
  if (TypeIsStructOrUnion(type) && type->info.struct_info != NULL &&
      type->info.struct_info->is_union &&
      designator->designator_type == kDesignatorStruct) {
    StructMember* member = ConstexprDesignatorMember(type, designator);
    if (member == NULL || member->symbol == NULL) {
      return false;
    }
    slot_type = member->symbol->type;
    ConstexprActivateUnionMember(object, member);
  }
  if (slot == NULL || slot_type == NULL) {
    return false;
  }

  if (designator_index + 1 == designators->length) {
    return EvaluateConstexprInitializer(ctx, slot_type, initializer, slot);
  }

  if (!TypeIsFixedArray(slot_type) && !TypeIsStructOrUnion(slot_type)) {
    return false;
  }
  if (!slot->is_object || slot->object == NULL) {
    slot->is_object = true;
    slot->is_address = false;
    slot->is_floating = false;
    slot->ivalue = 0;
    slot->fvalue = 0;
    slot->object = NewConstexprObject(ctx, slot_type,
                                      ConstexprObjectSlotCount(slot_type));
  }
  return EvaluateConstexprDesignatedInitializer(
      ctx, slot_type, slot->object, designators, designator_index + 1,
      initializer);
}

static bool EvaluateConstexprObjectExpressionInitializer(ConstEvalContext* ctx,
                                                        TypeRecord* type,
                                                        ASTNode* initializer,
                                                        ConstexprValue* result) {
  if (initializer == NULL || type == NULL ||
      (!TypeIsFixedArray(type) && !TypeIsStructOrUnion(type))) {
    return false;
  }

  if (initializer->op == AST_OP(comma)) {
    BinaryASTNode* comma = (BinaryASTNode*)initializer;
    ASTNode* constructor = NULL;
    IdentifierASTNode* temporary = NULL;
    if (comma->left != NULL && comma->left->op == AST_OP(call) &&
        comma->right != NULL && comma->right->op == AST_OP(identifier)) {
      constructor = comma->left;
      temporary = (IdentifierASTNode*)comma->right;
    } else if (comma->left != NULL &&
               comma->left->op == AST_OP(identifier) &&
               comma->right != NULL &&
               comma->right->op == AST_OP(call)) {
      temporary = (IdentifierASTNode*)comma->left;
      constructor = comma->right;
    } else {
      return false;
    }
    result->is_object = true;
    result->is_address = false;
    result->is_floating = false;
    result->ivalue = 0;
    result->fvalue = 0;
    result->object = NewConstexprObject(ctx, type,
                                        ConstexprObjectSlotCount(type));
    size_t mark = ctx->bindings.length;
    PushConstexprBinding(ctx, temporary->symbol, *result);
    bool ok = EvaluateConstexprConstructorCall(ctx, constructor);
    PopConstexprBindings(ctx, mark);
    return ok;
  }

  if (initializer->op == AST_OP(call)) {
    (void)ConstexprFunctionDefinition(ConstexprCallSymbol(initializer));
    ConstexprObject* pcode_object = NULL;
    if (ConstexprPCodeEvaluateCallObjectResult(ctx, initializer,
                                              &pcode_object)) {
      result->is_object = true;
      result->is_address = false;
      result->is_floating = false;
      result->ivalue = 0;
      result->fvalue = 0;
      result->object = CloneConstexprObject(ctx, pcode_object);
      ConstexprPCodeDeleteObject(pcode_object);
      return result->object != NULL;
    }
    ConstexprValue value;
    if (EvaluateConstexprCall(ctx, initializer, &value) &&
        value.is_object && value.object != NULL) {
      result->is_object = true;
      result->is_address = false;
      result->is_floating = false;
      result->ivalue = 0;
      result->fvalue = 0;
      result->object = CloneConstexprObject(ctx, value.object);
      return result->object != NULL;
    }
    result->is_object = true;
    result->is_address = false;
    result->is_floating = false;
    result->ivalue = 0;
    result->fvalue = 0;
    result->object = NewConstexprObject(ctx, type,
                                        ConstexprObjectSlotCount(type));
    return EvaluateConstexprConstructorCallForObject(ctx, initializer,
                                                     result->object);
  }

  ConstexprValue value;
  if (EvaluateConstexprObjectAccess(ctx, initializer, &value) &&
      value.is_object && value.object != NULL) {
    result->is_object = true;
    result->is_address = false;
    result->is_floating = false;
    result->ivalue = 0;
    result->fvalue = 0;
    result->object = CloneConstexprObject(ctx, value.object);
    return result->object != NULL;
  }

  return false;
}

// Applies the default member initializers of the members a braced aggregate
// initializer left out ([dcl.init.aggr]/5).  A member with no default member
// initializer keeps the zeroed slot the object was created with, which is what
// value initialization yields for the scalars this evaluator models.
static bool ApplyConstexprDefaultMemberInitializers(ConstEvalContext* ctx,
                                                    TypeRecord* type,
                                                    ConstexprObject* object,
                                                    const bool* initialized,
                                                    size_t slot_count) {
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL ||
      type->info.struct_info->is_union || initialized == NULL) {
    return true;
  }
  Struct* str = type->info.struct_info;
  for (size_t i = 0; i < str->members.length && i < slot_count; i++) {
    StructMember* member = str->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function || member->is_using_declaration ||
        member->default_initializer == NULL || initialized[i]) {
      continue;
    }
    ConstexprValue* slot = ConstexprObjectSlot(object, i);
    if (slot == NULL ||
        !EvaluateConstexprInitializer(ctx, member->symbol->type,
                                      member->default_initializer, slot)) {
      return false;
    }
  }
  return true;
}

static bool EvaluateConstexprInitializer(ConstEvalContext* ctx,
                                         TypeRecord* type,
                                         ASTNode* initializer,
                                         ConstexprValue* result) {
  // For aggregate targets, peel transparent initializer wrappers (including a
  // compound literal `T{...}` produced for `return T{...};`) while preserving a
  // braced initializer.  ConstexprInitializerExpression would otherwise collapse
  // a single-element brace `{x}` into `x`, which loses the aggregate structure
  // for a one-member struct or one-element array.
  if (type != NULL && (TypeIsFixedArray(type) || TypeIsStructOrUnion(type))) {
    ASTNode* aggregate = ConstexprAggregateInitializerExpression(initializer);
    initializer = aggregate != NULL
                      ? aggregate
                      : ConstexprInitializerExpression(initializer);
  } else {
    initializer = ConstexprInitializerExpression(initializer);
  }
  if (initializer == NULL || type == NULL) {
    return false;
  }
  if (TypeIsIntegral(type) || TypeIsFloatingPoint(type)) {
    return EvaluateConstexprValue(ctx, initializer, type, result);
  }
  if (!TypeIsFixedArray(type) && !TypeIsStructOrUnion(type)) {
    return false;
  }
  if (TypeIsFixedArray(type) && initializer->op == AST_OP(string)) {
    ConstexprObject* object = ConstexprStringObject(ctx, initializer, type);
    if (object == NULL) {
      return false;
    }
    *result = (ConstexprValue){.is_object = true, .object = object};
    return true;
  }
  // Materializing a class prvalue can produce a compound literal whose
  // normalized braced initializer contains one designated entry initialized
  // by the complete source object. Treat that entry as whole-object
  // initialization, not as an initializer for the first scalar member.
  if (initializer->op == AST_OP(braced_init)) {
    BracedInitializerASTNode* braced =
        (BracedInitializerASTNode*)initializer;
    if (braced->initializers != NULL &&
        braced->initializers->length == 1) {
      ASTNode* entry = braced->initializers->value.p[0];
      if (entry != NULL && entry->op == AST_OP(designated_init)) {
        ASTNode* whole =
            ((DesignatedInitializerASTNode*)entry)->init;
        if (whole != NULL && whole->type != NULL &&
            TypeEqual(whole->type, type) &&
            EvaluateConstexprObjectExpressionInitializer(ctx, type, whole,
                                                         result)) {
          return true;
        }
      }
    }
  }
  if (EvaluateConstexprObjectExpressionInitializer(ctx, type, initializer,
                                                  result)) {
    return true;
  }
  size_t slot_count = ConstexprObjectSlotCount(type);
  ConstexprObject* object = NewConstexprObject(ctx, type, slot_count);
  result->is_object = true;
  result->is_address = false;
  result->is_floating = false;
  result->ivalue = 0;
  result->fvalue = 0;
  result->object = object;

  if (initializer->op != AST_OP(braced_init)) {
    return false;
  }
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)initializer;
  bool* initialized = slot_count > 0 ? calloc(slot_count, sizeof(bool)) : NULL;
  bool ok = slot_count == 0 || initialized != NULL;
  size_t next_index = 0;
  for (size_t i = 0; ok && i < braced->initializers->length; i++) {
    ASTNode* entry = braced->initializers->value.p[i];
    size_t slot_index = next_index;
    ASTNode* entry_init = entry;
    bool is_designated_entry = false;
    if (entry != NULL && entry->op == AST_OP(designated_init)) {
      is_designated_entry = true;
      DesignatedInitializerASTNode* designated =
          (DesignatedInitializerASTNode*)entry;
      if (designated->designators == NULL ||
          designated->designators->length == 0) {
        ok = false;
        break;
      }
      if (!ConstexprDesignatorSlotIndex(
              type, designated->designators->value.p[0], &slot_index)) {
        ok = false;
        break;
      }
      if (designated->designators->length > 1) {
        if (!EvaluateConstexprDesignatedInitializer(
                ctx, type, object, designated->designators, 0,
                designated->init)) {
          ok = false;
          break;
        }
        if (slot_index < slot_count) {
          initialized[slot_index] = true;
        }
        next_index = slot_index + 1;
        continue;
      }
      entry_init = designated->init;
    }
    ConstexprValue* slot = ConstexprObjectSlot(object, slot_index);
    if (slot == NULL) {
      ok = false;
      break;
    }
    TypeRecord* slot_type = ConstexprObjectSlotType(type, slot_index);
    if (!is_designated_entry &&
        TypeIsStructOrUnion(type) && type->info.struct_info != NULL &&
        type->info.struct_info->is_union) {
      if (i > 0 || type->info.struct_info->members.length == 0) {
        ok = false;
        break;
      }
      StructMember* member = type->info.struct_info->members.value.p[0];
      if (member == NULL || member->symbol == NULL) {
        ok = false;
        break;
      }
      slot_type = member->symbol->type;
      ConstexprActivateUnionMember(object, member);
    } else if (is_designated_entry &&
               TypeIsStructOrUnion(type) &&
               type->info.struct_info != NULL &&
               type->info.struct_info->is_union &&
               entry != NULL && entry->op == AST_OP(designated_init)) {
      DesignatedInitializerASTNode* designated =
          (DesignatedInitializerASTNode*)entry;
      Designator* designator = designated->designators->value.p[0];
      StructMember* member = ConstexprDesignatorMember(type, designator);
      if (member != NULL && member->symbol != NULL) {
        slot_type = member->symbol->type;
        ConstexprActivateUnionMember(object, member);
      }
    }
    if (!EvaluateConstexprInitializer(ctx, slot_type, entry_init, slot)) {
      ok = false;
      break;
    }
    if (slot_index < slot_count) {
      initialized[slot_index] = true;
    }
    next_index = slot_index + 1;
  }
  if (ok) {
    ok = ApplyConstexprDefaultMemberInitializers(ctx, type, object, initialized,
                                                 slot_count);
  }
  free(initialized);
  return ok;
}

ASTNode* ConstexprObjectInitializerForExpression(TypeRecord* type,
                                                 ASTNode* expression) {
  if (type == NULL || expression == NULL ||
      (!TypeIsFixedArray(type) && !TypeIsStructOrUnion(type))) {
    return NULL;
  }
  ConstEvalContext ctx;
  ConstEvalContextInit(&ctx);
  ConstexprValue value = {0};
  bool ok = EvaluateConstexprInitializer(&ctx, type, expression, &value) &&
            value.is_object && value.object != NULL;
  ASTNode* initializer =
      ok ? ConstexprValueInitializer(&value, type, expression->location) : NULL;
  ConstEvalContextDestruct(&ctx);
  return initializer;
}

bool EvaluateConstexprObjectAccess(ConstEvalContext* ctx,
                                          ASTNode* node,
                                          ConstexprValue* result) {
  if (node == NULL) {
    return false;
  }
  if (node->op == AST_OP(string)) {
    TypeRecord* type = node->type;
    if (type != NULL && TypeIsPointer(type)) {
      type = type->next;
    }
    ConstexprObject* object = ConstexprStringObject(ctx, node, type);
    if (object == NULL) {
      return false;
    }
    *result = (ConstexprValue){.is_object = true, .object = object};
    return true;
  }
  if (node->op == AST_OP(call)) {
    // A constexpr function, operator, or conversion that returns a class or
    // array by value produces an object we can subsequently access members of
    // (e.g. `make().field` or `widen(x) < 0`, where `< 0` is a member operator
    // whose receiver is the by-value result of `widen`).
    ConstexprValue value;
    if (!EvaluateConstexprCall(ctx, node, &value) || !value.is_object ||
        value.object == NULL) {
      return false;
    }
    *result = value;
    return true;
  }
  if (node->op == AST_OP(compound_literal)) {
    // A materialized aggregate temporary, e.g. the `S{...}` receiver of a member
    // call.  Build its object from the compound literal's braced initializer.
    if (node->type == NULL ||
        (!TypeIsStructOrUnion(node->type) && !TypeIsFixedArray(node->type))) {
      return false;
    }
    if (!EvaluateConstexprInitializer(ctx, node->type, node, result) ||
        !result->is_object || result->object == NULL) {
      return false;
    }
    return true;
  }
  if (node->op == AST_OP(cast)) {
    // A cast to a class/array type is transparent for object access: any
    // user-defined conversion has already been spliced onto the operand (which
    // therefore already has the target object type), e.g. `(W)s` becomes a cast
    // wrapping the `s.operator W()` call.  Read the object off the operand.
    if (node->type == NULL ||
        (!TypeIsStructOrUnion(node->type) && !TypeIsFixedArray(node->type))) {
      return false;
    }
    ASTNode* operand = ((CastASTNode*)node)->expr;
    if (EvaluateConstexprObjectAccess(ctx, operand, result)) {
      return true;
    }
    if (operand != NULL &&
        EvaluateConstexprConvertingConstruction(
            ctx, operand, node->type, /*allow_explicit=*/true, result)) {
      return true;
    }
    return operand != NULL &&
           EvaluateConstexprValue(ctx, operand, node->type, result) &&
           result->is_object && result->object != NULL;
  }
  if (node->op == AST_OP(spaceship)) {
    // The built-in three-way comparison of scalar operands yields a comparison
    // category temporary (strong_ordering / partial_ordering, etc.).  Codegen
    // stores the ordering value straight into the single `_v` member; mirror
    // that here so that e.g. `(a <=> b) < 0` and `std::is_lt(a <=> b)` are
    // constant expressions.  Encoding: less = -1, equal = 0, greater = 1,
    // unordered = 2 (see <compare>).
    BinaryASTNode* cmp = (BinaryASTNode*)node;
    if (node->type == NULL || !TypeIsStructOrUnion(node->type) ||
        cmp->left == NULL || cmp->right == NULL) {
      return false;
    }
    int64_t v;
    if (TypeIsFloatingPoint(cmp->left->type) ||
        TypeIsFloatingPoint(cmp->right->type)) {
      double a, b;
      if (!EvaluateFloatingPointExpressionInContext(ctx, cmp->left, &a) ||
          !EvaluateFloatingPointExpressionInContext(ctx, cmp->right, &b)) {
        return false;
      }
      v = a < b ? -1 : a > b ? 1 : a == b ? 0 : 2;
    } else {
      int64_t a, b;
      if (!EvaluateIntegerExpressionInContext(ctx, cmp->left, &a) ||
          !EvaluateIntegerExpressionInContext(ctx, cmp->right, &b)) {
        return false;
      }
      if (TypeIsUnsigned(cmp->left->type) || TypeIsUnsigned(cmp->right->type)) {
        uint64_t ua = (uint64_t)a;
        uint64_t ub = (uint64_t)b;
        v = ua < ub ? -1 : ua > ub ? 1 : 0;
      } else {
        v = a < b ? -1 : a > b ? 1 : 0;
      }
    }
    ConstexprObject* object =
        NewConstexprObject(ctx, node->type, ConstexprObjectSlotCount(node->type));
    ConstexprValue* slot = ConstexprObjectSlot(object, 0);
    if (slot == NULL) {
      return false;
    }
    slot->is_object = false;
    slot->is_address = false;
    slot->is_floating = false;
    slot->ivalue = v;
    slot->fvalue = 0;
    slot->object = NULL;
    result->is_object = true;
    result->is_address = false;
    result->is_floating = false;
    result->ivalue = 0;
    result->fvalue = 0;
    result->object = object;
    return true;
  }
  if (node->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node;
    ConstexprBinding* binding = FindConstexprBinding(ctx, id->symbol);
    if (binding == NULL && id->symbol != NULL &&
        !id->symbol->flags.value_set &&
        id->symbol->constexpr_initializer != NULL &&
        (TypeIsFixedArray(id->symbol->type) ||
         TypeIsStructOrUnion(id->symbol->type))) {
      ConstexprEvaluateObjectConstantForSymbol(
          id->symbol, id->symbol->constexpr_initializer);
    }
    if (binding == NULL && id->symbol != NULL &&
        id->symbol->flags.value_set &&
        (TypeIsFixedArray(id->symbol->type) ||
         TypeIsStructOrUnion(id->symbol->type))) {
      result->is_object = true;
      result->is_address = false;
      result->is_floating = false;
      result->ivalue = 0;
      result->fvalue = 0;
      result->object = id->symbol->value.other;
      return result->object != NULL;
    }
    if (binding == NULL) {
      return false;
    }
    ConstexprValue bound = {
        .is_object = binding->object != NULL,
        .is_address = binding->is_address,
        .object = binding->object,
        .address_binding = binding->address_binding,
        .address_slot = binding->address_slot,
    };
    // References can be forwarded through several constexpr calls, forming a
    // chain such as `d -> other -> lhs -> temporary`. Follow the complete
    // chain rather than only one address_binding edge.
    while (bound.is_address) {
      ConstexprValue dereferenced;
      if (!ConstexprDereferenceAddress(bound, &dereferenced)) {
        return false;
      }
      bound = dereferenced;
    }
    ConstexprObject* bound_object = bound.object;
    if (bound_object == NULL) {
      return false;
    }
    result->is_object = true;
    result->is_address = false;
    result->is_floating = false;
    result->ivalue = 0;
    result->fvalue = 0;
    result->object = bound_object;
    return true;
  }
  if (node->op == AST_OP(subscript) &&
      ASTNodeGetShape(node) == kASTShapeBinary) {
    BinaryASTNode* subscript = (BinaryASTNode*)node;
    ConstexprValue object_value;
    int64_t index;
    if (!EvaluateIntegerExpressionInContext(ctx, subscript->right, &index)) {
      return false;
    }
    ConstexprValue* slot = NULL;
    if (EvaluateConstexprObjectAccess(ctx, subscript->left, &object_value) &&
        object_value.object != NULL) {
      if (index < 0) {
        return false;
      }
      slot = ConstexprObjectSlot(object_value.object, (size_t)index);
    } else {
      ConstexprValue address;
      if (!EvaluateConstexprAddressValue(ctx, subscript->left, &address) ||
          address.address_object == NULL || index < 0 ||
          (uint64_t)index > SIZE_MAX - address.address_index) {
        return false;
      }
      slot = ConstexprObjectSlot(address.address_object,
                                 address.address_index + (size_t)index);
    }
    if (slot == NULL) {
      return false;
    }
    *result = *slot;
    return true;
  }
  if (node->op == AST_OP(dot) || node->op == AST_OP(arrow)) {
    BinaryASTNode* member_access = (BinaryASTNode*)node;
    if (member_access->right == NULL ||
        member_access->right->op != AST_OP(structmember)) {
      return false;
    }
    ConstexprValue object_value;
    if (!EvaluateConstexprObjectAccess(ctx, member_access->left,
                                       &object_value) ||
        object_value.object == NULL) {
      return false;
    }
    StructMemberASTNode* member_node =
        (StructMemberASTNode*)member_access->right;
    if (member_node->member == NULL) {
      return false;
    }
    if (!ConstexprUnionMemberActive(object_value.object,
                                    member_node->member)) {
      return false;
    }
    ConstexprValue* slot =
        ConstexprObjectSlot(
            object_value.object,
            ConstexprMemberSlotIndex(object_value.object, member_node->member));
    if (slot == NULL) {
      return false;
    }
    TypeRecord* member_type =
        member_node->member->symbol != NULL
            ? member_node->member->symbol->type : NULL;
    if (member_type != NULL &&
        (TypeIsFixedArray(member_type) ||
         TypeIsStructOrUnion(member_type)) &&
        (!slot->is_object || slot->object == NULL)) {
      slot->is_object = true;
      slot->is_address = false;
      slot->object = NewConstexprObject(
          ctx, member_type, ConstexprObjectSlotCount(member_type));
    }
    *result = *slot;
    return true;
  }
  return false;
}

bool ConstexprEvaluateObjectAccessAsInteger(ConstEvalContext* ctx,
                                            ASTNode* node, int64_t* result) {
  ConstexprValue value;
  return EvaluateConstexprObjectAccess(ctx, node, &value) &&
         ConstexprValueAsInteger(value, result);
}

bool ConstexprEvaluateObjectAccessAsFloating(ConstEvalContext* ctx,
                                             ASTNode* node, double* result) {
  ConstexprValue value;
  return EvaluateConstexprObjectAccess(ctx, node, &value) &&
         ConstexprValueAsFloating(value, result);
}

static bool EvaluateConstexprObjectLValue(ConstEvalContext* ctx,
                                          ASTNode* node,
                                          ConstexprValue** slot,
                                          bool allow_object) {
  if (node == NULL) {
    return false;
  }
  if (node->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node;
    ConstexprBinding* binding = FindConstexprBinding(ctx, id->symbol);
    if (binding != NULL && binding->is_address &&
        binding->address_slot != NULL) {
      *slot = binding->address_slot;
      return allow_object || !(*slot)->is_object;
    }
  }
  if (node->op == AST_OP(subscript) &&
      ASTNodeGetShape(node) == kASTShapeBinary) {
    BinaryASTNode* subscript = (BinaryASTNode*)node;
    ConstexprValue object_value;
    int64_t index;
    if (!EvaluateIntegerExpressionInContext(ctx, subscript->right, &index)) {
      return false;
    }
    if (EvaluateConstexprObjectAccess(ctx, subscript->left, &object_value) &&
        object_value.object != NULL) {
      if (index < 0) {
        return false;
      }
      *slot = ConstexprObjectSlot(object_value.object, (size_t)index);
    } else {
      ConstexprValue address;
      if (!EvaluateConstexprAddressValue(ctx, subscript->left, &address) ||
          address.address_object == NULL || index < 0 ||
          (uint64_t)index > SIZE_MAX - address.address_index) {
        return false;
      }
      *slot = ConstexprObjectSlot(address.address_object,
                                  address.address_index + (size_t)index);
    }
    return *slot != NULL && (allow_object || !(*slot)->is_object);
  }
  if (node->op == AST_OP(contents)) {
    UnaryASTNode* contents = (UnaryASTNode*)node;
    ConstexprValue address;
    if (!EvaluateConstexprAddressValue(ctx, contents->sub, &address) ||
        address.address_slot == NULL) {
      return false;
    }
    *slot = address.address_slot;
    return allow_object || !(*slot)->is_object;
  }
  if (node->op == AST_OP(dot) || node->op == AST_OP(arrow)) {
    BinaryASTNode* member_access = (BinaryASTNode*)node;
    if (member_access->right == NULL ||
        member_access->right->op != AST_OP(structmember)) {
      return false;
    }
    ConstexprValue object_value;
    if (!EvaluateConstexprObjectAccess(ctx, member_access->left,
                                       &object_value) ||
        object_value.object == NULL) {
      return false;
    }
    StructMemberASTNode* member_node =
        (StructMemberASTNode*)member_access->right;
    if (member_node->member == NULL) {
      return false;
    }
    if (allow_object) {
      ConstexprActivateUnionMember(object_value.object, member_node->member);
    } else if (!ConstexprUnionMemberActive(object_value.object,
                                           member_node->member)) {
      return false;
    }
    *slot = ConstexprObjectSlot(
        object_value.object,
        ConstexprMemberSlotIndex(object_value.object, member_node->member));
    TypeRecord* member_type =
        member_node->member->symbol != NULL
            ? member_node->member->symbol->type : NULL;
    if (*slot != NULL && member_type != NULL &&
        (TypeIsFixedArray(member_type) ||
         TypeIsStructOrUnion(member_type)) &&
        (!(*slot)->is_object || (*slot)->object == NULL)) {
      (*slot)->is_object = true;
      (*slot)->is_address = false;
      (*slot)->object = NewConstexprObject(
          ctx, member_type, ConstexprObjectSlotCount(member_type));
    }
    return *slot != NULL && (allow_object || !(*slot)->is_object);
  }
  return false;
}

static bool EvaluateConstexprAddressValue(ConstEvalContext* ctx, ASTNode* node,
                                          ConstexprValue* result) {
  if (node == NULL) {
    return false;
  }
  if (node->op == AST_OP(expr_init)) {
    ExpressionInitializerASTNode* init = (ExpressionInitializerASTNode*)node;
    return EvaluateConstexprAddressValue(ctx, init->expr, result);
  }
  if (node->op == AST_OP(cast)) {
    CastASTNode* cast = (CastASTNode*)node;
    return EvaluateConstexprAddressValue(ctx, cast->expr, result);
  }
  if (node->op == AST_OP(string)) {
    ConstexprObject* object = ConstexprStringObject(ctx, node, node->type);
    if (object == NULL) {
      return false;
    }
    *result = (ConstexprValue){
        .is_address = true, .address_object = object, .address_index = 0};
    return true;
  }
  if (node->op == AST_OP(number)) {
    ConstantASTNode* constant = (ConstantASTNode*)node;
    return constant->value.ivalue == 0 && ConstexprNullAddress(result);
  }
  if (node->op == AST_OP(address)) {
    UnaryASTNode* address = (UnaryASTNode*)node;
    if (address->sub != NULL &&
        address->sub->op == AST_OP(subscript) &&
        ASTNodeGetShape(address->sub) == kASTShapeBinary) {
      BinaryASTNode* subscript = (BinaryASTNode*)address->sub;
      ConstexprValue base;
      int64_t index;
      if (EvaluateConstexprAddressValue(ctx, subscript->left, &base) &&
          base.address_object != NULL &&
          EvaluateIntegerExpressionInContext(ctx, subscript->right, &index) &&
          index >= 0 &&
          (uint64_t)index <= SIZE_MAX - base.address_index) {
        size_t address_index = base.address_index + (size_t)index;
        if (address_index < base.address_object->slots.length) {
          base.address_index = address_index;
          *result = base;
          return true;
        }
      }
      return false;
    }
    ConstexprBinding* binding = NULL;
    ConstexprValue* slot = NULL;
    if (EvaluateConstexprLValue(ctx, address->sub, &binding)) {
      *result = (ConstexprValue){.is_address = true,
                                 .address_binding = binding};
      return true;
    }
    if (EvaluateConstexprObjectLValue(ctx, address->sub, &slot, true)) {
      *result = (ConstexprValue){.is_address = true, .address_slot = slot};
      return true;
    }
    return false;
  }
  if (node->op == AST_OP(contents)) {
    UnaryASTNode* contents = (UnaryASTNode*)node;
    ConstexprValue address;
    return EvaluateConstexprAddressValue(ctx, contents->sub, &address) &&
           ConstexprDereferenceAddress(address, result);
  }
  if (node->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node;
    ConstexprBinding* binding = FindConstexprBinding(ctx, id->symbol);
    if (binding != NULL && binding->is_address) {
      *result = (ConstexprValue){.is_address = true,
                                 .address_binding = binding->address_binding,
                                 .address_slot = binding->address_slot,
                                 .address_object = binding->address_object,
                                 .address_index = binding->address_index};
      return true;
    }
    if (binding != NULL && binding->object != NULL &&
        TypeIsFixedArray(binding->object->type)) {
      *result = (ConstexprValue){.is_address = true,
                                 .address_object = binding->object,
                                 .address_index = 0};
      return true;
    }
  }
  if (node->op == AST_OP(plus) || node->op == AST_OP(minus)) {
    BinaryASTNode* arithmetic = (BinaryASTNode*)node;
    ConstexprValue base;
    int64_t offset;
    ASTNode* offset_expr = arithmetic->right;
    bool offset_is_elements = false;
    if (offset_expr != NULL && offset_expr->op == AST_OP(ptr_scale)) {
      offset_expr = ((PtrScaleASTNode*)offset_expr)->expr;
      offset_is_elements = true;
    }
    if (!EvaluateConstexprAddressValue(ctx, arithmetic->left, &base) ||
        base.address_object == NULL ||
        !EvaluateIntegerExpressionInContext(ctx, offset_expr, &offset)) {
      return false;
    }
    if (node->op == AST_OP(minus)) {
      if (offset == INT64_MIN) {
        return false;
      }
      offset = -offset;
    }
    TypeRecord* pointer_type = node->type;
    if (!offset_is_elements && TypeIsPointer(pointer_type) &&
        pointer_type->next != NULL &&
        pointer_type->next->size > 1) {
      int element_size = pointer_type->next->size;
      if (offset % element_size != 0) {
        return false;
      }
      offset /= element_size;
    }
    if ((offset < 0 && (uint64_t)(-offset) > base.address_index) ||
        (offset >= 0 && (uint64_t)offset > SIZE_MAX - base.address_index)) {
      return false;
    }
    size_t new_index =
        offset < 0 ? base.address_index - (size_t)(-offset)
                   : base.address_index + (size_t)offset;
    if (base.address_object != NULL &&
        new_index > base.address_object->slots.length) {
      return false;
    }
    base.address_index = new_index;
    *result = base;
    return true;
  }
  if (node->op == AST_OP(subscript)) {
    ConstexprValue* slot = NULL;
    if (EvaluateConstexprObjectLValue(ctx, node, &slot, false) &&
        slot != NULL) {
      *result = (ConstexprValue){.is_address = true, .address_slot = slot};
      return true;
    }
  }
  if (node->op == AST_OP(dot) || node->op == AST_OP(arrow)) {
    ConstexprValue value;
    if (EvaluateConstexprObjectAccess(ctx, node, &value)) {
      if (value.is_address) {
        *result = value;
        return true;
      }
      if (value.is_object && value.object != NULL &&
          TypeIsFixedArray(value.object->type)) {
        *result = (ConstexprValue){.is_address = true,
                                   .address_object = value.object,
                                   .address_index = 0};
        return true;
      }
    }
  }
  if (node->op == AST_OP(call)) {
    // The AST evaluator preserves an address's containing object and element
    // index.  The pcode bridge can only recover the pointed-to slot, which is
    // sufficient for dereference but loses the identity needed for pointer
    // arithmetic and same-object distance calculations.
    if (EvaluateConstexprCall(ctx, node, result) && result->is_address) {
      return true;
    }
    return ConstexprPCodeEvaluateCallAsAddress(ctx, node, result);
  }
  return false;
}

static bool EvaluateConstexprReferenceInitializer(ConstEvalContext* ctx,
                                                  ASTNode* node,
                                                  TypeRecord* formal_object_type,
                                                  ConstexprValue* result) {
  ConstexprBinding* binding = NULL;
  ConstexprValue* slot = NULL;
  if (EvaluateConstexprLValue(ctx, node, &binding)) {
    *result = (ConstexprValue){.is_address = true, .address_binding = binding};
    return true;
  }
  if (EvaluateConstexprObjectLValue(ctx, node, &slot, true)) {
    *result = (ConstexprValue){.is_address = true, .address_slot = slot};
    return true;
  }
  // A reference parameter can bind to a class/array prvalue that has been
  // materialized into a temporary (e.g. the `const S&` source operand of a
  // defaulted `operator<=>` called as `S{...} <=> S{...}`).  Such a temporary is
  // not an addressable lvalue binding/slot, so evaluate it to an object and let
  // the reference alias that object directly; subsequent member accesses read it
  // through the pushed binding.
  ConstexprValue object_value;
  if (node != NULL && node->type != NULL && formal_object_type != NULL &&
      TypeIsStructOrUnion(formal_object_type) &&
      TypeIsStructOrUnion(node->type) &&
      !TypeEqual(node->type, formal_object_type) &&
      EvaluateConstexprConvertingConstruction(
          ctx, node, formal_object_type, /*allow_explicit=*/false,
          &object_value)) {
    *result = object_value;
    return true;
  }
  if (node != NULL && node->type != NULL && formal_object_type != NULL &&
      TypeIsStructOrUnion(formal_object_type) &&
      TypeIsStructOrUnion(node->type) &&
      !TypeEqual(node->type, formal_object_type)) {
    return false;
  }
  if (node != NULL && node->type != NULL &&
      (TypeIsStructOrUnion(node->type) || TypeIsFixedArray(node->type)) &&
      EvaluateConstexprObjectAccess(ctx, node, &object_value) &&
      object_value.is_object && object_value.object != NULL) {
    *result = object_value;
    return true;
  }
  return EvaluateConstexprAddressValue(ctx, node, result);
}

static bool BindConstexprReferenceArgument(ConstEvalContext* ctx,
                                           ASTNode* actual,
                                           TypeRecord* formal_object_type,
                                           ConstexprValue* value) {
  ASTNode* binding_expr = ConstexprInitializerExpression(actual);
  if (binding_expr == NULL) {
    binding_expr = actual;
  }
  if (binding_expr != NULL &&
      (TypeIsIntegral(formal_object_type) ||
       TypeIsFloatingPoint(formal_object_type)) &&
      binding_expr->op == AST_OP(compound_literal)) {
    binding_expr = ConstexprInitializerExpression(
        ((CompoundLiteralASTNode*)binding_expr)->initializer);
  }
  ConstexprBinding* binding = NULL;
  ConstexprValue* slot = NULL;
  if (EvaluateConstexprLValue(ctx, binding_expr, &binding)) {
    *value = (ConstexprValue){.is_address = true, .address_binding = binding};
    return true;
  }
  if (EvaluateConstexprObjectLValue(ctx, binding_expr, &slot, true)) {
    *value = (ConstexprValue){.is_address = true, .address_slot = slot};
    return true;
  }
  TypeRecord* plain_type = ConstexprPlainObjectType(formal_object_type);
  bool ok = EvaluateConstexprValue(ctx, binding_expr, plain_type, value);
  ConstexprReleasePlainObjectType(formal_object_type, plain_type);
  return ok;
}

bool ConstexprEvaluatePointerComparison(ConstEvalContext* ctx, ASTNode* node,
                                        int64_t* result) {
  if (node == NULL ||
      (node->op != AST_OP(equal) && node->op != AST_OP(noteq) &&
       node->op != AST_OP(less) && node->op != AST_OP(lesseq) &&
       node->op != AST_OP(greater) && node->op != AST_OP(greatereq))) {
    return false;
  }
  BinaryASTNode* binary = (BinaryASTNode*)node;
  ConstexprValue left;
  ConstexprValue right;
  if (!EvaluateConstexprAddressValue(ctx, binary->left, &left) ||
      !EvaluateConstexprAddressValue(ctx, binary->right, &right)) {
    return false;
  }
  bool equal = ConstexprAddressEqual(left, right);
  if (node->op == AST_OP(equal) || node->op == AST_OP(noteq)) {
    *result = node->op == AST_OP(equal) ? equal : !equal;
    return true;
  }
  // Relational pointer comparisons are constant only when both pointers name
  // elements (or the one-past element) of the same array object.
  if (left.address_object == NULL ||
      left.address_object != right.address_object) {
    return false;
  }
  if (node->op == AST_OP(less)) {
    *result = left.address_index < right.address_index;
  } else if (node->op == AST_OP(lesseq)) {
    *result = left.address_index <= right.address_index;
  } else if (node->op == AST_OP(greater)) {
    *result = left.address_index > right.address_index;
  } else {
    *result = left.address_index >= right.address_index;
  }
  return true;
}

bool ConstexprSameObjectPointerDistance(ConstEvalContext* ctx,
                                        ASTNode* begin_expr, ASTNode* end_expr,
                                        size_t* count) {
  if (ctx == NULL || begin_expr == NULL || end_expr == NULL ||
      count == NULL) {
    return false;
  }
  ConstexprValue begin_value = {0};
  ConstexprValue end_value = {0};
  if (!EvaluateConstexprAddressValue(ctx, begin_expr, &begin_value) ||
      !EvaluateConstexprAddressValue(ctx, end_expr, &end_value) ||
      begin_value.address_object == NULL ||
      begin_value.address_object != end_value.address_object ||
      end_value.address_index < begin_value.address_index) {
    return false;
  }
  *count = (size_t)(end_value.address_index - begin_value.address_index);
  return true;
}

bool ConstexprEvaluatePointerDereferenceAsInteger(ConstEvalContext* ctx,
                                                  ASTNode* node,
                                                  int64_t* result) {
  if (node == NULL || node->op != AST_OP(contents)) {
    return false;
  }
  ConstexprValue value;
  return EvaluateConstexprAddressValue(ctx, node, &value) &&
         ConstexprValueAsInteger(value, result);
}

bool ConstexprEvaluatePointerDereferenceAsFloating(ConstEvalContext* ctx,
                                                   ASTNode* node,
                                                   double* result) {
  if (node == NULL || node->op != AST_OP(contents)) {
    return false;
  }
  ConstexprValue value;
  return EvaluateConstexprAddressValue(ctx, node, &value) &&
         ConstexprValueAsFloating(value, result);
}

static bool EvaluateConstexprObjectAddress(ConstEvalContext* ctx,
                                           ASTNode* node,
                                           ConstexprObject** object) {
  if (node == NULL) {
    return false;
  }
  if (node->op == AST_OP(address)) {
    UnaryASTNode* address = (UnaryASTNode*)node;
    ConstexprValue* slot = NULL;
    if (address->sub != NULL && address->sub->type != NULL &&
        (TypeIsStructOrUnion(address->sub->type) ||
         TypeIsFixedArray(address->sub->type)) &&
        EvaluateConstexprObjectLValue(ctx, address->sub, &slot, true) &&
        slot != NULL) {
      if (!slot->is_object || slot->object == NULL) {
        slot->is_object = true;
        slot->is_address = false;
        slot->is_floating = false;
        slot->ivalue = 0;
        slot->fvalue = 0;
        slot->object = NewConstexprObject(
            ctx, address->sub->type,
            ConstexprObjectSlotCount(address->sub->type));
      }
      *object = slot->object;
      return *object != NULL;
    }
    ConstexprValue value;
    bool ok = EvaluateConstexprObjectAccess(ctx, address->sub, &value);
    if ((!ok || value.object == NULL) && address->sub != NULL) {
      ok = EvaluateConstexprValue(ctx, address->sub, address->sub->type,
                                  &value);
    }
    if (!ok || value.object == NULL) {
      return false;
    }
    *object = value.object;
    return true;
  }
  ConstexprValue value;
  if (EvaluateConstexprObjectAccess(ctx, node, &value) &&
      value.object != NULL) {
    *object = value.object;
    return true;
  }
  return false;
}

bool ConstexprEvaluateObjectAddress(ConstEvalContext* ctx, ASTNode* node,
                                    ConstexprObject** object) {
  return EvaluateConstexprObjectAddress(ctx, node, object);
}

bool ConstexprEvaluateCharacterSequence(ASTNode* pointer, size_t count,
                                        String* result) {
  if (pointer == NULL || result == NULL) {
    return false;
  }
  ConstEvalContext ctx;
  ConstEvalContextInit(&ctx);
  ConstexprValue address = {0};
  bool evaluated = EvaluateConstexprAddressValue(&ctx, pointer, &address) &&
                   address.is_address;
  ConstexprObject* object = address.address_object;
  size_t index = address.address_index;
  if (evaluated && object == NULL && address.address_slot != NULL &&
      address.address_slot->is_object) {
    object = address.address_slot->object;
    index = 0;
  }
  bool ok = evaluated && object != NULL && index <= object->slots.length &&
            count <= object->slots.length - index;
  if (ok) {
    StringClear(result);
    for (size_t i = 0; i < count; i++) {
      ConstexprValue* value =
          ConstexprObjectSlot(object, index + i);
      if (value == NULL || value->is_object || value->is_address ||
          value->is_floating) {
        ok = false;
        break;
      }
      StringAppendChar(result, (char)value->ivalue);
    }
  }
  ConstEvalContextDestruct(&ctx);
  if (!ok) {
    StringClear(result);
  }
  return ok;
}

Symbol* ConstexprFunctionDefinition(Symbol* symbol) {
  if (symbol == NULL || symbol->type == NULL || !TypeIsFunction(symbol->type)) {
    return NULL;
  }
  if (symbol->type->info.function.body == NULL &&
      symbol->type->info.function.cxx_member_owner != NULL) {
    TypeEnsureTemplateMemberFunctionDefinition(&compiler->syntax, symbol);
  }
  Symbol* definition = NULL;
  if (symbol->type->info.function.body != NULL) {
    definition = symbol;
  }
  if (definition == NULL &&
      symbol->type->info.function.template_origin != NULL &&
      symbol->type->template_arguments != NULL) {
    Symbol* origin = symbol->type->info.function.template_origin;
    Vector* args = symbol->type->template_arguments;
    definition = FindFunctionTemplateInstantiation(origin, symbol->type, args);
    if (definition == NULL) {
      TypeRecord* origin_type = origin->type;
      if (origin_type != NULL && TypeIsFunction(origin_type) &&
          origin_type->info.function.template_parameters.length == 0 &&
          origin->value.func_defn != NULL &&
          origin->value.func_defn->type != NULL &&
          TypeIsFunction(origin->value.func_defn->type) &&
          origin->value.func_defn->type->info.function.template_parameters.length > 0) {
        origin_type = origin->value.func_defn->type;
      }
      if (origin_type != NULL && TypeIsFunction(origin_type) &&
          origin_type->info.function.template_parameters.length > 0) {
        definition = TypeInstantiateFunctionTemplate(
            &compiler->syntax, origin, args);
      }
    }
  }
  if (definition == NULL && symbol->value.func_defn != NULL &&
      symbol->value.func_defn->type != NULL &&
      TypeIsFunction(symbol->value.func_defn->type)) {
    definition = symbol->value.func_defn;
  }
  if (definition == NULL || definition->type == NULL ||
      !TypeIsFunction(definition->type) ||
      definition->type->info.function.body == NULL) {
    return NULL;
  }
  ASTNode* body = definition->type->info.function.body;
  bool being_analyzed = false;
  for (size_t i = 0; i < compiler->functions_being_analyzed.length; i++) {
    if (compiler->functions_being_analyzed.value.p[i] == definition->type) {
      being_analyzed = true;
      break;
    }
  }
  // A primary template body is intentionally only partially analyzed. Re-enter
  // semantic analysis only after instantiation has produced a fully concrete
  // function type; otherwise dependent names and `if constexpr` conditions are
  // diagnosed before substitution.
  if ((body->flags & kASTAnalyzed) == 0 && !being_analyzed &&
      !TypeContainsTemplateParameter(definition->type)) {
    TypeRecord* saved_function = compiler->current_function;
    compiler->current_function = definition->type;
    ASTNode* declaration = NewVariableDeclarationASTNode(
        definition, NULL, definition->location);
    SemanticAnalyzeFunction(&compiler->syntax, declaration);
    ASTNodeDelete(declaration);
    compiler->current_function = saved_function;
  }
  return definition;
}

static TypeRecord* ConstexprPlainObjectType(TypeRecord* type) {
  if (type == NULL || type->qualifiers == kQualPlain) {
    return type;
  }
  TypeRecord* plain = TypeRecordCopy(type);
  plain->qualifiers = kQualPlain;
  return plain;
}

static void ConstexprReleasePlainObjectType(TypeRecord* requested,
                                            TypeRecord* plain) {
  if (plain != NULL && plain != requested) {
    TypeRecordDelete(plain);
  }
}

static bool EvaluateConstexprConvertingConstruction(ConstEvalContext* ctx,
                                                    ASTNode* from,
                                                    TypeRecord* to,
                                                    bool allow_explicit,
                                                    ConstexprValue* result) {
  if (!CompilerIsCXX() || ctx == NULL || from == NULL || to == NULL ||
      result == NULL || !TypeIsStructOrUnion(to)) {
    return false;
  }
  TypeRecord* plain_to = ConstexprPlainObjectType(to);
  StructMember* ctor =
      CXXFindConvertingConstructorCandidate(plain_to, from,
                                            allow_explicit);
  if (ctor == NULL || ctor->symbol == NULL) {
    ConstexprReleasePlainObjectType(to, plain_to);
    return false;
  }
  Symbol* ctor_symbol = ctor->symbol;
  Symbol* callee = NULL;
  if (ctor_symbol->flags.is_template) {
    Vector actuals;
    VectorInit(&actuals);
    VectorAppend(&actuals, from);
    DiagnosticSuppressBegin();
    Symbol* candidate = TypeCreateFunctionTemplateCandidate(
        &compiler->syntax, ctor_symbol, NULL, &actuals,
        /*first_formal_arg=*/1);
    DiagnosticSuppressEnd();
    VectorDestruct(&actuals);
    if (candidate == NULL || candidate->type == NULL ||
        candidate->type->template_arguments == NULL) {
      if (candidate != NULL) {
        SymbolDelete(candidate);
      }
      ConstexprReleasePlainObjectType(to, plain_to);
      return false;
    }
    DiagnosticSuppressBegin();
    callee = TypeInstantiateFunctionTemplate(
        &compiler->syntax, ctor_symbol, candidate->type->template_arguments);
    DiagnosticSuppressEnd();
    SymbolDelete(candidate);
  } else {
    callee = ConstexprFunctionDefinition(ctor_symbol);
  }
  if (callee == NULL || callee->type == NULL ||
      !TypeIsFunction(callee->type) ||
      !callee->type->info.function.is_constexpr ||
      callee->type->info.function.body == NULL ||
      !callee->type->info.function.is_constructor) {
    ConstexprReleasePlainObjectType(to, plain_to);
    return false;
  }
  result->is_object = true;
  result->is_address = false;
  result->is_floating = false;
  result->ivalue = 0;
  result->fvalue = 0;
  result->object =
      NewConstexprObject(ctx, plain_to, ConstexprObjectSlotCount(plain_to));
  Vector actuals;
  VectorInit(&actuals);
  VectorAppend(&actuals, from);
  size_t mark = ctx->bindings.length;
  ctx->call_depth++;
  ConstexprValue ignored = {0};
  bool bound = BindConstexprConstructorObjectActuals(
      ctx, callee, result->object, &actuals);
  bool evaluated =
      bound && EvaluateConstexprStatement(ctx,
                                          callee->type->info.function.body,
                                          callee->type->next, &ignored) ==
                   kConstexprStmtNormal;
  bool ok = evaluated;
  ctx->call_depth--;
  PopConstexprBindings(ctx, mark);
  VectorDestruct(&actuals);
  ConstexprReleasePlainObjectType(to, plain_to);
  return ok;
}

static Symbol* ConstexprCallSymbol(ASTNode* node) {
  if (node == NULL || node->op != AST_OP(call)) {
    return NULL;
  }
  VectorASTNode* call = (VectorASTNode*)node;
  if (call->left == NULL || call->left->op != AST_OP(identifier)) {
    return NULL;
  }
  return ((IdentifierASTNode*)call->left)->symbol;
}

static bool ConstexprParameterTypeSupported(TypeRecord* type) {
  return TypeIsIntegral(type) || TypeIsFloatingPoint(type) ||
         TypeIsFixedArray(type) || TypeIsStructOrUnion(type) ||
         TypeIsPointer(type) || TypeIsReference(type);
}

static bool BindConstexprActuals(ConstEvalContext* ctx, Symbol* function,
                                 Vector* actuals) {
  TypeRecord* func = function->type;
  size_t count = actuals->length;
  if (count != func->info.function.prototype.length) {
    return false;
  }
  // Evaluate every actual argument in the *caller's* binding context before
  // binding any formal parameter.  Pushing a formal's binding eagerly (as the
  // arguments are evaluated one by one) would let a later actual resolve a
  // name against the freshly bound, not-yet-complete parameters instead of the
  // caller's arguments.  For example, the recursive call `gcd(b, a % b)` would
  // otherwise evaluate `a % b` after `a` had already been rebound to the value
  // of `b`, yielding `b % b == 0` and a wrong result.
  Symbol** formals = count > 0 ? malloc(count * sizeof(Symbol*)) : NULL;
  ConstexprValue* values = count > 0 ? malloc(count * sizeof(ConstexprValue)) : NULL;
  bool ok = true;
  for (size_t i = 0; i < count; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    ASTNode* actual = actuals->value.p[i];
    ConstexprValue value = {0};
    if (formal != NULL && StringEqual(&formal->name, "this")) {
      ConstexprObject* object = NULL;
      if (!EvaluateConstexprObjectAddress(ctx, actual, &object)) {
        ok = false;
        break;
      }
      value.is_object = true;
      value.object = object;
    } else if (formal == NULL || actual == NULL ||
               !ConstexprParameterTypeSupported(formal->type)) {
      ok = false;
      break;
    } else if (TypeIsReference(formal->type)) {
      if (!BindConstexprReferenceArgument(ctx, actual, formal->type->next,
                                          &value)) {
        ok = false;
        break;
      }
    } else if (!EvaluateConstexprValue(ctx, actual, formal->type, &value)) {
      ok = false;
      break;
    }
    formals[i] = formal;
    values[i] = value;
  }
  if (ok) {
    for (size_t i = 0; i < count; i++) {
      PushConstexprBinding(ctx, formals[i], values[i]);
    }
  }
  free(formals);
  free(values);
  return ok;
}

static bool BindConstexprConstructorObjectActuals(ConstEvalContext* ctx,
                                                  Symbol* function,
                                                  ConstexprObject* object,
                                                  Vector* actuals) {
  if (object == NULL || function == NULL || function->type == NULL) {
    return false;
  }
  TypeRecord* func = function->type;
  if (func->info.function.prototype.length != actuals->length + 1) {
    return false;
  }
  Symbol* this_formal = func->info.function.prototype.value.p[0];
  if (this_formal == NULL || !StringEqual(&this_formal->name, "this")) {
    return false;
  }
  PushConstexprBinding(ctx, this_formal,
                       (ConstexprValue){.is_object = true, .object = object});
  for (size_t i = 0; i < actuals->length; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i + 1];
    ASTNode* actual = actuals->value.p[i];
    if (formal == NULL || actual == NULL ||
        !ConstexprParameterTypeSupported(formal->type)) {
      return false;
    }
    ConstexprValue value;
    if (TypeIsReference(formal->type)) {
      if (!BindConstexprReferenceArgument(ctx, actual, formal->type->next,
                                          &value)) {
        return false;
      }
    } else if (!EvaluateConstexprValue(ctx, actual, formal->type, &value)) {
      return false;
    }
    PushConstexprBinding(ctx, formal, value);
  }
  return true;
}

static bool ConstexprConstructorCandidateMatches(Symbol* candidate,
                                                 VectorASTNode* call) {
  if (candidate == NULL || candidate->type == NULL ||
      !TypeIsFunction(candidate->type) ||
      !candidate->type->info.function.is_constructor ||
      candidate->type->info.function.prototype.length !=
          call->children->length + 1) {
    return false;
  }
  for (size_t a = 0; a < call->children->length; a++) {
    ASTNode* actual = call->children->value.p[a];
    Symbol* formal =
        candidate->type->info.function.prototype.value.p[a + 1];
    TypeRecord* formal_type =
        formal != NULL && TypeIsReference(formal->type)
            ? formal->type->next : formal != NULL ? formal->type : NULL;
    if (actual == NULL || actual->type == NULL || formal_type == NULL ||
        !TypeAssignmentCompatible(actual->type, formal_type)) {
      return false;
    }
  }
  return true;
}

Symbol* ConstexprRawConstructorCallSymbol(ASTNode* node, ASTNode** receiver) {
  if (node == NULL || node->op != AST_OP(call)) {
    return NULL;
  }
  VectorASTNode* call = (VectorASTNode*)node;
  if (call->left == NULL ||
      (call->left->op != AST_OP(dot) && call->left->op != AST_OP(arrow))) {
    return NULL;
  }
  BinaryASTNode* member_access = (BinaryASTNode*)call->left;
  if (member_access->left == NULL || member_access->right == NULL) {
    return NULL;
  }
  TypeRecord* receiver_type = member_access->left->type;
  if (member_access->base.op == AST_OP(arrow) &&
      TypeIsPointerOrArray(receiver_type)) {
    receiver_type = receiver_type->next;
  }
  if (!TypeIsStructOrUnion(receiver_type) ||
      receiver_type->info.struct_info == NULL) {
    return NULL;
  }

  StructMember* member = NULL;
  if (member_access->right->op == AST_OP(structmember)) {
    member = ((StructMemberASTNode*)member_access->right)->member;
  } else if (member_access->right->op == AST_OP(string)) {
    ConstantASTNode* member_name = (ConstantASTNode*)member_access->right;
    member = FindStructMember(receiver_type->info.struct_info,
                              member_name->value.string);
  }
  while (member != NULL) {
    if (member->symbol != NULL && member->symbol->type != NULL &&
        TypeIsFunction(member->symbol->type)) {
      Symbol* constructor_template =
          member->symbol->flags.is_template ? member->symbol : NULL;
      if (constructor_template == NULL &&
          member->symbol->value.func_defn != NULL &&
          member->symbol->value.func_defn->type != NULL &&
          TypeIsFunction(member->symbol->value.func_defn->type) &&
          member->symbol->value.func_defn->type
                  ->info.function.template_parameters.length > 0) {
        constructor_template = member->symbol->value.func_defn;
      }
      if (constructor_template != NULL) {
        DiagnosticSuppressBegin();
        Symbol* candidate = TypeCreateFunctionTemplateCandidate(
            &compiler->syntax, constructor_template, NULL, call->children,
            /*first_formal_arg=*/1);
        Symbol* instantiated =
            candidate != NULL && candidate->type != NULL &&
                    candidate->type->template_arguments != NULL
                ? TypeInstantiateFunctionTemplate(
                      &compiler->syntax, constructor_template,
                      candidate->type->template_arguments)
                : NULL;
        DiagnosticSuppressEnd();
        if (candidate != NULL) {
          SymbolDelete(candidate);
        }
        if (instantiated != NULL && instantiated->type != NULL &&
            TypeIsFunction(instantiated->type) &&
            instantiated->type->info.function.is_constructor &&
            instantiated->type->info.function.prototype.length ==
                call->children->length + 1) {
          *receiver = member_access->left;
          return instantiated;
        }
      }
      for (Symbol* candidate = member->symbol->overload_next;
           candidate != NULL; candidate = candidate->overload_next) {
        if (ConstexprConstructorCandidateMatches(candidate, call)) {
          *receiver = member_access->left;
          return candidate;
        }
      }
      Vector* instantiations =
          &member->symbol->type->info.function.template_instantiations;
      for (size_t i = 0; i < instantiations->length; i++) {
        Symbol* candidate = instantiations->value.p[i];
        if (ConstexprConstructorCandidateMatches(candidate, call)) {
          *receiver = member_access->left;
          return candidate;
        }
      }
    }
    if (member->is_member_function && member->symbol != NULL &&
        member->symbol->type != NULL && TypeIsFunction(member->symbol->type) &&
        member->symbol->type->info.function.is_constructor &&
        member->symbol->type->info.function.prototype.length ==
            call->children->length + 1) {
      *receiver = member_access->left;
      return member->symbol;
    }
    member = member->overload_next;
  }
  return NULL;
}

Symbol* ConstexprConstructorForObjectType(TypeRecord* type,
                                          size_t actual_count) {
  if (type == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL) {
    return NULL;
  }
  Struct* str = type->info.struct_info;
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    while (member != NULL) {
      if (member->is_member_function && member->symbol != NULL &&
          member->symbol->type != NULL &&
          TypeIsFunction(member->symbol->type) &&
          member->symbol->type->info.function.is_constructor &&
          member->symbol->type->info.function.prototype.length ==
              actual_count + 1) {
        return member->symbol;
      }
      member = member->overload_next;
    }
  }
  return NULL;
}

static bool BindConstexprConstructorActuals(ConstEvalContext* ctx,
                                            Symbol* function,
                                            ASTNode* receiver,
                                            Vector* actuals) {
  if (receiver == NULL) {
    return BindConstexprActuals(ctx, function, actuals);
  }
  TypeRecord* func = function->type;
  if (func->info.function.prototype.length != actuals->length + 1) {
    return false;
  }
  Symbol* this_formal = func->info.function.prototype.value.p[0];
  if (this_formal == NULL || !StringEqual(&this_formal->name, "this")) {
    return false;
  }
  ConstexprObject* object = NULL;
  if (!EvaluateConstexprObjectAddress(ctx, receiver, &object)) {
    return false;
  }
  PushConstexprBinding(ctx, this_formal,
                       (ConstexprValue){.is_object = true, .object = object});
  for (size_t i = 0; i < actuals->length; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i + 1];
    ASTNode* actual = actuals->value.p[i];
    if (formal == NULL || actual == NULL ||
        !ConstexprParameterTypeSupported(formal->type)) {
      return false;
    }
    ConstexprValue value;
    if (TypeIsReference(formal->type)) {
      if (!BindConstexprReferenceArgument(ctx, actual, formal->type->next,
                                          &value)) {
        return false;
      }
    } else if (!EvaluateConstexprValue(ctx, actual, formal->type, &value)) {
      return false;
    }
    PushConstexprBinding(ctx, formal, value);
  }
  return true;
}

// Peel transparent initializer wrappers without collapsing a braced
// initializer, so that aggregate (array / struct) structure is preserved even
// for single-element initializers.  In particular a compound literal `T{...}`
// is reduced to its braced initializer.
static ASTNode* ConstexprAggregateInitializerExpression(ASTNode* initializer) {
  while (initializer != NULL) {
    switch (initializer->op) {
      case AST_OP(init):
        initializer = ((BinaryASTNode*)initializer)->right;
        break;
      case AST_OP(expr_init):
        initializer = ((ExpressionInitializerASTNode*)initializer)->expr;
        break;
      case AST_OP(compound_literal):
        initializer = ((CompoundLiteralASTNode*)initializer)->initializer;
        break;
      default:
        return initializer;
    }
  }
  return NULL;
}

ASTNode* ConstexprInitializerExpression(ASTNode* initializer) {
  while (initializer != NULL) {
    switch (initializer->op) {
      case AST_OP(init): {
        BinaryASTNode* init = (BinaryASTNode*)initializer;
        initializer = init->right;
        break;
      }
      case AST_OP(expr_init): {
        ExpressionInitializerASTNode* expr_init =
            (ExpressionInitializerASTNode*)initializer;
        initializer = expr_init->expr;
        break;
      }
      case AST_OP(braced_init): {
        BracedInitializerASTNode* braced =
            (BracedInitializerASTNode*)initializer;
        if (braced->initializers == NULL ||
            braced->initializers->length != 1) {
          return initializer;
        }
        initializer = braced->initializers->value.p[0];
        break;
      }
      case AST_OP(designated_init): {
        DesignatedInitializerASTNode* designated =
            (DesignatedInitializerASTNode*)initializer;
        initializer = designated->init;
        break;
      }
      default:
        return initializer;
    }
  }
  return NULL;
}

static bool PushConstexprSymbolValue(ConstEvalContext* ctx, Symbol* symbol) {
  if (symbol == NULL || symbol->type == NULL || !symbol->flags.value_set) {
    return false;
  }
  ConstexprValue value = {0};
  if (TypeIsFloatingPoint(symbol->type)) {
    value.is_object = false;
    value.object = NULL;
    value.is_floating = true;
    value.fvalue = symbol->value.fvalue;
    value.ivalue = (int64_t)symbol->value.fvalue;
  } else if (TypeIsIntegral(symbol->type)) {
    value.is_object = false;
    value.object = NULL;
    value.is_floating = false;
    value.ivalue = symbol->value.ivalue;
    value.fvalue = (double)symbol->value.ivalue;
  } else {
    return false;
  }
  PushConstexprBinding(ctx, symbol, value);
  return true;
}

static bool EvaluateConstexprVariableDeclaration(ConstEvalContext* ctx,
                                                VariableDeclarationASTNode* decl) {
  if (decl == NULL || decl->symbol == NULL ||
      TypeIsFunction(decl->symbol->type)) {
    return false;
  }
  if (StorageIs(decl->symbol->storage, STO(static) | STO(thread))) {
    if (!CompilerCXXAtLeast(kLanguageStandardCXX23) ||
        !decl->symbol->flags.is_constexpr) {
      return false;
    }
  }
  if (PushConstexprSymbolValue(ctx, decl->symbol)) {
    return true;
  }
  if (TypeIsPointer(decl->symbol->type) || TypeIsReference(decl->symbol->type)) {
    ASTNode* initializer = ConstexprInitializerExpression(decl->initializer);
    if (initializer == NULL) {
      return false;
    }
    ConstexprValue value;
    bool ok = TypeIsReference(decl->symbol->type)
        ? EvaluateConstexprReferenceInitializer(ctx, initializer,
                                                decl->symbol->type->next, &value)
        : EvaluateConstexprAddressValue(ctx, initializer, &value);
    if (!ok) {
      return false;
    }
    PushConstexprBinding(ctx, decl->symbol, value);
    return true;
  }
  if (TypeIsFixedArray(decl->symbol->type) ||
      TypeIsStructOrUnion(decl->symbol->type)) {
    ASTNode* initializer = ConstexprInitializerExpression(decl->initializer);
    if (initializer != NULL && initializer->op == AST_OP(call)) {
      ConstexprValue object_value;
      if (EvaluateConstexprCall(ctx, initializer, &object_value) &&
          object_value.is_object && object_value.object != NULL) {
        PushConstexprBinding(ctx, decl->symbol, object_value);
        return true;
      }
      object_value = (ConstexprValue){
          .is_object = true,
          .object = NewConstexprObject(
              ctx, decl->symbol->type,
              ConstexprObjectSlotCount(decl->symbol->type)),
      };
      size_t mark = ctx->bindings.length;
      PushConstexprBinding(ctx, decl->symbol, object_value);
      if (!EvaluateConstexprConstructorCall(ctx, initializer)) {
        PopConstexprBindings(ctx, mark);
        return false;
      }
      return true;
    }
    ConstexprValue object_value;
    if (!EvaluateConstexprInitializer(ctx, decl->symbol->type,
                                      decl->initializer, &object_value)) {
      return false;
    }
    PushConstexprBinding(ctx, decl->symbol, object_value);
    return true;
  }
  if (!TypeIsIntegral(decl->symbol->type) &&
      !TypeIsFloatingPoint(decl->symbol->type)) {
    return false;
  }
  ASTNode* expr = ConstexprInitializerExpression(decl->initializer);
  if (expr == NULL) {
    return false;
  }
  ConstexprValue value;
  if (!EvaluateConstexprValue(ctx, expr, decl->symbol->type, &value)) {
    return false;
  }
  PushConstexprBinding(ctx, decl->symbol, value);
  return true;
}

bool ConstexprBindVariableDeclaration(ConstEvalContext* ctx,
                                      VariableDeclarationASTNode* decl) {
  return EvaluateConstexprVariableDeclaration(ctx, decl);
}

bool ConstexprBindExpansionRangeHidden(ConstEvalContext* ctx,
                                       VariableDeclarationASTNode* hidden_decl,
                                       ASTNode* init_expr) {
  if (ctx == NULL || hidden_decl == NULL || hidden_decl->symbol == NULL) {
    return false;
  }
  if (init_expr != NULL && init_expr->op == AST_OP(identifier)) {
    Symbol* range_symbol = ((IdentifierASTNode*)init_expr)->symbol;
    if (range_symbol != NULL && !range_symbol->flags.value_set &&
        range_symbol->constexpr_initializer != NULL) {
      ConstexprEvaluateObjectConstantForSymbol(
          range_symbol, range_symbol->constexpr_initializer);
    }
  }
  if (ConstexprBindVariableDeclaration(ctx, hidden_decl)) {
    return true;
  }
  if (init_expr == NULL) {
    return false;
  }
  TypeRecord* object_type = hidden_decl->symbol->type;
  if (object_type != NULL && TypeIsReference(object_type)) {
    object_type = object_type->next;
  }
  ConstexprValue value = {0};
  if (EvaluateConstexprObjectAccess(ctx, init_expr, &value) &&
      value.is_object && value.object != NULL) {
    PushConstexprBinding(ctx, hidden_decl->symbol, value);
    if (init_expr->op == AST_OP(identifier)) {
      Symbol* range_sym = ((IdentifierASTNode*)init_expr)->symbol;
      if (range_sym != NULL && range_sym != hidden_decl->symbol) {
        PushConstexprBinding(ctx, range_sym, value);
      }
    }
    return true;
  }
  if (object_type != NULL &&
      EvaluateConstexprReferenceInitializer(ctx, init_expr, object_type,
                                            &value)) {
    PushConstexprBinding(ctx, hidden_decl->symbol, value);
    if (init_expr->op == AST_OP(identifier)) {
      Symbol* range_sym = ((IdentifierASTNode*)init_expr)->symbol;
      if (range_sym != NULL && range_sym != hidden_decl->symbol) {
        PushConstexprBinding(ctx, range_sym, value);
      }
    }
    return true;
  }
  if (object_type != NULL &&
      EvaluateConstexprInitializer(ctx, object_type, init_expr, &value) &&
      value.is_object && value.object != NULL) {
    PushConstexprBinding(ctx, hidden_decl->symbol, value);
    if (init_expr->op == AST_OP(identifier)) {
      Symbol* range_sym = ((IdentifierASTNode*)init_expr)->symbol;
      if (range_sym != NULL && range_sym != hidden_decl->symbol) {
        PushConstexprBinding(ctx, range_sym, value);
      }
    }
    return true;
  }
  return false;
}

static bool EvaluateConstexprDeclarationList(ConstEvalContext* ctx,
                                             DeclarationListASTNode* node) {
  for (size_t i = 0; i < node->declarations->length; i++) {
    ASTNode* decl = node->declarations->value.p[i];
    if (decl == NULL || decl->op != AST_OP(vardecl) ||
        !EvaluateConstexprVariableDeclaration(
            ctx, (VariableDeclarationASTNode*)decl)) {
      return false;
    }
  }
  return true;
}

static bool EvaluateConstexprCondition(ConstEvalContext* ctx, ASTNode* cond,
                                       bool* result) {
  int64_t value;
  if (!EvaluateIntegerExpressionInContext(ctx, cond, &value)) {
    return false;
  }
  *result = value != 0;
  return true;
}

static bool ConstexprVoidExpressionThrows(ASTNode* expr) {
  if (expr == NULL) {
    return false;
  }
  if (expr->op == AST_OP(throw)) {
    return true;
  }
  if (expr->op == AST_OP(comma)) {
    BinaryASTNode* comma = (BinaryASTNode*)expr;
    return ConstexprVoidExpressionThrows(comma->left) ||
           ConstexprVoidExpressionThrows(comma->right);
  }
  return false;
}

static ConstexprStatementResult EvaluateConstexprCompound(
    ConstEvalContext* ctx, CompoundStatementASTNode* body,
    TypeRecord* return_type, ConstexprValue* result) {
  size_t mark = ctx->bindings.length;
  for (size_t i = 0; i < body->statements->length; i++) {
    ASTNode* stmt = body->statements->value.p[i];
    ConstexprStatementResult stmt_result =
        EvaluateConstexprStatement(ctx, stmt, return_type, result);
    if (stmt_result != kConstexprStmtNormal) {
      PopConstexprBindings(ctx, mark);
      return stmt_result;
    }
  }
  PopConstexprBindings(ctx, mark);
  return kConstexprStmtNormal;
}

static ConstexprStatementResult EvaluateConstexprWhile(
    ConstEvalContext* ctx, CombinedStatementASTNode* loop,
    TypeRecord* return_type, ConstexprValue* result) {
  for (;;) {
    bool condition;
    if (!EvaluateConstexprCondition(ctx, loop->cond, &condition)) {
      return kConstexprStmtInvalid;
    }
    if (!condition) {
      return kConstexprStmtNormal;
    }
    ConstexprStatementResult stmt_result =
        EvaluateConstexprStatement(ctx, loop->stmt, return_type, result);
    switch (stmt_result) {
      case kConstexprStmtNormal:
      case kConstexprStmtContinue:
        break;
      case kConstexprStmtBreak:
        return kConstexprStmtNormal;
      default:
        return stmt_result;
    }
  }
}

static ConstexprStatementResult EvaluateConstexprDo(
    ConstEvalContext* ctx, CombinedStatementASTNode* loop,
    TypeRecord* return_type, ConstexprValue* result) {
  for (;;) {
    ConstexprStatementResult stmt_result =
        EvaluateConstexprStatement(ctx, loop->stmt, return_type, result);
    switch (stmt_result) {
      case kConstexprStmtNormal:
      case kConstexprStmtContinue:
        break;
      case kConstexprStmtBreak:
        return kConstexprStmtNormal;
      default:
        return stmt_result;
    }
    bool condition;
    if (!EvaluateConstexprCondition(ctx, loop->cond, &condition)) {
      return kConstexprStmtInvalid;
    }
    if (!condition) {
      return kConstexprStmtNormal;
    }
  }
}

static ConstexprStatementResult EvaluateConstexprFor(
    ConstEvalContext* ctx, ForStatementASTNode* loop,
    TypeRecord* return_type, ConstexprValue* result) {
  size_t mark = ctx->bindings.length;
  if (loop->c1 != NULL) {
    if (loop->c1->op == AST_OP(decl_list)) {
      if (!EvaluateConstexprDeclarationList(ctx,
                                            (DeclarationListASTNode*)loop->c1)) {
        PopConstexprBindings(ctx, mark);
        return kConstexprStmtInvalid;
      }
    } else {
      ConstexprValue ignored;
      if (!EvaluateConstexprValue(ctx, loop->c1, loop->c1->type, &ignored)) {
        PopConstexprBindings(ctx, mark);
        return kConstexprStmtInvalid;
      }
    }
  }
  for (;;) {
    if (loop->c2 != NULL) {
      bool condition;
      if (!EvaluateConstexprCondition(ctx, loop->c2, &condition)) {
        PopConstexprBindings(ctx, mark);
        return kConstexprStmtInvalid;
      }
      if (!condition) {
        PopConstexprBindings(ctx, mark);
        return kConstexprStmtNormal;
      }
    }
    ConstexprStatementResult stmt_result =
        EvaluateConstexprStatement(ctx, loop->stmt, return_type, result);
    switch (stmt_result) {
      case kConstexprStmtNormal:
      case kConstexprStmtContinue:
        break;
      case kConstexprStmtBreak:
        PopConstexprBindings(ctx, mark);
        return kConstexprStmtNormal;
      default:
        PopConstexprBindings(ctx, mark);
        return stmt_result;
    }
    if (loop->c3 != NULL) {
      ConstexprValue ignored;
      if (!EvaluateConstexprValue(ctx, loop->c3, loop->c3->type, &ignored)) {
        PopConstexprBindings(ctx, mark);
        return kConstexprStmtInvalid;
      }
    }
  }
}

static bool ConstexprSwitchMatches(ConstEvalContext* ctx,
                                   CaseLabelASTNode* label,
                                   int64_t value,
                                   bool* matches) {
  (void)ctx;
  if (label->expr == NULL) {
    *matches = false;
    return true;
  }
  *matches = label->value == value;
  return true;
}

static ConstexprStatementResult EvaluateConstexprSwitchCompound(
    ConstEvalContext* ctx, SwitchStatementASTNode* sw,
    CompoundStatementASTNode* compound, int64_t value,
    TypeRecord* return_type, ConstexprValue* result) {
  bool active = false;
  bool matched = false;
  for (size_t i = 0; i < compound->statements->length; i++) {
    ASTNode* stmt = compound->statements->value.p[i];
    if (stmt == NULL) {
      continue;
    }
    if (stmt->op == AST_OP(case)) {
      CaseLabelASTNode* label = (CaseLabelASTNode*)stmt;
      bool label_matches;
      if (label->expr == NULL) {
        label_matches = !matched &&
                        (sw->default_node == NULL || sw->default_node == label);
      } else if (!ConstexprSwitchMatches(ctx, label, value, &label_matches)) {
        return kConstexprStmtInvalid;
      }
      if (label_matches) {
        active = true;
        matched = true;
      }
      if (active && label->stmt != NULL) {
        ConstexprStatementResult stmt_result =
            EvaluateConstexprStatement(ctx, label->stmt, return_type, result);
        if (stmt_result == kConstexprStmtBreak) {
          return kConstexprStmtNormal;
        }
        if (stmt_result != kConstexprStmtNormal) {
          return stmt_result;
        }
      }
      continue;
    }
    if (!active) {
      continue;
    }
    ConstexprStatementResult stmt_result =
        EvaluateConstexprStatement(ctx, stmt, return_type, result);
    if (stmt_result == kConstexprStmtBreak) {
      return kConstexprStmtNormal;
    }
    if (stmt_result != kConstexprStmtNormal) {
      return stmt_result;
    }
  }
  return kConstexprStmtNormal;
}

static ConstexprStatementResult EvaluateConstexprSwitch(
    ConstEvalContext* ctx, SwitchStatementASTNode* sw,
    TypeRecord* return_type, ConstexprValue* result) {
  int64_t value;
  if (!EvaluateIntegerExpressionInContext(ctx, sw->expr, &value)) {
    return kConstexprStmtInvalid;
  }
  if (sw->stmt != NULL && sw->stmt->op == AST_OP(compound)) {
    return EvaluateConstexprSwitchCompound(ctx, sw,
                                           (CompoundStatementASTNode*)sw->stmt,
                                           value, return_type, result);
  }
  CaseLabelASTNode* selected = NULL;
  for (size_t i = 0; i < sw->cases.length; i++) {
    CaseLabelASTNode* label = sw->cases.value.p[i];
    bool matches;
    if (!ConstexprSwitchMatches(ctx, label, value, &matches)) {
      return kConstexprStmtInvalid;
    }
    if (matches) {
      selected = label;
      break;
    }
  }
  if (selected == NULL) {
    selected = sw->default_node;
  }
  if (selected == NULL || selected->stmt == NULL) {
    return kConstexprStmtNormal;
  }
  ConstexprStatementResult stmt_result =
      EvaluateConstexprStatement(ctx, selected->stmt, return_type, result);
  return stmt_result == kConstexprStmtBreak ? kConstexprStmtNormal
                                           : stmt_result;
}

static ConstexprStatementResult EvaluateConstexprStatement(
    ConstEvalContext* ctx, ASTNode* stmt, TypeRecord* return_type,
    ConstexprValue* result) {
  if (stmt == NULL) {
    return kConstexprStmtNormal;
  }
  if (!ConstEvalStep(ctx)) {
    return kConstexprStmtInvalid;
  }
  switch (stmt->op) {
    case AST_OP(decl_list):
      return EvaluateConstexprDeclarationList(ctx,
                                              (DeclarationListASTNode*)stmt)
                 ? kConstexprStmtNormal
                 : kConstexprStmtInvalid;
    case AST_OP(vardecl):
      return EvaluateConstexprVariableDeclaration(
                 ctx, (VariableDeclarationASTNode*)stmt)
                 ? kConstexprStmtNormal
                 : kConstexprStmtInvalid;
    case AST_OP(expr): {
      ExpressionStatementASTNode* expr = (ExpressionStatementASTNode*)stmt;
      if (expr->expr == NULL) {
        return kConstexprStmtNormal;
      }
      if (TypeIsVoid(expr->expr->type)) {
        if (ConstexprVoidExpressionThrows(expr->expr)) {
          return kConstexprStmtInvalid;
        }
        if (expr->expr->op == AST_OP(call)) {
          Symbol* callee = ConstexprFunctionDefinition(
              ConstexprCallSymbol(expr->expr));
          if (callee != NULL && callee->type != NULL &&
              TypeIsFunction(callee->type)) {
            if (callee->type->info.function.is_constructor &&
                !EvaluateConstexprConstructorCall(ctx, expr->expr)) {
              return kConstexprStmtInvalid;
            } else if (callee->type->info.function.is_destructor &&
                !EvaluateConstexprDestructorCall(ctx, expr->expr)) {
              return kConstexprStmtInvalid;
            } else if (!callee->type->info.function.is_constructor &&
                       !callee->type->info.function.is_destructor) {
              ConstexprValue ignored = {0};
              if (!EvaluateConstexprCall(ctx, expr->expr, &ignored)) {
                return kConstexprStmtInvalid;
              }
            }
          }
        }
        return kConstexprStmtNormal;
      }
      ConstexprValue ignored;
      bool ok =
          EvaluateConstexprValue(ctx, expr->expr, expr->expr->type, &ignored);
      return ok ? kConstexprStmtNormal : kConstexprStmtInvalid;
    }
    case AST_OP(compound):
      return EvaluateConstexprCompound(ctx, (CompoundStatementASTNode*)stmt,
                                       return_type, result);
    case AST_OP(try):
      // Before C++26 a reached throw-expression is not a core constant
      // expression, even when a handler could catch it.  A try block whose
      // evaluated path does not throw is nevertheless permitted in a constexpr
      // function, so evaluate that path and leave handlers unreachable here.
      return EvaluateConstexprStatement(ctx, ((TryASTNode*)stmt)->try_stmt,
                                        return_type, result);
    case AST_OP(if): {
      IfStatementASTNode* if_stmt = (IfStatementASTNode*)stmt;
      if (if_stmt->is_consteval) {
        return EvaluateConstexprStatement(
            ctx,
            if_stmt->consteval_negated ? if_stmt->else_part
                                       : if_stmt->if_part,
            return_type, result);
      }
      bool condition;
      if (!EvaluateConstexprCondition(ctx, if_stmt->cond, &condition)) {
        return kConstexprStmtInvalid;
      }
      return EvaluateConstexprStatement(ctx,
                                        condition ? if_stmt->if_part
                                                  : if_stmt->else_part,
                                        return_type, result);
    }
    case AST_OP(while):
      return EvaluateConstexprWhile(ctx, (CombinedStatementASTNode*)stmt,
                                    return_type, result);
    case AST_OP(do):
      return EvaluateConstexprDo(ctx, (CombinedStatementASTNode*)stmt,
                                 return_type, result);
    case AST_OP(for):
      return EvaluateConstexprFor(ctx, (ForStatementASTNode*)stmt, return_type,
                                  result);
    case AST_OP(expansion_for):
      return kConstexprStmtInvalid;
    case AST_OP(switch):
      return EvaluateConstexprSwitch(ctx, (SwitchStatementASTNode*)stmt,
                                     return_type, result);
    case AST_OP(case): {
      CaseLabelASTNode* label = (CaseLabelASTNode*)stmt;
      return EvaluateConstexprStatement(ctx, label->stmt, return_type, result);
    }
    case AST_OP(label):
      return CompilerCXXAtLeast(kLanguageStandardCXX23)
                 ? EvaluateConstexprStatement(
                       ctx, ((LabelASTNode*)stmt)->stmt, return_type, result)
                 : kConstexprStmtInvalid;
    case AST_OP(goto):
      // P2242 permits a goto in a constexpr function definition, but a
      // constant-evaluated path still may not execute it.
      return kConstexprStmtInvalid;
    case AST_OP(return): {
      CombinedStatementASTNode* ret = (CombinedStatementASTNode*)stmt;
      ConstexprValue return_value = {0};
      if (!EvaluateConstexprValue(ctx, ret->cond, return_type, &return_value)) {
        return kConstexprStmtInvalid;
      }
      if (ret->stmt != NULL) {
        ConstexprValue cleanup_value = {0};
        ConstexprStatementResult cleanup = EvaluateConstexprStatement(
            ctx, ret->stmt, return_type, &cleanup_value);
        if (cleanup != kConstexprStmtNormal) {
          return kConstexprStmtInvalid;
        }
      }
      *result = return_value;
      return kConstexprStmtReturn;
    }
    case AST_OP(break):
      return kConstexprStmtBreak;
    case AST_OP(continue):
      return kConstexprStmtContinue;
    default:
      return kConstexprStmtInvalid;
  }
}

static bool EvaluateConstexprFunctionBody(ConstEvalContext* ctx, TypeRecord* func,
                                          ConstexprValue* result) {
  if (func->info.function.body == NULL) {
    return false;
  }
  ConstexprStatementResult stmt_result = EvaluateConstexprStatement(
      ctx, func->info.function.body, func->next, result);
  return stmt_result == kConstexprStmtReturn ||
         (TypeIsVoid(func->next) && stmt_result == kConstexprStmtNormal);
}

// Resolve a non-static member function call of the form `obj.f(args)` or
// `obj->f(args)` (including operator and conversion functions, which are
// member calls under the hood).  On success the receiver expression is
// returned through `*receiver` so the caller can bind it to the implicit
// `this` parameter.  Constructors are intentionally excluded: those have a
// dedicated evaluation path.
static Symbol* ConstexprMemberCallSymbol(ASTNode* node, ASTNode** receiver) {
  if (node == NULL || node->op != AST_OP(call)) {
    return NULL;
  }
  VectorASTNode* call = (VectorASTNode*)node;
  if (call->left == NULL ||
      (call->left->op != AST_OP(dot) && call->left->op != AST_OP(arrow))) {
    return NULL;
  }
  BinaryASTNode* member_access = (BinaryASTNode*)call->left;
  if (member_access->left == NULL || member_access->right == NULL ||
      member_access->right->op != AST_OP(structmember)) {
    return NULL;
  }
  StructMember* member = ((StructMemberASTNode*)member_access->right)->member;
  if (member == NULL || !member->is_member_function || member->symbol == NULL ||
      member->symbol->type == NULL ||
      !TypeIsFunction(member->symbol->type) ||
      member->symbol->type->info.function.is_constructor) {
    return NULL;
  }
  *receiver = member_access->left;
  return member->symbol;
}

bool EvaluateConstexprCall(ConstEvalContext* ctx, ASTNode* node,
                                  ConstexprValue* result) {
  if (ctx->call_depth >= CONSTEXPR_MAX_CALL_DEPTH) {
    return false;
  }
  ASTNode* receiver = NULL;
  Symbol* call_symbol = ConstexprCallSymbol(node);
  Symbol* callee = ConstexprFunctionDefinition(call_symbol);
  if (callee == NULL) {
    callee =
        ConstexprFunctionDefinition(ConstexprMemberCallSymbol(node, &receiver));
  }
  if (callee == NULL || callee->type == NULL || !TypeIsFunction(callee->type)) {
    return false;
  }
  TypeRecord* func = callee->type;
  if (!func->info.function.is_constexpr ||
      func->info.function.body == NULL ||
      func->info.function.is_constructor ||
      func->info.function.is_destructor ||
      func->info.function.is_virtual ||
      func->info.function.varargs) {
    return false;
  }
  // Refuse to interpret a body that is still being semantically analyzed (its
  // nodes are not yet fully typed); this is the recursive/mutually-recursive
  // constexpr case reached during the function's own analysis.
  for (size_t i = 0; i < compiler->functions_being_analyzed.length; i++) {
    if (compiler->functions_being_analyzed.value.p[i] == func) {
      return false;
    }
  }

  VectorASTNode* call = (VectorASTNode*)node;
  size_t mark = ctx->bindings.length;
  ctx->call_depth++;
  // A member call binds the receiver to the implicit `this` parameter and the
  // explicit arguments to the remaining parameters; an ordinary call binds the
  // arguments positionally.  BindConstexprConstructorActuals implements exactly
  // the former (it is not constructor specific despite its name).
  bool ok = (receiver != NULL
                 ? BindConstexprConstructorActuals(ctx, callee, receiver,
                                                   call->children)
                 : BindConstexprActuals(ctx, callee, call->children)) &&
            EvaluateConstexprFunctionBody(ctx, func, result);
  ctx->call_depth--;
  PopConstexprBindings(ctx, mark);
  return ok;
}

bool ConstexprEvaluateCallAsInteger(ConstEvalContext* ctx, ASTNode* node,
                                    int64_t* result) {
  (void)ConstexprFunctionDefinition(ConstexprCallSymbol(node));
  ConstexprEvalMode mode = compiler->constexpr_eval_mode;
  bool use_overlay_result = false;
  int64_t overlay_result = 0;
  if ((mode == kConstexprEvalPCode || mode == kConstexprEvalAuto) &&
      ConstexprPCodeRequiresASTOverlay(node)) {
    compiler->constexpr_eval_mode = kConstexprEvalAST;
    ConstexprValue overlay_value;
    bool overlay_ok = EvaluateConstexprCall(ctx, node, &overlay_value) &&
                      ConstexprValueAsInteger(overlay_value, &overlay_result);
    compiler->constexpr_eval_mode = mode;
    if (!overlay_ok) {
      return false;
    }
    use_overlay_result = true;
  }
  int64_t pcode_result = 0;
  bool pcode_ok =
      mode != kConstexprEvalAST &&
      ConstexprPCodeEvaluateCallAsInteger(ctx, node, &pcode_result);
  if (mode == kConstexprEvalPCode) {
    if (!pcode_ok) {
      ReportConstexprPCodeFailure(node, false);
      return false;
    }
    *result = use_overlay_result ? overlay_result : pcode_result;
    return true;
  }
  if (mode == kConstexprEvalAuto && pcode_ok) {
    *result = use_overlay_result ? overlay_result : pcode_result;
    return true;
  }
  if (mode == kConstexprEvalAudit) {
    compiler->constexpr_eval_mode = kConstexprEvalAST;
  }
  ConstexprValue value;
  bool ast_ok = EvaluateConstexprCall(ctx, node, &value) &&
                ConstexprValueAsInteger(value, result);
  compiler->constexpr_eval_mode = mode;
  if (mode == kConstexprEvalAudit &&
      (pcode_ok != ast_ok || (pcode_ok && pcode_result != *result))) {
    if (pcode_ok && ast_ok) {
      SemanticError(node,
                    "constexpr evaluator mismatch: pcode=%lld, ast=%lld",
                    (long long)pcode_result, (long long)*result);
    } else {
      SemanticError(node,
                    pcode_ok
                        ? "constexpr evaluator mismatch"
                        : "constexpr evaluator mismatch: pcode failed: %s",
                    ConstexprPCodeFailureReason());
    }
    return false;
  }
  if (pcode_ok) {
    *result = pcode_result;
    return true;
  }
  return ast_ok;
}

bool ConstexprEvaluateCallAsFloating(ConstEvalContext* ctx, ASTNode* node,
                                     double* result) {
  (void)ConstexprFunctionDefinition(ConstexprCallSymbol(node));
  ConstexprEvalMode mode = compiler->constexpr_eval_mode;
  bool use_overlay_result = false;
  double overlay_result = 0;
  if ((mode == kConstexprEvalPCode || mode == kConstexprEvalAuto) &&
      ConstexprPCodeRequiresASTOverlay(node)) {
    compiler->constexpr_eval_mode = kConstexprEvalAST;
    ConstexprValue overlay_value;
    bool overlay_ok = EvaluateConstexprCall(ctx, node, &overlay_value) &&
                      ConstexprValueAsFloating(overlay_value, &overlay_result);
    compiler->constexpr_eval_mode = mode;
    if (!overlay_ok) {
      return false;
    }
    use_overlay_result = true;
  }
  double pcode_result = 0;
  bool pcode_ok =
      mode != kConstexprEvalAST &&
      ConstexprPCodeEvaluateCallAsFloating(ctx, node, &pcode_result);
  if (mode == kConstexprEvalPCode) {
    if (!pcode_ok) {
      ReportConstexprPCodeFailure(node, false);
      return false;
    }
    *result = use_overlay_result ? overlay_result : pcode_result;
    return true;
  }
  if (mode == kConstexprEvalAuto && pcode_ok) {
    *result = use_overlay_result ? overlay_result : pcode_result;
    return true;
  }
  if (mode == kConstexprEvalAudit) {
    compiler->constexpr_eval_mode = kConstexprEvalAST;
  }
  ConstexprValue value;
  bool ast_ok = EvaluateConstexprCall(ctx, node, &value) &&
                ConstexprValueAsFloating(value, result);
  compiler->constexpr_eval_mode = mode;
  if (mode == kConstexprEvalAudit &&
      (pcode_ok != ast_ok ||
       (pcode_ok &&
        memcmp(&pcode_result, result, sizeof(pcode_result)) != 0))) {
    SemanticError(node, "constexpr evaluator mismatch%s",
                  pcode_ok ? "" : ": pcode evaluation failed");
    return false;
  }
  if (pcode_ok) {
    *result = pcode_result;
    return true;
  }
  return ast_ok;
}

bool ConstexprEvaluateCallAsObject(ConstEvalContext* ctx, ASTNode* node) {
  (void)ConstexprFunctionDefinition(ConstexprCallSymbol(node));
  ConstexprEvalMode mode = compiler->constexpr_eval_mode;
  if ((mode == kConstexprEvalPCode || mode == kConstexprEvalAuto) &&
      ConstexprPCodeRequiresASTOverlay(node)) {
    compiler->constexpr_eval_mode = kConstexprEvalAST;
    ConstexprValue overlay_value;
    bool overlay_ok = EvaluateConstexprCall(ctx, node, &overlay_value) &&
                      overlay_value.is_object &&
                      overlay_value.object != NULL;
    compiler->constexpr_eval_mode = mode;
    if (!overlay_ok) {
      return false;
    }
  }
  bool pcode_ok =
      mode != kConstexprEvalAST &&
      ConstexprPCodeEvaluateCallAsObject(ctx, node);
  if (mode == kConstexprEvalPCode) {
    if (!pcode_ok) {
      ReportConstexprPCodeFailure(node, false);
    }
    return pcode_ok;
  }
  if (mode == kConstexprEvalAuto && pcode_ok) {
    return true;
  }
  if (mode == kConstexprEvalAudit) {
    compiler->constexpr_eval_mode = kConstexprEvalAST;
  }
  ConstexprValue value;
  bool ast_ok = EvaluateConstexprCall(ctx, node, &value) && value.is_object &&
                value.object != NULL;
  compiler->constexpr_eval_mode = mode;
  if (mode == kConstexprEvalAudit && pcode_ok != ast_ok) {
    SemanticError(node,
                  pcode_ok
                      ? "constexpr evaluator mismatch"
                      : "constexpr evaluator mismatch: pcode failed: %s",
                  ConstexprPCodeFailureReason());
    return false;
  }
  return pcode_ok || ast_ok;
}

bool ConstexprEvaluateConstructorCallForSymbol(ConstEvalContext* ctx,
                                               ASTNode* node,
                                               Symbol* symbol) {
  ASTNode* receiver = NULL;
  Symbol* constructor = ConstexprRawConstructorCallSymbol(node, &receiver);
  if (constructor == NULL) {
    constructor = ConstexprFunctionDefinition(ConstexprCallSymbol(node));
    if (constructor != NULL && constructor->type != NULL &&
        TypeIsFunction(constructor->type) &&
        constructor->type->info.function.is_constructor &&
        node->op == AST_OP(call) &&
        ((VectorASTNode*)node)->children->length > 0) {
      receiver = ((VectorASTNode*)node)->children->value.p[0];
      if (receiver != NULL && receiver->op == AST_OP(address)) {
        receiver = ((UnaryASTNode*)receiver)->sub;
      }
    }
  }
  if (constructor == NULL || receiver == NULL ||
      receiver->op != AST_OP(identifier) || receiver->type == NULL ||
      ((IdentifierASTNode*)receiver)->symbol != symbol) {
    return false;
  }
  size_t mark = ctx->bindings.length;
  ConstexprObject* object = NewConstexprObject(
      ctx, receiver->type, ConstexprObjectSlotCount(receiver->type));
  PushConstexprBinding(
      ctx, symbol, (ConstexprValue){.is_object = true, .object = object});
  bool ok = EvaluateConstexprConstructorCall(ctx, node);
  PopConstexprBindings(ctx, mark);
  if (ok) {
    symbol->flags.value_set = true;
    symbol->value.other = object;
  }
  return ok;
}

bool ConstexprEvaluateCall(ConstEvalContext* ctx, ASTNode* node) {
  ConstexprEvalMode mode = compiler->constexpr_eval_mode;
  if ((mode == kConstexprEvalPCode || mode == kConstexprEvalAuto) &&
      ConstexprPCodeRequiresASTOverlay(node)) {
    compiler->constexpr_eval_mode = kConstexprEvalAST;
    ConstexprValue overlay_value = {0};
    bool overlay_ok = EvaluateConstexprCall(ctx, node, &overlay_value);
    compiler->constexpr_eval_mode = mode;
    if (!overlay_ok) {
      return false;
    }
  }
  bool pcode_ok =
      (mode == kConstexprEvalPCode || mode == kConstexprEvalAudit) &&
      ConstexprPCodeEvaluateCall(ctx, node);
  if (mode == kConstexprEvalPCode) {
    if (!pcode_ok) {
      ReportConstexprPCodeFailure(node, false);
    }
    return pcode_ok;
  }
  if (mode == kConstexprEvalAudit) {
    compiler->constexpr_eval_mode = kConstexprEvalAST;
  }
  bool ast_ok = false;
  if (EvaluateConstexprConstructorCall(ctx, node)) {
    ast_ok = true;
    goto done;
  }
  ASTNode* receiver = NULL;
  Symbol* constructor = ConstexprRawConstructorCallSymbol(node, &receiver);
  if (constructor == NULL) {
    constructor = ConstexprFunctionDefinition(ConstexprCallSymbol(node));
    if (constructor != NULL && constructor->type != NULL &&
        TypeIsFunction(constructor->type) &&
        constructor->type->info.function.is_constructor &&
        node->op == AST_OP(call) &&
        ((VectorASTNode*)node)->children->length > 0) {
      receiver = ((VectorASTNode*)node)->children->value.p[0];
      if (receiver != NULL && receiver->op == AST_OP(address)) {
        receiver = ((UnaryASTNode*)receiver)->sub;
      }
    }
  }
  if (constructor != NULL && receiver != NULL &&
      receiver->op == AST_OP(identifier) && receiver->type != NULL) {
    size_t mark = ctx->bindings.length;
    ConstexprObject* object = NewConstexprObject(
        ctx, receiver->type, ConstexprObjectSlotCount(receiver->type));
    PushConstexprBinding(
        ctx, ((IdentifierASTNode*)receiver)->symbol,
        (ConstexprValue){.is_object = true, .object = object});
    bool ok = EvaluateConstexprConstructorCall(ctx, node);
    PopConstexprBindings(ctx, mark);
    ast_ok = ok;
    goto done;
  }
  if (EvaluateConstexprDestructorCall(ctx, node)) {
    ast_ok = true;
    goto done;
  }
  ConstexprValue value = {0};
  ast_ok = EvaluateConstexprCall(ctx, node, &value);
done:
  compiler->constexpr_eval_mode = mode;
  if (mode == kConstexprEvalAudit && pcode_ok != ast_ok) {
    SemanticError(node,
                  pcode_ok
                      ? "constexpr evaluator mismatch"
                      : "constexpr evaluator mismatch: pcode failed: %s",
                  ConstexprPCodeFailureReason());
    return false;
  }
  return ast_ok;
}

static bool EvaluateConstexprDestructorCall(ConstEvalContext* ctx,
                                            ASTNode* node) {
  if (ctx->call_depth > 32) {
    return false;
  }
  Symbol* callee = ConstexprFunctionDefinition(ConstexprCallSymbol(node));
  if (callee == NULL || callee->type == NULL || !TypeIsFunction(callee->type)) {
    return false;
  }
  TypeRecord* func = callee->type;
  if (!func->info.function.is_constexpr ||
      func->info.function.body == NULL ||
      !func->info.function.is_destructor ||
      func->info.function.is_constructor ||
      func->info.function.is_virtual ||
      func->info.function.varargs) {
    return false;
  }

  VectorASTNode* call = (VectorASTNode*)node;
  size_t mark = ctx->bindings.length;
  ctx->call_depth++;
  ConstexprValue ignored = {0};
  bool ok = BindConstexprActuals(ctx, callee, call->children) &&
            EvaluateConstexprStatement(ctx, func->info.function.body,
                                       func->next, &ignored) ==
                kConstexprStmtNormal;
  ctx->call_depth--;
  PopConstexprBindings(ctx, mark);
  return ok;
}

static bool EvaluateConstexprConstructorCall(ConstEvalContext* ctx,
                                             ASTNode* node) {
  if (ctx->call_depth > 32) {
    return false;
  }
  ASTNode* receiver = NULL;
  Symbol* callee = ConstexprFunctionDefinition(ConstexprCallSymbol(node));
  if (callee == NULL) {
    callee = ConstexprFunctionDefinition(
        ConstexprRawConstructorCallSymbol(node, &receiver));
  }
  if (callee == NULL || callee->type == NULL || !TypeIsFunction(callee->type)) {
    return false;
  }
  TypeRecord* func = callee->type;
  if (!func->info.function.is_constexpr ||
      func->info.function.body == NULL ||
      !func->info.function.is_constructor ||
      func->info.function.is_destructor ||
      func->info.function.is_virtual ||
      func->info.function.varargs) {
    return false;
  }

  VectorASTNode* call = (VectorASTNode*)node;
  size_t mark = ctx->bindings.length;
  ctx->call_depth++;
  ConstexprValue ignored = {0};
  bool ok = BindConstexprConstructorActuals(ctx, callee, receiver,
                                           call->children) &&
            EvaluateConstexprStatement(ctx, func->info.function.body,
                                       func->next, &ignored) ==
                kConstexprStmtNormal;
  ctx->call_depth--;
  PopConstexprBindings(ctx, mark);
  return ok;
}

static bool EvaluateConstexprConstructorCallForObject(ConstEvalContext* ctx,
                                                      ASTNode* node,
                                                      ConstexprObject* object) {
  if (ctx->call_depth > 32 || object == NULL) {
    return false;
  }
  VectorASTNode* call = (VectorASTNode*)node;
  Symbol* callee = ConstexprFunctionDefinition(ConstexprCallSymbol(node));
  if (callee == NULL) {
    callee = ConstexprFunctionDefinition(
        ConstexprConstructorForObjectType(object->type, call->children->length));
  }
  if (callee == NULL || callee->type == NULL || !TypeIsFunction(callee->type)) {
    return false;
  }
  TypeRecord* func = callee->type;
  if (!func->info.function.is_constexpr ||
      func->info.function.body == NULL ||
      !func->info.function.is_constructor ||
      func->info.function.is_destructor ||
      func->info.function.is_virtual ||
      func->info.function.varargs) {
    return false;
  }

  size_t mark = ctx->bindings.length;
  ctx->call_depth++;
  ConstexprValue ignored = {0};
  bool ok = BindConstexprConstructorObjectActuals(ctx, callee, object,
                                                  call->children) &&
            EvaluateConstexprStatement(ctx, func->info.function.body,
                                       func->next, &ignored) ==
                kConstexprStmtNormal;
  ctx->call_depth--;
  PopConstexprBindings(ctx, mark);
  return ok;
}
