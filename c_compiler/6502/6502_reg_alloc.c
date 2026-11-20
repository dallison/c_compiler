//
//  6502_reg_alloc.c
//  c_compiler
//
//  Created by David Allison on 6/5/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include "6502_reg_alloc.h"
#include <assert.h>
#include "6502_codegen.h"
#include "6502_machine.h"
#include "target_basic_block.h"

static void InitializeZeroPageRegister(W65C02Register* reg, int num,
                                       W65C02RegisterType type, bool temp) {
  TargetRegisterInit(&reg->base, num);
  reg->type = type;
  reg->temp = temp;
}

#define INIT_REGS(set, type) \
for (int i = 0; i < W65C02_NUM_TEMP_##type##_REGS; i++) { \
  InitializeZeroPageRegister(&allocator->set[i], i, k6502RegType##type, true); \
} \
for (int i = W65C02_NUM_TEMP_##type##_REGS; i < W65C02_NUM_##type##_REGS; i++) { \
  InitializeZeroPageRegister(&allocator->set[i], i, k6502RegType##type, false); \
}

void W65C02RegisterAllocatorInit(W65C02RegisterAllocator* allocator,
                                W65C02Generator* g) {
  allocator->g = g;

  INIT_REGS(b_regs, B);
  INIT_REGS(i_regs, I);
  INIT_REGS(l_regs, L);
  INIT_REGS(x_regs, X);
  INIT_REGS(f_regs, F);

#undef INIT_REGS

  InitializeZeroPageRegister(&allocator->sp_reg, W65C02_SP_REG, k6502RegTypeI, false);
  InitializeZeroPageRegister(&allocator->fp_reg, W65C02_FP_REG, k6502RegTypeI, false);
  BitSetInit(&allocator->used_b_regs);
  BitSetInit(&allocator->used_i_regs);
  BitSetInit(&allocator->used_l_regs);
  BitSetInit(&allocator->used_x_regs);
  BitSetInit(&allocator->used_f_regs);
  allocator->current_spilled_region_size = 0;
  allocator->max_spilled_region_size = 0;
  MapInitForInt64Keys(&allocator->spill_points);
}

W65C02RegisterAllocator* New6502RegisterAllocator(W65C02Generator* g) {
  W65C02RegisterAllocator* reg_alloc = malloc(sizeof(W65C02RegisterAllocator));
  W65C02RegisterAllocatorInit(reg_alloc, g);
  return reg_alloc;
}

void W65C02RegisterAllocatorDestruct(W65C02RegisterAllocator* allocator) {
  BitSetDestruct(&allocator->used_b_regs);
  BitSetDestruct(&allocator->used_i_regs);
  BitSetDestruct(&allocator->used_l_regs);
  BitSetDestruct(&allocator->used_x_regs);
  BitSetDestruct(&allocator->used_f_regs);
  MapDestruct(&allocator->spill_points);
}

void W65C02RegisterAllocatorDelete(W65C02RegisterAllocator* alloc) {
  W65C02RegisterAllocatorDestruct(alloc);
  free(alloc);
}

void W65C02RegisterAllocatorAddSpillPoint(W65C02RegisterAllocator* alloc,
                                            int expr, TargetInstruction* spill_point) {
  MapKeyValue kv = {.key.w = expr, .value.p = spill_point};
  MapInsert(&alloc->spill_points, kv);
}

void W65C02RegisterAllocatorRemoveSpillPoint(W65C02RegisterAllocator* alloc,
                                               int expr) {
  MapKeyType key = {.w = expr};
  MapRemove(&alloc->spill_points, key);
}


static int RegisterSize(W65C02Register* reg) {
  switch (reg->type) {
      case k6502RegTypeB:
      return 1;
      case k6502RegTypeI:
        return 2;
      case k6502RegTypeL:
        return 4;
      case k6502RegTypeX:
       return 8;
      case k6502RegTypeF:
        return 4;
   }
}


static W65C02RegisterType RegisterTypeFromTypeRecord(TypeRecord* type) {
  if (TypeIsPointerOrArray(type) || TypeIsStructOrUnion(type) ||
      TypeIsFunction(type)) {
    return k6502RegTypeI;
  } else if (TypeIsFloatingPoint(type)) {
    return k6502RegTypeF;
  } else if (TypeIsBool(type)) {
    return k6502RegTypeB;
  } else if (TypeIsChar16(type)) {
    return k6502RegTypeI;
  } else if (TypeIsChar32(type)) {
    return k6502RegTypeL;
  } else if (TypeIsCharFamily(type)) {
    return k6502RegTypeB;
  } else if (TypeIsBitInt(type)) {
    // Match storage size: 1-byte _BitInt uses B regs (like char) so the
    // existing __arg_value1_bN helpers apply; wider widths follow int/long.
    int bytes = type->size;
    if (bytes <= 1) {
      return k6502RegTypeB;
    }
    if (bytes <= 2) {
      return k6502RegTypeI;
    }
    if (bytes <= 4) {
      return k6502RegTypeL;
    }
    return k6502RegTypeX;
  } else if (TypeIsLong(type)) {
    return k6502RegTypeL;
  } else if (TypeIsLongLong(type)) {
    return k6502RegTypeX;
  } else if (TypeIsShort(type)) {
    return k6502RegTypeI;
  }
  return k6502RegTypeI;
}

static W65C02RegisterType RegisterTypeFromInstruction(TargetInstruction* inst) {
  switch ((W65C02Opcode)inst->opcode) {
    case W65C02_OP(symbol):
    case W65C02_OP(localvar):
    case W65C02_OP(argument): {
      // For a symbol we need to get the type of the symbol:
      TargetSymbol* target_sym = (TargetSymbol*)inst;
      Symbol* sym = target_sym->symbol;
      TypeRecord* type = sym->type;
      return RegisterTypeFromTypeRecord(type);
    }

    case W65C02_OP(ssavar):
    case W65C02_OP(phi): {
      TargetSymbol* target_sym = (TargetSymbol*)inst->operand[0];
      Symbol* sym = target_sym->symbol;
      TypeRecord* type = sym->type;
      return RegisterTypeFromTypeRecord(type);
      break;
    }

    case W65C02_OP(expr1):
    case W65C02_OP(bvarreg):
      return k6502RegTypeB;

    case W65C02_OP(expr2):
    case W65C02_OP(ivarreg):
      return k6502RegTypeI;

    case W65C02_OP(structreturn):
      return k6502RegTypeI;

    case W65C02_OP(expr4):
    case W65C02_OP(lvarreg):
      return k6502RegTypeL;

    case W65C02_OP(expr8):
    case W65C02_OP(xvarreg):
      return k6502RegTypeX;

    case W65C02_OP(fvarreg):
    case W65C02_OP(exprf):
      return k6502RegTypeF;
      
    case W65C02_OP(dvarreg):
    case W65C02_OP(exprd):
      return k6502RegTypeF;

    default:
      printf("opcode: %s\n", W65C02OpcodeName(inst->opcode));
      assert(false);
      return 0;
  }
}


const char* W65C02RegisterAsString(W65C02Register* reg, int byte, char* buf,
                                  size_t len) {
  char bytebuf[16] = {0};
  if (byte > 0) {
    snprintf(bytebuf, sizeof(bytebuf), "+%d", byte);
  }
  switch (reg->type) {
    case k6502RegTypeB:
      snprintf(buf, len, "__b%d%s", reg->base.num, bytebuf);
      break;
    case k6502RegTypeI:
      if (reg->base.num == W65C02_SP_REG) {
        snprintf(buf, len, "__sp%s", bytebuf);
        break;
      }
      if (reg->base.num == W65C02_FP_REG) {
        snprintf(buf, len, "__fp%s", bytebuf);
        break;
      }
      snprintf(buf, len, "__i%d%s", reg->base.num, bytebuf);
      break;
    case k6502RegTypeL:
      snprintf(buf, len, "__l%d%s", reg->base.num, bytebuf);
      break;
    case k6502RegTypeX:
      snprintf(buf, len, "__x%d%s", reg->base.num, bytebuf);
      break;
    case k6502RegTypeF:
      snprintf(buf, len, "__f%d%s", reg->base.num, bytebuf);
      break;
  }

  return buf;
}


W65C02RegisterType W65C02InstructionRegisterType(TargetInstruction* inst) {
  return RegisterTypeFromInstruction(inst);
}

// Makes the copies that cells start as, now that they have registers.
static void ExpandCopies(W65C02RegisterAllocator* allocator) {
  TargetGenerator* gen = &allocator->g->base;
  for (TargetInstruction* inst = TargetFirstInstruction(gen); inst != NULL;
       inst = TargetNext(inst)) {
    TargetInstruction* src = W65C02CopySource(inst);
    if (src == NULL) {
      continue;
    }
    W65C02Register* to = (W65C02Register*)inst->reg;
    W65C02Register* from = (W65C02Register*)src->reg;
    TargetReplaceOperand(inst, 0, NULL);
    if (to == from || inst->users.length == 0) {
      continue;
    }
    TargetInstruction* pos = inst;
    for (int i = 0; i < RegisterSize(to); i++) {
      TargetInstruction* byte =
          TargetGetIntConstant(gen, NULL, kTargetType8Bit, i);
      TargetInstruction* load =
          TargetNewInstruction2((TargetOpcode)W65C02_OP(lda), src, byte);
      load->flags |= (int)kAddrModeZeroPage << 16;
      TargetInstruction* store =
          TargetNewInstruction2((TargetOpcode)W65C02_OP(sta), inst, byte);
      store->flags |= (int)kAddrModeZeroPage << 16;
      if (inst->block != NULL) {
        TargetBasicBlockEmitAfter(gen, inst->block, load, pos);
        TargetBasicBlockEmitAfter(gen, inst->block, store, load);
      } else {
        TargetEmitAfter(gen, load, pos);
        TargetEmitAfter(gen, store, load);
      }
      pos = store;
    }
  }
}

void W65C02AllocateRegisters(W65C02RegisterAllocator* allocator) {
  W65C02AllocateRegistersByColoring(allocator);
  ExpandCopies(allocator);
  W65C02ShrinkWrap(allocator);
}

uint32_t W65C02RegisterAllocatorBuildRegMask(W65C02RegisterAllocator* alloc) {
  uint32_t result= 0;
  static int shifts[] = {0, 5, 4, 4, 3};
  BitSet* reg_sets[5];
  reg_sets[0] = &alloc->used_i_regs;
  reg_sets[1] = &alloc->used_b_regs;
  reg_sets[2] = &alloc->used_l_regs;
  reg_sets[3] = &alloc->used_x_regs;
  reg_sets[4] = &alloc->used_f_regs;
  for (int i = 4; i >= 0; i--) {
    BitSetIterator it;
    BitSetIteratorStart(&it, reg_sets[i]);
    // __enter saves the first N callee-saved registers of each class
    // (after scratch).  Save through the highest used callee-saved
    // index so holes are covered.  Scratch must not count, or a use of
    // the last register encodes one past the file and __enter writes
    // into the next class.
    static const int temps[] = {W65C02_NUM_TEMP_I_REGS, W65C02_NUM_TEMP_B_REGS,
                                W65C02_NUM_TEMP_L_REGS, W65C02_NUM_TEMP_X_REGS,
                                W65C02_NUM_TEMP_F_REGS};
    static const int preserved[] = {
        W65C02_NUM_PRESERVED_I_REGS, W65C02_NUM_PRESERVED_B_REGS,
        W65C02_NUM_PRESERVED_L_REGS, W65C02_NUM_PRESERVED_X_REGS,
        W65C02_NUM_PRESERVED_F_REGS};
    int highest = -1;
    while (!BitSetIteratorDone(&it)) {
      int n = (int)BitSetIteratorValue(&it);
      if (n > highest) {
        highest = n;
      }
      BitSetIteratorNext(&it);
    }
    int num_regs = 0;
    if (highest >= temps[i]) {
      num_regs = highest - temps[i] + 1;
      if (num_regs > preserved[i]) {
        num_regs = preserved[i];
      }
    }
    result |= num_regs;
    result <<= shifts[i];
  }
  return result;
}

const char* W65C02RegisterAllocatorPrintRegMask(uint32_t mask, char* buf) {
  static int shifts[] = {5, 4, 4, 3, 3};
  static int masks[] = {31,15,15,7,7};
  static const char* reg_types[] = {
    "i",
    "b",
    "l",
    "x",
    "f",
  };
  const char* r = buf;
  for (int i = 0; i < 5; i++) {
    int n = mask & masks[i];
    buf += snprintf(buf, 6, "%s:%d ", reg_types[i], n);
    mask >>= shifts[i];
  }
  return r;
}
