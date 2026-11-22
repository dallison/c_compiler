//
//  alias.c
//  c_compiler
//
//  Conservative, object-based IR alias analysis.
//

#include "alias.h"

#include <limits.h>
#include <string.h>

#include "checked_math.h"
#include "loop_info.h"
#include "type_compare.h"

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
    case IR_OP(inc64):
    case IR_OP(uinc64):
    case IR_OP(dec64):
    case IR_OP(udec64):
      return 8;
    case IR_OP(loadd):
    case IR_OP(stored):
      if (inst->type != NULL && inst->type->size > 0 &&
          inst->type->size <= UINT32_MAX) {
        return (uint32_t)inst->type->size;
      }
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

static bool DecodeAddress(IRNode* address, Symbol** base, IRNode** pointer,
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
      }
      if (constant != NULL) {
        int64_t combined;
        if (Int64AddOverflow(*offset, IRIntConstValue(constant), &combined)) {
          return false;
        }
        *offset = combined;
        address = next;
        continue;
      }
    }
    if ((address->opcode == IR_OP(addressof) ||
         address->opcode == IR_OP(cast)) &&
        address->inputs.length == 1) {
      address = address->inputs.value.p[0];
      continue;
    }
    Symbol* symbol = VariableSymbol(address);
    if (symbol != NULL) {
      *base = symbol;
      *pointer = NULL;
      *kind = RootKind(address);
      return *kind != kIRMemoryUnknown;
    }
    // Pointer value from a load (e.g. `loada` of a pointer parameter): treat
    // the node identity as the object root so consecutive field stores through
    // the same pointer MustAlias.
    if (address->opcode == IR_OP(loada) || address->opcode == IR_OP(load64) ||
        address->opcode == IR_OP(mova) ||
        (address->type != NULL && TypeIsPointerOrArray(address->type))) {
      *base = NULL;
      *pointer = address;
      *kind = kIRMemoryIndirect;
      return true;
    }
    return false;
  }
  return false;
}

static bool AddIndexOperands(IRNode* inst, size_t start, int64_t* offset) {
  for (size_t i = start; i < inst->inputs.length; i++) {
    IRNode* extra = inst->inputs.value.p[i];
    if (!IRIsIntConst(extra)) {
      return false;
    }
    int64_t combined;
    if (Int64AddOverflow(*offset, IRIntConstValue(extra), &combined)) {
      return false;
    }
    *offset = combined;
  }
  return true;
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
  location->pointer = NULL;
  if (!DecodeAddress(inst->inputs.value.p[0], &location->base,
                     &location->pointer, &location->kind, &location->offset)) {
    location->offset = IR_MEMORY_OFFSET_UNKNOWN;
    return false;
  }
  // Extra integer operands are byte offsets: load [base, off...], store
  // [base, value, off...].  Ignoring them makes a _Complex imag store look
  // like a store to the real part.
  if (IRIsLoadOnly(inst)) {
    if (!AddIndexOperands(inst, 1, &location->offset)) {
      location->offset = IR_MEMORY_OFFSET_UNKNOWN;
      return false;
    }
  } else if (IRIsStoreOnly(inst)) {
    if (!AddIndexOperands(inst, 2, &location->offset)) {
      location->offset = IR_MEMORY_OFFSET_UNKNOWN;
      return false;
    }
  }
  location->size = AccessSize(inst);
  location->atomic_access = IsAtomic(inst);
  IRNode* address = inst->inputs.value.p[0];
  location->volatile_access =
      TypeContainsVolatile(inst->type) ||
      (address != NULL && TypeContainsVolatile(address->type));
  if (location->base != NULL) {
    location->escaped =
        location->base->flags.address_taken ||
        TypeIsArray(location->base->type);
  } else {
    // Indirect pointer roots escape by definition.
    location->escaped = true;
  }
  return true;
}

static bool SameObjectRoot(const IRMemoryLocation* lhs,
                           const IRMemoryLocation* rhs) {
  if (lhs->kind == kIRMemoryIndirect || rhs->kind == kIRMemoryIndirect) {
    return lhs->kind == kIRMemoryIndirect && rhs->kind == kIRMemoryIndirect &&
           lhs->pointer != NULL && lhs->pointer == rhs->pointer;
  }
  return lhs->base != NULL && lhs->base == rhs->base;
}

IRAliasResult IRAliasClassify(const IRMemoryLocation* lhs,
                              const IRMemoryLocation* rhs) {
  if (!SameObjectRoot(lhs, rhs)) {
    // An indirect pointer may touch globals/escaped objects, but it does not
    // alias a distinct non-escaping local, temporary, or argument slot.
    if (lhs->kind == kIRMemoryIndirect || rhs->kind == kIRMemoryIndirect) {
      const IRMemoryLocation* named =
          lhs->kind == kIRMemoryIndirect ? rhs : lhs;
      if ((named->kind == kIRMemoryLocal || named->kind == kIRMemoryTemporary ||
           named->kind == kIRMemoryArgument) &&
          !named->escaped) {
        return kIRNoAlias;
      }
      return kIRMayAlias;
    }
    if (lhs->base == NULL || rhs->base == NULL) {
      return kIRMayAlias;
    }
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
  if (Int64AddOverflow(lhs->offset, (int64_t)lhs->size, &lhs_end) ||
      Int64AddOverflow(rhs->offset, (int64_t)rhs->size, &rhs_end)) {
    return kIRMayAlias;
  }
  if (lhs_end <= rhs->offset || rhs_end <= lhs->offset) {
    return kIRNoAlias;
  }
  return kIRMayAlias;
}

static bool UnknownEffectMayClobber(const IRMemoryLocation* location) {
  if (location->kind == kIRMemoryIndirect || location->base == NULL ||
      location->escaped) {
    return true;
  }
  return location->kind == kIRMemoryStatic ||
         location->kind == kIRMemoryExtern;
}

static const Struct* DirectStructOf(TypeRecord* type) {
  if (type == NULL || !TypeIsPrimitive(type) ||
      (type->type & kTypeStruct) == 0 || (type->type & kTypeUnion) != 0) {
    return NULL;
  }
  const Struct* str = type->info.struct_info;
  if (str == NULL || str->is_union) {
    return NULL;
  }
  return str;
}

static const Struct* PointerStructTarget(TypeRecord* ptr_type) {
  if (ptr_type == NULL || !TypeIsPointerOrArray(ptr_type)) {
    return NULL;
  }
  TypeRecord* pointee = ptr_type->next;
  while (pointee != NULL && TypeIsArray(pointee)) {
    pointee = pointee->next;
  }
  return DirectStructOf(pointee);
}

static bool SameStructIdentity(const Struct* a, const Struct* b) {
  if (a == b) {
    return true;
  }
  if (a == NULL || b == NULL) {
    return false;
  }
  return a->tag_symbol != NULL && a->tag_symbol == b->tag_symbol;
}

static bool StructContainsSubobject(const Struct* outer, const Struct* target,
                                    int depth) {
  if (outer == NULL || target == NULL || depth > 16) {
    return true;
  }
  if (SameStructIdentity(outer, target)) {
    return true;
  }
  for (size_t i = 0; i < outer->bases.length; i++) {
    const CXXBaseSpecifier* base = outer->bases.value.p[i];
    if (base == NULL || base->type == NULL) {
      continue;
    }
    const Struct* base_str = DirectStructOf(base->type);
    if (base_str == NULL ||
        StructContainsSubobject(base_str, target, depth + 1)) {
      return true;
    }
  }
  for (size_t i = 0; i < outer->virtual_bases.length; i++) {
    const CXXVirtualBaseInfo* vbase = outer->virtual_bases.value.p[i];
    if (vbase == NULL || vbase->type == NULL) {
      continue;
    }
    const Struct* vbase_str = DirectStructOf(vbase->type);
    if (vbase_str == NULL ||
        StructContainsSubobject(vbase_str, target, depth + 1)) {
      return true;
    }
  }
  for (size_t i = 0; i < outer->members.length; i++) {
    const StructMember* member = outer->members.value.p[i];
    if (member == NULL || member->is_static || member->is_member_function ||
        member->is_using_declaration || member->symbol == NULL ||
        member->symbol->type == NULL) {
      continue;
    }
    TypeRecord* mtype = member->symbol->type;
    while (mtype != NULL && TypeIsArray(mtype)) {
      mtype = mtype->next;
    }
    if (mtype != NULL && TypeIsPrimitive(mtype) &&
        (mtype->type & kTypeUnion) != 0) {
      return true;
    }
    const Struct* mstr = DirectStructOf(mtype);
    if (mstr != NULL && StructContainsSubobject(mstr, target, depth + 1)) {
      return true;
    }
  }
  return false;
}

static bool DistinctNonOverlappingStructs(const Struct* loc_str,
                                          TypeRecord* candidate_ptr_type) {
  const Struct* arg_str = PointerStructTarget(candidate_ptr_type);
  if (loc_str == NULL || arg_str == NULL) {
    return false;
  }
  if (SameStructIdentity(loc_str, arg_str)) {
    return false;
  }
  return !StructContainsSubobject(loc_str, arg_str, 0) &&
         !StructContainsSubobject(arg_str, loc_str, 0);
}

static IRNode* PeelPointerArithmeticBase(IRNode* ptr) {
  for (int depth = 0; ptr != NULL && depth < 8; depth++) {
    if (ptr->opcode == IR_OP(cast) && ptr->inputs.length == 1) {
      IRNode* in = ptr->inputs.value.p[0];
      if (in != NULL && in->type != NULL && TypeIsPointerOrArray(in->type)) {
        ptr = in;
        continue;
      }
      break;
    }
    if (ptr->opcode == IR_OP(adda) && ptr->inputs.length == 2) {
      IRNode* lhs = ptr->inputs.value.p[0];
      IRNode* rhs = ptr->inputs.value.p[1];
      if (lhs != NULL && lhs->type != NULL && TypeIsPointerOrArray(lhs->type)) {
        ptr = lhs;
        continue;
      }
      if (rhs != NULL && rhs->type != NULL && TypeIsPointerOrArray(rhs->type)) {
        ptr = rhs;
        continue;
      }
    }
    break;
  }
  return ptr;
}

static bool IndirectPointersMayAlias(IRNode* loc_ptr, IRNode* arg_ptr,
                                     IRNode* stripped_arg, IRNode* raw_arg) {
  if (loc_ptr == NULL || arg_ptr == NULL) {
    return true;
  }
  IRNode* loc_base = PeelPointerArithmeticBase(loc_ptr);
  IRNode* arg_base = PeelPointerArithmeticBase(arg_ptr);
  if (loc_ptr == arg_ptr || loc_ptr == stripped_arg || loc_ptr == raw_arg ||
      loc_base == arg_base || loc_base == arg_ptr || loc_ptr == arg_base) {
    return true;
  }
  const Struct* loc_str = PointerStructTarget(loc_ptr->type);
  if (loc_str == NULL) {
    return true;
  }
  if (loc_base != loc_ptr &&
      !SameStructIdentity(loc_str, PointerStructTarget(loc_base->type))) {
    return true;
  }
  if (!DistinctNonOverlappingStructs(loc_str, arg_ptr->type)) {
    return true;
  }
  if (arg_base != arg_ptr &&
      !DistinctNonOverlappingStructs(loc_str, arg_base->type)) {
    return true;
  }
  if (stripped_arg != NULL && stripped_arg != arg_ptr &&
      !DistinctNonOverlappingStructs(loc_str, stripped_arg->type)) {
    return true;
  }
  if (raw_arg != NULL && raw_arg != stripped_arg &&
      !DistinctNonOverlappingStructs(loc_str, raw_arg->type)) {
    return true;
  }
  return false;
}

// True when |arg| is a pointer into the same object as |location| (the callee
 // can therefore write that object).  A pointer *value* loaded from a field of
 // the object (e.g. `Sounding(interpreter->bbc)`) roots a different struct
 // type and does not give the callee access to |location|, whereas `void*`,
 // character pointers, or pointers to compatible/overlapping structs may alias.
static bool CallArgProvidesAccessTo(const IRMemoryLocation* location,
                                    IRNode* arg) {
  IRNode* raw_arg = arg;
  while (arg != NULL && arg->opcode == IR_OP(cast) && arg->inputs.length == 1) {
    arg = arg->inputs.value.p[0];
  }
  if (arg == NULL) {
    return false;
  }
  if (location->kind == kIRMemoryIndirect && arg == location->pointer) {
    return true;
  }
  if (location->base != NULL && VariableSymbol(arg) == location->base) {
    return true;
  }

  Symbol* base = NULL;
  IRNode* pointer = NULL;
  IRMemoryRootKind kind = kIRMemoryUnknown;
  int64_t offset = 0;
  if (!DecodeAddress(arg, &base, &pointer, &kind, &offset)) {
    // Opaque pointer-typed value: conservatively assume it may reach an
    // escaped / indirect object.
    return location->kind == kIRMemoryIndirect || location->escaped ||
           location->kind == kIRMemoryStatic ||
           location->kind == kIRMemoryExtern || location->base == NULL;
  }
  if (location->kind == kIRMemoryIndirect) {
    return kind == kIRMemoryIndirect &&
           IndirectPointersMayAlias(location->pointer, pointer, arg, raw_arg);
  }
  return base != NULL && base == location->base;
}

static bool CallMayClobberLocation(const IRMemoryLocation* location,
                                   IRNode* call) {
  // Named escaped objects and globals may have leaked; keep the conservative
  // call clobber.  Refine only SSA-pointer (indirect) locations, which is the
  // ChargeCycles pattern: `Sounding(interp->bbc)` must not kill `interp->x`.
  if (location->kind != kIRMemoryIndirect) {
    return UnknownEffectMayClobber(location);
  }
  if (call->inputs.length == 0 || call->inputs.value.p[0] == NULL) {
    return true;
  }
  // Indirect calls have an unknown callee; assume they can reach anything.
  IRNode* target = call->inputs.value.p[0];
  if (target->opcode != IR_OP(staticvar) &&
      target->opcode != IR_OP(externvar)) {
    return true;
  }

  for (size_t i = 1; i < call->inputs.length; i++) {
    IRNode* arg = call->inputs.value.p[i];
    if (arg != NULL && arg->opcode == IR_OP(pusharg) &&
        arg->inputs.length > 0) {
      arg = arg->inputs.value.p[0];
    }
    if (arg == NULL) {
      continue;
    }
    if (CallArgProvidesAccessTo(location, arg)) {
      return true;
    }
  }
  return false;
}

bool IRAliasInstMayClobber(const IRMemoryLocation* location, IRNode* inst) {
  if (IRIsCall(inst)) {
    return CallMayClobberLocation(location, inst);
  }
  if (IRAsmClobbersMemory(inst)) {
    return UnknownEffectMayClobber(location);
  }
  if (IsAtomic(inst)) {
    return true;
  }
  // Register bitfield ops never touch memory.  Keep this explicit so a future
  // reclassification of setbit as IRIsStore cannot revive the unknown-clobber
  // path through IRAliasDecode on the word operand.
  if (inst != NULL &&
      (inst->opcode == IR_OP(setbit) || inst->opcode == IR_OP(getbit))) {
    return false;
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
