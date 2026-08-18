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
#include "reflection.h"
#include "reflection_meta_synthesis.h"
#include "reflection_semantics.h"
#include "semantics.h"
#include "type.h"
#include "type_internal.h"
#include "type_member.h"
#include "type_template.h"

typedef struct ConstexprBinding ConstexprBinding;

static void ReportConstexprPCodeFailure(ConstEvalContext* ctx, ASTNode* node,
                                        bool always) {
  if (always ||
      (compiler->current_function == NULL &&
       compiler->constant_evaluation_required_depth > 0)) {
    SemanticError(node, "constexpr pcode evaluation failed: %s",
                  ConstexprPCodeFailureReason(ctx));
  }
}

struct ConstexprHeapBlock {
  unsigned char* memory;
  size_t size;
  size_t allocation_size;
  bool live;
  TypeRecord* object_type;
};

struct ConstexprValue {
  bool is_object;
  bool is_address;
  bool is_floating;
  bool lifetime_ended;
  int64_t ivalue;
  double fvalue;
  ConstexprObject* object;
  ConstexprBinding* address_binding;
  ConstexprValue* address_slot;
  ConstexprObject* address_object;
  size_t address_index;
  ConstexprHeapBlock* heap_block;
  size_t heap_index;
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
  ConstexprHeapBlock* heap_block;
  size_t heap_index;
  TypeRecord* address_type;
  bool address_storage_began;
};

struct ConstexprObject {
  TypeRecord* type;
  Vector slots;  // ConstexprValue*
  StructMember* active_union_member;
  bool lifetime_ended;
  ConstexprObject* complete_object;
  size_t complete_offset;
};

struct ConstexprException {
  TypeRecord* type;
  ConstexprValue value;
  ASTNode* throw_node;
  SourceLocation throw_location;
  bool handling;
  bool reported;
  bool active;
  bool destroyed;
  size_t references;
  void* token;
  ConstexprException* previous;
};

typedef enum {
  kConstexprStmtInvalid,
  kConstexprStmtNormal,
  kConstexprStmtReturn,
  kConstexprStmtBreak,
  kConstexprStmtContinue,
  kConstexprStmtThrow,
} ConstexprStatementResult;

#define CONSTEXPR_MAX_CALL_DEPTH 512
#define CONSTEXPR_MAX_STEPS 1000000

static int template_argument_object_evaluation_depth;
static int symbolic_constexpr_reference_depth;
#define CONSTEXPR_HEAP_SIZE (256 * 1024)

static ConstexprObject* CloneConstexprObject(ConstEvalContext* ctx,
                                             ConstexprObject* object);
static void ConstexprCanonicalizeAddressValue(ConstexprValue* value);
static ConstexprObject* ConstexprAddressTargetObject(ConstEvalContext* ctx,
                                                     ConstexprValue value);
static ConstexprValue ConstexprResolveForwardedAddress(ConstexprValue value);
static bool TypeIsBasicStringViewType(TypeRecord* type);
static bool ConstexprStringViewDataCharacter(ConstEvalContext* ctx,
                                             ConstexprValue* data_slot,
                                             size_t index,
                                             int64_t* character);
static ConstexprObject* NewConstexprObject(ConstEvalContext* ctx,
                                           TypeRecord* type,
                                           size_t slot_count);
static size_t ConstexprObjectSlotCount(TypeRecord* type);
static ConstexprValue* ConstexprObjectSlot(ConstexprObject* object,
                                           size_t index);
static void PushConstexprBinding(ConstEvalContext* ctx, Symbol* symbol,
                                 ConstexprValue value);
static void PopConstexprBindings(ConstEvalContext* ctx, size_t mark);
bool ConstexprValueAsInteger(ConstexprValue value, int64_t* result);
static bool EvaluateConstexprValue(ConstEvalContext* ctx, ASTNode* node,
                                   TypeRecord* type, ConstexprValue* result);
static bool EvaluateConstexprVoidExpression(ConstEvalContext* ctx,
                                            ASTNode* expr);
static bool EvaluateConstexprAddressValue(ConstEvalContext* ctx, ASTNode* node,
                                          ConstexprValue* result);
static Symbol* ConstexprCallSymbol(ASTNode* node);
static TypeRecord* ConstexprPlainObjectType(TypeRecord* type);
static void ConstexprReleasePlainObjectType(TypeRecord* requested,
                                            TypeRecord* plain);
static bool EvaluateConstexprFunctionContracts(ConstEvalContext* ctx,
                                               TypeRecord* func,
                                               ContractAssertionKind kind,
                                               ConstexprValue* result);
static ConstexprStatementResult EvaluateConstexprStatement(
    ConstEvalContext* ctx, ASTNode* stmt, TypeRecord* return_type,
    ConstexprValue* result);
static bool ConstexprNullAddress(ConstexprValue* result);
static Symbol* ConstexprVirtualCallSymbol(ConstEvalContext* ctx, ASTNode* node,
                                          ASTNode** receiver);
static bool ConstexprObjectsTemplateArgumentEquivalent(ConstexprObject* left,
                                                       ConstexprObject* right);

static void ReportConstexprPlacementFailure(ASTNode* node,
                                            const char* message) {
  if (node != NULL && !DiagnosticsSuppressed() &&
      (node->flags & kASTConstexprPlacementDiagnosed) == 0) {
    SemanticError(node, "%s", message);
    node->flags |= kASTConstexprPlacementDiagnosed;
  }
}

static size_t ConstexprAlignHeapSize(size_t size) {
  return (size + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1);
}

static ConstexprHeapBlock* ConstexprFindHeapBlock(ConstEvalContext* ctx,
                                                  void* memory) {
  if (ctx == NULL || memory == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < ctx->heap_blocks.length; i++) {
    ConstexprHeapBlock* block = ctx->heap_blocks.value.p[i];
    if (block != NULL && block->live && block->memory == memory) {
      return block;
    }
  }
  return NULL;
}

static bool ConstexprHasHeapBlock(ConstEvalContext* ctx,
                                  ConstexprHeapBlock* requested) {
  if (requested == NULL) {
    return false;
  }
  for (size_t i = 0; i < ctx->heap_blocks.length; i++) {
    if (ctx->heap_blocks.value.p[i] == requested) {
      return true;
    }
  }
  return false;
}

static bool ConstexprContextOwnsObject(ConstEvalContext* ctx,
                                       ConstexprObject* requested) {
  if (ctx == NULL || requested == NULL) {
    return false;
  }
  for (size_t i = 0; i < ctx->objects.length; ++i) {
    if (ctx->objects.value.p[i] == requested) {
      return true;
    }
  }
  return false;
}

static bool ConstexprContextOwnsSlot(ConstEvalContext* ctx,
                                     ConstexprValue* requested) {
  if (ctx == NULL || requested == NULL) {
    return false;
  }
  for (size_t i = 0; i < ctx->objects.length; ++i) {
    ConstexprObject* object = ctx->objects.value.p[i];
    if (object == NULL) {
      continue;
    }
    for (size_t j = 0; j < object->slots.length; ++j) {
      if (object->slots.value.p[j] == requested) {
        return true;
      }
    }
  }
  return false;
}

static ConstexprValue* ConstexprSlotOwningObject(ConstEvalContext* ctx,
                                                ConstexprObject* requested) {
  if (ctx == NULL || requested == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < ctx->objects.length; i++) {
    ConstexprObject* object = ctx->objects.value.p[i];
    if (object == NULL) {
      continue;
    }
    for (size_t j = 0; j < object->slots.length; j++) {
      ConstexprValue* slot = object->slots.value.p[j];
      if (slot != NULL && slot->is_object && slot->object == requested) {
        return slot;
      }
    }
  }
  return NULL;
}

static bool ConstexprContextOwnsBinding(ConstEvalContext* ctx,
                                        ConstexprBinding* requested) {
  if (ctx == NULL || requested == NULL) {
    return false;
  }
  for (size_t i = 0; i < ctx->bindings.length; ++i) {
    if (ctx->bindings.value.p[i] == requested) {
      return true;
    }
  }
  return false;
}

static TypeRecord* ConstexprAddressPointeeType(ConstEvalContext* ctx,
                                               ConstexprValue value) {
  if (value.heap_block != NULL && value.heap_block->object_type != NULL) {
    return value.heap_block->object_type;
  }
  if (ConstexprContextOwnsBinding(ctx, value.address_binding)) {
    ConstexprBinding* binding = value.address_binding;
    if (binding->address_type != NULL) {
      return binding->address_type;
    }
    if (binding->symbol != NULL && binding->symbol->type != NULL) {
      TypeRecord* type = binding->symbol->type;
      return (TypeIsPointer(type) || TypeIsReference(type)) && type->next != NULL
                 ? type->next : type;
    }
  }
  if (ConstexprContextOwnsObject(ctx, value.address_object) &&
      value.address_object->type != NULL) {
    TypeRecord* type = value.address_object->type;
    return TypeIsFixedArray(type) && type->next != NULL ? type->next : type;
  }
  if (ConstexprContextOwnsSlot(ctx, value.address_slot) &&
      value.address_slot->is_object &&
      value.address_slot->object != NULL) {
    return value.address_slot->object->type;
  }
  return NULL;
}

static bool ConstexprAddressStorageBegan(ConstEvalContext* ctx,
                                         ConstexprValue value) {
  if (value.heap_block != NULL ||
      ConstexprContextOwnsObject(ctx, value.address_object) ||
      ConstexprContextOwnsSlot(ctx, value.address_slot)) {
    return true;
  }
  if (ConstexprContextOwnsBinding(ctx, value.address_binding)) {
    ConstexprBinding* binding = value.address_binding;
    return binding->address_storage_began ||
           (binding->symbol != NULL && !binding->symbol->flags.is_argument);
  }
  return false;
}

static bool ConstexprHasLiveExceptionHandles(ConstEvalContext* ctx) {
  for (size_t i = 0; i < ctx->exception_handles.length; i++) {
    ConstexprException* exception = ctx->exception_handles.value.p[i];
    if (exception != NULL && exception->references != 0) {
      return true;
    }
  }
  return false;
}

static bool ConstexprObjectHasInvalidAddress(ConstEvalContext* ctx,
                                             ConstexprObject* object) {
  if (object == NULL) {
    return false;
  }
  if (object->type != NULL && TypeIsStructOrUnion(object->type) &&
      object->type->info.struct_info != NULL &&
      object->type->info.struct_info->tag_name != NULL &&
      object->type->info.struct_info->tag_name->value != NULL &&
      strcmp(object->type->info.struct_info->tag_name->value,
             "exception_ptr") == 0) {
    ConstexprValue* pointer = ConstexprObjectSlot(object, 0);
    if (pointer != NULL &&
        (pointer->ivalue != 0 || pointer->heap_block != NULL ||
         pointer->address_binding != NULL || pointer->address_slot != NULL ||
         pointer->address_object != NULL)) {
      return true;
    }
  }
  for (size_t i = 0; i < object->slots.length; i++) {
    ConstexprValue* slot = object->slots.value.p[i];
    if (slot == NULL) {
      continue;
    }
    if (slot->is_address && slot->heap_block != NULL &&
        (!ConstexprHasHeapBlock(ctx, slot->heap_block) ||
         !slot->heap_block->live)) {
      return true;
    }
    if (slot->is_object &&
        ConstexprObjectHasInvalidAddress(ctx, slot->object)) {
      return true;
    }
  }
  return false;
}

static void* ConstexprHeapMalloc(ConstEvalContext* ctx, size_t size,
                                 size_t* allocated_size) {
  if (ctx == NULL) {
    return NULL;
  }
  size_t object_size = size == 0 ? 1 : size;
  size_t allocation_size = ConstexprAlignHeapSize(object_size);
  unsigned char* memory = calloc(1, allocation_size);
  if (memory == NULL) {
    return NULL;
  }
  ConstexprHeapBlock* block = malloc(sizeof(*block));
  if (block == NULL) {
    free(memory);
    return NULL;
  }
  *block = (ConstexprHeapBlock){
      .memory = memory,
      .size = object_size,
      .allocation_size = allocation_size,
      .live = true,
  };
  VectorAppend(&ctx->heap_blocks, block);
  if (allocated_size != NULL) {
    *allocated_size = allocation_size;
  }
  return memory;
}

static void ConstexprHeapFree(ConstEvalContext* ctx, void* memory) {
  ConstexprHeapBlock* block = ConstexprFindHeapBlock(ctx, memory);
  if (block != NULL) {
    block->live = false;
  }
}

static bool ConstexprHeapAddress(ConstEvalContext* ctx, void* memory,
                                 size_t index, ConstexprValue* result) {
  ConstexprHeapBlock* block = ConstexprFindHeapBlock(ctx, memory);
  if (block == NULL || index >= block->size) {
    return false;
  }
  *result = (ConstexprValue){
      .is_address = true,
      .heap_block = block,
      .heap_index = index,
  };
  return true;
}

static bool ConstexprValueFromHeapAddress(ConstEvalContext* ctx,
                                          ConstexprValue address,
                                          TypeRecord* type,
                                          ConstexprValue* result) {
  if (!address.is_address || address.heap_block == NULL ||
      !address.heap_block->live ||
      address.heap_index >= address.heap_block->size) {
    return false;
  }
  unsigned char* byte =
      address.heap_block->memory + address.heap_index;
  if (type != NULL && TypeIsCharFamily(type)) {
    *result = (ConstexprValue){.ivalue = (int64_t)(signed char)*byte};
    return true;
  }
  if (type != NULL && TypeIsIntegral(type)) {
    int64_t value = 0;
    size_t size = (size_t)type->size;
    if (address.heap_index + size > address.heap_block->size) {
      return false;
    }
    memcpy(&value, byte, size);
    *result = (ConstexprValue){.ivalue = value};
    return true;
  }
  (void)ctx;
  return false;
}

static bool ConstexprPointerValueFromInteger(ConstEvalContext* ctx,
                                             int64_t ivalue,
                                             ConstexprValue* result) {
  if (ivalue == 0) {
    return ConstexprNullAddress(result);
  }
  void* memory = (void*)(intptr_t)ivalue;
  return ConstexprHeapAddress(ctx, memory, 0, result);
}

static Symbol* ConstexprFindSpecialMember(TypeRecord* type,
                                          CXXSpecialMemberKind kind) {
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL) {
    return NULL;
  }
  Struct* str = type->info.struct_info;
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    for (StructMember* candidate = member; candidate != NULL;
         candidate = candidate->overload_next) {
      Symbol* symbol = candidate->symbol;
      if (symbol != NULL && TypeIsFunction(symbol->type) &&
          symbol->type->info.function.cxx_special_member_kind == kind &&
          !symbol->type->info.function.is_deleted) {
        return symbol;
      }
    }
  }
  return NULL;
}

static bool ConstexprStructCopySlots(ConstEvalContext* ctx,
                                     ConstexprObject* source,
                                     ConstexprObject* dest) {
  if (source == NULL || dest == NULL ||
      !TypeEqual(source->type, dest->type)) {
    return false;
  }
  dest->active_union_member = source->active_union_member;
  dest->lifetime_ended = source->lifetime_ended;
  for (size_t i = 0; i < source->slots.length; i++) {
    ConstexprValue* from = source->slots.value.p[i];
    ConstexprValue* to = dest->slots.value.p[i];
    if (from == NULL || to == NULL) {
      return false;
    }
    if (from->is_object) {
      to->is_object = true;
      to->is_address = false;
      to->is_floating = false;
      to->ivalue = 0;
      to->fvalue = 0;
      to->heap_block = NULL;
      to->heap_index = 0;
      to->object = CloneConstexprObject(ctx, from->object);
      if (to->object == NULL) {
        return false;
      }
      continue;
    }
    *to = *from;
    if (from->is_address && from->address_slot != NULL) {
      to->address_slot = dest->slots.value.p[i];
    }
  }
  return true;
}

static bool ConstexprInvokeSpecialConstructor(ConstEvalContext* ctx,
                                              TypeRecord* type,
                                              ConstexprObject* dest,
                                              ConstexprObject* source,
                                              bool move) {
  if (dest == NULL || source == NULL || type == NULL) {
    return false;
  }
  CXXSpecialMemberKind kind = move ? kCXXSpecialMemberMoveConstructor
                                   : kCXXSpecialMemberCopyConstructor;
  Symbol* ctor_symbol = ConstexprFindSpecialMember(type, kind);
  if (ctor_symbol == NULL && move) {
    kind = kCXXSpecialMemberCopyConstructor;
    move = false;
    ctor_symbol = ConstexprFindSpecialMember(type, kind);
  }
  if (ctor_symbol == NULL ||
      ctor_symbol->type->info.function.is_trivial_special_member) {
    return ConstexprStructCopySlots(ctx, source, dest);
  }
  Symbol* ctor = ConstexprFunctionDefinition(ctor_symbol);
  if (ctor == NULL || ctor->type == NULL ||
      !ctor->type->info.function.is_constexpr ||
      ctor->type->info.function.body == NULL ||
      !ctor->type->info.function.is_constructor) {
    return ConstexprStructCopySlots(ctx, source, dest);
  }
  if (ctor->type->info.function.prototype.length != 2) {
    return false;
  }
  Symbol* this_formal = ctor->type->info.function.prototype.value.p[0];
  Symbol* source_formal = ctor->type->info.function.prototype.value.p[1];
  if (this_formal == NULL || source_formal == NULL) {
    return false;
  }
  size_t mark = ctx->bindings.length;
  ctx->call_depth++;
  PushConstexprBinding(ctx, this_formal,
                       (ConstexprValue){.is_object = true, .object = dest});
  PushConstexprBinding(ctx, source_formal,
                       (ConstexprValue){.is_object = true, .object = source});
  ConstexprValue ignored = {0};
  bool pre = EvaluateConstexprFunctionContracts(
      ctx, ctor->type, kContractPrecondition, &ignored);
  ConstexprStatementResult body =
      pre ? EvaluateConstexprStatement(ctx, ctor->type->info.function.body,
                                       ctor->type->next, &ignored)
          : kConstexprStmtInvalid;
  bool post =
      body == kConstexprStmtNormal &&
      EvaluateConstexprFunctionContracts(
          ctx, ctor->type, kContractPostcondition, &ignored);
  bool ok = pre && body == kConstexprStmtNormal && post;
  ctx->call_depth--;
  PopConstexprBindings(ctx, mark);
  return ok;
}

static bool ConstexprCopyConstructObject(ConstEvalContext* ctx,
                                         TypeRecord* type,
                                         ConstexprObject* source,
                                         ConstexprObject** result,
                                         bool move) {
  if (ctx == NULL || source == NULL || result == NULL || type == NULL) {
    return false;
  }
  TypeRecord* plain = ConstexprPlainObjectType(type);
  ConstexprObject* dest =
      NewConstexprObject(ctx, plain, ConstexprObjectSlotCount(plain));
  if (dest == NULL) {
    ConstexprReleasePlainObjectType(type, plain);
    return false;
  }
  bool ok = ConstexprInvokeSpecialConstructor(ctx, plain, dest, source, move);
  ConstexprReleasePlainObjectType(type, plain);
  if (!ok) {
    return false;
  }
  *result = dest;
  return true;
}

static Symbol* ConstexprResolveVirtualMember(ConstexprObject* object,
                                             Symbol* member) {
  if (object == NULL || member == NULL || member->type == NULL ||
      !TypeIsFunction(member->type) ||
      !member->type->info.function.is_virtual) {
    return member;
  }
  int vindex = member->type->info.function.virtual_index;
  if (vindex < 0 || object->type == NULL ||
      !TypeIsStructOrUnion(object->type) ||
      object->type->info.struct_info == NULL) {
    return member;
  }
  Struct* str = object->type->info.struct_info;
  if ((size_t)vindex >= str->virtual_members.length) {
    return member;
  }
  StructMember* resolved = str->virtual_members.value.p[vindex];
  if (resolved == NULL || resolved->symbol == NULL) {
    return member;
  }
  return resolved->symbol;
}

static bool ConstexprIsAllocationFunction(Symbol* symbol) {
  return symbol != NULL &&
         (StringEqual(&symbol->name, "operator new") ||
          StringEqual(&symbol->name, "operator new[]") ||
          StringEqual(&symbol->name, "malloc"));
}

static bool ConstexprIsDeallocationFunction(Symbol* symbol) {
  return symbol != NULL &&
         (StringEqual(&symbol->name, "operator delete") ||
          StringEqual(&symbol->name, "operator delete[]") ||
          StringEqual(&symbol->name, "free"));
}

static CastASTNode* ConstexprAllocationNewCast(ASTNode* allocation) {
  for (ASTNode* parent = allocation != NULL ? allocation->parent : NULL;
       parent != NULL; parent = parent->parent) {
    if (parent->op == AST_OP(cast) &&
        (parent->flags & kASTCXXNewExpression) != 0) {
      return (CastASTNode*)parent;
    }
    if (parent->op == AST_OP(expr) || parent->op == AST_OP(compound) ||
        parent->op == AST_OP(call)) {
      break;
    }
  }
  return NULL;
}

static bool ConstexprEvaluateAllocationCall(ConstEvalContext* ctx,
                                            ASTNode* node,
                                            ConstexprValue* result) {
  if (node == NULL || node->op != AST_OP(call) || result == NULL) {
    return false;
  }
  VectorASTNode* call = (VectorASTNode*)node;
  if (call->children == NULL || call->children->length == 0) {
    return false;
  }
  if (call->children->length >= 2) {
    ASTNode* placement = call->children->value.p[1];
    Symbol* allocation = ConstexprCallSymbol(node);
    TypeRecord* allocation_type =
        allocation != NULL ? allocation->type : NULL;
    Symbol* placement_formal =
        allocation_type != NULL && TypeIsFunction(allocation_type) &&
                allocation_type->info.function.prototype.length >= 2
            ? allocation_type->info.function.prototype.value.p[1] : NULL;
    TypeRecord* placement_type =
        placement_formal != NULL ? placement_formal->type : NULL;
    if (placement != NULL && placement_type != NULL &&
        TypeIsPointer(placement_type) &&
        placement_type->next != NULL &&
        TypeIsVoid(placement_type->next)) {
      CastASTNode* new_cast = ConstexprAllocationNewCast(node);
      if (new_cast == NULL && ctx->allocation_new_expression != NULL &&
          ctx->allocation_new_expression->op == AST_OP(cast)) {
        new_cast = (CastASTNode*)ctx->allocation_new_expression;
      }
      if (!CompilerCXXAtLeast(kLanguageStandardCXX26)) {
        ReportConstexprPlacementFailure(
            node, "placement new is not permitted in this constant expression");
        return false;
      }
      if (new_cast == NULL) {
        ReportConstexprPlacementFailure(
            node, "placement allocation is not associated with a new-expression");
        return false;
      }
      ASTNode* placement_core = placement;
      while (placement_core != NULL &&
             (placement_core->op == AST_OP(cast) ||
              placement_core->op == AST_OP(expr_init))) {
        placement_core =
            placement_core->op == AST_OP(cast)
                ? ((CastASTNode*)placement_core)->expr
                : ((ExpressionInitializerASTNode*)placement_core)->expr;
      }
      if (placement_core != NULL &&
          placement_core->op == AST_OP(address) &&
          ((UnaryASTNode*)placement_core)->sub != NULL &&
          ((UnaryASTNode*)placement_core)->sub->op == AST_OP(identifier)) {
        Symbol* target =
            ((IdentifierASTNode*)((UnaryASTNode*)placement_core)->sub)->symbol;
        if (target != NULL && !target->flags.is_local &&
            !StorageIs(target->storage, STO(static))) {
          ReportConstexprPlacementFailure(
              node, "placement new target is not a constant address");
          return false;
        }
      }
      if (!EvaluateConstexprAddressValue(ctx, placement, result)) {
        ReportConstexprPlacementFailure(
            node, "placement new target is not a constant address");
        return false;
      }
      if (result->address_binding != NULL &&
          result->address_binding->symbol != NULL &&
          !result->address_binding->symbol->flags.is_local &&
          !StorageIs(result->address_binding->symbol->storage, STO(static))) {
        ReportConstexprPlacementFailure(
            node, "placement new target is not a constant address");
        return false;
      }
      TypeRecord* allocated_type =
          TypeIsPointer(new_cast->cast_type) ? new_cast->cast_type->next : NULL;
      TypeRecord* placement_pointee =
          placement->type != NULL && TypeIsPointer(placement->type)
              ? placement->type->next : NULL;
      TypeRecord* target_type =
          placement_pointee != NULL && !TypeIsVoid(placement_pointee)
              ? placement_pointee : ConstexprAddressPointeeType(ctx, *result);
      if (allocated_type == NULL || target_type == NULL ||
          !TypeEqualIgnoringTopLevelQualifierMask(
              allocated_type, target_type,
              kQualConst | kQualVolatile | kQualRestrict)) {
        ReportConstexprPlacementFailure(
            node, "placement new target does not point to an object of the "
                  "allocated type");
        return false;
      }
      bool current_storage = ConstexprAddressStorageBegan(ctx, *result);
      if (!current_storage) {
        ReportConstexprPlacementFailure(
            node, "placement new target storage did not begin within this "
                  "constant evaluation");
        return false;
      }
      int64_t requested_size = 0;
      if (!EvaluateIntegerExpressionInContext(
              ctx, call->children->value.p[0], &requested_size) ||
          requested_size < 0) {
        ReportConstexprPlacementFailure(
            node, "placement new size is not a constant expression");
        return false;
      }
      size_t available = 0;
      if (result->heap_block != NULL &&
          result->heap_index <= result->heap_block->size) {
        available = result->heap_block->size - result->heap_index;
      } else if (result->address_object != NULL) {
        TypeRecord* storage_type = result->address_object->type;
        if (TypeIsFixedArray(storage_type) && storage_type->next != NULL &&
            result->address_index <= result->address_object->slots.length) {
          available =
              (result->address_object->slots.length - result->address_index) *
              (size_t)storage_type->next->size;
        } else if (result->address_index == 0 && storage_type != NULL) {
          available = (size_t)storage_type->size;
        }
      } else if (result->address_binding != NULL &&
                 result->address_binding->symbol != NULL &&
                 result->address_binding->symbol->type != NULL) {
        available = (size_t)result->address_binding->symbol->type->size;
      } else if (result->address_slot != NULL && allocated_type->size >= 0) {
        available = (size_t)allocated_type->size;
      }
      if ((uint64_t)requested_size > available) {
        ReportConstexprPlacementFailure(
            node, "placement new exceeds the bounds of its target storage");
        return false;
      }
      ConstexprObject* target_object =
          ConstexprAddressTargetObject(ctx, *result);
      if (target_object != NULL) {
        target_object->lifetime_ended = false;
      }
      ConstexprValue* target_slot = result->address_slot;
      if (target_slot == NULL && result->address_object != NULL) {
        target_slot = ConstexprObjectSlot(result->address_object,
                                          result->address_index);
      }
      if (target_slot != NULL) {
        target_slot->lifetime_ended = false;
      }
      return true;
    }
    ReportConstexprPlacementFailure(
        node, "selected placement allocation function is not permitted in a "
              "constant expression");
    return false;
  }
  ASTNode* size_expr = call->children->value.p[0];
  int64_t size = 0;
  if (!EvaluateIntegerExpressionInContext(ctx, size_expr, &size) || size < 0) {
    return false;
  }
  size_t allocation_size = 0;
  void* memory = ConstexprHeapMalloc(ctx, (size_t)size, &allocation_size);
  if (memory == NULL) {
    return false;
  }
  *result = (ConstexprValue){0};
  return ConstexprHeapAddress(ctx, memory, 0, result);
}

static bool ConstexprEvaluateDeallocationCall(ConstEvalContext* ctx,
                                              ASTNode* node) {
  if (node == NULL || node->op != AST_OP(call)) {
    return false;
  }
  VectorASTNode* call = (VectorASTNode*)node;
  if (call->children == NULL || call->children->length == 0) {
    return false;
  }
  ConstexprValue pointer = {0};
  ASTNode* pointer_arg = call->children->value.p[0];
  if (!EvaluateConstexprValue(ctx, pointer_arg, pointer_arg->type, &pointer)) {
    return false;
  }
  int64_t address = 0;
  if (pointer.is_address && pointer.heap_block != NULL) {
    address = (int64_t)(intptr_t)pointer.heap_block->memory;
  } else if (!ConstexprValueAsInteger(pointer, &address)) {
    return false;
  }
  if (address != 0) {
    ConstexprHeapFree(ctx, (void*)(intptr_t)address);
  }
  return true;
}

static bool ConstexprDereferenceAddress(ConstexprValue address,
                                        ConstexprValue* result);
static bool ConstexprNullAddress(ConstexprValue* result);
static ConstexprValue* ConstexprObjectSlot(ConstexprObject* object,
                                           size_t index);
static bool ConstexprMaterializeBaseSubobjectSlots(ConstEvalContext* ctx,
                                                   ConstexprObject* object);
static TypeRecord* ConstexprExceptionObjectType(TypeRecord* type);

void ConstEvalContextInit(ConstEvalContext* ctx) {
  VectorInit(&ctx->bindings);
  VectorInit(&ctx->objects);
  VectorInit(&ctx->heap_blocks);
  VectorInit(&ctx->exception_handles);
  ctx->exception = NULL;
  ctx->call_depth = 0;
  ctx->steps = 0;
  ctx->max_steps = CONSTEXPR_MAX_STEPS;
  ctx->unwinding_exceptions = 0;
  ctx->destroy_at_depth = 0;
  ctx->allocation_new_expression = NULL;
  ctx->pcode_failure_reason = NULL;
  ctx->pcode_failure_kind = kConstexprPCodeFailureUnsupported;
}

void ConstEvalContextDestruct(ConstEvalContext* ctx) {
  for (size_t i = 0; i < ctx->exception_handles.length; i++) {
    free(ctx->exception_handles.value.p[i]);
  }
  VectorDestruct(&ctx->exception_handles);
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
  for (size_t i = 0; i < ctx->heap_blocks.length; i++) {
    ConstexprHeapBlock* block = ctx->heap_blocks.value.p[i];
    if (block != NULL) {
      free(block->memory);
      free(block);
    }
  }
  VectorDestruct(&ctx->heap_blocks);
}

bool ConstEvalStep(ConstEvalContext* ctx) {
  ctx->steps++;
  return ctx->steps <= ctx->max_steps;
}

bool ConstexprReferenceUsableInCurrentFunction(Symbol* symbol) {
  if (symbol == NULL || !symbol->is_constexpr_representable ||
      symbol->constexpr_reference_scope == NULL ||
      compiler->current_function == NULL) {
    return false;
  }
  TypeRecord* scope = symbol->constexpr_reference_scope;
  TypeRecord* current = compiler->current_function;
  if (scope == current) {
    return true;
  }
  Symbol* scope_symbol =
      TypeIsFunction(scope) ? scope->info.function.symbol : NULL;
  Symbol* current_symbol =
      TypeIsFunction(current) ? current->info.function.symbol : NULL;
  Symbol* current_origin =
      TypeIsFunction(current) ? current->info.function.template_origin : NULL;
  Symbol* scope_origin =
      TypeIsFunction(scope) ? scope->info.function.template_origin : NULL;
  if (scope_symbol != NULL && current_symbol != NULL &&
      StringEqualString(&scope_symbol->name, &current_symbol->name) &&
      scope_symbol->location == current_symbol->location &&
      scope->info.function.cxx_member_owner ==
          current->info.function.cxx_member_owner) {
    return true;
  }
  bool usable = current_origin != NULL &&
         (current_origin == scope_symbol ||
          (scope_origin != NULL && current_origin == scope_origin));
  return usable;
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
  binding->heap_block = value.heap_block;
  binding->heap_index = value.heap_index;
  binding->address_type =
      value.is_address ? ConstexprAddressPointeeType(ctx, value) : NULL;
  binding->address_storage_began =
      value.is_address && ConstexprAddressStorageBegan(ctx, value);
  VectorAppend(&ctx->bindings, binding);
}

bool ConstexprValueAsInteger(ConstexprValue value, int64_t* result) {
  if (value.is_address) {
    if (value.heap_block != NULL) {
      *result = (int64_t)(intptr_t)(value.heap_block->memory +
                                    value.heap_index);
      return true;
    }
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
  object->lifetime_ended = false;
  object->complete_object = object;
  object->complete_offset = 0;
  VectorInit(&object->slots);
  for (size_t i = 0; i < slot_count; i++) {
    VectorAppend(&object->slots, NewConstexprValueSlot());
  }
  if (ctx != NULL) {
    VectorAppend(&ctx->objects, object);
  }
  return object;
}

static void ConstexprCanonicalizeAddressValue(ConstexprValue* value) {
  if (value == NULL || !value->is_address) {
    return;
  }
  *value = ConstexprResolveForwardedAddress(*value);
  for (size_t depth = 0;
       depth < 1024 && value->is_address && value->address_object == NULL &&
       value->heap_block == NULL && value->address_binding != NULL;
       depth++) {
    ConstexprBinding* binding = value->address_binding;
    if (binding->address_object != NULL) {
      value->address_object = binding->address_object;
      if ((uint64_t)value->address_index + (uint64_t)binding->address_index <=
          SIZE_MAX) {
        value->address_index += binding->address_index;
      }
      value->address_binding = NULL;
      value->address_slot = NULL;
      break;
    }
    if (binding->heap_block != NULL) {
      value->heap_block = binding->heap_block;
      value->heap_index += binding->heap_index;
      value->address_binding = NULL;
      value->address_slot = NULL;
      break;
    }
    if (binding->object != NULL) {
      value->address_object = binding->object;
      value->address_binding = NULL;
      value->address_slot = NULL;
      break;
    }
    if (binding->is_address) {
      value->address_binding = binding->address_binding;
      value->address_slot = binding->address_slot;
      value->address_object = binding->address_object;
      value->address_index = binding->address_index;
      value->heap_block = binding->heap_block;
      value->heap_index = binding->heap_index;
      continue;
    }
    break;
  }
}

static ConstexprObject* ConstexprAddressTargetObject(ConstEvalContext* ctx,
                                                     ConstexprValue value) {
  if (ConstexprContextOwnsBinding(ctx, value.address_binding) &&
      value.address_binding->object != NULL) {
    return value.address_binding->object;
  }
  if (ConstexprContextOwnsSlot(ctx, value.address_slot) &&
      value.address_slot->is_object) {
    return value.address_slot->object;
  }
  if (ConstexprContextOwnsObject(ctx, value.address_object)) {
    if (TypeIsFixedArray(value.address_object->type)) {
      ConstexprValue* slot =
          ConstexprObjectSlot(value.address_object, value.address_index);
      return slot != NULL && slot->is_object ? slot->object : NULL;
    }
    return value.address_index == 0 ? value.address_object : NULL;
  }
  return NULL;
}

static ConstexprObject* CloneConstexprObjectImpl(ConstEvalContext* ctx,
                                                 ConstexprObject* object,
                                                 Vector* sources,
                                                 Vector* clones) {
  if (object == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < sources->length; i++) {
    if (sources->value.p[i] == object) {
      return clones->value.p[i];
    }
  }
  ConstexprObject* clone =
      NewConstexprObject(ctx, object->type, object->slots.length);
  VectorAppend(sources, object);
  VectorAppend(clones, clone);
  clone->active_union_member = object->active_union_member;
  clone->lifetime_ended = object->lifetime_ended;
  clone->complete_offset = object->complete_offset;
  for (size_t i = 0; i < object->slots.length; i++) {
    ConstexprValue* from = object->slots.value.p[i];
    ConstexprValue* to = clone->slots.value.p[i];
    *to = *from;
    if (from->is_object) {
      to->object =
          CloneConstexprObjectImpl(ctx, from->object, sources, clones);
      if (to->object == NULL) {
        return NULL;
      }
    }
  }
  clone->complete_object =
      object->complete_object != NULL
          ? CloneConstexprObjectImpl(ctx, object->complete_object, sources,
                                     clones)
          : clone;
  return clone;
}

static ConstexprObject* CloneNonVirtualConstexprObject(
    ConstEvalContext* ctx, ConstexprObject* object) {
  if (object == NULL) {
    return NULL;
  }
  ConstexprObject* clone =
      NewConstexprObject(ctx, object->type, object->slots.length);
  clone->active_union_member = object->active_union_member;
  clone->lifetime_ended = object->lifetime_ended;
  for (size_t i = 0; i < object->slots.length; i++) {
    ConstexprValue* from = object->slots.value.p[i];
    ConstexprValue* to = clone->slots.value.p[i];
    *to = *from;
    if (from->is_object) {
      to->object = CloneNonVirtualConstexprObject(ctx, from->object);
      if (to->object == NULL) {
        return NULL;
      }
    }
  }
  return clone;
}

static ConstexprObject* CloneConstexprObject(ConstEvalContext* ctx,
                                             ConstexprObject* object) {
  ConstexprObject* root =
      object != NULL && object->complete_object != NULL
          ? object->complete_object
          : object;
  if (root != NULL && root->type != NULL &&
      TypeIsStructOrUnion(root->type) &&
      root->type->info.struct_info != NULL &&
      !StructHasVirtualBases(root->type->info.struct_info)) {
    return CloneNonVirtualConstexprObject(ctx, object);
  }
  Vector sources;
  Vector clones;
  VectorInit(&sources);
  VectorInit(&clones);
  ConstexprObject* clone =
      CloneConstexprObjectImpl(ctx, object, &sources, &clones);
  VectorDestruct(&sources);
  VectorDestruct(&clones);
  return clone;
}

static size_t ConstexprNonVirtualBaseCount(Struct* str) {
  if (str == NULL) {
    return 0;
  }
  size_t count = 0;
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base != NULL && !base->is_virtual) {
      count++;
    }
  }
  return count;
}

static size_t ConstexprBaseStorageIndex(Struct* str, size_t base_vector_index) {
  size_t storage = 0;
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base == NULL || base->is_virtual) {
      continue;
    }
    if (i == base_vector_index) {
      return storage;
    }
    storage++;
  }
  return storage;
}

static size_t ConstexprVirtualBaseStorageIndex(Struct* str,
                                               size_t virtual_base_index) {
  return ConstexprNonVirtualBaseCount(str) + virtual_base_index;
}

static size_t ConstexprMemberStorageIndex(Struct* str, StructMember* member) {
  if (str == NULL || member == NULL || str->is_union) {
    return member != NULL ? member->index : 0;
  }
  return ConstexprNonVirtualBaseCount(str) + str->virtual_bases.length +
         member->index;
}

static size_t ConstexprObjectSlotCount(TypeRecord* type) {
  if (type != NULL && TypeIsFixedArray(type)) {
    return type->info.array.size.fixed;
  }
  if (type != NULL && TypeIsStructOrUnion(type) &&
      type->info.struct_info != NULL) {
    Struct* str = type->info.struct_info;
    return str->is_union
               ? 1
               : str->members.length + ConstexprNonVirtualBaseCount(str) +
                     str->virtual_bases.length;
  }
  return 0;
}

static ConstexprValue* ConstexprSlotForOffset(TypeRecord* type,
                                              ConstexprObject* object,
                                              size_t offset) {
  if (type == NULL || object == NULL) {
    return NULL;
  }
  if (TypeIsFixedArray(type)) {
    size_t elem_size =
        type->next != NULL ? (size_t)type->next->size : 0;
    if (elem_size == 0) {
      return NULL;
    }
    size_t index = offset / elem_size;
    size_t elem_offset = offset % elem_size;
    ConstexprValue* slot = ConstexprObjectSlot(object, index);
    if (slot == NULL) {
      return NULL;
    }
    if (elem_offset == 0 || !slot->is_object) {
      return elem_offset == 0 ? slot : NULL;
    }
    return ConstexprSlotForOffset(type->next, slot->object, elem_offset);
  }
  if (TypeIsStructOrUnion(type) && type->info.struct_info != NULL) {
    Struct* str = type->info.struct_info;
    for (size_t i = 0; i < str->bases.length; i++) {
      CXXBaseSpecifier* base = str->bases.value.p[i];
      if (base == NULL || base->type == NULL || base->is_virtual) {
        continue;
      }
      size_t base_size = (size_t)base->type->size;
      if (TypeIsStructOrUnion(base->type) &&
          base->type->info.struct_info != NULL) {
        base_size =
            (size_t)base->type->info.struct_info->non_virtual_size;
      }
      if (offset < (size_t)base->byte_offset ||
          offset >= (size_t)base->byte_offset + base_size) {
        continue;
      }
      ConstexprValue* base_slot = ConstexprObjectSlot(
          object, ConstexprBaseStorageIndex(str, i));
      if (base_slot == NULL) {
        return NULL;
      }
      if (!base_slot->is_object || base_slot->object == NULL) {
        base_slot->is_object = true;
        base_slot->is_address = false;
        base_slot->is_floating = false;
        base_slot->ivalue = 0;
        base_slot->fvalue = 0;
        base_slot->object = NewConstexprObject(
            NULL, base->type, ConstexprObjectSlotCount(base->type));
        if (base_slot->object == NULL ||
            !ConstexprMaterializeBaseSubobjectSlots(NULL, base_slot->object)) {
          return NULL;
        }
      }
      return ConstexprSlotForOffset(
          base->type, base_slot->object, offset - (size_t)base->byte_offset);
    }
    for (size_t i = 0; i < str->virtual_bases.length; i++) {
      CXXVirtualBaseInfo* base = str->virtual_bases.value.p[i];
      if (base == NULL || base->type == NULL) {
        continue;
      }
      size_t base_size = (size_t)base->type->size;
      if (offset < (size_t)base->byte_offset ||
          offset >= (size_t)base->byte_offset + base_size) {
        continue;
      }
      ConstexprValue* base_slot = ConstexprObjectSlot(
          object, ConstexprVirtualBaseStorageIndex(str, i));
      if (base_slot == NULL || !base_slot->is_object ||
          base_slot->object == NULL) {
        return NULL;
      }
      return ConstexprSlotForOffset(
          base->type, base_slot->object, offset - (size_t)base->byte_offset);
    }
    for (size_t i = 0; i < str->members.length; i++) {
      StructMember* member = str->members.value.p[i];
      if (member == NULL || member->symbol == NULL || member->is_static ||
          member->is_member_function ||
          StorageIs(member->symbol->storage, STO(typedef))) {
        continue;
      }
      size_t member_size = member->symbol->type != NULL
                               ? (size_t)member->symbol->type->size
                               : 0;
      if (offset < (size_t)member->byte_offset ||
          offset >= (size_t)member->byte_offset + member_size) {
        continue;
      }
      ConstexprValue* slot = ConstexprObjectSlot(
          object, ConstexprMemberStorageIndex(str, member));
      size_t member_offset = offset - (size_t)member->byte_offset;
      if (slot == NULL) {
        return NULL;
      }
      if (member_offset == 0 || !slot->is_object) {
        return member_offset == 0 ? slot : NULL;
      }
      return ConstexprSlotForOffset(member->symbol->type, slot->object,
                                    member_offset);
    }
  }
  return offset == 0 ? ConstexprObjectSlot(object, 0) : NULL;
}

static ConstexprObject* ConstexprSubobjectAtOffset(ConstexprObject* object,
                                                   TypeRecord* target_type,
                                                   size_t offset) {
  if (object == NULL || object->type == NULL || target_type == NULL) {
    return NULL;
  }
  if (offset == 0 && TypeEqual(object->type, target_type)) {
    return object;
  }
  if (!TypeIsStructOrUnion(object->type) ||
      object->type->info.struct_info == NULL) {
    return NULL;
  }
  Struct* str = object->type->info.struct_info;
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base == NULL || base->is_virtual || base->type == NULL ||
        offset < (size_t)base->byte_offset) {
      continue;
    }
    ConstexprValue* slot = ConstexprObjectSlot(
        object, ConstexprBaseStorageIndex(str, i));
    if (slot == NULL || !slot->is_object || slot->object == NULL) {
      continue;
    }
    size_t relative = offset - (size_t)base->byte_offset;
    ConstexprObject* found =
        ConstexprSubobjectAtOffset(slot->object, target_type, relative);
    if (found != NULL) {
      return found;
    }
  }
  for (size_t i = 0; i < str->virtual_bases.length; i++) {
    CXXVirtualBaseInfo* base = str->virtual_bases.value.p[i];
    if (base == NULL || base->type == NULL ||
        offset < (size_t)base->byte_offset) {
      continue;
    }
    ConstexprValue* slot = ConstexprObjectSlot(
        object, ConstexprVirtualBaseStorageIndex(str, i));
    if (slot == NULL || !slot->is_object || slot->object == NULL) {
      continue;
    }
    size_t relative = offset - (size_t)base->byte_offset;
    ConstexprObject* found =
        ConstexprSubobjectAtOffset(slot->object, target_type, relative);
    if (found != NULL) {
      return found;
    }
  }
  return NULL;
}

static bool ConstexprMaterializeBaseSubobjectSlots(ConstEvalContext* ctx,
                                                   ConstexprObject* object) {
  if (ctx == NULL || object == NULL || object->type == NULL ||
      !TypeIsStructOrUnion(object->type) ||
      object->type->info.struct_info == NULL ||
      object->type->info.struct_info->is_union) {
    return true;
  }
  Struct* str = object->type->info.struct_info;
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base == NULL || base->is_virtual || base->type == NULL) {
      continue;
    }
    ConstexprValue* slot =
        ConstexprObjectSlot(object, ConstexprBaseStorageIndex(str, i));
    if (slot == NULL) {
      return false;
    }
    if (slot->is_object && slot->object != NULL) {
      if (!ConstexprMaterializeBaseSubobjectSlots(ctx, slot->object)) {
        return false;
      }
      continue;
    }
    slot->is_object = true;
    slot->is_address = false;
    slot->is_floating = false;
    slot->ivalue = 0;
    slot->fvalue = 0;
    slot->object =
        NewConstexprObject(ctx, base->type, ConstexprObjectSlotCount(base->type));
    ConstexprObject* root =
        object->complete_object != NULL ? object->complete_object : object;
    if (root->type != NULL && TypeIsStructOrUnion(root->type) &&
        root->type->info.struct_info != NULL &&
        StructHasVirtualBases(root->type->info.struct_info)) {
      slot->object->complete_object = root;
      slot->object->complete_offset =
          object->complete_offset + (size_t)base->byte_offset;
    }
    if (slot->object == NULL ||
        !ConstexprMaterializeBaseSubobjectSlots(ctx, slot->object)) {
      return false;
    }
  }
  ConstexprObject* complete =
      object->complete_object != NULL ? object->complete_object : object;
  Struct* complete_struct =
      complete->type != NULL && TypeIsStructOrUnion(complete->type)
          ? complete->type->info.struct_info
          : NULL;
  for (size_t i = 0; i < str->virtual_bases.length; i++) {
    CXXVirtualBaseInfo* info = str->virtual_bases.value.p[i];
    if (info == NULL || info->type == NULL || complete_struct == NULL) {
      continue;
    }
    size_t complete_index = SIZE_MAX;
    CXXVirtualBaseInfo* complete_info = NULL;
    for (size_t j = 0; j < complete_struct->virtual_bases.length; j++) {
      CXXVirtualBaseInfo* candidate =
          complete_struct->virtual_bases.value.p[j];
      if (candidate != NULL && candidate->type != NULL &&
          TypeEqual(candidate->type, info->type)) {
        complete_index = j;
        complete_info = candidate;
        break;
      }
    }
    if (complete_index == SIZE_MAX || complete_info == NULL) {
      continue;
    }
    ConstexprValue* complete_slot = ConstexprObjectSlot(
        complete,
        ConstexprVirtualBaseStorageIndex(complete_struct, complete_index));
    ConstexprValue* local_slot = ConstexprObjectSlot(
        object, ConstexprVirtualBaseStorageIndex(str, i));
    if (complete_slot == NULL || local_slot == NULL) {
      return false;
    }
    if (!complete_slot->is_object || complete_slot->object == NULL) {
      complete_slot->is_object = true;
      complete_slot->object = NewConstexprObject(
          ctx, complete_info->type,
          ConstexprObjectSlotCount(complete_info->type));
      complete_slot->object->complete_object = complete;
      complete_slot->object->complete_offset =
          (size_t)complete_info->byte_offset;
      if (!ConstexprMaterializeBaseSubobjectSlots(ctx,
                                                 complete_slot->object)) {
        return false;
      }
    }
    *local_slot = *complete_slot;
  }
  return true;
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
  if (ConstexprObjectIsUnion(object)) {
    return 0;
  }
  if (object == NULL || member == NULL || object->type == NULL ||
      object->type->info.struct_info == NULL) {
    return member != NULL ? member->index : 0;
  }
  return ConstexprMemberStorageIndex(object->type->info.struct_info, member);
}

static ConstexprObject* ConstexprObjectForMember(ConstexprObject* object,
                                                 StructMember* member) {
  if (object == NULL || member == NULL || object->type == NULL ||
      !TypeIsStructOrUnion(object->type) ||
      object->type->info.struct_info == NULL) {
    return NULL;
  }
  Struct* str = object->type->info.struct_info;
  for (size_t i = 0; i < str->members.length; i++) {
    if (str->members.value.p[i] == member) {
      return object;
    }
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base == NULL || base->is_virtual) {
      continue;
    }
    ConstexprValue* slot = ConstexprObjectSlot(
        object, ConstexprBaseStorageIndex(str, i));
    ConstexprObject* found =
        slot != NULL && slot->is_object
            ? ConstexprObjectForMember(slot->object, member)
            : NULL;
    if (found != NULL) {
      return found;
    }
  }
  for (size_t i = 0; i < str->virtual_bases.length; i++) {
    ConstexprValue* slot = ConstexprObjectSlot(
        object, ConstexprVirtualBaseStorageIndex(str, i));
    ConstexprObject* found =
        slot != NULL && slot->is_object
            ? ConstexprObjectForMember(slot->object, member)
            : NULL;
    if (found != NULL) {
      return found;
    }
  }
  return NULL;
}

static bool ConstexprObjectHasVirtualBases(ConstexprObject* object) {
  ConstexprObject* root =
      object != NULL && object->complete_object != NULL
          ? object->complete_object
          : object;
  return root != NULL && root->type != NULL &&
         TypeIsStructOrUnion(root->type) &&
         root->type->info.struct_info != NULL &&
         StructHasVirtualBases(root->type->info.struct_info);
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

static ConstexprValue ConstexprResolveForwardedAddress(ConstexprValue value) {
  size_t forwarded = 0;
  while (value.is_address && value.address_binding != NULL &&
         forwarded++ < 1024) {
    ConstexprBinding* binding = value.address_binding;
    if (binding->is_address) {
      if (binding->address_binding == binding) {
        break;
      }
      value = (ConstexprValue){
          .is_address = true,
          .ivalue = binding->ivalue,
          .address_binding = binding->address_binding,
          .address_slot = binding->address_slot,
          .address_object = binding->address_object,
          .address_index = binding->address_index,
          .heap_block = binding->heap_block,
          .heap_index = binding->heap_index,
      };
      continue;
    }
    if (binding->object != NULL && TypeIsFixedArray(binding->object->type)) {
      value.address_binding = NULL;
      value.address_object = binding->object;
      value.address_index = 0;
    }
    break;
  }
  if (value.is_address && value.address_slot != NULL &&
      value.address_slot->is_object && value.address_slot->object != NULL &&
      TypeIsFixedArray(value.address_slot->object->type)) {
    value.address_object = value.address_slot->object;
    value.address_slot = NULL;
    value.address_index = 0;
  }
  return value;
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
        .heap_block = binding->heap_block,
        .heap_index = binding->heap_index,
    };
    return true;
  }
  if (address.heap_block != NULL && address.heap_block->live) {
    if (address.heap_index >= address.heap_block->size) {
      return false;
    }
    *result = (ConstexprValue){
        .ivalue = (int64_t)(signed char)address.heap_block->memory[address.heap_index],
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
         left.address_index == right.address_index &&
         left.heap_block == right.heap_block &&
         left.heap_index == right.heap_index;
}

static bool EvaluateConstexprAddressValue(ConstEvalContext* ctx, ASTNode* node,
                                          ConstexprValue* result);

static bool EvaluateConstexprLValue(ConstEvalContext* ctx, ASTNode* node,
                                    ConstexprBinding** binding) {
  if (node == NULL || node->op != AST_OP(identifier)) {
    if (node != NULL && node->op == AST_OP(contents)) {
      UnaryASTNode* contents = (UnaryASTNode*)node;
      ConstexprValue address;
      if (EvaluateConstexprAddressValue(ctx, contents->sub, &address)) {
        if (address.address_binding != NULL) {
          *binding = address.address_binding;
          return true;
        }
        if (address.address_slot != NULL && address.address_slot->is_address &&
            address.address_slot->address_binding != NULL) {
          *binding = address.address_slot->address_binding;
          return true;
        }
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
    if (TypeIsReference(id->symbol->type)) {
      if ((*binding)->address_binding != NULL) {
        *binding = (*binding)->address_binding;
        return true;
      }
      return false;
    }
    return true;
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
    value = ConstexprResolveForwardedAddress(value);
    binding->is_address = true;
    binding->address_binding = value.address_binding;
    binding->address_slot = value.address_slot;
    binding->address_object = value.address_object;
    binding->address_index = value.address_index;
    binding->heap_block = value.heap_block;
    binding->heap_index = value.heap_index;
    binding->address_type = ConstexprAddressPointeeType(ctx, value);
    binding->address_storage_began = ConstexprAddressStorageBegan(ctx, value);
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
  binding->heap_block = NULL;
  binding->heap_index = 0;
  binding->address_type = NULL;
  binding->address_storage_began = false;
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
    value = ConstexprResolveForwardedAddress(value);
    ConstexprCanonicalizeAddressValue(&value);
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
    slot->heap_block = value.heap_block;
    slot->heap_index = value.heap_index;
    return true;
  }
  slot->is_address = false;
  slot->address_binding = NULL;
  slot->address_slot = NULL;
  slot->address_object = NULL;
  slot->address_index = 0;
  slot->heap_block = NULL;
  slot->heap_index = 0;
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

static bool StoreConstexprHeapAddress(ConstexprValue address, TypeRecord* type,
                                      ConstexprValue value) {
  if (!address.is_address || address.heap_block == NULL ||
      !address.heap_block->live || type == NULL ||
      (!TypeIsIntegral(type) && !TypeIsFloatingPoint(type))) {
    return false;
  }
  size_t size = (size_t)type->size;
  if (size == 0 || address.heap_index > address.heap_block->size ||
      size > address.heap_block->size - address.heap_index) {
    return false;
  }
  unsigned char* memory =
      address.heap_block->memory + address.heap_index;
  if (TypeIsFloatingPoint(type)) {
    double fvalue = 0;
    if (!ConstexprValueAsFloating(value, &fvalue) ||
        size > sizeof(fvalue)) {
      return false;
    }
    memcpy(memory, &fvalue, size);
    return true;
  }
  int64_t ivalue = 0;
  if (!ConstexprValueAsInteger(value, &ivalue) || size > sizeof(ivalue)) {
    return false;
  }
  memcpy(memory, &ivalue, size);
  return true;
}

static void PopConstexprBindings(ConstEvalContext* ctx, size_t mark);

static bool EvaluateConstexprObjectDestructor(ConstEvalContext* ctx,
                                              TypeRecord* object_type,
                                              ConstexprObject* object);

static bool DestroyConstexprBindingsFromMark(ConstEvalContext* ctx,
                                             size_t mark) {
  while (ctx->bindings.length > mark) {
    ConstexprBinding* binding =
        ctx->bindings.value.p[ctx->bindings.length - 1];
    bool ok = true;
    if (binding->object != NULL && binding->symbol != NULL) {
      ConstexprException* unwinding = ctx->exception;
      bool suppress_pending =
          unwinding != NULL && !unwinding->handling;
      if (suppress_pending) {
        unwinding->handling = true;
        ctx->unwinding_exceptions++;
      }
      ok = EvaluateConstexprObjectDestructor(ctx, binding->symbol->type,
                                             binding->object);
      if (suppress_pending) {
        ctx->unwinding_exceptions--;
        unwinding->handling = false;
      }
    }
    free(binding);
    ctx->bindings.length--;
    if (!ok) {
      return false;
    }
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
static Symbol* ConstexprMemberCallSymbol(ASTNode* node, ASTNode** receiver);
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
  if (TypeIsStructOrUnion(type) && type->info.struct_info != NULL) {
    Struct* str = type->info.struct_info;
    size_t base_count = ConstexprNonVirtualBaseCount(str);
    if (!str->is_union && slot_index < base_count) {
      size_t seen = 0;
      for (size_t i = 0; i < str->bases.length; i++) {
        CXXBaseSpecifier* base = str->bases.value.p[i];
        if (base == NULL || base->is_virtual || base->type == NULL) {
          continue;
        }
        if (seen == slot_index) {
          return base->type;
        }
        seen++;
      }
    }
    size_t virtual_count = str->virtual_bases.length;
    if (!str->is_union && slot_index >= base_count &&
        slot_index < base_count + virtual_count) {
      CXXVirtualBaseInfo* virtual_base =
          str->virtual_bases.value.p[slot_index - base_count];
      return virtual_base != NULL ? virtual_base->type : NULL;
    }
    size_t member_index =
        str->is_union ? slot_index
                      : slot_index - base_count - virtual_count;
    if (member_index < str->members.length) {
      StructMember* member = str->members.value.p[member_index];
      return member != NULL && member->symbol != NULL ? member->symbol->type
                                                      : NULL;
    }
  }
  return NULL;
}

static void ConstexprPersistAddressObjects(ConstexprObject* object) {
  if (object == NULL) {
    return;
  }
  for (size_t i = 0; i < object->slots.length; i++) {
    ConstexprValue* slot = object->slots.value.p[i];
    if (slot == NULL) {
      continue;
    }
    if (slot->is_address) {
      ConstexprCanonicalizeAddressValue(slot);
      if (slot->address_object != NULL && slot->address_object->type != NULL &&
          TypeIsFixedArray(slot->address_object->type)) {
        slot->address_object =
            CloneConstexprObject(NULL, slot->address_object);
      }
      slot->address_binding = NULL;
      slot->address_slot = NULL;
    }
    if (slot->is_object && slot->object != NULL) {
      ConstexprPersistAddressObjects(slot->object);
    }
  }
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
      if (StructHasVirtualBases(symbol->type->info.struct_info) &&
          !ConstexprMaterializeBaseSubobjectSlots(&ctx,
                                                  object_value.object)) {
        ConstEvalContextDestruct(&ctx);
        return false;
      }
      size_t mark = ctx.bindings.length;
      PushConstexprBinding(&ctx, symbol, object_value);
      ok = EvaluateConstexprConstructorCall(&ctx, expr);
      PopConstexprBindings(&ctx, mark);
    }
  } else {
    ok = EvaluateConstexprInitializer(&ctx, symbol->type, initializer,
                                      &object_value);
  }

  if (ok && object_value.object != NULL &&
      (ConstexprHasLiveExceptionHandles(&ctx) ||
       ConstexprObjectHasInvalidAddress(&ctx, object_value.object))) {
    ok = false;
  }
  if (ok && object_value.object != NULL) {
    ConstexprObject* stored =
        CloneConstexprObject(NULL, object_value.object);
    if (stored != NULL) {
      ConstexprPersistAddressObjects(stored);
    }
    symbol->value.other = stored;
    symbol->flags.value_set = stored != NULL;
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
  ConstexprPCodeCapability capability =
      TypeIsConstevalOnly(symbol->type)
          ? kConstexprPCodeASTOnly
          : ConstexprPCodeCapabilityForExpression(initializer);
  bool pcode_attempted =
      mode != kConstexprEvalAST && capability != kConstexprPCodeASTOnly;
  ConstEvalContext pcode_context;
  ConstEvalContextInit(&pcode_context);
  bool pcode_ok =
      pcode_attempted &&
      ConstexprPCodeEvaluateObjectConstantForSymbol(
          &pcode_context, symbol, initializer);
  if (mode == kConstexprEvalPCode &&
      capability != kConstexprPCodeASTOnly) {
    if (!pcode_ok) {
      ReportConstexprPCodeFailure(&pcode_context, initializer, false);
    }
    ConstEvalContextDestruct(&pcode_context);
    return pcode_ok;
  }
  if (mode == kConstexprEvalAuto && pcode_ok) {
    ConstEvalContextDestruct(&pcode_context);
    return true;
  }
  if (mode == kConstexprEvalAuto && pcode_attempted && !pcode_ok &&
      TypeIsStructOrUnion(symbol->type) &&
      symbol->type->info.struct_info != NULL &&
      symbol->type->info.struct_info->tag_name != NULL &&
      symbol->type->info.struct_info->tag_name->value != NULL &&
      strcmp(symbol->type->info.struct_info->tag_name->value,
             "exception_ptr") == 0) {
    ConstEvalContextDestruct(&pcode_context);
    return false;
  }
  ConstexprObject* pcode_object =
      pcode_ok ? (ConstexprObject*)symbol->value.other : NULL;
  if (pcode_ok) {
    symbol->flags.value_set = false;
    symbol->value.other = NULL;
  }
  if (mode == kConstexprEvalAudit ||
      capability == kConstexprPCodeASTOnly) {
    compiler->constexpr_eval_mode = kConstexprEvalAST;
  }
  bool ast_ok =
      ConstexprEvaluateObjectConstantForSymbolAST(symbol, initializer);
  compiler->constexpr_eval_mode = mode;
  ConstexprObject* ast_object =
      ast_ok ? (ConstexprObject*)symbol->value.other : NULL;
  bool values_match =
      pcode_ok && ast_ok &&
      ConstexprObjectsTemplateArgumentEquivalent(pcode_object, ast_object);
  if (mode == kConstexprEvalAudit && pcode_attempted && pcode_ok &&
      (!ast_ok || !values_match)) {
    SemanticError(
        initializer,
        pcode_ok
            ? "constexpr evaluator object mismatch"
            : "constexpr evaluator mismatch: pcode failed: %s",
        ConstexprPCodeFailureReason(&pcode_context));
    ast_ok = false;
  }
  if (pcode_object != NULL) {
    ConstexprPCodeDeleteObject(pcode_object);
  }
  ConstEvalContextDestruct(&pcode_context);
  return ast_ok;
}

static ASTNode* ConstexprValueInitializer(ConstexprValue* value,
                                          TypeRecord* type,
                                          SourceLocation location,
                                          bool preserve_external_addresses);

static bool EvaluateConstexprStatementExpression(ConstEvalContext* ctx,
                                                 ASTNode* node,
                                                 TypeRecord* type,
                                                 ConstexprValue* result) {
  if (ctx == NULL || node == NULL || node->op != AST_OP(stmt_expr) ||
      result == NULL) {
    return false;
  }
  ASTNode* body_node = ((UnaryASTNode*)node)->sub;
  if (body_node == NULL || body_node->op != AST_OP(compound)) {
    return false;
  }
  CompoundStatementASTNode* body = (CompoundStatementASTNode*)body_node;
  if (body->statements == NULL || body->statements->length == 0) {
    return TypeIsVoid(type);
  }
  size_t mark = ctx->bindings.length;
  bool ok = true;
  for (size_t i = 0; i + 1 < body->statements->length; ++i) {
    ConstexprValue ignored = {0};
    if (EvaluateConstexprStatement(ctx, body->statements->value.p[i],
                                   type, &ignored) != kConstexprStmtNormal) {
      ok = false;
      break;
    }
  }
  ASTNode* last =
      body->statements->value.p[body->statements->length - 1];
  if (ok && last != NULL && last->op == AST_OP(expr)) {
    ASTNode* expression = ((ExpressionStatementASTNode*)last)->expr;
    ok = expression != NULL &&
         EvaluateConstexprValue(ctx, expression, type, result);
  } else if (ok) {
    ok = false;
  }
  PopConstexprBindings(ctx, mark);
  return ok;
}

static ASTNode* ConstexprObjectInitializer(ConstexprObject* object,
                                           SourceLocation location,
                                           bool preserve_external_addresses) {
  if (object == NULL || object->type == NULL) {
    return NULL;
  }
  Vector* initializers = NewVector();
  bool has_inactive_subobjects = false;
  if (TypeIsFixedArray(object->type)) {
    for (size_t i = 0; i < object->slots.length; i++) {
      ConstexprValue* slot = object->slots.value.p[i];
      if (slot == NULL || slot->lifetime_ended) {
        has_inactive_subobjects = true;
        continue;
      }
      ASTNode* init = ConstexprValueInitializer(
          slot, object->type->next, location,
          preserve_external_addresses);
      if (init != NULL) {
        Vector* designators = NewVector();
        VectorAppend(designators,
                     NewArrayDesignator(object->type, (int)i));
        VectorAppend(initializers, NewDesignatedInitializerASTNode(
                                       designators, init, location));
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
          member->symbol->type, location, preserve_external_addresses);
      if (init != NULL) {
        if (str->is_union) {
          Vector* designators = NewVector();
          VectorAppend(designators, NewStructMemberDesignator(member));
          init = NewDesignatedInitializerASTNode(designators, init, location);
        }
        VectorAppend(initializers, init);
      }
      if (str->is_union) {
        break;
      }
    }
  }
  ASTNode* result =
      NewBracedInitializerASTNode(initializers, object->type, location);
  if (has_inactive_subobjects) {
    result->flags |= kASTConstexprLifetimeInitializer;
  }
  return result;
}

static ASTNode* ConstexprValueInitializer(ConstexprValue* value,
                                          TypeRecord* type,
                                          SourceLocation location,
                                          bool preserve_external_addresses) {
  if (value == NULL || type == NULL) {
    return NULL;
  }
  if (value->lifetime_ended) {
    return NULL;
  }
  if (value->is_object) {
    return ConstexprObjectInitializer(value->object, location,
                                      preserve_external_addresses);
  }
  ConstexprValue resolved = ConstexprResolveForwardedAddress(*value);
  ConstexprCanonicalizeAddressValue(&resolved);
  value = &resolved;
  ASTNode* expr = NULL;
  if (TypeIsPointer(type) && value->is_address &&
      value->address_object != NULL &&
      value->address_object->type != NULL &&
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
  } else if (preserve_external_addresses &&
             (TypeIsPointer(type) || TypeIsReference(type)) &&
             value->is_address && value->address_binding != NULL &&
             value->address_index == 0) {
    Symbol* symbol = value->address_binding->symbol;
    ASTNode* object =
        NewIdentifierASTNode(symbol, location);
    expr = NewUnaryASTNode(AST_OP(address), NULL, location, object);
    ASTNodeSetType(expr, TypeRecordCopy(type));
  } else if (TypeIsReflection(type)) {
    expr = NewReflectionConstantASTNode(
        (ReflectionValue*)(intptr_t)value->ivalue, location);
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
                                    location,
                                    /*preserve_external_addresses=*/false);
}

static bool EvaluateConstexprValue(ConstEvalContext* ctx, ASTNode* node,
                                   TypeRecord* type,
                                   ConstexprValue* result) {
  if (node != NULL && node->op == AST_OP(stmt_expr)) {
    return EvaluateConstexprStatementExpression(ctx, node, type, result);
  }
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
  if (type != NULL && TypeIsReference(type) && node != NULL &&
      node->op == AST_OP(contents)) {
    // Binding a reference to `*p` preserves the pointee's address. The regular
    // contents path below dereferences that address to obtain a value, which is
    // correct for scalar reads but loses the lvalue identity required here.
    return EvaluateConstexprAddressValue(
        ctx, ((UnaryASTNode*)node)->sub, result);
  }
  if (type != NULL && (TypeIsPointer(type) || TypeIsReference(type))) {
    return EvaluateConstexprAddressValue(ctx, node, result);
  }
  if (node != NULL && node->op == AST_OP(contents)) {
    return EvaluateConstexprAddressValue(ctx, node, result);
  }
  if (type != NULL && TypeIsReflection(type)) {
    ReflectionValue* reflection =
        ConstexprEvaluateReflectionExpression(ctx, node);
    if (reflection == NULL) {
      return false;
    }
    memset(result, 0, sizeof(*result));
    result->ivalue = (int64_t)(intptr_t)reflection;
    return true;
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

static bool ConstexprHasPendingException(ConstEvalContext* ctx) {
  return ctx != NULL && ctx->exception != NULL && !ctx->exception->handling;
}

bool ConstexprEvaluateThrowExpression(ConstEvalContext* ctx, ASTNode* node) {
  if (ctx == NULL || node == NULL || node->op != AST_OP(throw) ||
      !CompilerCXXAtLeast(kLanguageStandardCXX26)) {
    return false;
  }
  ThrowASTNode* throw_node = (ThrowASTNode*)node;
  if (throw_node->expr == NULL) {
    if (ctx->exception == NULL || !ctx->exception->handling) {
      return false;
    }
    ctx->exception->handling = false;
    return true;
  }
  if (ConstexprHasPendingException(ctx)) {
    return false;
  }
  ConstexprValue value = {0};
  if (!EvaluateConstexprValue(ctx, throw_node->expr, throw_node->expr->type,
                              &value)) {
    return false;
  }
  if (value.is_object) {
    TypeRecord* object_type =
        ConstexprExceptionObjectType(throw_node->expr->type);
    ConstexprObject* copied = NULL;
    bool move = throw_node->expr->value_category == kValueCategoryXvalue;
    if (!ConstexprCopyConstructObject(ctx, object_type, value.object, &copied,
                                      move)) {
      return false;
    }
    value.object = copied;
  }
  ConstexprException* exception = malloc(sizeof(ConstexprException));
  exception->type = throw_node->expr->type;
  exception->value = value;
  exception->throw_node = node;
  exception->throw_location = node->location;
  exception->handling = false;
  exception->reported = false;
  exception->active = true;
  exception->destroyed = false;
  exception->references = 0;
  exception->token = NULL;
  exception->previous = ctx->exception;
  VectorAppend(&ctx->exception_handles, exception);
  ctx->exception = exception;
  return true;
}

static void ReportUncaughtConstexprException(ConstEvalContext* ctx) {
  if (!ConstexprHasPendingException(ctx) || ctx->call_depth != 0 ||
      ctx->exception->reported) {
    return;
  }
  if (ctx->exception->throw_node != NULL &&
      (ctx->exception->throw_node->flags &
       kASTConstexprExceptionDiagnosed) != 0) {
    ctx->exception->reported = true;
    return;
  }
  TypeRecord* type = ConstexprExceptionObjectType(ctx->exception->type);
  String type_name;
  StringInit(&type_name, "");
  if (type != NULL) {
    TypeRecordToString(type, &type_name);
  } else {
    StringAppend(&type_name, "<unknown>");
  }
  SemanticError(ctx->exception->throw_node,
                "constant evaluation ended with an uncaught exception of type "
                "%s",
                type_name.value);
  if (ctx->exception->throw_node != NULL) {
    ctx->exception->throw_node->flags |= kASTConstexprExceptionDiagnosed;
  }
  StringDestruct(&type_name);
  ctx->exception->reported = true;
}

ReflectionValue* ConstexprEvaluateReflectionExpression(ConstEvalContext* ctx,
                                                       ASTNode* node) {
  ASTNode* expression = ConstexprInitializerExpression(node);
  expression = AnalyzeExpression(expression);
  ReflectionValue* value =
      SemanticReflectionValueFromExpression(expression);
  if (value != NULL) {
    return value;
  }
  if (expression == NULL || expression->op != AST_OP(identifier)) {
    ConstexprValue resolved = {0};
    if (expression != NULL &&
        EvaluateConstexprObjectAccess(ctx, expression, &resolved) &&
        !resolved.is_object && !resolved.is_address) {
      return (ReflectionValue*)(intptr_t)resolved.ivalue;
    }
    resolved = ConstexprResolveForwardedAddress(resolved);
    if (resolved.is_address && resolved.address_slot != NULL &&
        !resolved.address_slot->is_object) {
      return (ReflectionValue*)(intptr_t)resolved.address_slot->ivalue;
    }
    if (resolved.is_address && resolved.address_object != NULL) {
      ConstexprValue* slot =
          ConstexprObjectSlot(resolved.address_object, resolved.address_index);
      if (slot != NULL && !slot->is_object && !slot->is_address) {
        return (ReflectionValue*)(intptr_t)slot->ivalue;
      }
    }
    return NULL;
  }
  Symbol* symbol = ((IdentifierASTNode*)expression)->symbol;
  if (symbol == NULL || !TypeIsReflection(symbol->type)) {
    return NULL;
  }
  ConstexprBinding* binding = FindConstexprBinding(ctx, symbol);
  if (binding != NULL && !binding->is_address && binding->object == NULL) {
    return (ReflectionValue*)(intptr_t)binding->ivalue;
  }
  if (symbol->flags.value_set) {
    return (ReflectionValue*)symbol->value.other;
  }
  return NULL;
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

static bool ConstexprIsVirtualPointerAssignment(BinaryASTNode* node) {
  if (node == NULL || node->base.op != AST_OP(assign) ||
      node->left == NULL ||
      (node->left->op != AST_OP(dot) &&
       node->left->op != AST_OP(arrow))) {
    return false;
  }
  ASTNode* member_node = ((BinaryASTNode*)node->left)->right;
  if (member_node == NULL || member_node->op != AST_OP(structmember)) {
    return false;
  }
  StructMember* member = ((StructMemberASTNode*)member_node)->member;
  return member != NULL && member->symbol != NULL &&
         (StringStartsWith(&member->symbol->name, "__vptr") ||
          StringStartsWith(&member->symbol->name, "__vbptr"));
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

  // Compiler-generated vptr stores describe runtime dispatch state. The AST
  // evaluator records the dynamic type directly on ConstexprObject and resolves
  // virtual calls from that type, so evaluating the linker-owned vtable address
  // is both unnecessary and impossible during constant evaluation.
  if (ConstexprIsVirtualPointerAssignment(node)) {
    *result = (ConstexprValue){0};
    return true;
  }

  if (node->base.op == AST_OP(assign) && node->left != NULL &&
      (node->left->op == AST_OP(contents) ||
       node->left->op == AST_OP(subscript))) {
    ConstexprValue address = {0};
    ASTNode* address_expr =
        node->left->op == AST_OP(contents)
            ? ((UnaryASTNode*)node->left)->sub : node->left;
    if (EvaluateConstexprAddressValue(ctx, address_expr, &address) &&
        ConstexprHasHeapBlock(ctx, address.heap_block)) {
      ConstexprValue right = {0};
      TypeRecord* left_type = node->left->type;
      if (TypeIsReference(left_type)) {
        left_type = left_type->next;
      }
      if (!EvaluateConstexprValue(ctx, node->right, left_type, &right) ||
          !StoreConstexprHeapAddress(address, left_type, right)) {
        return false;
      }
      *result = right;
      return true;
    }
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
    if (slot != NULL) {
      slot->lifetime_ended = false;
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
            .heap_block = binding->heap_block,
            .heap_index = binding->heap_index,
        }
      : *slot;
  ConstexprValue new_value = old_value;
  bool increment = node->base.op == AST_OP(preinc) ||
                   node->base.op == AST_OP(postinc);
  TypeRecord* value_type = node->sub->type;
  if (TypeIsReference(value_type)) {
    value_type = value_type->next;
  }
  if (value_type != NULL && TypeIsPointer(value_type)) {
    new_value = ConstexprResolveForwardedAddress(new_value);
    old_value = new_value;
    if (new_value.address_object != NULL) {
      if ((increment &&
           new_value.address_index >= new_value.address_object->slots.length) ||
          (!increment && new_value.address_index == 0)) {
        return false;
      }
      if (increment) {
        new_value.address_index++;
      } else {
        new_value.address_index--;
      }
    } else if (new_value.heap_block != NULL) {
      size_t element_size = value_type->next != NULL &&
              value_type->next->size > 0
          ? (size_t)value_type->next->size
          : 1;
      if ((increment &&
           (element_size > SIZE_MAX - new_value.heap_index ||
            new_value.heap_index + element_size >
                new_value.heap_block->size)) ||
          (!increment && element_size > new_value.heap_index)) {
        return false;
      }
      if (increment) {
        new_value.heap_index += element_size;
      } else {
        new_value.heap_index -= element_size;
      }
    } else {
      return false;
    }
  } else if (value_type != NULL && TypeIsFloatingPoint(value_type)) {
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
      ? StoreConstexprBinding(ctx, binding, value_type, new_value)
      : StoreConstexprSlot(ctx, slot, value_type, new_value);
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
    if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL) {
      return false;
    }
    Struct* str = type->info.struct_info;
    for (size_t i = 0; i < str->bases.length; i++) {
      CXXBaseSpecifier* base = str->bases.value.p[i];
      if (base == designator->value.base) {
        *slot_index = ConstexprBaseStorageIndex(str, i);
        return true;
      }
    }
    return false;
  }
  StructMember* member = ConstexprDesignatorMember(type, designator);
  if (member == NULL) {
    return false;
  }
  *slot_index = type->info.struct_info->is_union
      ? 0
      : ConstexprMemberStorageIndex(type->info.struct_info, member);
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
  if (TypeIsArray(type) && type->info.array.is_flexible) {
    while (object->slots.length <= slot_index) {
      VectorAppend(&object->slots, NewConstexprValueSlot());
    }
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
    VectorASTNode* constructor_call = (VectorASTNode*)constructor;
    Symbol* constructor_symbol =
        ConstexprFunctionDefinition(ConstexprCallSymbol(constructor));
    CXXSpecialMemberKind constructor_kind =
        constructor_symbol != NULL && constructor_symbol->type != NULL &&
                TypeIsFunction(constructor_symbol->type)
            ? constructor_symbol->type->info.function.cxx_special_member_kind
            : kCXXSpecialMemberNone;
    if (constructor_call->children != NULL &&
        constructor_call->children->length >= 2) {
      ASTNode* source = constructor_call->children->value.p[
          constructor_call->children->length - 1];
      TypeRecord* source_type = source != NULL ? source->type : NULL;
      while (source_type != NULL && TypeIsReference(source_type)) {
        source_type = source_type->next;
      }
      bool direct_copy =
          source_type != NULL && TypeIsStructOrUnion(source_type) &&
          TypeIsStructOrUnion(type) &&
          (source_type->info.struct_info == type->info.struct_info ||
           (source_type->info.struct_info != NULL &&
            type->info.struct_info != NULL &&
            source_type->info.struct_info->tag_name != NULL &&
            type->info.struct_info->tag_name != NULL &&
            StringEqual(source_type->info.struct_info->tag_name,
                        type->info.struct_info->tag_name->value)));
      if ((constructor_kind == kCXXSpecialMemberCopyConstructor ||
           constructor_kind == kCXXSpecialMemberMoveConstructor ||
           direct_copy) &&
          source_type != NULL && TypeIsStructOrUnion(source_type)) {
        ConstexprValue source_value = {0};
        if (EvaluateConstexprObjectAccess(ctx, source, &source_value) &&
            source_value.is_object && source_value.object != NULL) {
          result->is_object = true;
          result->is_address = false;
          result->is_floating = false;
          result->ivalue = 0;
          result->fvalue = 0;
          result->object = CloneConstexprObject(ctx, source_value.object);
          return result->object != NULL;
        }
      }
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
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function || member->is_using_declaration ||
        member->default_initializer == NULL) {
      continue;
    }
    size_t storage_index = ConstexprMemberStorageIndex(str, member);
    if (storage_index >= slot_count || initialized[storage_index]) {
      continue;
    }
    ConstexprValue* slot = ConstexprObjectSlot(object, storage_index);
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
  if (TypeIsIntegral(type) || TypeIsFloatingPoint(type) ||
      TypeIsPointer(type) || TypeIsReference(type) ||
      TypeIsReflection(type)) {
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
  if (TypeIsArray(type) && type->info.array.is_flexible &&
      initializer->op == AST_OP(braced_init)) {
    BracedInitializerASTNode* braced =
        (BracedInitializerASTNode*)initializer;
    slot_count =
        braced->initializers != NULL ? braced->initializers->length : 0;
  }
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
  bool sparse_lifetime_initializer =
      TypeIsFixedArray(type) &&
      (initializer->flags & kASTConstexprLifetimeInitializer) != 0;
  if (sparse_lifetime_initializer) {
    for (size_t i = 0; i < object->slots.length; i++) {
      ConstexprValue* slot = object->slots.value.p[i];
      if (slot != NULL) {
        slot->lifetime_ended = true;
      }
    }
  }
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
    if (sparse_lifetime_initializer) {
      slot->lifetime_ended = false;
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
      if (!is_designated_entry && TypeIsStructOrUnion(slot_type) &&
          slot_type->info.struct_info != NULL &&
          !slot_type->info.struct_info->is_union && entry_init != NULL &&
          entry_init->op != AST_OP(braced_init) &&
          entry_init->op != AST_OP(designated_init)) {
        if (!slot->is_object || slot->object == NULL) {
          slot->is_object = true;
          slot->is_address = false;
          slot->is_floating = false;
          slot->ivalue = 0;
          slot->fvalue = 0;
          slot->object = NewConstexprObject(
              ctx, slot_type, ConstexprObjectSlotCount(slot_type));
          if (slot->object == NULL ||
              !ConstexprMaterializeBaseSubobjectSlots(ctx, slot->object)) {
            ok = false;
            break;
          }
        }
        TypeRecord* member_type = ConstexprObjectSlotType(slot_type, 0);
        ConstexprValue* member_slot = ConstexprObjectSlot(slot->object, 0);
        if (member_type == NULL || member_slot == NULL ||
            !EvaluateConstexprInitializer(ctx, member_type, entry_init,
                                          member_slot)) {
          ok = false;
          break;
        }
      } else {
        ok = false;
        break;
      }
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
  if (ok && TypeIsStructOrUnion(type) && type->info.struct_info != NULL &&
      !type->info.struct_info->is_union &&
      !type->info.struct_info->is_aggregate) {
    ok = ConstexprMaterializeBaseSubobjectSlots(ctx, object);
    if (ok) {
      Symbol* ctor_symbol = ConstexprConstructorForObjectType(type, 0);
      Symbol* ctor = ConstexprFunctionDefinition(ctor_symbol);
      if (ctor != NULL && ctor->type != NULL &&
          TypeIsFunction(ctor->type) &&
          ctor->type->info.function.is_constructor &&
          !ctor->type->info.function.is_destructor &&
          !ctor->type->info.function.is_virtual &&
          !ctor->type->info.function.varargs &&
          ctor->type->info.function.is_constexpr &&
          ctor->type->info.function.body != NULL) {
        Vector actuals;
        VectorInit(&actuals);
        size_t mark = ctx->bindings.length;
        ctx->call_depth++;
        ConstexprValue ignored = {0};
        ok = BindConstexprConstructorObjectActuals(ctx, ctor, object,
                                                   &actuals) &&
             EvaluateConstexprStatement(ctx, ctor->type->info.function.body,
                                        ctor->type->next, &ignored) ==
                 kConstexprStmtNormal;
        ctx->call_depth--;
        PopConstexprBindings(ctx, mark);
        VectorDestruct(&actuals);
      }
    }
  }
  return ok;
}

static bool ConstexprValuePermittedInTemplateArgument(ConstexprValue* value);

static bool ConstexprObjectPermittedInTemplateArgument(ConstexprObject* object) {
  if (object == NULL) {
    return false;
  }
  for (size_t i = 0; i < object->slots.length; i++) {
    if (!ConstexprValuePermittedInTemplateArgument(object->slots.value.p[i])) {
      return false;
    }
  }
  return true;
}

static bool ConstexprValuePermittedInTemplateArgument(ConstexprValue* value) {
  if (value == NULL) {
    return true;
  }
  if (value->is_object) {
    return ConstexprObjectPermittedInTemplateArgument(value->object);
  }
  if (!value->is_address) {
    return true;
  }
  if (value->heap_block != NULL) {
    return false;
  }
  if (value->address_binding != NULL) {
    Symbol* symbol = value->address_binding->symbol;
    return symbol != NULL && !symbol->flags.is_temp &&
           !symbol->flags.is_argument &&
           !StorageIs(symbol->storage, STO(auto)) &&
           !StorageIs(symbol->storage, STO(register));
  }
  if (value->address_object != NULL) {
    return false;
  }
  return value->address_slot == NULL && value->address_index == 0;
}

static ASTNode* ConstexprObjectInitializerForExpressionImpl(
    TypeRecord* type, ASTNode* expression, bool template_argument) {
  if (type == NULL || expression == NULL ||
      (!TypeIsFixedArray(type) && !TypeIsStructOrUnion(type))) {
    return NULL;
  }
  ConstEvalContext ctx;
  ConstEvalContextInit(&ctx);
  if (template_argument) {
    template_argument_object_evaluation_depth++;
  }
  ConstexprValue value = {0};
  bool ok = EvaluateConstexprInitializer(&ctx, type, expression, &value) &&
            value.is_object && value.object != NULL &&
            (!template_argument ||
             ConstexprObjectPermittedInTemplateArgument(value.object));
  if (template_argument) {
    template_argument_object_evaluation_depth--;
  }
  ASTNode* initializer =
      ok ? ConstexprValueInitializer(&value, type, expression->location,
                                     template_argument)
         : NULL;
  ConstEvalContextDestruct(&ctx);
  return initializer;
}

ASTNode* ConstexprObjectInitializerForExpression(TypeRecord* type,
                                                 ASTNode* expression) {
  return ConstexprObjectInitializerForExpressionImpl(
      type, expression, /*template_argument=*/false);
}

ASTNode* ConstexprTemplateArgumentObjectInitializerForExpression(
    TypeRecord* type, ASTNode* expression) {
  return ConstexprObjectInitializerForExpressionImpl(
      type, expression, /*template_argument=*/true);
}

static bool ConstexprObjectsTemplateArgumentEquivalent(ConstexprObject* left,
                                                       ConstexprObject* right);

static bool ConstexprValuesTemplateArgumentEquivalent(ConstexprValue* left,
                                                      ConstexprValue* right) {
  if (left == NULL || right == NULL) {
    return left == right;
  }
  if (left->is_object || right->is_object) {
    return left->is_object == right->is_object &&
           ConstexprObjectsTemplateArgumentEquivalent(left->object,
                                                      right->object);
  }
  if (left->is_address || right->is_address) {
    if (left->is_address != right->is_address ||
        left->address_index != right->address_index ||
        left->heap_index != right->heap_index) {
      return false;
    }
    if (left->address_binding != NULL || right->address_binding != NULL) {
      return left->address_binding != NULL && right->address_binding != NULL &&
             left->address_binding->symbol == right->address_binding->symbol;
    }
    if (left->address_object != NULL || right->address_object != NULL) {
      return left->address_object == right->address_object;
    }
    return left->address_slot == right->address_slot &&
           left->heap_block == right->heap_block;
  }
  if (left->is_floating || right->is_floating) {
    return left->is_floating == right->is_floating &&
           memcmp(&left->fvalue, &right->fvalue, sizeof(left->fvalue)) == 0;
  }
  return left->ivalue == right->ivalue;
}

static bool ConstexprObjectsTemplateArgumentEquivalent(ConstexprObject* left,
                                                       ConstexprObject* right) {
  if (left == NULL || right == NULL) {
    return left == right;
  }
  if (!TypeEqual(left->type, right->type) ||
      left->active_union_member != right->active_union_member ||
      left->slots.length != right->slots.length) {
    return false;
  }
  for (size_t i = 0; i < left->slots.length; i++) {
    ConstexprValue* left_slot = left->slots.value.p[i];
    ConstexprValue* right_slot = right->slots.value.p[i];
    if (left_slot != NULL && right_slot != NULL &&
        (left_slot->lifetime_ended || right_slot->lifetime_ended)) {
      if (left_slot->lifetime_ended != right_slot->lifetime_ended) {
        return false;
      }
      continue;
    }
    if (!ConstexprValuesTemplateArgumentEquivalent(left->slots.value.p[i],
                                                   right->slots.value.p[i])) {
      return false;
    }
  }
  return true;
}

bool ConstexprObjectInitializersEquivalent(TypeRecord* type, ASTNode* left,
                                           ASTNode* right) {
  if (type == NULL || left == NULL || right == NULL) {
    return left == right;
  }
  ConstEvalContext left_context;
  ConstEvalContext right_context;
  ConstEvalContextInit(&left_context);
  ConstEvalContextInit(&right_context);
  ConstexprValue left_value = {0};
  ConstexprValue right_value = {0};
  bool equivalent =
      EvaluateConstexprInitializer(&left_context, type, left, &left_value) &&
      EvaluateConstexprInitializer(&right_context, type, right, &right_value) &&
      left_value.is_object && right_value.is_object &&
      ConstexprObjectsTemplateArgumentEquivalent(left_value.object,
                                                 right_value.object);
  ConstEvalContextDestruct(&left_context);
  ConstEvalContextDestruct(&right_context);
  return equivalent;
}

static void AppendConstexprTemplateArgumentValueKey(ConstexprValue* value,
                                                    String* result);

static void AppendConstexprTemplateArgumentObjectKey(ConstexprObject* object,
                                                     String* result) {
  StringAppendChar(result, '{');
  if (object != NULL) {
    StringPrintf(
        result, "T%016llxU%lld:",
        (unsigned long long)TypeRecordSemanticIdentityHash(object->type),
        object->active_union_member != NULL
            ? (long long)object->active_union_member->index : -1LL);
    for (size_t i = 0; i < object->slots.length; i++) {
      if (i != 0) {
        StringAppendChar(result, ',');
      }
      ConstexprValue* slot = object->slots.value.p[i];
      if (slot != NULL && slot->lifetime_ended) {
        StringAppend(result, "X");
      } else {
        AppendConstexprTemplateArgumentValueKey(slot, result);
      }
    }
  }
  StringAppendChar(result, '}');
}

static void AppendConstexprTemplateArgumentValueKey(ConstexprValue* value,
                                                    String* result) {
  if (value == NULL) {
    StringAppend(result, "?");
  } else if (value->is_object) {
    AppendConstexprTemplateArgumentObjectKey(value->object, result);
  } else if (value->is_address) {
    StringAppend(result, "A");
    if (value->address_binding != NULL &&
        value->address_binding->symbol != NULL) {
      Symbol* symbol = value->address_binding->symbol;
      if (symbol->asm_name.length != 0) {
        StringAppendString(result, &symbol->asm_name);
      } else {
        StringAppendString(result, &symbol->name);
      }
    } else {
      StringAppend(result, "-");
    }
    StringPrintf(result, ":%d:%lld", value->heap_index,
                 (long long)value->address_index);
    if (value->address_object != NULL) {
      StringPrintf(
          result, ":O%016llx",
          (unsigned long long)TypeRecordSemanticIdentityHash(
              value->address_object->type));
    } else if (value->address_slot != NULL) {
      StringAppend(result, ":S");
    }
  } else if (value->is_floating) {
    unsigned char bytes[sizeof(value->fvalue)];
    memcpy(bytes, &value->fvalue, sizeof(bytes));
    StringAppendChar(result, 'F');
    for (size_t i = 0; i < sizeof(bytes); i++) {
      StringPrintf(result, "%02x", bytes[i]);
    }
  } else {
    StringPrintf(result, "I%lld", (long long)value->ivalue);
  }
}

bool ConstexprObjectInitializerTemplateKey(TypeRecord* type, ASTNode* expression,
                                           String* result) {
  if (type == NULL || expression == NULL || result == NULL) {
    return false;
  }
  ConstEvalContext context;
  ConstEvalContextInit(&context);
  ConstexprValue value = {0};
  bool ok = EvaluateConstexprInitializer(&context, type, expression, &value) &&
            value.is_object;
  if (ok) {
    AppendConstexprTemplateArgumentObjectKey(value.object, result);
  }
  ConstEvalContextDestruct(&context);
  return ok;
}

static ConstexprObject* ConstexprVirtualMemberRootObject(
    ConstEvalContext* ctx, ASTNode* node, StructMember* member) {
  if (node == NULL) {
    return NULL;
  }
  if (node->op == AST_OP(address)) {
    ASTNode* target = ((UnaryASTNode*)node)->sub;
    ConstexprValue value = {0};
    if (target != NULL &&
        EvaluateConstexprObjectAccess(ctx, target, &value) &&
        value.object != NULL &&
        ConstexprObjectForMember(value.object, member) != NULL) {
      return value.object;
    }
  }
  if (node->op == AST_OP(cast)) {
    return ConstexprVirtualMemberRootObject(
        ctx, ((CastASTNode*)node)->expr, member);
  }
  if (node->op == AST_OP(ptr_scale)) {
    return ConstexprVirtualMemberRootObject(
        ctx, ((PtrScaleASTNode*)node)->expr, member);
  }
  if (node->op == AST_OP(expr_init)) {
    return ConstexprVirtualMemberRootObject(
        ctx, ((ExpressionInitializerASTNode*)node)->expr, member);
  }
  ASTNodeShape shape = ASTNodeGetShape(node);
  if (shape == kASTShapeBinary) {
    BinaryASTNode* binary = (BinaryASTNode*)node;
    ConstexprObject* object =
        ConstexprVirtualMemberRootObject(ctx, binary->left, member);
    return object != NULL
               ? object
               : ConstexprVirtualMemberRootObject(ctx, binary->right, member);
  }
  if (shape == kASTShapeUnary) {
    return ConstexprVirtualMemberRootObject(
        ctx, ((UnaryASTNode*)node)->sub, member);
  }
  if (shape == kASTShapeVector) {
    VectorASTNode* vector = (VectorASTNode*)node;
    ConstexprObject* object =
        ConstexprVirtualMemberRootObject(ctx, vector->left, member);
    for (size_t i = 0;
         object == NULL && vector->children != NULL &&
         i < vector->children->length;
         i++) {
      object = ConstexprVirtualMemberRootObject(
          ctx, vector->children->value.p[i], member);
    }
    return object;
  }
  return NULL;
}

bool EvaluateConstexprObjectAccess(ConstEvalContext* ctx,
                                          ASTNode* node,
                                          ConstexprValue* result) {
  if (node == NULL) {
    return false;
  }
  if (node->op == AST_OP(comma) && node->type != NULL &&
      (TypeIsStructOrUnion(node->type) || TypeIsFixedArray(node->type))) {
    return EvaluateConstexprObjectExpressionInitializer(ctx, node->type, node,
                                                        result);
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
    if (EvaluateConstexprCall(ctx, node, &value)) {
      if (node->type != NULL && TypeContainsReflection(node->type) &&
          !value.is_object) {
        *result = value;
        return true;
      }
      if (value.is_object && value.object != NULL) {
        *result = value;
        return true;
      }
    }
    if (node->type != NULL && TypeIsStructOrUnion(node->type)) {
      ConstexprObject* object =
          NewConstexprObject(ctx, node->type,
                             ConstexprObjectSlotCount(node->type));
      if (object != NULL &&
          EvaluateConstexprConstructorCallForObject(ctx, node, object)) {
        *result = (ConstexprValue){.is_object = true, .object = object};
        return true;
      }
    }
    return false;
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
        ConstexprReferenceUsableInCurrentFunction(id->symbol) &&
        TypeIsReference(id->symbol->type) &&
        id->symbol->constexpr_initializer != NULL) {
      ConstexprValue address = {0};
      symbolic_constexpr_reference_depth++;
      bool ok = EvaluateConstexprAddressValue(
          ctx, ConstexprInitializerExpression(
                   id->symbol->constexpr_initializer),
          &address);
      symbolic_constexpr_reference_depth--;
      if (!ok ||
          (address.address_binding != NULL &&
           address.address_binding->symbol != NULL &&
           !address.address_binding->symbol->flags.value_set)) {
        return false;
      }
      while (address.is_address) {
        ConstexprValue dereferenced;
        if (!ConstexprDereferenceAddress(address, &dereferenced)) {
          return false;
        }
        address = dereferenced;
      }
      *result = address;
      return true;
    }
    if (binding == NULL && id->symbol != NULL &&
        CompilerSymbolIsMetaPromotedStatic(id->symbol)) {
      ConstexprEnsureMetaPromotedStaticObject(id->symbol);
    }
    if (binding == NULL && id->symbol != NULL &&
        TypeIsFixedArray(id->symbol->type) &&
        id->symbol->constexpr_initializer != NULL &&
        (CompilerSymbolIsMetaPromotedStatic(id->symbol) ||
         !id->symbol->flags.value_set)) {
      ConstexprEvaluateObjectConstantForSymbol(
          id->symbol, id->symbol->constexpr_initializer);
    }
    if (binding == NULL && id->symbol != NULL &&
        !id->symbol->flags.value_set &&
        id->symbol->constexpr_initializer != NULL &&
        TypeIsStructOrUnion(id->symbol->type)) {
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
      if (result->object != NULL && result->object->lifetime_ended) {
        ReportConstexprPlacementFailure(
            node, "read of object outside its lifetime in constant expression");
        return false;
      }
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
    if (bound_object != NULL && bound_object->lifetime_ended) {
      ReportConstexprPlacementFailure(
          node, "read of object outside its lifetime in constant expression");
      return false;
    }
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
      if (TypeIsBasicStringViewType(object_value.object->type)) {
        ConstexprValue* size_slot = ConstexprObjectSlot(object_value.object, 1);
        ConstexprValue* data_slot = ConstexprObjectSlot(object_value.object, 0);
        int64_t size = 0;
        if (size_slot == NULL || data_slot == NULL ||
            !ConstexprValueAsInteger(*size_slot, &size) ||
            (size_t)index >= (size_t)size) {
          return false;
        }
        result->is_object = false;
        result->is_address = false;
        result->is_floating = false;
        result->fvalue = 0;
        result->object = NULL;
        return ConstexprStringViewDataCharacter(ctx, data_slot, (size_t)index,
                                                &result->ivalue);
      }
      slot = ConstexprObjectSlot(object_value.object, (size_t)index);
    } else {
      ConstexprValue address;
      if (!EvaluateConstexprAddressValue(ctx, subscript->left, &address) ||
          index < 0) {
        return false;
      }
      if (address.heap_block != NULL) {
        size_t element_size =
            node->type != NULL && node->type->size > 0
                ? (size_t)node->type->size : 1;
        if ((uint64_t)index > SIZE_MAX / element_size) {
          return false;
        }
        size_t byte_offset = (size_t)index * element_size;
        if (byte_offset > SIZE_MAX - address.heap_index) {
          return false;
        }
        address.heap_index += byte_offset;
        return ConstexprValueFromHeapAddress(ctx, address, node->type, result);
      }
      if (address.address_object == NULL ||
          (uint64_t)index > SIZE_MAX - address.address_index) {
        return false;
      }
      slot = ConstexprObjectSlot(address.address_object,
                                 address.address_index + (size_t)index);
    }
    if (slot == NULL) {
      return false;
    }
    if (slot->lifetime_ended) {
      ReportConstexprPlacementFailure(
          node, "read of object outside its lifetime in constant expression");
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
    StructMemberASTNode* member_node =
        (StructMemberASTNode*)member_access->right;
    if (member_node->member == NULL) {
      return false;
    }
    if (!EvaluateConstexprObjectAccess(ctx, member_access->left,
                                       &object_value) ||
        object_value.object == NULL) {
      object_value = (ConstexprValue){
          .is_object = true,
          .object = ConstexprVirtualMemberRootObject(
              ctx, member_access->left, member_node->member),
      };
      if (object_value.object == NULL) {
        return false;
      }
    }
    if (ConstexprObjectHasVirtualBases(object_value.object)) {
      ConstexprObject* member_object =
          ConstexprObjectForMember(object_value.object, member_node->member);
      if (member_object != NULL) {
        object_value.object = member_object;
      }
    }
    if (!ConstexprUnionMemberActive(object_value.object,
                                    member_node->member)) {
      return false;
    }
    size_t byte_offset = (size_t)member_node->member->byte_offset;
    ConstexprValue* slot = ConstexprSlotForOffset(
        object_value.object->type, object_value.object, byte_offset);
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

bool ConstexprEvaluateObjectSlotInteger(ASTNode* node, size_t slot,
                                        int64_t* result) {
  if (node == NULL || result == NULL) {
    return false;
  }
  ConstEvalContext context;
  ConstEvalContextInit(&context);
  ConstexprValue value = {0};
  bool ok = EvaluateConstexprObjectAccess(&context, node, &value) &&
            value.is_object && value.object != NULL &&
            slot < value.object->slots.length;
  if (ok) {
    ConstexprValue* member = value.object->slots.value.p[slot];
    ok = member != NULL && ConstexprValueAsInteger(*member, result);
  }
  ConstEvalContextDestruct(&context);
  return ok;
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
    if (!EvaluateConstexprAddressValue(ctx, contents->sub, &address)) {
      return false;
    }
    if (address.address_binding != NULL) {
      return false;
    }
    ConstexprValue* pointee_slot = NULL;
    if (address.address_object != NULL) {
      pointee_slot =
          ConstexprObjectSlot(address.address_object, address.address_index);
    } else if (address.address_slot != NULL) {
      ConstexprValue* pointer_slot = address.address_slot;
      if (pointer_slot->is_address) {
        if (pointer_slot->address_binding != NULL) {
          return false;
        }
        if (pointer_slot->address_object != NULL) {
          pointee_slot = ConstexprObjectSlot(pointer_slot->address_object,
                                             pointer_slot->address_index);
        } else if (pointer_slot->address_slot != NULL) {
          pointee_slot = pointer_slot->address_slot;
        }
      } else {
        pointee_slot = pointer_slot;
      }
    }
    if (pointee_slot == NULL) {
      return false;
    }
    *slot = pointee_slot;
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
    if (ConstexprObjectHasVirtualBases(object_value.object)) {
      ConstexprObject* member_object =
          ConstexprObjectForMember(object_value.object, member_node->member);
      if (member_object != NULL) {
        object_value.object = member_object;
      }
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
    if (*slot != NULL && member_node->member->byte_offset >= 0) {
      ConstexprValue* offset_slot = ConstexprSlotForOffset(
          object_value.object->type, object_value.object,
          (size_t)member_node->member->byte_offset);
      if (offset_slot != NULL) {
        *slot = offset_slot;
      }
    }
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
  if (node->op == AST_OP(stmt_expr)) {
    return EvaluateConstexprStatementExpression(ctx, node, node->type,
                                                result) &&
           result->is_address;
  }
  if (node->op == AST_OP(call)) {
    ASTNode* synthesized =
        SemanticTryAnalyzeMetaSynthesisCall((VectorASTNode*)node);
    if (synthesized != NULL) {
      return EvaluateConstexprAddressValue(ctx, synthesized, result);
    }
  }
  if (node->op == AST_OP(expr_init)) {
    ExpressionInitializerASTNode* init = (ExpressionInitializerASTNode*)node;
    return EvaluateConstexprAddressValue(ctx, init->expr, result);
  }
  if (node->op == AST_OP(cast)) {
    CastASTNode* cast = (CastASTNode*)node;
    ASTNode* saved_new_expression = ctx->allocation_new_expression;
    bool lowers_placement_new =
        cast->expr != NULL && cast->expr->op == AST_OP(call) &&
        ((VectorASTNode*)cast->expr)->children != NULL &&
        ((VectorASTNode*)cast->expr)->children->length > 1 &&
        ConstexprIsAllocationFunction(ConstexprCallSymbol(cast->expr));
    if ((node->flags & kASTCXXNewExpression) != 0 ||
        lowers_placement_new) {
      ctx->allocation_new_expression = node;
    }
    bool operand_ok =
        EvaluateConstexprAddressValue(ctx, cast->expr, result);
    ctx->allocation_new_expression = saved_new_expression;
    if (!operand_ok) {
      return false;
    }
    TypeRecord* source_pointee =
        cast->expr != NULL && TypeIsPointer(cast->expr->type)
            ? cast->expr->type->next : NULL;
    TypeRecord* target_pointee =
        TypeIsPointer(cast->cast_type) ? cast->cast_type->next : NULL;
    bool establishes_allocated_type =
        (node->flags & kASTCXXNewExpression) != 0 ||
        (result->heap_block != NULL && cast->expr != NULL &&
         cast->expr->op == AST_OP(call) &&
         ConstexprIsAllocationFunction(ConstexprCallSymbol(cast->expr)));
    if (source_pointee != NULL && TypeIsVoid(source_pointee) &&
        target_pointee != NULL && !TypeIsVoid(target_pointee) &&
        !establishes_allocated_type) {
      TypeRecord* address_type = ConstexprAddressPointeeType(ctx, *result);
      if (!CompilerCXXAtLeast(kLanguageStandardCXX26) ||
          address_type == NULL ||
          !TypeEqualIgnoringTopLevelQualifierMask(
              target_pointee, address_type,
              kQualConst | kQualVolatile | kQualRestrict)) {
        ReportConstexprPlacementFailure(
            node, "constexpr conversion from void pointer requires an object "
                  "of similar type");
        return false;
      }
    }
    if (establishes_allocated_type && target_pointee != NULL) {
      if (result->heap_block != NULL) {
        result->heap_block->object_type = target_pointee;
      }
    }
    return true;
  }
  if (node->op == AST_OP(compound_literal) && node->type != NULL &&
      TypeIsFixedArray(node->type)) {
    ConstexprValue value = {0};
    if (!EvaluateConstexprInitializer(ctx, node->type, node, &value) ||
        !value.is_object || value.object == NULL) {
      return false;
    }
    *result = (ConstexprValue){
        .is_address = true,
        .address_object = value.object,
        .address_index = 0,
    };
    return true;
  }
  if (node->op == AST_OP(string)) {
    ConstexprObject* object = ConstexprStringObject(ctx, node, node->type);
    if (object == NULL) {
      return false;
    }
    *result = (ConstexprValue){
        .is_address = true,
        .address_object = object,
        .address_index = 0};
    return true;
  }
  if (node->op == AST_OP(number)) {
    ConstantASTNode* constant = (ConstantASTNode*)node;
    return constant->value.ivalue == 0 && ConstexprNullAddress(result);
  }
  if (node->op == AST_OP(address)) {
    UnaryASTNode* address = (UnaryASTNode*)node;
    if (address->sub != NULL &&
        address->sub->op == AST_OP(identifier)) {
      Symbol* symbol = ((IdentifierASTNode*)address->sub)->symbol;
      if (symbol != NULL && TypeIsReference(symbol->type) &&
          EvaluateConstexprAddressValue(ctx, address->sub, result)) {
        return true;
      }
    }
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
      *result = (ConstexprValue){
          .is_address = true,
          .address_slot = slot,
      };
      return true;
    }
    if ((template_argument_object_evaluation_depth > 0 ||
         symbolic_constexpr_reference_depth > 0) &&
        address->sub != NULL &&
        address->sub->op == AST_OP(identifier)) {
      Symbol* symbol = ((IdentifierASTNode*)address->sub)->symbol;
      bool template_argument_target =
          template_argument_object_evaluation_depth > 0 && symbol != NULL &&
          !symbol->flags.is_temp && !symbol->flags.is_argument &&
          (!symbol->flags.is_local ||
           StorageIs(symbol->storage, STO(static)));
      bool symbolic_reference_target =
          symbolic_constexpr_reference_depth > 0 && symbol != NULL &&
          !StorageIs(symbol->storage, STO(thread));
      if (template_argument_target || symbolic_reference_target) {
        ConstexprValue initial = {0};
        if (symbol->flags.value_set) {
          initial.is_floating = TypeIsFloatingPoint(symbol->type);
          initial.ivalue = symbol->value.ivalue;
          initial.fvalue = symbol->value.fvalue;
          if (TypeIsFixedArray(symbol->type) ||
              TypeIsStructOrUnion(symbol->type)) {
            initial.is_object = true;
            initial.object = symbol->value.other;
          }
        }
        PushConstexprBinding(ctx, symbol, initial);
        ConstexprBinding* external = FindConstexprBinding(ctx, symbol);
        *result = (ConstexprValue){
            .is_address = true,
            .address_binding = external,
        };
        return external != NULL;
      }
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
      if (binding->heap_block != NULL || binding->address_binding != NULL ||
          binding->address_slot != NULL || binding->address_object != NULL) {
        *result = (ConstexprValue){.is_address = true,
                                   .address_binding = binding->address_binding,
                                   .address_slot = binding->address_slot,
                                   .address_object = binding->address_object,
                                   .address_index = binding->address_index,
                                   .heap_block = binding->heap_block,
                                   .heap_index = binding->heap_index};
        return true;
      }
      if (id->symbol != NULL && TypeIsPointer(id->symbol->type)) {
        return ConstexprNullAddress(result);
      }
      *result = (ConstexprValue){.is_address = true,
                                 .address_binding = binding};
      return true;
    }
    if ((template_argument_object_evaluation_depth > 0 ||
         ConstexprReferenceUsableInCurrentFunction(id->symbol)) &&
        binding == NULL &&
        id->symbol != NULL &&
        id->symbol->constexpr_initializer != NULL &&
        (TypeIsPointer(id->symbol->type) ||
         TypeIsReference(id->symbol->type))) {
      bool symbolic = id->symbol->is_constexpr_representable;
      if (symbolic) {
        symbolic_constexpr_reference_depth++;
      }
      bool ok = EvaluateConstexprAddressValue(
          ctx, ConstexprInitializerExpression(
                   id->symbol->constexpr_initializer),
          result);
      if (symbolic) {
        symbolic_constexpr_reference_depth--;
      }
      return ok;
    }
    if (binding != NULL && binding->object == NULL && !binding->is_floating &&
        id->symbol != NULL && TypeIsPointer(id->symbol->type) &&
        ConstexprPointerValueFromInteger(ctx, binding->ivalue, result)) {
      return true;
    }
    if (binding != NULL && binding->object != NULL &&
        id->symbol != NULL &&
        (TypeIsPointer(id->symbol->type) ||
         TypeIsReference(id->symbol->type))) {
      // Implicit object parameters are represented by an object-bearing
      // binding even though their declared type is a pointer. Preserve an
      // address to that binding for expressions such as `return *this;`.
      *result = (ConstexprValue){
          .is_address = true,
          .address_binding = binding,
      };
      return true;
    }
    Symbol* promoted_target = CompilerMetaPromotedPointerTarget(id->symbol);
    if (promoted_target != NULL) {
      if ((TypeIsFixedArray(promoted_target->type) ||
           TypeIsStructOrUnion(promoted_target->type)) &&
          ConstexprEnsureMetaPromotedStaticObject(promoted_target) &&
          promoted_target->value.other != NULL) {
        *result = (ConstexprValue){
            .is_address = true,
            .address_object = (ConstexprObject*)promoted_target->value.other,
            .address_index = 0,
        };
        return true;
      }
      if (TypeIsIntegral(promoted_target->type) &&
          promoted_target->constexpr_initializer != NULL) {
        int64_t value = 0;
        ASTNode* initializer = ConstexprInitializerExpression(
            promoted_target->constexpr_initializer);
        if (EvaluateIntegerExpressionInContext(ctx, initializer, &value)) {
          ConstexprObject* object =
              NewConstexprObject(ctx, promoted_target->type, 1);
          ConstexprValue* slot = ConstexprObjectSlot(object, 0);
          if (slot != NULL) {
            slot->ivalue = value;
            *result = (ConstexprValue){
                .is_address = true,
                .address_object = object,
                .address_index = 0,
            };
            return true;
          }
        }
      }
    }
    if (binding != NULL && binding->object != NULL &&
        TypeIsFixedArray(binding->object->type)) {
      *result = (ConstexprValue){.is_address = true,
                                 .address_object = binding->object,
                                 .address_index = 0};
      return true;
    }
    if (id->symbol != NULL && CompilerSymbolIsMetaPromotedStatic(id->symbol)) {
      if (ConstexprEnsureMetaPromotedStaticObject(id->symbol) &&
          id->symbol->value.other != NULL &&
          TypeIsFixedArray(id->symbol->type)) {
        *result = (ConstexprValue){
            .is_address = true,
            .address_object = (ConstexprObject*)id->symbol->value.other,
            .address_index = 0,
        };
        return true;
      }
    }
    if (id->symbol != NULL && TypeIsFixedArray(id->symbol->type) &&
        id->symbol->constexpr_initializer != NULL &&
        (CompilerSymbolIsMetaPromotedStatic(id->symbol) ||
         !id->symbol->flags.value_set)) {
      ConstexprEvaluateObjectConstantForSymbol(
          id->symbol, id->symbol->constexpr_initializer);
    }
    if (id->symbol != NULL && id->symbol->flags.value_set &&
        TypeIsFixedArray(id->symbol->type) &&
        id->symbol->value.other != NULL) {
      *result = (ConstexprValue){
          .is_address = true,
          .address_object = (ConstexprObject*)id->symbol->value.other,
          .address_index = 0,
      };
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
    bool base_ok =
        EvaluateConstexprAddressValue(ctx, arithmetic->left, &base);
    bool offset_ok =
        EvaluateIntegerExpressionInContext(ctx, offset_expr, &offset);
    if (!base_ok || !offset_ok) {
      return false;
    }
    base = ConstexprResolveForwardedAddress(base);
    ConstexprValue canonical_base = base;
    ConstexprCanonicalizeAddressValue(&canonical_base);
    if (ConstexprObjectHasVirtualBases(canonical_base.address_object)) {
      base = canonical_base;
    }
    if (base.heap_block != NULL) {
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
      if ((offset < 0 && (uint64_t)(-offset) > base.heap_index) ||
          (offset >= 0 &&
           (uint64_t)offset > SIZE_MAX - base.heap_index) ||
          base.heap_index + (size_t)offset >= base.heap_block->size) {
        return false;
      }
      base.heap_index += (size_t)offset;
      *result = base;
      return true;
    }
    TypeRecord* pointer_type = node->type;
    TypeRecord* pointee =
        TypeIsPointer(pointer_type) ? pointer_type->next : NULL;
    if (!offset_is_elements && offset >= 0 &&
        base.address_object != NULL && pointee != NULL &&
        TypeIsStructOrUnion(pointee)) {
      ConstexprObject* subobject = ConstexprSubobjectAtOffset(
          base.address_object, pointee, (size_t)offset);
      if (subobject != NULL) {
        *result = (ConstexprValue){
            .is_address = true,
            .address_object = subobject,
            .address_index = 0,
        };
        return true;
      }
    }
    if (offset == 0) {
      *result = base;
      return true;
    }
    if (base.address_object == NULL) {
      return false;
    }
    if (node->op == AST_OP(minus)) {
      if (offset == INT64_MIN) {
        return false;
      }
      offset = -offset;
    }
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
    BinaryASTNode* subscript = (BinaryASTNode*)node;
    ConstexprValue base;
    int64_t index;
    if (EvaluateConstexprAddressValue(ctx, subscript->left, &base) &&
        EvaluateIntegerExpressionInContext(ctx, subscript->right, &index) &&
        index >= 0) {
      base = ConstexprResolveForwardedAddress(base);
      if (base.heap_block != NULL) {
        size_t element_size =
            node->type != NULL && node->type->size > 0
                ? (size_t)node->type->size : 1;
        if ((uint64_t)index > SIZE_MAX / element_size) {
          return false;
        }
        size_t byte_offset = (size_t)index * element_size;
        if (byte_offset > SIZE_MAX - base.heap_index ||
            base.heap_index + byte_offset >= base.heap_block->size) {
          return false;
        }
        base.heap_index += byte_offset;
        *result = base;
        return true;
      }
      if (base.address_object != NULL &&
          (uint64_t)index <= SIZE_MAX - base.address_index &&
          base.address_index + (size_t)index <
              base.address_object->slots.length) {
        base.address_index += (size_t)index;
        *result = base;
        return true;
      }
    }
    ConstexprValue* slot = NULL;
    if (EvaluateConstexprObjectLValue(ctx, node, &slot, false) &&
        slot != NULL) {
      *result = (ConstexprValue){.is_address = true, .address_slot = slot};
      return true;
    }
  }
  if (node->op == AST_OP(dot) || node->op == AST_OP(arrow)) {
    if (node->type != NULL && TypeIsPointer(node->type)) {
      ConstexprValue value = {0};
      if (EvaluateConstexprObjectAccess(ctx, node, &value) &&
          value.is_address) {
        *result = value;
        return true;
      }
    }
    ConstexprValue* slot = NULL;
    if (EvaluateConstexprObjectLValue(ctx, node, &slot, true) &&
        slot != NULL) {
      if (slot->is_object && slot->object != NULL &&
          TypeIsFixedArray(slot->object->type)) {
        *result = (ConstexprValue){
            .is_address = true,
            .address_object = slot->object,
            .address_index = 0,
        };
        return true;
      }
      *result = (ConstexprValue){
          .is_address = true,
          .address_slot = slot,
      };
      return true;
    }
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
      if (result->heap_block != NULL && result->heap_block->live &&
          ctx->call_depth == 0) {
        return false;
      }
      if (result->heap_block != NULL && TypeIsPointer(node->type) &&
          node->type->next != NULL && !TypeIsVoid(node->type->next) &&
          result->heap_block->object_type == NULL) {
        result->heap_block->object_type = node->type->next;
      }
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
  if (binding_expr != NULL && binding_expr->op == AST_OP(contents)) {
    if (EvaluateConstexprAddressValue(
            ctx, ((UnaryASTNode*)binding_expr)->sub, value)) {
      return true;
    }
  }
  if (binding_expr != NULL && binding_expr->op == AST_OP(identifier)) {
    Symbol* symbol = ((IdentifierASTNode*)binding_expr)->symbol;
    if (symbol != NULL && TypeIsReference(symbol->type) &&
        EvaluateConstexprAddressValue(ctx, binding_expr, value)) {
      return true;
    }
  }
  if (binding_expr != NULL && binding_expr->op == AST_OP(cast) &&
      TypeIsStructOrUnion(formal_object_type)) {
    ConstexprValue object_value = {0};
    ASTNode* operand = ((CastASTNode*)binding_expr)->expr;
    if (EvaluateConstexprObjectAccess(ctx, operand, &object_value) &&
        object_value.is_object && object_value.object != NULL) {
      *value = object_value;
      return true;
    }
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
  if (binding_expr != NULL && binding_expr->op == AST_OP(call)) {
    ConstexprValue call_value = {0};
    if (EvaluateConstexprCall(ctx, binding_expr, &call_value) &&
        (call_value.is_address ||
         (call_value.is_object && call_value.object != NULL))) {
      *value = call_value;
      return true;
    }
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
  bool left_is_pointer =
      binary->left != NULL && binary->left->type != NULL &&
      (TypeIsPointer(binary->left->type) ||
       TypeIsNullPointer(binary->left->type));
  bool right_is_pointer =
      binary->right != NULL && binary->right->type != NULL &&
      (TypeIsPointer(binary->right->type) ||
       TypeIsNullPointer(binary->right->type));
  if (!left_is_pointer && !right_is_pointer) {
    return false;
  }
  ConstexprValue left;
  ConstexprValue right;
  if (!EvaluateConstexprAddressValue(ctx, binary->left, &left) ||
      !EvaluateConstexprAddressValue(ctx, binary->right, &right)) {
    return false;
  }
  left = ConstexprResolveForwardedAddress(left);
  right = ConstexprResolveForwardedAddress(right);
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

bool ConstexprEvaluatePointerDifference(ConstEvalContext* ctx, ASTNode* node,
                                        int64_t* result) {
  if (ctx == NULL || node == NULL || node->op != AST_OP(minus) ||
      result == NULL) {
    return false;
  }
  BinaryASTNode* difference = (BinaryASTNode*)node;
  if (difference->left == NULL || difference->right == NULL ||
      !TypeIsPointer(difference->left->type) ||
      !TypeIsPointer(difference->right->type)) {
    return false;
  }
  ConstexprValue left = {0};
  ConstexprValue right = {0};
  if (!EvaluateConstexprAddressValue(ctx, difference->left, &left) ||
      !EvaluateConstexprAddressValue(ctx, difference->right, &right)) {
    return false;
  }
  left = ConstexprResolveForwardedAddress(left);
  right = ConstexprResolveForwardedAddress(right);

  size_t left_index;
  size_t right_index;
  if (left.address_object != NULL &&
      left.address_object == right.address_object) {
    left_index = left.address_index;
    right_index = right.address_index;
  } else if (left.heap_block != NULL &&
             left.heap_block == right.heap_block) {
    size_t element_size = difference->left->type->next != NULL &&
            difference->left->type->next->size > 0
        ? (size_t)difference->left->type->next->size
        : 1;
    if (left.heap_index % element_size != 0 ||
        right.heap_index % element_size != 0) {
      return false;
    }
    left_index = left.heap_index / element_size;
    right_index = right.heap_index / element_size;
  } else if (ConstexprAddressEqual(left, right)) {
    *result = 0;
    return true;
  } else {
    return false;
  }

  if (left_index >= right_index) {
    size_t distance = left_index - right_index;
    if (distance > INT64_MAX) {
      return false;
    }
    *result = (int64_t)distance;
  } else {
    size_t distance = right_index - left_index;
    if (distance > INT64_MAX) {
      return false;
    }
    *result = -(int64_t)distance;
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
      !EvaluateConstexprAddressValue(ctx, end_expr, &end_value)) {
    return false;
  }
  begin_value = ConstexprResolveForwardedAddress(begin_value);
  end_value = ConstexprResolveForwardedAddress(end_value);
  if (begin_value.address_object == NULL ||
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

ReflectionValue* ConstexprEvaluatePointerDereferenceAsReflection(
    ConstEvalContext* ctx, ASTNode* node) {
  if (node == NULL || node->op != AST_OP(contents)) {
    return NULL;
  }
  ConstexprValue value = {0};
  if (!EvaluateConstexprAddressValue(ctx, node, &value) ||
      value.is_object || value.is_address) {
    return NULL;
  }
  return (ReflectionValue*)(intptr_t)value.ivalue;
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
  ConstexprValue address = {0};
  ConstexprValue pointee = {0};
  bool address_ok = EvaluateConstexprAddressValue(ctx, node, &address);
  if (address_ok &&
      address.address_object != NULL && address.address_index == 0 &&
      node->type != NULL && TypeIsPointer(node->type) &&
      TypeEqual(address.address_object->type, node->type->next)) {
    *object = address.address_object;
    return true;
  }
  if (address_ok && address.is_address &&
      ConstexprDereferenceAddress(address, &pointee) &&
      pointee.is_object && pointee.object != NULL) {
    *object = pointee.object;
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
  ConstexprEvalMode mode = compiler->constexpr_eval_mode;
  compiler->constexpr_eval_mode = kConstexprEvalAST;
  bool evaluated = EvaluateConstexprAddressValue(&ctx, pointer, &address) &&
                   address.is_address;
  compiler->constexpr_eval_mode = mode;
  ConstexprObject* object = address.address_object;
  size_t index = address.address_index;
  if (evaluated && object == NULL && address.heap_block != NULL) {
    bool ok = address.heap_index + count <= address.heap_block->size;
    if (ok) {
      StringClear(result);
      for (size_t i = 0; i < count; i++) {
        StringAppendChar(result,
                         (char)address.heap_block->memory[address.heap_index +
                                                          i]);
      }
    }
    ConstEvalContextDestruct(&ctx);
    if (!ok) {
      StringClear(result);
    }
    return ok;
  }
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
  ASTNode* callee = call->left;
  while (callee != NULL && callee->op == AST_OP(comma)) {
    callee = ((BinaryASTNode*)callee)->right;
  }
  if (callee == NULL || callee->op != AST_OP(identifier)) {
    return NULL;
  }
  Symbol* symbol = ((IdentifierASTNode*)callee)->symbol;
  Symbol* function = CompilerConstantFunctionPointerTarget(symbol);
  return function != NULL ? function : symbol;
}

static bool ConstexprParameterTypeSupported(TypeRecord* type) {
  return TypeIsIntegral(type) || TypeIsFloatingPoint(type) ||
         TypeIsFixedArray(type) || TypeIsStructOrUnion(type) ||
         TypeIsPointer(type) || TypeIsReference(type) ||
         TypeIsReflection(type);
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
      if (actual != NULL && actual->op == AST_OP(identifier) &&
          ((IdentifierASTNode*)actual)->symbol != NULL &&
          StringEqual(&((IdentifierASTNode*)actual)->symbol->name, "this")) {
        ConstexprObject* fallback_object = NULL;
        TypeRecord* required_object_type =
            formal->type != NULL && TypeIsPointer(formal->type)
                ? formal->type->next : NULL;
        for (size_t binding_index = ctx->bindings.length;
             binding_index > 0; --binding_index) {
          ConstexprBinding* this_binding =
              ctx->bindings.value.p[binding_index - 1];
          if (this_binding->symbol != NULL &&
              StringEqual(&this_binding->symbol->name, "this") &&
              this_binding->object != NULL) {
            if (fallback_object == NULL) {
              fallback_object = this_binding->object;
            }
            if (required_object_type != NULL &&
                TypeEqual(this_binding->object->type,
                          required_object_type)) {
              object = this_binding->object;
              break;
            }
          }
        }
        if (object == NULL) {
          object = fallback_object;
        }
      }
      if (object == NULL &&
          !EvaluateConstexprObjectAddress(ctx, actual, &object)) {
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
  size_t formal_offset = 1;
  bool has_complete_object_parameter =
      func->info.function.prototype.length == actuals->length + 2 &&
      func->info.function.prototype.value.p[1] != NULL &&
      StringEqual(
          &((Symbol*)func->info.function.prototype.value.p[1])->name,
          "__complete_object");
  if (func->info.function.prototype.length !=
      actuals->length + 1 + (has_complete_object_parameter ? 1 : 0)) {
    return false;
  }
  Symbol* this_formal = func->info.function.prototype.value.p[0];
  if (this_formal == NULL || !StringEqual(&this_formal->name, "this")) {
    return false;
  }
  PushConstexprBinding(ctx, this_formal,
                       (ConstexprValue){.is_object = true, .object = object});
  if (has_complete_object_parameter) {
    Symbol* complete_formal = func->info.function.prototype.value.p[1];
    PushConstexprBinding(ctx, complete_formal,
                         (ConstexprValue){.ivalue = 1});
    formal_offset++;
  }
  for (size_t i = 0; i < actuals->length; i++) {
    Symbol* formal =
        func->info.function.prototype.value.p[i + formal_offset];
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

static ASTNode* ConstexprFindClassResultCall(ASTNode* node) {
  if (node == NULL) {
    return NULL;
  }
  if (node->op == AST_OP(call)) {
    Symbol* symbol = ConstexprCallSymbol(node);
    if (symbol != NULL && symbol->name.value != NULL &&
        (strcmp(symbol->name.value, "current_exception") == 0 ||
         strcmp(symbol->name.value, "make_exception_ptr") == 0)) {
      return node;
    }
  }
  if (node->op == AST_OP(expr_init)) {
    return ConstexprFindClassResultCall(
        ((ExpressionInitializerASTNode*)node)->expr);
  }
  if (node->op == AST_OP(compound_literal)) {
    return ConstexprFindClassResultCall(
        ((CompoundLiteralASTNode*)node)->initializer);
  }
  if (node->op == AST_OP(braced_init)) {
    Vector* initializers = ((BracedInitializerASTNode*)node)->initializers;
    if (initializers != NULL) {
      for (size_t i = 0; i < initializers->length; i++) {
        ASTNode* result =
            ConstexprFindClassResultCall(initializers->value.p[i]);
        if (result != NULL) {
          return result;
        }
      }
    }
  }
  if (node->op == AST_OP(designated_init)) {
    return ConstexprFindClassResultCall(
        ((DesignatedInitializerASTNode*)node)->init);
  }
  ASTNodeShape shape = ASTNodeGetShape(node);
  if (shape == kASTShapeBinary) {
    BinaryASTNode* binary = (BinaryASTNode*)node;
    ASTNode* result = ConstexprFindClassResultCall(binary->left);
    return result != NULL ? result
                          : ConstexprFindClassResultCall(binary->right);
  }
  if (shape == kASTShapeVector) {
    VectorASTNode* vector = (VectorASTNode*)node;
    if (vector->children != NULL) {
      for (size_t i = 0; i < vector->children->length; i++) {
        ASTNode* result =
            ConstexprFindClassResultCall(vector->children->value.p[i]);
        if (result != NULL) {
          return result;
        }
      }
    }
  }
  return NULL;
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
  if (TypeIsReflection(decl->symbol->type)) {
    ReflectionValue* reflection =
        ConstexprEvaluateReflectionExpression(ctx, decl->initializer);
    if (reflection == NULL) {
      return false;
    }
    PushConstexprBinding(
        ctx, decl->symbol,
        (ConstexprValue){.ivalue = (int64_t)(intptr_t)reflection});
    return true;
  }
  if (TypeIsPointer(decl->symbol->type) || TypeIsReference(decl->symbol->type)) {
    ASTNode* initializer = ConstexprInitializerExpression(decl->initializer);
    if (initializer == NULL) {
      if (TypeIsPointer(decl->symbol->type) &&
          StringStartsWith(&decl->symbol->name, "__invented__")) {
        ConstexprValue null_value = {0};
        ConstexprNullAddress(&null_value);
        PushConstexprBinding(ctx, decl->symbol, null_value);
        return true;
      }
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
    if (initializer == NULL &&
        CompilerCXXAtLeast(kLanguageStandardCXX26) &&
        TypeIsStructOrUnion(decl->symbol->type) &&
        decl->symbol->type->info.struct_info != NULL &&
        decl->symbol->type->info.struct_info->is_union) {
      ConstexprValue object_value = {
          .is_object = true,
          .object = NewConstexprObject(
              ctx, decl->symbol->type,
              ConstexprObjectSlotCount(decl->symbol->type)),
      };
      PushConstexprBinding(ctx, decl->symbol, object_value);
      return true;
    }
    ASTNode* result_call = ConstexprFindClassResultCall(initializer);
    if (result_call != NULL) {
      ConstexprValue object_value = {0};
      if (EvaluateConstexprCall(ctx, result_call, &object_value) &&
          object_value.is_object && object_value.object != NULL) {
        PushConstexprBinding(ctx, decl->symbol, object_value);
        return true;
      }
    }
    ASTNode* class_call = initializer;
    ASTNode* class_cleanup = NULL;
    if (initializer != NULL && initializer->op == AST_OP(comma)) {
      BinaryASTNode* comma = (BinaryASTNode*)initializer;
      class_call = comma->left;
      class_cleanup = comma->right;
    }
    if (class_call != NULL && class_call->op == AST_OP(call)) {
      ConstexprValue object_value;
      if (EvaluateConstexprCall(ctx, class_call, &object_value) &&
          object_value.is_object && object_value.object != NULL) {
        PushConstexprBinding(ctx, decl->symbol, object_value);
        return class_cleanup == NULL ||
               EvaluateConstexprVoidExpression(ctx, class_cleanup);
      }
      object_value = (ConstexprValue){
          .is_object = true,
          .object = NewConstexprObject(
              ctx, decl->symbol->type,
              ConstexprObjectSlotCount(decl->symbol->type)),
      };
      if (StructHasVirtualBases(decl->symbol->type->info.struct_info) &&
          !ConstexprMaterializeBaseSubobjectSlots(ctx,
                                                  object_value.object)) {
        return false;
      }
      size_t mark = ctx->bindings.length;
      PushConstexprBinding(ctx, decl->symbol, object_value);
      if (!EvaluateConstexprConstructorCall(ctx, class_call) ||
          (class_cleanup != NULL &&
           !EvaluateConstexprVoidExpression(ctx, class_cleanup))) {
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

static ConstexprStatementResult ConstexprFailureStatementResult(
    ConstEvalContext* ctx) {
  return ConstexprHasPendingException(ctx) ? kConstexprStmtThrow
                                           : kConstexprStmtInvalid;
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

static bool EvaluateConstexprStartLifetime(ConstEvalContext* ctx,
                                           ASTNode* expr) {
  if (expr == NULL || expr->op != AST_OP(builtin_start_lifetime)) {
    return false;
  }
  VectorASTNode* builtin = (VectorASTNode*)expr;
  if (builtin->children == NULL || builtin->children->length != 1) {
    return false;
  }
  ASTNode* operand = builtin->children->value.p[0];
  ConstexprValue address = {0};
  if (!EvaluateConstexprAddressValue(ctx, operand, &address)) {
    return false;
  }
  ConstexprObject* object = ConstexprAddressTargetObject(ctx, address);
  if (object == NULL && address.address_slot != NULL &&
      address.address_slot->is_object) {
    object = address.address_slot->object;
  }
  if (object == NULL) {
    return false;
  }
  object->lifetime_ended = false;
  if (TypeIsStructOrUnion(object->type) &&
      object->type->info.struct_info != NULL &&
      object->type->info.struct_info->is_union) {
    object->active_union_member = NULL;
  }
  for (size_t i = 0; i < object->slots.length; i++) {
    ConstexprValue* slot = object->slots.value.p[i];
    if (slot != NULL) {
      slot->lifetime_ended = true;
      if (slot->is_object && slot->object != NULL) {
        slot->object->lifetime_ended = true;
      }
    }
  }
  if (address.address_slot != NULL) {
    address.address_slot->lifetime_ended = false;
  }
  return true;
}

static bool EvaluateConstexprVoidExpression(ConstEvalContext* ctx,
                                            ASTNode* expr) {
  if (expr == NULL) {
    return true;
  }
  if (expr->op == AST_OP(throw)) {
    (void)ConstexprEvaluateThrowExpression(ctx, expr);
    return false;
  }
  if (expr->op == AST_OP(builtin_start_lifetime)) {
    return EvaluateConstexprStartLifetime(ctx, expr);
  }
  if (expr->op == AST_OP(comma)) {
    BinaryASTNode* comma = (BinaryASTNode*)expr;
    ASTNode* result_call = ConstexprFindClassResultCall(expr);
    if (result_call != NULL && comma->left != NULL &&
        comma->left->op == AST_OP(call)) {
      VectorASTNode* constructor = (VectorASTNode*)comma->left;
      if (constructor->children != NULL &&
          constructor->children->length != 0) {
        ASTNode* destination = constructor->children->value.p[0];
        if (destination != NULL && destination->op == AST_OP(address)) {
          destination = ((UnaryASTNode*)destination)->sub;
        }
        ConstexprValue* slot = NULL;
        ConstexprValue value = {0};
        if (destination != NULL &&
            EvaluateConstexprObjectLValue(ctx, destination, &slot, true) &&
            EvaluateConstexprCall(ctx, result_call, &value) &&
            value.is_object && value.object != NULL && slot != NULL) {
          *slot = value;
          return true;
        }
      }
    }
    ConstexprValue ignored = {0};
    bool left_ok =
        comma->left == NULL || TypeIsVoid(comma->left->type)
            ? EvaluateConstexprVoidExpression(ctx, comma->left)
            : EvaluateConstexprValue(ctx, comma->left, comma->left->type,
                                     &ignored);
    if (!left_ok) {
      return false;
    }
    bool right_ok = comma->right == NULL || TypeIsVoid(comma->right->type)
               ? EvaluateConstexprVoidExpression(ctx, comma->right)
               : EvaluateConstexprValue(ctx, comma->right, comma->right->type,
                                        &ignored);
    return right_ok;
  }
  if (expr->op == AST_OP(cast)) {
    ASTNode* operand = ((CastASTNode*)expr)->expr;
    if (operand == NULL) {
      return true;
    }
    if (TypeIsVoid(operand->type) || operand->op == AST_OP(call) ||
        operand->op == AST_OP(comma) || operand->op == AST_OP(cast) ||
        operand->op == AST_OP(throw)) {
      return EvaluateConstexprVoidExpression(ctx, operand);
    }
    ConstexprValue ignored = {0};
    return EvaluateConstexprValue(ctx, operand, operand->type, &ignored);
  }
  if (expr->op != AST_OP(call)) {
    return true;
  }
  ASTNode* receiver = NULL;
  Symbol* callee =
      ConstexprFunctionDefinition(ConstexprCallSymbol(expr));
  if (callee == NULL) {
    callee = ConstexprFunctionDefinition(
        ConstexprMemberCallSymbol(expr, &receiver));
  }
  if (callee == NULL) {
    callee =
        ConstexprFunctionDefinition(ConstexprVirtualCallSymbol(ctx, expr,
                                                               &receiver));
  }
  if (callee == NULL || callee->type == NULL ||
      !TypeIsFunction(callee->type)) {
    return true;
  }
  if (callee->type->info.function.is_constructor) {
    return EvaluateConstexprConstructorCall(ctx, expr);
  }
  if (callee->type->info.function.is_destructor) {
    return EvaluateConstexprDestructorCall(ctx, expr);
  }
  ConstexprValue ignored = {0};
  return EvaluateConstexprCall(ctx, expr, &ignored);
}

static TypeRecord* ConstexprExceptionObjectType(TypeRecord* type) {
  while (type != NULL && TypeIsReference(type)) {
    type = type->next;
  }
  return type;
}

static bool ConstexprExceptionTypesMatch(TypeRecord* thrown,
                                         TypeRecord* caught) {
  thrown = ConstexprExceptionObjectType(thrown);
  caught = ConstexprExceptionObjectType(caught);
  if (thrown == NULL || caught == NULL) {
    return false;
  }
  TypeRecord* plain_thrown = TypeRecordCopy(thrown);
  TypeRecord* plain_caught = TypeRecordCopy(caught);
  plain_thrown->qualifiers = kQualPlain;
  plain_caught->qualifiers = kQualPlain;
  bool matches = TypeEqual(plain_thrown, plain_caught);
  TypeRecordDelete(plain_thrown);
  TypeRecordDelete(plain_caught);
  return matches;
}

static size_t ConstexprPublicBasePathCount(Struct* derived, Struct* target,
                                           size_t limit) {
  if (derived == NULL || target == NULL || limit == 0) {
    return 0;
  }
  size_t count = 0;
  for (size_t i = 0; i < derived->bases.length && count < limit; i++) {
    CXXBaseSpecifier* base = derived->bases.value.p[i];
    if (base == NULL || base->access != kAccessPublic ||
        base->type == NULL || !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    Struct* base_struct = base->type->info.struct_info;
    if (base_struct == target) {
      count++;
    } else {
      count += ConstexprPublicBasePathCount(
          base_struct, target, limit - count);
    }
  }
  return count;
}

static bool ConstexprCatchMatches(CatchASTNode* handler,
                                  ConstexprException* exception) {
  if (handler == NULL || exception == NULL) {
    return false;
  }
  if (handler->is_catch_all) {
    return true;
  }
  if (handler->symbol == NULL) {
    return false;
  }
  if (ConstexprExceptionTypesMatch(exception->type, handler->symbol->type)) {
    return true;
  }
  TypeRecord* thrown = ConstexprExceptionObjectType(exception->type);
  TypeRecord* caught = ConstexprExceptionObjectType(handler->symbol->type);
  return TypeIsStructOrUnion(thrown) && TypeIsStructOrUnion(caught) &&
         thrown->info.struct_info != NULL && caught->info.struct_info != NULL &&
         ConstexprPublicBasePathCount(thrown->info.struct_info,
                                      caught->info.struct_info, 2) == 1;
}

static void ConstexprRemoveException(ConstEvalContext* ctx,
                                     ConstexprException* exception) {
  if (ctx == NULL || exception == NULL) {
    return;
  }
  if (ctx->exception == exception) {
    ctx->exception = exception->previous;
  } else {
    for (ConstexprException* current = ctx->exception;
         current != NULL && current->previous != NULL;
         current = current->previous) {
      if (current->previous == exception) {
        current->previous = exception->previous;
        break;
      }
    }
  }
  exception->active = false;
  exception->previous = NULL;
  if (exception->references != 0) {
    return;
  }
  if (exception->token != NULL) {
    ConstexprHeapFree(ctx, exception->token);
  }
  for (size_t i = 0; i < ctx->exception_handles.length; i++) {
    if (ctx->exception_handles.value.p[i] == exception) {
      ctx->exception_handles.value.p[i] = NULL;
      break;
    }
  }
  free(exception);
}

static bool ConstexprBindCatch(ConstEvalContext* ctx, CatchASTNode* handler,
                               ConstexprException* exception) {
  if (handler->symbol == NULL) {
    return true;
  }
  ConstexprValue value = exception->value;
  if (TypeIsReference(handler->symbol->type)) {
    value = (ConstexprValue){
        .is_address = true,
        .address_slot = &exception->value,
    };
  } else if (value.is_object) {
    TypeRecord* object_type =
        ConstexprExceptionObjectType(exception->type);
    ConstexprObject* copied = NULL;
    if (!ConstexprCopyConstructObject(ctx, object_type, value.object, &copied,
                                      false)) {
      return false;
    }
    value.object = copied;
  }
  PushConstexprBinding(ctx, handler->symbol, value);
  return true;
}

static Symbol* ConstexprFindDestructorSymbol(TypeRecord* type) {
  if (type == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < type->info.struct_info->members.length; i++) {
    StructMember* member = type->info.struct_info->members.value.p[i];
    if (member != NULL && member->is_member_function &&
        member->symbol != NULL && member->symbol->type != NULL &&
        TypeIsFunction(member->symbol->type) &&
        member->symbol->type->info.function.is_destructor) {
      Symbol* destructor = ConstexprFunctionDefinition(member->symbol);
      if (destructor != NULL && destructor->type != NULL &&
          destructor->type->info.function.cxx_member_owner ==
              type->info.struct_info) {
        return destructor;
      }
    }
  }
  return NULL;
}

static bool ConstexprDestroyObjectMembersAndBases(ConstEvalContext* ctx,
                                                  TypeRecord* type,
                                                  ConstexprObject* object) {
  if (type == NULL || object == NULL || type->info.struct_info == NULL) {
    return true;
  }
  Struct* str = type->info.struct_info;
  for (size_t i = str->members.length; i > 0; --i) {
    StructMember* member = str->members.value.p[i - 1];
    if (member == NULL || member->is_static || member->is_member_function ||
        member->symbol == NULL || member->symbol->type == NULL) {
      continue;
    }
    ConstexprValue* slot = ConstexprObjectSlot(
        object, ConstexprMemberStorageIndex(str, member));
    if (slot != NULL && slot->is_object && slot->object != NULL &&
        !EvaluateConstexprObjectDestructor(ctx, member->symbol->type,
                                           slot->object)) {
      return false;
    }
  }
  for (size_t i = str->bases.length; i > 0; --i) {
    CXXBaseSpecifier* base = str->bases.value.p[i - 1];
    if (base == NULL || base->is_virtual || base->type == NULL) {
      continue;
    }
    ConstexprValue* slot = ConstexprObjectSlot(
        object, ConstexprBaseStorageIndex(str, i - 1));
    if (slot != NULL && slot->is_object && slot->object != NULL &&
        !EvaluateConstexprObjectDestructor(ctx, base->type, slot->object)) {
      return false;
    }
  }
  return true;
}

static bool EvaluateConstexprObjectDestructor(ConstEvalContext* ctx,
                                              TypeRecord* object_type,
                                              ConstexprObject* object) {
  TypeRecord* type = ConstexprExceptionObjectType(object_type);
  if (type != NULL && object != NULL && object->type != NULL &&
      !TypeEqual(object->type, type) &&
      !TypeIsDerivedFrom(object->type, type)) {
    type = object->type;
  }
  if (type == NULL || !TypeIsStructOrUnion(type) || object == NULL ||
      type->info.struct_info == NULL) {
    return true;
  }
  Symbol* destructor = ConstexprFindDestructorSymbol(type);
  if (destructor != NULL && destructor->type != NULL &&
      destructor->type->info.function.is_virtual) {
    destructor = ConstexprResolveVirtualMember(object, destructor);
  }
  if (destructor == NULL || destructor->type == NULL ||
      destructor->type->info.function.is_trivial_special_member) {
    return ConstexprDestroyObjectMembersAndBases(ctx, type, object);
  }
  if (!destructor->type->info.function.is_constexpr) {
    return ConstexprDestroyObjectMembersAndBases(ctx, type, object);
  }
  if (destructor->type->info.function.body == NULL) {
    if (destructor->type->info.function.is_defaulted) {
      return ConstexprDestroyObjectMembersAndBases(ctx, type, object);
    }
    return false;
  }
  Vector actuals;
  VectorInit(&actuals);
  size_t mark = ctx->bindings.length;
  ctx->call_depth++;
  ConstexprValue ignored = {0};
  bool bound = BindConstexprConstructorObjectActuals(
      ctx, destructor, object, &actuals);
  ConstexprStatementResult destructor_result =
      bound ? EvaluateConstexprStatement(
                  ctx, destructor->type->info.function.body,
                  destructor->type->next, &ignored)
            : kConstexprStmtInvalid;
  ctx->call_depth--;
  PopConstexprBindings(ctx, mark);
  VectorDestruct(&actuals);
  if (destructor_result != kConstexprStmtNormal) {
    return false;
  }
  return ConstexprDestroyObjectMembersAndBases(ctx, type, object);
}

static bool EvaluateConstexprExceptionDestructor(
    ConstEvalContext* ctx, ConstexprException* exception) {
  if (exception == NULL || exception->destroyed ||
      exception->references != 0) {
    return true;
  }
  bool ok = EvaluateConstexprObjectDestructor(ctx, exception->type,
                                              exception->value.object);
  if (ok) {
    exception->destroyed = true;
  }
  return ok;
}

static ConstexprStatementResult EvaluateConstexprTry(
    ConstEvalContext* ctx, TryASTNode* try_stmt, TypeRecord* return_type,
    ConstexprValue* result) {
  size_t try_mark = ctx->bindings.length;
  ConstexprStatementResult try_result = EvaluateConstexprStatement(
      ctx, try_stmt->try_stmt, return_type, result);
  if (try_result != kConstexprStmtThrow) {
    return try_result;
  }
  if (!DestroyConstexprBindingsFromMark(ctx, try_mark)) {
    return kConstexprStmtInvalid;
  }
  ConstexprException* exception = ctx->exception;
  if (exception == NULL) {
    return kConstexprStmtInvalid;
  }
  for (size_t i = 0; i < try_stmt->catches->length; i++) {
    CatchASTNode* handler = try_stmt->catches->value.p[i];
    if (!ConstexprCatchMatches(handler, exception)) {
      continue;
    }
    size_t mark = ctx->bindings.length;
    exception->handling = true;
    if (!ConstexprBindCatch(ctx, handler, exception)) {
      exception->handling = false;
      return kConstexprStmtInvalid;
    }
    ConstexprObject* catch_object = NULL;
    if (handler->symbol != NULL &&
        !TypeIsReference(handler->symbol->type)) {
      ConstexprBinding* binding =
          FindConstexprBinding(ctx, handler->symbol);
      catch_object = binding != NULL ? binding->object : NULL;
    }
    ConstexprStatementResult handler_result = EvaluateConstexprStatement(
        ctx, handler->stmt, return_type, result);
    if (handler_result != kConstexprStmtThrow &&
        ctx->exception == exception &&
        !EvaluateConstexprExceptionDestructor(ctx, exception)) {
      ConstexprRemoveException(ctx, exception);
      return kConstexprStmtInvalid;
    }
    bool catch_destroyed =
        catch_object == NULL ||
        EvaluateConstexprObjectDestructor(
            ctx, handler->symbol->type, catch_object);
    PopConstexprBindings(ctx, mark);
    if (!catch_destroyed) {
      if (ctx->exception == exception) {
        ConstexprRemoveException(ctx, exception);
      }
      return kConstexprStmtInvalid;
    }
    bool exception_consumed =
        handler_result != kConstexprStmtThrow || ctx->exception != exception;
    if (exception_consumed) {
      ConstexprException* unwinding = ctx->exception;
      bool suppress_pending =
          unwinding != NULL && unwinding != exception && !unwinding->handling;
      if (suppress_pending) {
        unwinding->handling = true;
        ctx->unwinding_exceptions++;
      }
      bool destroyed = EvaluateConstexprExceptionDestructor(ctx, exception);
      if (suppress_pending) {
        ctx->unwinding_exceptions--;
        unwinding->handling = false;
      }
      if (!destroyed) {
        return kConstexprStmtInvalid;
      }
      ConstexprRemoveException(ctx, exception);
    }
    return handler_result;
  }
  return kConstexprStmtThrow;
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
      if (stmt_result == kConstexprStmtThrow) {
        if (!DestroyConstexprBindingsFromMark(ctx, mark)) {
          return kConstexprStmtInvalid;
        }
        return kConstexprStmtThrow;
      }
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
      return ConstexprFailureStatementResult(ctx);
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
      return ConstexprFailureStatementResult(ctx);
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
        return ConstexprFailureStatementResult(ctx);
      }
    } else {
      ConstexprValue ignored;
      if (!EvaluateConstexprValue(ctx, loop->c1, loop->c1->type, &ignored)) {
        PopConstexprBindings(ctx, mark);
        return ConstexprFailureStatementResult(ctx);
      }
    }
  }
  for (;;) {
    if (loop->c2 != NULL) {
      bool condition;
      if (!EvaluateConstexprCondition(ctx, loop->c2, &condition)) {
        PopConstexprBindings(ctx, mark);
        return ConstexprFailureStatementResult(ctx);
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
        return ConstexprFailureStatementResult(ctx);
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
    return ConstexprFailureStatementResult(ctx);
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
      return EvaluateConstexprDeclarationList(
                 ctx, (DeclarationListASTNode*)stmt)
                 ? kConstexprStmtNormal
                 : ConstexprFailureStatementResult(ctx);
    case AST_OP(vardecl):
      return EvaluateConstexprVariableDeclaration(
                 ctx, (VariableDeclarationASTNode*)stmt)
                 ? kConstexprStmtNormal
                 : ConstexprFailureStatementResult(ctx);
    case AST_OP(expr): {
      ExpressionStatementASTNode* expr = (ExpressionStatementASTNode*)stmt;
      if (expr->expr == NULL) {
        return kConstexprStmtNormal;
      }
      if (expr->expr->op == AST_OP(call)) {
        ASTNode* receiver = NULL;
        Symbol* callee =
            ConstexprFunctionDefinition(ConstexprCallSymbol(expr->expr));
        if (callee == NULL) {
          callee = ConstexprFunctionDefinition(
              ConstexprMemberCallSymbol(expr->expr, &receiver));
        }
        if (callee == NULL) {
          callee = ConstexprFunctionDefinition(
              ConstexprVirtualCallSymbol(ctx, expr->expr, &receiver));
        }
        if (callee != NULL && callee->type != NULL &&
            TypeIsFunction(callee->type)) {
          bool ok = false;
          if (callee->type->info.function.is_constructor) {
            ok = EvaluateConstexprConstructorCall(ctx, expr->expr);
          } else if (callee->type->info.function.is_destructor) {
            ok = EvaluateConstexprDestructorCall(ctx, expr->expr);
          } else {
            ConstexprValue ignored = {0};
            ok = EvaluateConstexprCall(ctx, expr->expr, &ignored);
          }
          return ok ? kConstexprStmtNormal
                    : ConstexprFailureStatementResult(ctx);
        }
      }
      if (expr->expr->op == AST_OP(throw)) {
        if (!ConstexprEvaluateThrowExpression(ctx, expr->expr)) {
          return ConstexprFailureStatementResult(ctx);
        }
        return kConstexprStmtThrow;
      }
      if (TypeIsVoid(expr->expr->type)) {
        if (!CompilerCXXAtLeast(kLanguageStandardCXX26)) {
          if (ConstexprVoidExpressionThrows(expr->expr)) {
            return kConstexprStmtInvalid;
          }
          return kConstexprStmtNormal;
        }
        if (!EvaluateConstexprVoidExpression(ctx, expr->expr)) {
          return ConstexprFailureStatementResult(ctx);
        }
        return kConstexprStmtNormal;
      }
      ConstexprValue ignored;
      bool ok =
          EvaluateConstexprValue(ctx, expr->expr, expr->expr->type, &ignored);
      return ok ? kConstexprStmtNormal
                : ConstexprFailureStatementResult(ctx);
    }
    case AST_OP(contract_assert): {
      if (compiler->contract_semantic == kContractSemanticIgnore) {
        return kConstexprStmtNormal;
      }
      bool satisfied = false;
      ContractAssertASTNode* assertion = (ContractAssertASTNode*)stmt;
      return EvaluateConstexprCondition(ctx, assertion->predicate,
                                        &satisfied) &&
                     satisfied
                 ? kConstexprStmtNormal
                 : kConstexprStmtInvalid;
    }
    case AST_OP(compound):
      return EvaluateConstexprCompound(ctx, (CompoundStatementASTNode*)stmt,
                                       return_type, result);
    case AST_OP(try):
      if (!CompilerCXXAtLeast(kLanguageStandardCXX26)) {
        // Before C++26 a reached throw-expression is not a core constant
        // expression, even when a handler could catch it.  A non-throwing path
        // through a try block is nevertheless permitted.
        return EvaluateConstexprStatement(
            ctx, ((TryASTNode*)stmt)->try_stmt, return_type, result);
      }
      return EvaluateConstexprTry(ctx, (TryASTNode*)stmt, return_type, result);
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
        return ConstexprFailureStatementResult(ctx);
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
        return ConstexprFailureStatementResult(ctx);
      }
      if (ret->stmt != NULL) {
        ConstexprValue cleanup_value = {0};
        ConstexprStatementResult cleanup = EvaluateConstexprStatement(
            ctx, ret->stmt, return_type, &cleanup_value);
        if (cleanup != kConstexprStmtNormal) {
          return cleanup;
        }
      }
      *result = ConstexprResolveForwardedAddress(return_value);
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

static bool EvaluateConstexprFunctionContracts(
    ConstEvalContext* ctx, TypeRecord* func, ContractAssertionKind kind,
    ConstexprValue* result) {
  if (compiler->contract_semantic == kContractSemanticIgnore) {
    return true;
  }
  Vector* assertions = &func->info.function.contract_assertions;
  for (size_t i = 0; i < assertions->length; i++) {
    ContractAssertion* assertion = assertions->value.p[i];
    if (assertion->kind != kind) {
      continue;
    }
    size_t mark = ctx->bindings.length;
    if (assertion->result_binding != NULL) {
      PushConstexprBinding(ctx, assertion->result_binding, *result);
    }
    bool satisfied = false;
    bool ok = EvaluateConstexprCondition(ctx, assertion->predicate,
                                         &satisfied) &&
              satisfied;
    PopConstexprBindings(ctx, mark);
    if (!ok) {
      return false;
    }
  }
  return true;
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

static Symbol* ConstexprVirtualCallSymbol(ConstEvalContext* ctx, ASTNode* node,
                                          ASTNode** receiver) {
  if (ctx == NULL || node == NULL || node->op != AST_OP(call) ||
      receiver == NULL) {
    return NULL;
  }
  VectorASTNode* call = (VectorASTNode*)node;
  if (call->left == NULL || call->left->op != AST_OP(subscript) ||
      call->children == NULL || call->children->length == 0) {
    return NULL;
  }
  BinaryASTNode* vtable_slot = (BinaryASTNode*)call->left;
  if (vtable_slot->left == NULL ||
      (vtable_slot->left->op != AST_OP(dot) &&
       vtable_slot->left->op != AST_OP(arrow)) ||
      vtable_slot->right == NULL ||
      vtable_slot->right->op != AST_OP(number)) {
    return NULL;
  }
  ASTNode* vptr_node = ((BinaryASTNode*)vtable_slot->left)->right;
  if (vptr_node == NULL || vptr_node->op != AST_OP(structmember)) {
    return NULL;
  }
  StructMember* vptr = ((StructMemberASTNode*)vptr_node)->member;
  if (vptr == NULL || vptr->symbol == NULL ||
      !StringStartsWith(&vptr->symbol->name, "__vptr")) {
    return NULL;
  }
  int64_t virtual_index = ((ConstantASTNode*)vtable_slot->right)->value.ivalue;
  if (virtual_index < 0) {
    return NULL;
  }
  ASTNode* object_actual = call->children->value.p[0];
  ConstexprObject* object = NULL;
  if (!EvaluateConstexprObjectAddress(ctx, object_actual, &object) ||
      object == NULL || object->type == NULL ||
      !TypeIsStructOrUnion(object->type) ||
      object->type->info.struct_info == NULL ||
      (uint64_t)virtual_index >=
          object->type->info.struct_info->virtual_members.length) {
    return NULL;
  }
  StructMember* member =
      object->type->info.struct_info->virtual_members.value.p[virtual_index];
  if (member == NULL || member->symbol == NULL) {
    return NULL;
  }
  *receiver = object_actual;
  return member->symbol;
}

static bool ConstexprExceptionFunction(Symbol* symbol, const char* name) {
  return symbol != NULL && symbol->name.value != NULL &&
         strcmp(symbol->name.value, name) == 0;
}

static ConstexprException* ConstexprFindExceptionHandle(
    ConstEvalContext* ctx, ConstexprValue value) {
  if (value.is_object && value.object != NULL) {
    ConstexprValue* slot = ConstexprObjectSlot(value.object, 0);
    if (slot == NULL) {
      return NULL;
    }
    value = *slot;
  }
  void* token = NULL;
  if (value.is_address && value.heap_block != NULL) {
    token = value.heap_block->memory + value.heap_index;
  } else if (!value.is_address && !value.is_object) {
    token = (void*)(intptr_t)value.ivalue;
  }
  if (token == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < ctx->exception_handles.length; i++) {
    ConstexprException* exception = ctx->exception_handles.value.p[i];
    if (exception != NULL && exception->token == token) {
      return exception;
    }
  }
  return NULL;
}

static bool ConstexprEnsureExceptionToken(ConstEvalContext* ctx,
                                          ConstexprException* exception,
                                          ConstexprValue* value) {
  if (exception->token == NULL) {
    size_t allocation_size = 0;
    exception->token = ConstexprHeapMalloc(ctx, 1, &allocation_size);
    if (exception->token == NULL) {
      return false;
    }
  }
  return ConstexprHeapAddress(ctx, exception->token, 0, value);
}

static bool ConstexprMakeExceptionPtrValue(ConstEvalContext* ctx,
                                           TypeRecord* type,
                                           ConstexprException* exception,
                                           ConstexprValue* result) {
  ConstexprValue pointer = {0};
  if (exception != NULL) {
    if (!ConstexprEnsureExceptionToken(ctx, exception, &pointer)) {
      return false;
    }
    exception->references++;
  } else {
    ConstexprNullAddress(&pointer);
  }
  ConstexprObject* object =
      NewConstexprObject(ctx, type, ConstexprObjectSlotCount(type));
  ConstexprValue* slot = ConstexprObjectSlot(object, 0);
  if (slot == NULL) {
    return false;
  }
  *slot = pointer;
  *result = (ConstexprValue){.is_object = true, .object = object};
  return true;
}

static bool ConstexprExceptionCallArgument(ConstEvalContext* ctx,
                                           ASTNode* node,
                                           ConstexprValue* value) {
  VectorASTNode* call = (VectorASTNode*)node;
  if (call->children == NULL || call->children->length == 0) {
    return false;
  }
  ASTNode* argument = call->children->value.p[0];
  if (argument->op == AST_OP(comma)) {
    ASTNode* construction = ((BinaryASTNode*)argument)->left;
    if (construction != NULL && construction->op == AST_OP(call)) {
      VectorASTNode* constructor_call = (VectorASTNode*)construction;
      Symbol* constructor = ConstexprCallSymbol(construction);
      if (constructor != NULL && constructor->name.value != NULL &&
          strcmp(constructor->name.value, "exception_ptr") == 0 &&
          constructor_call->children != NULL &&
          constructor_call->children->length >= 2) {
        ASTNode* source = constructor_call->children->value.p[
            constructor_call->children->length - 1];
        return EvaluateConstexprValue(ctx, source, source->type, value);
      }
    }
  }
  return EvaluateConstexprValue(ctx, argument, argument->type, value);
}

static bool EvaluateConstexprExceptionCall(ConstEvalContext* ctx,
                                           ASTNode* node, Symbol* symbol,
                                           ConstexprValue* result,
                                           bool* handled) {
  if (compiler->functions_being_analyzed.length != 0 && symbol != NULL &&
      symbol->name.value != NULL &&
      (strstr(symbol->name.value, "exception") != NULL ||
       strstr(symbol->name.value, "uncaught") != NULL)) {
    // These calls depend on the dynamic exception state. Do not fold them
    // while semantically analyzing a function body; evaluate them only when
    // that function is actually invoked as a constant expression.
    *handled = true;
    return false;
  }
  *handled = true;
  if (ConstexprExceptionFunction(symbol, "current_exception")) {
    return ConstexprMakeExceptionPtrValue(ctx, node->type, ctx->exception,
                                          result);
  }
  if (ConstexprExceptionFunction(symbol,
                                 "__davecc_exception_ptr_current")) {
    if (ctx->exception == NULL) {
      return ConstexprNullAddress(result);
    }
    ctx->exception->references++;
    return ConstexprEnsureExceptionToken(ctx, ctx->exception, result);
  }
  if (ConstexprExceptionFunction(symbol, "make_exception_ptr")) {
    VectorASTNode* call = (VectorASTNode*)node;
    if (call->children == NULL || call->children->length == 0) {
      return false;
    }
    ASTNode* argument = call->children->value.p[0];
    ConstexprValue value = {0};
    if (!EvaluateConstexprValue(ctx, argument, argument->type, &value)) {
      return false;
    }
    if (value.is_object) {
      TypeRecord* type = ConstexprExceptionObjectType(argument->type);
      ConstexprObject* copy = NULL;
      if (!ConstexprCopyConstructObject(ctx, type, value.object, &copy,
                                        false)) {
        return false;
      }
      value.object = copy;
    }
    ConstexprException* exception = calloc(1, sizeof(*exception));
    if (exception == NULL) {
      return false;
    }
    exception->type = argument->type;
    exception->value = value;
    exception->throw_node = node;
    exception->throw_location = node->location;
    VectorAppend(&ctx->exception_handles, exception);
    return ConstexprMakeExceptionPtrValue(ctx, node->type, exception, result);
  }
  if (ConstexprExceptionFunction(symbol, "__davecc_exception_ptr_retain") ||
      ConstexprExceptionFunction(symbol, "__davecc_exception_ptr_release")) {
    ConstexprValue argument = {0};
    if (!ConstexprExceptionCallArgument(ctx, node, &argument)) {
      return false;
    }
    ConstexprException* exception =
        ConstexprFindExceptionHandle(ctx, argument);
    if (exception == NULL) {
      // Retaining or releasing a null exception_ptr is a no-op.
      return argument.is_address && argument.heap_block == NULL;
    }
    if (ConstexprExceptionFunction(symbol,
                                   "__davecc_exception_ptr_retain")) {
      exception->references++;
      return true;
    }
    if (exception->references == 0) {
      return false;
    }
    exception->references--;
    if (exception->references == 0 && !exception->active) {
      if (!EvaluateConstexprExceptionDestructor(ctx, exception)) {
        return false;
      }
      ConstexprRemoveException(ctx, exception);
    }
    return true;
  }
  if (ConstexprExceptionFunction(symbol, "rethrow_exception") ||
      ConstexprExceptionFunction(symbol,
                                 "__davecc_exception_ptr_rethrow")) {
    ConstexprValue argument = {0};
    if (!ConstexprExceptionCallArgument(ctx, node, &argument)) {
      return false;
    }
    ConstexprException* exception =
        ConstexprFindExceptionHandle(ctx, argument);
    if (exception == NULL && ctx->exception != NULL) {
      // A nested_exception subobject may have been copied as part of the
      // enclosing exception object. If the copied opaque token cannot be
      // resolved through the subobject slot, use the most recently retained
      // inactive exception rather than keeping it on the active throw chain.
      for (size_t i = ctx->exception_handles.length; i > 0; --i) {
        ConstexprException* candidate =
            ctx->exception_handles.value.p[i - 1];
        if (candidate != NULL && !candidate->active &&
            candidate->references != 0) {
          exception = candidate;
          break;
        }
      }
    }
    if (exception == NULL) {
      return false;
    }
    if (ctx->exception == exception) {
      if (!exception->handling) {
        return false;
      }
      exception->handling = false;
      return false;
    }
    if (exception->active) {
      ConstexprRemoveException(ctx, exception);
      exception->active = true;
    }
    exception->previous = ctx->exception;
    exception->handling = false;
    exception->reported = false;
    exception->active = true;
    ctx->exception = exception;
    return false;
  }
  if (ConstexprExceptionFunction(symbol, "uncaught_exceptions") ||
      ConstexprExceptionFunction(symbol, "__davecc_uncaught_exceptions")) {
    int64_t count = ctx->unwinding_exceptions;
    for (ConstexprException* exception = ctx->exception;
         exception != NULL; exception = exception->previous) {
      if (!exception->handling) {
        count++;
      }
    }
    *result = (ConstexprValue){.ivalue = count};
    return true;
  }
  *handled = false;
  return false;
}

static bool TypeIsBasicStringViewType(TypeRecord* type) {
  if (type == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL) {
    return false;
  }
  Struct* str = type->info.struct_info;
  return FindStructMemberByName(str, "__data") != NULL &&
         FindStructMemberByName(str, "__size") != NULL;
}

static bool ConstexprStringViewDataCharacter(ConstEvalContext* ctx,
                                             ConstexprValue* data_slot,
                                             size_t index,
                                             int64_t* character) {
  if (data_slot == NULL || character == NULL) {
    return false;
  }
  ConstexprValue data = *data_slot;
  ConstexprCanonicalizeAddressValue(&data);
  if (data.is_address && data.address_object != NULL) {
    ConstexprValue* char_slot = ConstexprObjectSlot(
        data.address_object, data.address_index + index);
    return char_slot != NULL &&
           ConstexprValueAsInteger(*char_slot, character);
  }
  if (data.is_address && data.heap_block != NULL &&
      data.heap_block->live) {
    size_t byte_index = data.heap_index + index;
    if (byte_index >= data.heap_block->size) {
      return false;
    }
    *character =
        (int64_t)(signed char)data.heap_block->memory[byte_index];
    return true;
  }
  (void)ctx;
  return false;
}

static ASTNode* ConstexprStringLiteralOperand(ASTNode* node) {
  if (node == NULL) {
    return NULL;
  }
  if (node->op == AST_OP(string)) {
    return node;
  }
  if (node->op == AST_OP(cast)) {
    return ConstexprStringLiteralOperand(((CastASTNode*)node)->expr);
  }
  if (node->op == AST_OP(expr_init)) {
    return ConstexprStringLiteralOperand(
        ((ExpressionInitializerASTNode*)node)->expr);
  }
  if (node->op == AST_OP(comma)) {
    BinaryASTNode* binary = (BinaryASTNode*)node;
    ASTNode* literal = ConstexprStringLiteralOperand(binary->left);
    return literal != NULL ? literal
                           : ConstexprStringLiteralOperand(binary->right);
  }
  if (node->op == AST_OP(call)) {
    VectorASTNode* call = (VectorASTNode*)node;
    ASTNode* literal = ConstexprStringLiteralOperand(call->left);
    if (literal != NULL) {
      return literal;
    }
    if (call->children != NULL) {
      for (size_t i = 0; i < call->children->length; i++) {
        literal = ConstexprStringLiteralOperand(call->children->value.p[i]);
        if (literal != NULL) {
          return literal;
        }
      }
    }
  }
  return NULL;
}

static bool ConstexprStringViewSubscriptChar(ConstEvalContext* ctx,
                                             ASTNode* view, size_t index,
                                             int64_t* character) {
  if (ctx == NULL || view == NULL || character == NULL) {
    return false;
  }
  ASTNode* index_node = NewIntConstantASTNode((int64_t)index, NewSizeTypeRecord(),
                                              view->location);
  BinaryASTNode* subscript = (BinaryASTNode*)NewBinaryASTNode(
      AST_OP(subscript), view->type, view->location, view, index_node);
  ConstexprValue value = {0};
  return EvaluateConstexprObjectAccess(ctx, (ASTNode*)subscript, &value) &&
         ConstexprValueAsInteger(value, character);
}

static bool ConstexprStringViewBytesEqual(ConstEvalContext* ctx,
                                          ASTNode* left_expr,
                                          ConstexprObject* left,
                                          ASTNode* right,
                                          bool* equal) {
  if (left == NULL || right == NULL || equal == NULL) {
    return false;
  }
  ConstexprValue* left_size_slot = ConstexprObjectSlot(left, 1);
  ConstexprValue* left_data_slot = ConstexprObjectSlot(left, 0);
  if (left_size_slot == NULL || left_data_slot == NULL) {
    return false;
  }
  int64_t left_size = 0;
  if (!ConstexprValueAsInteger(*left_size_slot, &left_size)) {
    return false;
  }

  ASTNode* literal = ConstexprStringLiteralOperand(right);
  if (literal != NULL && literal->op == AST_OP(string)) {
    ConstantASTNode* string_node = (ConstantASTNode*)literal;
    const char* text =
        string_node->value.string != NULL ? string_node->value.string->value
                                          : "";
    size_t right_size = text != NULL ? strlen(text) : 0;
    if ((int64_t)right_size != left_size) {
      *equal = false;
      return true;
    }
    for (size_t i = 0; i < right_size; i++) {
      int64_t left_ch = 0;
      bool found =
          ConstexprStringViewDataCharacter(ctx, left_data_slot, i, &left_ch) ||
          ConstexprStringViewSubscriptChar(ctx, left_expr, i, &left_ch);
      if (!found) {
        *equal = false;
        return true;
      }
      if (left_ch != (unsigned char)text[i]) {
        *equal = false;
        return true;
      }
    }
    *equal = true;
    return true;
  }

  ConstexprValue right_value;
  if (EvaluateConstexprObjectAccess(ctx, right, &right_value) &&
      right_value.object != NULL &&
      TypeIsBasicStringViewType(right_value.object->type)) {
    ConstexprValue* right_size_slot = ConstexprObjectSlot(right_value.object, 1);
    ConstexprValue* right_data_slot = ConstexprObjectSlot(right_value.object, 0);
    if (right_size_slot == NULL || right_data_slot == NULL) {
      return false;
    }
    int64_t right_size = 0;
    if (!ConstexprValueAsInteger(*right_size_slot, &right_size)) {
      return false;
    }
    if (left_size != right_size) {
      *equal = false;
      return true;
    }
    for (int64_t i = 0; i < left_size; i++) {
      int64_t left_ch = 0;
      int64_t right_ch = 0;
      if (!ConstexprStringViewDataCharacter(ctx, left_data_slot, (size_t)i,
                                            &left_ch) ||
          !ConstexprStringViewDataCharacter(ctx, right_data_slot, (size_t)i,
                                            &right_ch) ||
          left_ch != right_ch) {
        *equal = false;
        return true;
      }
    }
    *equal = true;
    return true;
  }
  ConstexprValue right_address = {0};
  if (EvaluateConstexprAddressValue(ctx, right, &right_address)) {
    right_address = ConstexprResolveForwardedAddress(right_address);
    ConstexprCanonicalizeAddressValue(&right_address);
    if (right_address.address_object != NULL &&
        right_address.address_object->type != NULL &&
        TypeIsFixedArray(right_address.address_object->type) &&
        TypeIsCharFamily(right_address.address_object->type->next)) {
      for (int64_t i = 0;; i++) {
        int64_t left_ch = 0;
        int64_t right_ch = 0;
        ConstexprValue* right_slot = ConstexprObjectSlot(
            right_address.address_object, right_address.address_index + (size_t)i);
        if (i >= left_size) {
          *equal = right_slot != NULL &&
                   ConstexprValueAsInteger(*right_slot, &right_ch) &&
                   right_ch == 0;
          return true;
        }
        if (right_slot == NULL ||
            !ConstexprValueAsInteger(*right_slot, &right_ch) ||
            !ConstexprStringViewDataCharacter(ctx, left_data_slot, (size_t)i,
                                              &left_ch) ||
            left_ch != right_ch) {
          *equal = false;
          return true;
        }
        if (right_ch == 0) {
          *equal = false;
          return true;
        }
      }
    }
  }
  return false;
}

static ASTNode* ConstexprStringViewExpr(ASTNode* node) {
  for (size_t depth = 0; node != NULL && depth < 64; depth++) {
    if (node->op == AST_OP(cast)) {
      node = ((CastASTNode*)node)->expr;
    } else if (node->op == AST_OP(expr_init)) {
      node = ((ExpressionInitializerASTNode*)node)->expr;
    } else if (node->op == AST_OP(identifier)) {
      Symbol* symbol = ((IdentifierASTNode*)node)->symbol;
      ASTNode* initializer =
          symbol != NULL && symbol->constexpr_initializer != NULL
              ? ConstexprInitializerExpression(symbol->constexpr_initializer)
              : NULL;
      if (initializer == NULL || initializer == node) {
        break;
      }
      node = initializer;
    } else {
      break;
    }
  }
  return node;
}

bool ConstexprEvaluateBasicStringViewEquality(ConstEvalContext* ctx,
                                              ASTNode* left, ASTNode* right,
                                              bool* equal) {
  if (ctx == NULL || left == NULL || right == NULL || equal == NULL) {
    return false;
  }
  left = ConstexprStringViewExpr(left);
  right = ConstexprStringViewExpr(right);
  ASTNode* left_literal = ConstexprStringLiteralOperand(left);
  ASTNode* right_literal = ConstexprStringLiteralOperand(right);
  if (left_literal != NULL && right_literal != NULL) {
    String* left_text = ((ConstantASTNode*)left_literal)->value.string;
    String* right_text = ((ConstantASTNode*)right_literal)->value.string;
    *equal = left_text != NULL && right_text != NULL &&
             left_text->length == right_text->length &&
             memcmp(left_text->value, right_text->value,
                    left_text->length) == 0;
    return true;
  }
  ConstexprValue left_value = {0};
  if (!EvaluateConstexprObjectAccess(ctx, left, &left_value) ||
      left_value.object == NULL ||
      !TypeIsBasicStringViewType(left_value.object->type)) {
    return false;
  }
  return ConstexprStringViewBytesEqual(ctx, left, left_value.object, right,
                                       equal);
}

static bool ConstexprTryEvaluateBasicStringViewEqual(ConstEvalContext* ctx,
                                                     VectorASTNode* call,
                                                     ConstexprValue* result) {
  if (ctx == NULL || call == NULL || result == NULL ||
      call->base.op != AST_OP(call) || call->children == NULL ||
      call->children->length != 2) {
    return false;
  }
  Symbol* callee = ConstexprCallSymbol((ASTNode*)call);
  ConstexprValue probe = {0};
  size_t view_index = 0;
  if (!EvaluateConstexprObjectAccess(ctx, call->children->value.p[0], &probe) ||
      probe.object == NULL ||
      !TypeIsBasicStringViewType(probe.object->type)) {
    probe = (ConstexprValue){0};
    view_index = 1;
    if (!EvaluateConstexprObjectAccess(ctx, call->children->value.p[1],
                                      &probe) ||
        probe.object == NULL ||
        !TypeIsBasicStringViewType(probe.object->type)) {
      return false;
    }
  }
  bool equal = false;
  if (!ConstexprEvaluateBasicStringViewEquality(
          ctx, call->children->value.p[view_index],
          call->children->value.p[1 - view_index],
          &equal)) {
    return false;
  }
  if (callee != NULL && callee->name.value != NULL &&
      strcmp(callee->name.value, "operator!=") == 0) {
    equal = !equal;
  }
  result->ivalue = equal ? 1 : 0;
  result->is_object = false;
  result->is_address = false;
  result->is_floating = false;
  result->fvalue = 0;
  result->object = NULL;
  return true;
}

static bool ConstexprTryEvaluateBasicStringViewCall(ConstEvalContext* ctx,
                                                    ASTNode* node,
                                                    ConstexprValue* result) {
  if (ctx == NULL || node == NULL || node->op != AST_OP(call) ||
      result == NULL) {
    return false;
  }
  VectorASTNode* call = (VectorASTNode*)node;
  if (call->left == NULL || call->left->op != AST_OP(dot)) {
    return false;
  }
  BinaryASTNode* member_access = (BinaryASTNode*)call->left;
  ASTNode* receiver = member_access->left;
  const char* name = NULL;
  if (member_access->right != NULL &&
      member_access->right->op == AST_OP(string)) {
    ConstantASTNode* member_name = (ConstantASTNode*)member_access->right;
    name = member_name->value.string != NULL ? member_name->value.string->value
                                             : NULL;
  } else if (member_access->right != NULL &&
             member_access->right->op == AST_OP(structmember)) {
    StructMember* member = ((StructMemberASTNode*)member_access->right)->member;
    name = member != NULL && member->symbol != NULL
               ? member->symbol->name.value
               : NULL;
  }
  if (receiver == NULL || name == NULL) {
    return false;
  }
  ConstexprValue object_value;
  if (!EvaluateConstexprObjectAccess(ctx, receiver, &object_value) ||
      object_value.object == NULL ||
      !TypeIsBasicStringViewType(object_value.object->type)) {
    return false;
  }
  ConstexprObject* object = object_value.object;
  ConstexprValue* size_slot = ConstexprObjectSlot(object, 1);
  ConstexprValue* data_slot = ConstexprObjectSlot(object, 0);
  if (size_slot == NULL || data_slot == NULL) {
    return false;
  }
  int64_t size = 0;
  if (!ConstexprValueAsInteger(*size_slot, &size)) {
    return false;
  }
  if (strcmp(name, "operator[]") == 0) {
    if (call->children == NULL || call->children->length != 1) {
      return false;
    }
    int64_t index = 0;
    if (!EvaluateIntegerExpressionInContext(ctx, call->children->value.p[0],
                                            &index) ||
        index < 0 || (size_t)index >= (size_t)size) {
      return false;
    }
    if (!ConstexprStringViewDataCharacter(ctx, data_slot, (size_t)index,
                                          &result->ivalue)) {
      return false;
    }
    result->is_object = false;
    result->is_address = false;
    result->is_floating = false;
    result->fvalue = 0;
    result->object = NULL;
    return true;
  }
  if ((strcmp(name, "size") == 0 || strcmp(name, "length") == 0 ||
       strcmp(name, "empty") == 0) &&
      (call->children == NULL || call->children->length == 0)) {
    if (strcmp(name, "empty") == 0) {
      result->ivalue = size == 0 ? 1 : 0;
    } else {
      result->ivalue = size;
    }
    result->is_object = false;
    result->is_address = false;
    result->is_floating = false;
    result->fvalue = 0;
    result->object = NULL;
    return true;
  }
  return false;
}

bool EvaluateConstexprCall(ConstEvalContext* ctx, ASTNode* node,
                                  ConstexprValue* result) {
  if (ctx->call_depth >= CONSTEXPR_MAX_CALL_DEPTH) {
    return false;
  }
  if (ConstexprTryEvaluateBasicStringViewCall(ctx, node, result)) {
    return true;
  }
  if (node->op == AST_OP(call)) {
    if (ConstexprTryEvaluateBasicStringViewEqual(
            ctx, (VectorASTNode*)node, result)) {
      return true;
    }
  }
  Symbol* allocation_symbol = ConstexprCallSymbol(node);
  bool exception_call = false;
  bool exception_result = EvaluateConstexprExceptionCall(
      ctx, node, allocation_symbol, result, &exception_call);
  if (exception_call) {
    return exception_result;
  }
  if (ConstexprIsAllocationFunction(allocation_symbol)) {
    return ConstexprEvaluateAllocationCall(ctx, node, result);
  }
  if (ConstexprIsDeallocationFunction(allocation_symbol)) {
    return ConstexprEvaluateDeallocationCall(ctx, node);
  }
  ASTNode* receiver = NULL;
  bool receiver_is_explicit_actual = false;
  Symbol* call_symbol = allocation_symbol;
  Symbol* callee = ConstexprFunctionDefinition(call_symbol);
  if (callee == NULL) {
    callee =
        ConstexprFunctionDefinition(ConstexprMemberCallSymbol(node, &receiver));
  }
  if (callee == NULL) {
    callee =
        ConstexprFunctionDefinition(ConstexprVirtualCallSymbol(ctx, node,
                                                               &receiver));
    receiver_is_explicit_actual = callee != NULL;
  }
  if (callee == NULL || callee->type == NULL || !TypeIsFunction(callee->type)) {
    return false;
  }
  if (receiver != NULL && callee->type->info.function.is_virtual) {
    ConstexprObject* receiver_object = NULL;
    if (EvaluateConstexprObjectAddress(ctx, receiver, &receiver_object) &&
        receiver_object != NULL) {
      callee = ConstexprResolveVirtualMember(receiver_object, callee);
      if (callee == NULL || callee->type == NULL ||
          !TypeIsFunction(callee->type)) {
        return false;
      }
    }
  }
  TypeRecord* func = callee->type;
  VectorASTNode* call = (VectorASTNode*)node;
  CXXSpecialMemberKind special_member_kind =
      func->info.function.cxx_special_member_kind;
  if (special_member_kind == kCXXSpecialMemberCopyAssignment ||
      special_member_kind == kCXXSpecialMemberMoveAssignment) {
    ASTNode* assignment_actual =
        receiver != NULL
            ? receiver
            : call->children != NULL && call->children->length != 0
                  ? call->children->value.p[0] : NULL;
    ConstexprValue* assigned_slot = NULL;
    bool assignment_lvalue =
        assignment_actual != NULL &&
        EvaluateConstexprObjectLValue(
            ctx, assignment_actual, &assigned_slot, /*allow_object=*/true);
    ConstexprObject* assigned_object = NULL;
    if (!assignment_lvalue && assignment_actual != NULL) {
      ConstexprValue assigned_address = {0};
      bool address_ok = EvaluateConstexprAddressValue(
          ctx, assignment_actual, &assigned_address);
      if (address_ok) {
        assigned_slot = assigned_address.address_slot;
        assigned_object =
            ConstexprAddressTargetObject(ctx, assigned_address);
        if (assigned_slot == NULL &&
            assigned_address.address_object != NULL) {
          assigned_slot = ConstexprObjectSlot(
              assigned_address.address_object,
              assigned_address.address_index);
          if (assigned_slot != NULL && assigned_slot->is_object) {
            assigned_object = assigned_slot->object;
          }
        }
        if (assigned_slot == NULL) {
          assigned_slot =
              ConstexprSlotOwningObject(ctx, assigned_object);
        }
        assignment_lvalue =
            assigned_slot != NULL || assigned_object != NULL;
      }
    }
    if (assignment_lvalue && assigned_slot != NULL) {
      assigned_slot->lifetime_ended = false;
      if (assigned_slot->is_object && assigned_slot->object != NULL) {
        assigned_slot->object->lifetime_ended = false;
      }
    }
    if (assignment_lvalue && assigned_object != NULL) {
      assigned_object->lifetime_ended = false;
    }
  }
  if (callee->name.value != NULL &&
      strcmp(callee->name.value, "construct_at") == 0 &&
      call->children != NULL && call->children->length == 2 &&
      func->next != NULL && TypeIsPointer(func->next) &&
      func->next->next != NULL) {
    TypeRecord* target_type = func->next->next;
    ConstexprValue address = {0};
    ConstexprValue constructed = {0};
    bool address_ok = EvaluateConstexprAddressValue(
        ctx, call->children->value.p[0], &address);
    ASTNode* argument = call->children->value.p[1];
    TypeRecord* argument_type = argument != NULL ? argument->type : NULL;
    while (TypeIsReference(argument_type)) {
      argument_type = argument_type->next;
    }
    bool same_class_type =
        TypeIsStructOrUnion(argument_type) &&
        TypeIsStructOrUnion(target_type) &&
        argument_type->info.struct_info == target_type->info.struct_info;
    bool constructed_ok =
        address_ok &&
        (TypeIsStructOrUnion(target_type) &&
                 !same_class_type
             ? EvaluateConstexprConvertingConstruction(
                   ctx, call->children->value.p[1], target_type,
                   /*allow_explicit=*/true, &constructed)
             : EvaluateConstexprValue(ctx, call->children->value.p[1],
                                      target_type, &constructed));
    if (!address_ok || !constructed_ok) {
      return false;
    }
    ConstexprValue* slot = address.address_slot;
    if (slot == NULL && address.address_object != NULL) {
      slot = ConstexprObjectSlot(address.address_object,
                                 address.address_index);
    }
    if (slot == NULL && address.address_binding != NULL &&
        StoreConstexprBinding(ctx, address.address_binding, target_type,
                              constructed)) {
      if (address.address_binding->object != NULL) {
        address.address_binding->object->lifetime_ended = false;
      }
      *result = address;
      return true;
    }
    if (slot == NULL ||
        !StoreConstexprSlot(ctx, slot, target_type, constructed)) {
      return false;
    }
    slot->lifetime_ended = false;
    *result = address;
    return true;
  }
  if (!func->info.function.is_constexpr ||
      func->info.function.body == NULL ||
      func->info.function.is_constructor ||
      func->info.function.is_destructor ||
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

  size_t mark = ctx->bindings.length;
  ctx->call_depth++;
  if (call->left != NULL && call->left->op == AST_OP(comma)) {
    ConstexprValue discarded;
    if (!EvaluateConstexprValue(
            ctx, ((BinaryASTNode*)call->left)->left,
            ((BinaryASTNode*)call->left)->left->type, &discarded)) {
      ctx->call_depth--;
      return false;
    }
  }
  // A member call binds the receiver to the implicit `this` parameter and the
  // explicit arguments to the remaining parameters; an ordinary call binds the
  // arguments positionally.  BindConstexprConstructorActuals implements exactly
  // the former (it is not constructor specific despite its name).
  bool bound =
      receiver != NULL && !receiver_is_explicit_actual
          ? BindConstexprConstructorActuals(ctx, callee, receiver,
                                            call->children)
          : BindConstexprActuals(ctx, callee, call->children);
  bool pre =
      bound && EvaluateConstexprFunctionContracts(
                   ctx, func, kContractPrecondition, result);
  bool tracks_destroyed_lifetime =
      callee->name.value != NULL &&
      strcmp(callee->name.value, "destroy_at") == 0;
  ConstexprValue destroyed_address = {0};
  bool has_destroyed_address = false;
  if (tracks_destroyed_lifetime && bound &&
      func->info.function.prototype.length != 0) {
    Symbol* location_formal = func->info.function.prototype.value.p[0];
    ConstexprBinding* location =
        FindConstexprBinding(ctx, location_formal);
    if (location != NULL && location->is_address) {
      destroyed_address = (ConstexprValue){
          .is_address = true,
          .address_binding = location->address_binding,
          .address_slot = location->address_slot,
          .address_object = location->address_object,
          .address_index = location->address_index,
          .heap_block = location->heap_block,
          .heap_index = location->heap_index,
      };
      ConstexprCanonicalizeAddressValue(&destroyed_address);
      has_destroyed_address = true;
    }
  }
  if (tracks_destroyed_lifetime) {
    ctx->destroy_at_depth++;
  }
  bool body = pre && EvaluateConstexprFunctionBody(ctx, func, result);
  if (tracks_destroyed_lifetime) {
    ctx->destroy_at_depth--;
  }
  bool post =
      body && EvaluateConstexprFunctionContracts(
                  ctx, func, kContractPostcondition, result);
  if (body && has_destroyed_address) {
    ConstexprObject* destroyed_object =
        ConstexprAddressTargetObject(ctx, destroyed_address);
    if (destroyed_object != NULL) {
      destroyed_object->lifetime_ended = true;
    }
    ConstexprValue* destroyed_slot = destroyed_address.address_slot;
    if (destroyed_slot == NULL) {
      destroyed_slot = ConstexprSlotOwningObject(ctx, destroyed_object);
    }
    if (destroyed_slot != NULL) {
      destroyed_slot->lifetime_ended = true;
    }
  }
  bool ok = bound && pre && body && post;
  ctx->call_depth--;
  PopConstexprBindings(ctx, mark);
  return ok;
}

static bool ConstexprIsExceptionPropagationCall(ASTNode* node) {
  Symbol* symbol = ConstexprCallSymbol(node);
  if (symbol == NULL || symbol->name.value == NULL) {
    return false;
  }
  const char* name = symbol->name.value;
  return strncmp(name, "__davecc_exception_ptr_", 23) == 0 ||
         strcmp(name, "__davecc_uncaught_exceptions") == 0 ||
         strcmp(name, "current_exception") == 0 ||
         strcmp(name, "rethrow_exception") == 0 ||
         strcmp(name, "make_exception_ptr") == 0 ||
         strcmp(name, "uncaught_exceptions") == 0 ||
         strcmp(name, "uncaught_exception") == 0;
}

bool ConstexprEvaluateCallAsInteger(ConstEvalContext* ctx, ASTNode* node,
                                    int64_t* result) {
  (void)ConstexprFunctionDefinition(ConstexprCallSymbol(node));
  ConstexprEvalMode mode = compiler->constexpr_eval_mode;
  ConstexprPCodeCapability capability =
      ConstexprPCodeCapabilityForExpression(node);
  bool use_overlay_result = false;
  int64_t overlay_result = 0;
  if ((mode == kConstexprEvalPCode || mode == kConstexprEvalAuto ||
       mode == kConstexprEvalAudit) &&
      capability != kConstexprPCodeEligible) {
    compiler->constexpr_eval_mode = kConstexprEvalAST;
    ConstexprValue overlay_value;
    bool overlay_ok = EvaluateConstexprCall(ctx, node, &overlay_value) &&
                      ConstexprValueAsInteger(overlay_value, &overlay_result);
    compiler->constexpr_eval_mode = mode;
    if (!overlay_ok) {
      ReportUncaughtConstexprException(ctx);
      return false;
    }
    use_overlay_result = true;
  }
  if (capability == kConstexprPCodeASTOnly && use_overlay_result) {
    *result = overlay_result;
    return true;
  }
  int64_t pcode_result = 0;
  if (mode == kConstexprEvalAuto) {
    compiler->constexpr_eval_mode = kConstexprEvalPCode;
  }
  bool pcode_ok = mode != kConstexprEvalAST &&
                  ConstexprPCodeEvaluateCallAsInteger(ctx, node, &pcode_result);
  compiler->constexpr_eval_mode = mode;
  if (mode == kConstexprEvalPCode) {
    if (!pcode_ok) {
      ReportConstexprPCodeFailure(ctx, node, false);
      return false;
    }
    *result = use_overlay_result ? overlay_result : pcode_result;
    return true;
  }
  if (mode == kConstexprEvalAuto && pcode_ok) {
    if (node->op == AST_OP(call)) {
      VectorASTNode* call = (VectorASTNode*)node;
      ConstexprValue probe = {0};
      if (call->children != NULL && call->children->length == 2 &&
          EvaluateConstexprObjectAccess(ctx, call->children->value.p[0], &probe) &&
          probe.object != NULL &&
          TypeIsBasicStringViewType(probe.object->type)) {
        ConstexprEvalMode saved = compiler->constexpr_eval_mode;
        compiler->constexpr_eval_mode = kConstexprEvalAST;
        ConstexprValue ast_value = {0};
        bool ast_ok = EvaluateConstexprCall(ctx, node, &ast_value) &&
                      ConstexprValueAsInteger(ast_value, result);
        compiler->constexpr_eval_mode = saved;
        if (ast_ok) {
          return true;
        }
      }
    }
    *result = use_overlay_result ? overlay_result : pcode_result;
    return true;
  }
  if (mode == kConstexprEvalAudit && use_overlay_result) {
    if (!pcode_ok || pcode_result != overlay_result) {
      if (pcode_ok) {
        SemanticError(node,
                      "constexpr evaluator mismatch: pcode=%lld, ast=%lld",
                      (long long)pcode_result, (long long)overlay_result);
      } else {
        SemanticError(node, "constexpr evaluator mismatch: pcode failed: %s",
                      ConstexprPCodeFailureReason(ctx));
      }
      return false;
    }
    *result = overlay_result;
    return true;
  }
  if (mode == kConstexprEvalAuto && !use_overlay_result && !pcode_ok &&
      ctx->pcode_failure_kind == kConstexprPCodeFailureInvalid) {
    ReportConstexprPCodeFailure(ctx, node, false);
    return false;
  }
  if (mode == kConstexprEvalAudit) {
    compiler->constexpr_eval_mode = kConstexprEvalAST;
  }
  ConstexprValue value;
  bool ast_ok = EvaluateConstexprCall(ctx, node, &value) &&
                ConstexprValueAsInteger(value, result);
  compiler->constexpr_eval_mode = mode;
  if (mode == kConstexprEvalAudit &&
      ctx->call_depth == 0 &&
      !ConstexprIsExceptionPropagationCall(node) &&
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
                    ConstexprPCodeFailureReason(ctx));
    }
    return false;
  }
  if (pcode_ok) {
    *result = pcode_result;
    return true;
  }
  if (!ast_ok) {
    ReportUncaughtConstexprException(ctx);
  }
  return ast_ok;
}

bool ConstexprEvaluateCallAsFloating(ConstEvalContext* ctx, ASTNode* node,
                                     double* result) {
  (void)ConstexprFunctionDefinition(ConstexprCallSymbol(node));
  ConstexprEvalMode mode = compiler->constexpr_eval_mode;
  ConstexprPCodeCapability capability =
      ConstexprPCodeCapabilityForExpression(node);
  bool use_overlay_result = false;
  double overlay_result = 0;
  if ((mode == kConstexprEvalPCode || mode == kConstexprEvalAuto ||
       mode == kConstexprEvalAudit) &&
      capability != kConstexprPCodeEligible) {
    compiler->constexpr_eval_mode = kConstexprEvalAST;
    ConstexprValue overlay_value;
    bool overlay_ok = EvaluateConstexprCall(ctx, node, &overlay_value) &&
                      ConstexprValueAsFloating(overlay_value, &overlay_result);
    compiler->constexpr_eval_mode = mode;
    if (!overlay_ok) {
      ReportUncaughtConstexprException(ctx);
      return false;
    }
    use_overlay_result = true;
  }
  if (capability == kConstexprPCodeASTOnly && use_overlay_result) {
    *result = overlay_result;
    return true;
  }
  double pcode_result = 0;
  if (mode == kConstexprEvalAuto) {
    compiler->constexpr_eval_mode = kConstexprEvalPCode;
  }
  bool pcode_ok = mode != kConstexprEvalAST &&
                  ConstexprPCodeEvaluateCallAsFloating(ctx, node, &pcode_result);
  compiler->constexpr_eval_mode = mode;
  if (mode == kConstexprEvalPCode) {
    if (!pcode_ok) {
      ReportConstexprPCodeFailure(ctx, node, false);
      return false;
    }
    *result = use_overlay_result ? overlay_result : pcode_result;
    return true;
  }
  if (mode == kConstexprEvalAuto && pcode_ok) {
    *result = use_overlay_result ? overlay_result : pcode_result;
    return true;
  }
  if (mode == kConstexprEvalAudit && use_overlay_result) {
    if (!pcode_ok ||
        memcmp(&pcode_result, &overlay_result, sizeof(pcode_result)) != 0) {
      SemanticError(node, "constexpr evaluator mismatch%s",
                    pcode_ok ? "" : ": pcode evaluation failed");
      return false;
    }
    *result = overlay_result;
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
  if (!ast_ok) {
    ReportUncaughtConstexprException(ctx);
  }
  return ast_ok;
}

bool ConstexprEvaluateCallAsObject(ConstEvalContext* ctx, ASTNode* node) {
  (void)ConstexprFunctionDefinition(ConstexprCallSymbol(node));
  ConstexprEvalMode mode = compiler->constexpr_eval_mode;
  ConstexprPCodeCapability capability =
      ConstexprPCodeCapabilityForExpression(node);
  if ((mode == kConstexprEvalPCode || mode == kConstexprEvalAuto ||
       mode == kConstexprEvalAudit) &&
      capability != kConstexprPCodeEligible) {
    compiler->constexpr_eval_mode = kConstexprEvalAST;
    ConstexprValue overlay_value;
    bool overlay_ok = EvaluateConstexprCall(ctx, node, &overlay_value) &&
                      overlay_value.is_object &&
                      overlay_value.object != NULL;
    compiler->constexpr_eval_mode = mode;
    if (!overlay_ok) {
      ReportUncaughtConstexprException(ctx);
      return false;
    }
    if (capability == kConstexprPCodeASTOnly) {
      return true;
    }
  }
  if (mode == kConstexprEvalAuto) {
    compiler->constexpr_eval_mode = kConstexprEvalPCode;
  }
  bool pcode_ok =
      mode != kConstexprEvalAST &&
      ConstexprPCodeEvaluateCallAsObject(ctx, node);
  compiler->constexpr_eval_mode = mode;
  if (mode == kConstexprEvalPCode) {
    if (!pcode_ok) {
      ReportConstexprPCodeFailure(ctx, node, false);
    }
    return pcode_ok;
  }
  if (mode == kConstexprEvalAuto && pcode_ok) {
    return true;
  }
  if (mode == kConstexprEvalAuto && !pcode_ok && node->type != NULL &&
      TypeIsStructOrUnion(node->type) && node->type->info.struct_info != NULL &&
      node->type->info.struct_info->tag_name != NULL &&
      node->type->info.struct_info->tag_name->value != NULL &&
      strcmp(node->type->info.struct_info->tag_name->value,
             "exception_ptr") == 0) {
    // A non-null exception_ptr cannot outlive the constant-evaluation
    // context. Preserve the p-code escape diagnosis instead of accepting an
    // AST fallback that would contain an evaluator-owned opaque token.
    return false;
  }
  if (mode == kConstexprEvalAudit) {
    compiler->constexpr_eval_mode = kConstexprEvalAST;
  }
  ConstexprValue value;
  bool ast_ok = EvaluateConstexprCall(ctx, node, &value) && value.is_object &&
                value.object != NULL;
  compiler->constexpr_eval_mode = mode;
  if (mode == kConstexprEvalAudit &&
      ctx->call_depth == 0 &&
      !ConstexprIsExceptionPropagationCall(node) && pcode_ok != ast_ok) {
    SemanticError(node,
                  pcode_ok
                      ? "constexpr evaluator mismatch"
                      : "constexpr evaluator mismatch: pcode failed: %s",
                  ConstexprPCodeFailureReason(ctx));
    return false;
  }
  if (!pcode_ok && !ast_ok) {
    ReportUncaughtConstexprException(ctx);
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
  ConstexprPCodeCapability capability =
      ConstexprPCodeCapabilityForExpression(node);
  if ((mode == kConstexprEvalPCode || mode == kConstexprEvalAuto ||
       mode == kConstexprEvalAudit) &&
      capability != kConstexprPCodeEligible) {
    compiler->constexpr_eval_mode = kConstexprEvalAST;
    ConstexprValue overlay_value = {0};
    bool overlay_ok = EvaluateConstexprCall(ctx, node, &overlay_value);
    compiler->constexpr_eval_mode = mode;
    if (!overlay_ok) {
      ReportUncaughtConstexprException(ctx);
      return false;
    }
    if (capability == kConstexprPCodeASTOnly) {
      return true;
    }
  }
  bool pcode_ok =
      (mode == kConstexprEvalPCode || mode == kConstexprEvalAudit) &&
      ConstexprPCodeEvaluateCall(ctx, node);
  if (mode == kConstexprEvalPCode) {
    if (!pcode_ok) {
      ReportConstexprPCodeFailure(ctx, node, false);
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
  if (mode == kConstexprEvalAudit &&
      ctx->call_depth == 0 &&
      !ConstexprIsExceptionPropagationCall(node) && pcode_ok != ast_ok) {
    SemanticError(node,
                  pcode_ok
                      ? "constexpr evaluator mismatch"
                      : "constexpr evaluator mismatch: pcode failed: %s",
                  ConstexprPCodeFailureReason(ctx));
    return false;
  }
  if (!ast_ok) {
    ReportUncaughtConstexprException(ctx);
  }
  return ast_ok;
}

static bool EvaluateConstexprDestructorCall(ConstEvalContext* ctx,
                                            ASTNode* node) {
  if (ctx->call_depth > 32) {
    return false;
  }
  ASTNode* receiver = NULL;
  bool receiver_is_explicit_actual = false;
  Symbol* callee = ConstexprFunctionDefinition(ConstexprCallSymbol(node));
  if (callee == NULL) {
    callee = ConstexprFunctionDefinition(
        ConstexprMemberCallSymbol(node, &receiver));
  }
  if (callee == NULL) {
    callee =
        ConstexprFunctionDefinition(ConstexprVirtualCallSymbol(ctx, node,
                                                               &receiver));
    receiver_is_explicit_actual = callee != NULL;
  }
  if (callee == NULL || callee->type == NULL || !TypeIsFunction(callee->type)) {
    return false;
  }
  TypeRecord* func = callee->type;
  VectorASTNode* call = (VectorASTNode*)node;
  ASTNode* object_actual =
      receiver != NULL
          ? receiver
          : call->children != NULL && call->children->length > 0
                ? call->children->value.p[0] : NULL;
  ConstexprObject* receiver_object = NULL;
  ConstexprValue* receiver_slot = NULL;
  if (object_actual != NULL) {
    (void)EvaluateConstexprObjectAddress(
        ctx, object_actual, &receiver_object);
    (void)EvaluateConstexprObjectLValue(
        ctx, object_actual, &receiver_slot, /*allow_object=*/true);
  }
  if (func->info.function.is_destructor &&
      func->info.function.body == NULL &&
      func->info.function.is_trivial_special_member) {
    if (ctx->destroy_at_depth > 0) {
      if (receiver_object != NULL) {
        receiver_object->lifetime_ended = true;
      }
      if (receiver_slot == NULL) {
        receiver_slot = ConstexprSlotOwningObject(ctx, receiver_object);
      }
      if (receiver_slot != NULL) {
        receiver_slot->lifetime_ended = true;
      }
    }
    return true;
  }
  if (receiver != NULL && func->info.function.is_virtual) {
    ConstexprObject* receiver_object = NULL;
    if (EvaluateConstexprObjectAddress(ctx, receiver, &receiver_object) &&
        receiver_object != NULL) {
      callee = ConstexprResolveVirtualMember(receiver_object, callee);
      if (callee == NULL || callee->type == NULL ||
          !TypeIsFunction(callee->type)) {
        return false;
      }
      func = callee->type;
    }
  }
  if (!func->info.function.is_constexpr ||
      func->info.function.body == NULL ||
      !func->info.function.is_destructor ||
      func->info.function.is_constructor ||
      func->info.function.varargs) {
    return false;
  }

  Symbol* this_formal =
      func->info.function.prototype.length > 0
          ? func->info.function.prototype.value.p[0] : NULL;
  TypeRecord* formal_object_type =
      this_formal != NULL && this_formal->type != NULL &&
              TypeIsPointer(this_formal->type)
          ? this_formal->type->next : NULL;
  if (object_actual != NULL && formal_object_type != NULL &&
      EvaluateConstexprObjectAddress(ctx, object_actual, &receiver_object) &&
      receiver_object != NULL &&
      !TypeEqual(receiver_object->type, formal_object_type) &&
      !TypeIsDerivedFrom(receiver_object->type, formal_object_type)) {
    // Template cleanup synthesis can retain a destructor symbol from the
    // enclosing class while targeting a member subobject. Such a mismatched
    // cleanup is not a call to that enclosing destructor.
    return true;
  }
  size_t mark = ctx->bindings.length;
  ctx->call_depth++;
  ConstexprValue ignored = {0};
  bool ok = (receiver != NULL && !receiver_is_explicit_actual
                 ? BindConstexprConstructorActuals(ctx, callee, receiver,
                                                   call->children)
                 : BindConstexprActuals(ctx, callee, call->children)) &&
            EvaluateConstexprFunctionContracts(
                ctx, func, kContractPrecondition, &ignored) &&
            EvaluateConstexprStatement(ctx, func->info.function.body,
                                       func->next, &ignored) ==
                kConstexprStmtNormal &&
            EvaluateConstexprFunctionContracts(
                ctx, func, kContractPostcondition, &ignored);
  ctx->call_depth--;
  PopConstexprBindings(ctx, mark);
  if (ok && receiver_object != NULL && ctx->destroy_at_depth > 0) {
    receiver_object->lifetime_ended = true;
  }
  if (ok && receiver_slot == NULL && ctx->destroy_at_depth > 0) {
    receiver_slot = ConstexprSlotOwningObject(ctx, receiver_object);
  }
  if (ok && receiver_slot != NULL && ctx->destroy_at_depth > 0) {
    receiver_slot->lifetime_ended = true;
  }
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
  if (func->info.function.is_constructor &&
      func->info.function.body == NULL &&
      func->info.function.is_trivial_special_member) {
    return true;
  }
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
  bool bound =
      BindConstexprConstructorActuals(ctx, callee, receiver, call->children);
  bool pre = bound && EvaluateConstexprFunctionContracts(
                          ctx, func, kContractPrecondition, &ignored);
  ConstexprStatementResult body_result =
      pre ? EvaluateConstexprStatement(ctx, func->info.function.body,
                                       func->next, &ignored)
          : kConstexprStmtInvalid;
  bool post = body_result == kConstexprStmtNormal &&
              EvaluateConstexprFunctionContracts(
                  ctx, func, kContractPostcondition, &ignored);
  bool ok = bound && pre && body_result == kConstexprStmtNormal && post;
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
  if (func->info.function.is_constructor &&
      func->info.function.body == NULL &&
      func->info.function.is_trivial_special_member) {
    if (TypeIsStructOrUnion(object->type) &&
        object->type->info.struct_info != NULL &&
        object->type->info.struct_info->is_union) {
      object->active_union_member = NULL;
    }
    return true;
  }
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
            EvaluateConstexprFunctionContracts(
                ctx, func, kContractPrecondition, &ignored) &&
            EvaluateConstexprStatement(ctx, func->info.function.body,
                                       func->next, &ignored) ==
                kConstexprStmtNormal &&
            EvaluateConstexprFunctionContracts(
                ctx, func, kContractPostcondition, &ignored);
  ctx->call_depth--;
  PopConstexprBindings(ctx, mark);
  return ok;
}
