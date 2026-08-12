//
//  alias.c
//  c_compiler
//
//  Conservative, object-based IR alias analysis.
//

#include "alias.h"

#include <limits.h>
#include <string.h>

#include "loop_info.h"

static bool TypeContainsVolatile(TypeRecord* type) {
  for (; type != NULL; type = type->next) {
    if (TypeIsVolatile(type)) {
      return true;
    }
  }
  return false;
}

static Symbol* VariableSymbol(IRNode* node) {
  switch (node->opcode) {
    case IR_OP(localvar):
    case IR_OP(argument):
    case IR_OP(tempvar):
    case IR_OP(staticvar):
    case IR_OP(externvar):
    case IR_OP(ssavar):
    case IR_OP(phi):
      return ((IRVariable*)node)->symbol;
    default:
      return NULL;
  }
}

static IRMemoryRootKind RootKind(IRNode* node) {
  switch (node->opcode) {
    case IR_OP(localvar):
      return kIRMemoryLocal;
    case IR_OP(argument):
      return kIRMemoryArgument;
    case IR_OP(tempvar):
      return kIRMemoryTemporary;
    case IR_OP(staticvar):
      return kIRMemoryStatic;
    case IR_OP(externvar):
      return kIRMemoryExtern;
    case IR_OP(ssavar):
    case IR_OP(phi): {
      Symbol* symbol = VariableSymbol(node);
      if (symbol->flags.is_argument) {
        return kIRMemoryArgument;
      }
      if (symbol->flags.is_temp) {
        return kIRMemoryTemporary;
      }
      if (symbol->flags.is_local &&
          !StorageIs(symbol->storage, STO(static))) {
        return kIRMemoryLocal;
      }
      return StorageIs(symbol->storage, STO(extern))
                 ? kIRMemoryExtern
                 : kIRMemoryStatic;
    }
    default:
      return kIRMemoryUnknown;
  }
}

static bool IsAtomic(IRNode* inst) {
  switch (inst->opcode) {
    case IR_OP(atomic_load):
    case IR_OP(atomic_store):
    case IR_OP(atomic_fetch_add):
    case IR_OP(atomic_fetch_sub):
    case IR_OP(atomic_add_fetch):
    case IR_OP(atomic_sub_fetch):
    case IR_OP(atomic_compare_exchange_bool):
    case IR_OP(atomic_compare_exchange_val):
    case IR_OP(atomic_compare_exchange_n):
    case IR_OP(atomic_fence):
      return true;
    default:
      return false;
  }
}

static uint32_t AccessSize(IRNode* inst) {
  switch (inst->opcode) {
    case IR_OP(load8):
    case IR_OP(loadu8):
    case IR_OP(store8):
    case IR_OP(inc8):
    case IR_OP(uinc8):
    case IR_OP(dec8):
    case IR_OP(udec8):
      return 1;
    case IR_OP(load16):
    case IR_OP(loadu16):
    case IR_OP(store16):
    case IR_OP(inc16):
    case IR_OP(uinc16):
    case IR_OP(dec16):
    case IR_OP(udec16):
      return 2;
    case IR_OP(load32):
    case IR_OP(loadu32):
    case IR_OP(store32):
    case IR_OP(loadf):
    case IR_OP(storef):
    case IR_OP(inc32):
    case IR_OP(uinc32):
    case IR_OP(dec32):
    case IR_OP(udec32):
      return 4;
    case IR_OP(load64):
    case IR_OP(store64):
    case IR_OP(loadd):
    case IR_OP(stored):
    case IR_OP(inc64):
    case IR_OP(uinc64):
    case IR_OP(dec64):
    case IR_OP(udec64):
      return 8;
    default:
      break;
  }
  if (inst->type != NULL && inst->type->size > 0 &&
      inst->type->size <= UINT32_MAX) {
    return (uint32_t)inst->type->size;
  }
  if (inst->inputs.length > 1) {
    IRNode* value = inst->inputs.value.p[1];
    if (value != NULL && value->type != NULL && value->type->size > 0 &&
        value->type->size <= UINT32_MAX) {
      return (uint32_t)value->type->size;
    }
  }
  return 0;
}

static bool DecodeAddress(IRNode* address, Symbol** base,
                          IRMemoryRootKind* kind, int64_t* offset) {
  while (address != NULL) {
    if (address->opcode == IR_OP(adda) && address->inputs.length == 2) {
      IRNode* constant = NULL;
      IRNode* next = NULL;
      if (IRIsIntConst(address->inputs.value.p[0])) {
        constant = address->inputs.value.p[0];
        next = address->inputs.value.p[1];
      } else if (IRIsIntConst(address->inputs.value.p[1])) {
        constant = address->inputs.value.p[1];
        next = address->inputs.value.p[0];
      } else {
        return false;
      }
      int64_t combined;
      if (__builtin_add_overflow(*offset, IRIntConstValue(constant),
                                 &combined)) {
        return false;
      }
      *offset = combined;
      address = next;
      continue;
    }
    if ((address->opcode == IR_OP(addressof) ||
         address->opcode == IR_OP(cast)) &&
        address->inputs.length == 1) {
      address = address->inputs.value.p[0];
      continue;
    }
    Symbol* symbol = VariableSymbol(address);
    if (symbol == NULL) {
      return false;
    }
    *base = symbol;
    *kind = RootKind(address);
    return *kind != kIRMemoryUnknown;
  }
  return false;
}

bool IRAliasDecode(IRNode* inst, IRMemoryLocation* location) {
  memset(location, 0, sizeof(*location));
  location->offset = IR_MEMORY_OFFSET_UNKNOWN;
  if (inst == NULL || inst->inputs.length == 0 ||
      (!IRIsLoadOnly(inst) && !IRIsStore(inst))) {
    return false;
  }
  if (inst->opcode == IR_OP(addressof) || inst->opcode == IR_OP(cast) ||
      inst->opcode == IR_OP(memcpy) || inst->opcode == IR_OP(memzero) ||
      inst->opcode == IR_OP(atomic_fence)) {
    return false;
  }

  location->offset = 0;
  if (!DecodeAddress(inst->inputs.value.p[0], &location->base,
                     &location->kind, &location->offset)) {
    location->offset = IR_MEMORY_OFFSET_UNKNOWN;
    return false;
  }
  location->size = AccessSize(inst);
  location->atomic_access = IsAtomic(inst);
  IRNode* address = inst->inputs.value.p[0];
  location->volatile_access =
      TypeContainsVolatile(inst->type) ||
      (address != NULL && TypeContainsVolatile(address->type));
  location->escaped =
      location->base->flags.address_taken ||
      TypeIsArray(location->base->type);
  return true;
}

IRAliasResult IRAliasClassify(const IRMemoryLocation* lhs,
                              const IRMemoryLocation* rhs) {
  if (lhs->base == NULL || rhs->base == NULL) {
    return kIRMayAlias;
  }
  if (lhs->base != rhs->base) {
    return kIRNoAlias;
  }
  if (lhs->offset == IR_MEMORY_OFFSET_UNKNOWN ||
      rhs->offset == IR_MEMORY_OFFSET_UNKNOWN ||
      lhs->size == 0 || rhs->size == 0) {
    return kIRMayAlias;
  }
  if (lhs->offset == rhs->offset && lhs->size == rhs->size) {
    return kIRMustAlias;
  }
  int64_t lhs_end;
  int64_t rhs_end;
  if (__builtin_add_overflow(lhs->offset, (int64_t)lhs->size, &lhs_end) ||
      __builtin_add_overflow(rhs->offset, (int64_t)rhs->size, &rhs_end)) {
    return kIRMayAlias;
  }
  if (lhs_end <= rhs->offset || rhs_end <= lhs->offset) {
    return kIRNoAlias;
  }
  return kIRMayAlias;
}

static bool UnknownEffectMayClobber(const IRMemoryLocation* location) {
  if (location->base == NULL || location->escaped) {
    return true;
  }
  return location->kind == kIRMemoryStatic ||
         location->kind == kIRMemoryExtern;
}

bool IRAliasInstMayClobber(const IRMemoryLocation* location, IRNode* inst) {
  if (IRIsCall(inst) || IRAsmClobbersMemory(inst)) {
    return UnknownEffectMayClobber(location);
  }
  if (IsAtomic(inst)) {
    return true;
  }
  if (!IRIsStore(inst) || inst->opcode == IR_OP(addressof) ||
      inst->opcode == IR_OP(cast)) {
    return false;
  }
  IRMemoryLocation effect;
  if (!IRAliasDecode(inst, &effect)) {
    return UnknownEffectMayClobber(location);
  }
  return IRAliasClassify(location, &effect) != kIRNoAlias;
}

bool IRAliasLoopMayClobber(Generator* gen, const LoopInfo* loop,
                           const IRMemoryLocation* location) {
  BitSetIterator it;
  BitSetIteratorStart(&it, (BitSet*)&loop->blocks);
  while (!BitSetIteratorDone(&it)) {
    BasicBlock* block =
        VectorGet(&gen->basic_blocks, BitSetIteratorValue(&it));
    for (IRNode* inst = BasicBlockBegin(block);
         !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
         inst = IRNext(inst)) {
      if (IRAliasInstMayClobber(location, inst)) {
        return true;
      }
    }
    BitSetIteratorNext(&it);
  }
  return false;
}
