//
//  sroa.c
//  c_compiler
//
//  Scalar replacement of small local aggregates.
//

#include "sroa.h"

#include "compiler.h"
#include "ir.h"
#include "syntax.h"
#include "type_compare.h"
#include "type_core.h"
#include "vector.h"

#include <stdint.h>
#include <stdlib.h>

enum {
  kMaxSroaObjectSize = 64,
  kMaxSroaSlices = 8,
};

typedef struct {
  int64_t offset;
  uint32_t size;
  TypeRecord* type;
  IRNode* scalar;
} SroaSlice;

static uint32_t AccessSize(IRNode* inst) {
  switch (inst->opcode) {
    case IR_OP(load8):
    case IR_OP(loadu8):
    case IR_OP(store8):
      return 1;
    case IR_OP(load16):
    case IR_OP(loadu16):
    case IR_OP(store16):
      return 2;
    case IR_OP(load32):
    case IR_OP(loadu32):
    case IR_OP(store32):
    case IR_OP(loadf):
    case IR_OP(storef):
      return 4;
    case IR_OP(load64):
    case IR_OP(store64):
    case IR_OP(loadd):
    case IR_OP(stored):
    case IR_OP(loada):
    case IR_OP(storea):
      return 8;
    default:
      break;
  }
  if (inst->type != NULL && inst->type->size > 0 &&
      inst->type->size <= UINT32_MAX) {
    return (uint32_t)inst->type->size;
  }
  return 0;
}

static bool IsPromotableAggregate(TypeRecord* type) {
  if (type == NULL || TypeIsVolatile(type) || TypeIsAtomic(type) ||
      TypeIsVLA(type) || type->size <= 0 ||
      type->size > kMaxSroaObjectSize) {
    return false;
  }
  if (TypeIsFixedArray(type)) {
    TypeRecord* element = type->next;
    return element != NULL && TypeIsScalar(element) &&
           !TypeIsVolatile(element) && !TypeIsAtomic(element);
  }
  if (!TypeIsStructOrUnion(type) || (type->type & kTypeUnion) != 0) {
    return false;
  }
  Struct* info = type->info.struct_info;
  if (info == NULL || info->bases.length != 0 || info->vptr_member != NULL) {
    return false;
  }
  return true;
}

static bool IsEligibleVariable(IRNode* node) {
  if (node == NULL || !IRIsVariable(node) || node->type == NULL) {
    return false;
  }
  switch (node->opcode) {
    case IR_OP(localvar):
    case IR_OP(tempvar):
      break;
    default:
      return false;
  }
  Symbol* symbol = ((IRVariable*)node)->symbol;
  if (symbol == NULL || symbol->flags.is_argument ||
      StorageIs(symbol->storage, STO(static) | STO(extern) | STO(thread))) {
    return false;
  }
  return IsPromotableAggregate(node->type);
}

static bool MatchConstOffsetAdda(IRNode* adda, IRNode* base, int64_t* offset) {
  if (adda == NULL || adda->opcode != IR_OP(adda) || adda->inputs.length != 2) {
    return false;
  }
  IRNode* lhs = adda->inputs.value.p[0];
  IRNode* rhs = adda->inputs.value.p[1];
  if (lhs == base && IRIsIntConst(rhs)) {
    *offset = IRIntConstValue(rhs);
    return true;
  }
  if (rhs == base && IRIsIntConst(lhs)) {
    *offset = IRIntConstValue(lhs);
    return true;
  }
  return false;
}

static bool AccessesOverlap(const SroaSlice* a, int64_t offset, uint32_t size) {
  if (a->offset == offset && a->size == size) {
    return false;
  }
  return a->offset < offset + (int64_t)size &&
         offset < a->offset + (int64_t)a->size;
}

static SroaSlice* FindSlice(SroaSlice* slices, size_t n, int64_t offset,
                            uint32_t size) {
  for (size_t i = 0; i < n; i++) {
    if (slices[i].offset == offset && slices[i].size == size) {
      return &slices[i];
    }
  }
  return NULL;
}

static bool CollectSlices(IRNode* var, SroaSlice* slices, size_t* n_slices) {
  int64_t object_size = var->type->size;
  *n_slices = 0;
  for (size_t i = 0; i < var->outputs.length; i++) {
    IRNode* adda = var->outputs.value.p[i];
    int64_t offset = 0;
    if (!MatchConstOffsetAdda(adda, var, &offset)) {
      return false;
    }
    if (offset < 0) {
      return false;
    }
    for (size_t u = 0; u < adda->outputs.length; u++) {
      IRNode* access = adda->outputs.value.p[u];
      if (access->inputs.length == 0 || access->inputs.value.p[0] != adda ||
          (!IRIsLoadOnly(access) && !IRIsStoreOnly(access)) ||
          access->dest != NULL) {
        return false;
      }
      uint32_t size = AccessSize(access);
      if (size == 0 || offset + (int64_t)size > object_size) {
        return false;
      }
      for (size_t s = 0; s < *n_slices; s++) {
        if (AccessesOverlap(&slices[s], offset, size)) {
          return false;
        }
      }
      SroaSlice* existing = FindSlice(slices, *n_slices, offset, size);
      if (existing != NULL) {
        if (existing->type == NULL && access->type != NULL) {
          existing->type = access->type;
        }
        continue;
      }
      if (*n_slices >= kMaxSroaSlices) {
        return false;
      }
      slices[*n_slices].offset = offset;
      slices[*n_slices].size = size;
      slices[*n_slices].type = access->type;
      slices[*n_slices].scalar = NULL;
      (*n_slices)++;
    }
  }
  return *n_slices > 0;
}

static TypeRecord* SliceType(const SroaSlice* slice) {
  if (slice->type != NULL) {
    return slice->type;
  }
  switch (slice->size) {
    case 1:
      return NewTypeRecordWithSize(kTypeChar, kQualPlain);
    case 2:
      return NewTypeRecordWithSize(kTypeShort, kQualPlain);
    case 8:
      return NewTypeRecordWithSize(kTypeLongLong, kQualPlain);
    default:
      return NewTypeRecordWithSize(kTypeInt, kQualPlain);
  }
}

static bool PromoteVariable(Generator* gen, IRNode* var) {
  SroaSlice slices[kMaxSroaSlices];
  size_t n_slices = 0;
  if (!CollectSlices(var, slices, &n_slices)) {
    return false;
  }

  for (size_t i = 0; i < n_slices; i++) {
    Symbol* symbol = SyntaxNewTemporary(gen->syntax, SliceType(&slices[i]));
    symbol->flags.address_taken = false;
    slices[i].scalar = GeneratorGetVariable(gen, symbol);
  }

  Vector addas;
  VectorInit(&addas);
  for (size_t i = 0; i < var->outputs.length; i++) {
    VectorAppend(&addas, var->outputs.value.p[i]);
  }

  for (size_t i = 0; i < addas.length; i++) {
    IRNode* adda = addas.value.p[i];
    int64_t offset = 0;
    if (!MatchConstOffsetAdda(adda, var, &offset)) {
      continue;
    }
    Vector accesses;
    VectorInit(&accesses);
    for (size_t u = 0; u < adda->outputs.length; u++) {
      VectorAppend(&accesses, adda->outputs.value.p[u]);
    }
    for (size_t u = 0; u < accesses.length; u++) {
      IRNode* access = accesses.value.p[u];
      uint32_t size = AccessSize(access);
      SroaSlice* slice = FindSlice(slices, n_slices, offset, size);
      if (slice == NULL || slice->scalar == NULL) {
        continue;
      }
      IRReplaceInput(access, 0, slice->scalar);
      Symbol* scalar_sym = ((IRVariable*)slice->scalar)->symbol;
      if (IRIsVarDef(access)) {
        access->var.def = scalar_sym;
      }
      if (IRIsVarRef(access)) {
        access->var.use = scalar_sym;
      }
    }
    VectorDestruct(&accesses);
    if (adda->outputs.length == 0) {
      GeneratorRemoveInstruction(gen, adda);
    }
  }
  VectorDestruct(&addas);
  return true;
}

void ScalarReplacementOptimization(Generator* gen) {
  if (!OptLevel2()) {
    return;
  }
  Vector vars;
  VectorInit(&vars);
  for (size_t i = 0; i < gen->variable_pool.length; i++) {
    PoolEntry* entry = gen->variable_pool.value.p[i];
    if (entry != NULL && IsEligibleVariable(entry->pooled)) {
      VectorAppend(&vars, entry->pooled);
    }
  }
  for (size_t i = 0; i < vars.length; i++) {
    PromoteVariable(gen, vars.value.p[i]);
  }
  VectorDestruct(&vars);
}
