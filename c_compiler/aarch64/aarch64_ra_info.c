//
//  aarch64_ra_info.c
//  c_compiler
//
//  How the shared register allocator code reads AArch64 instructions, and
//  the AArch64 side of the coloring allocator.  See target_ra_ir.h and
//  target_reg_alloc.h.
//

#include <stdlib.h>
#include <string.h>
#include "aarch64_reg_alloc.h"
#include "aarch64_codegen.h"
#include "aarch64_machine.h"
#include "target_basic_block.h"
#include "target_reg_alloc.h"

// Physical register keys: x0..x30 and sp are 0..32, v0..v31 are 64..95.
enum { kFloatKeyBase = 64 };

enum { kIntClass, kFloatClass };

static TargetRAKind Kind(TargetInstruction* inst) {
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(ivarreg):
    case AARCH64_OP(fvarreg):
    case AARCH64_OP(tmp):
    case AARCH64_OP(structreturn):
    case AARCH64_OP(resulti):
    case AARCH64_OP(resultf):
    case AARCH64_OP(resultd):
    case AARCH64_OP(r0):
    case AARCH64_OP(r1):
    case AARCH64_OP(r2):
    case AARCH64_OP(r3):
    case AARCH64_OP(r4):
    case AARCH64_OP(r5):
    case AARCH64_OP(r6):
    case AARCH64_OP(r7):
    case AARCH64_OP(r8):
    case AARCH64_OP(r9):
    case AARCH64_OP(d0):
    case AARCH64_OP(d1):
    case AARCH64_OP(d2):
    case AARCH64_OP(d3):
    case AARCH64_OP(d4):
    case AARCH64_OP(d5):
    case AARCH64_OP(d6):
    case AARCH64_OP(d7):
    case AARCH64_OP(fp):
    case AARCH64_OP(sp):
    case AARCH64_OP(lr):
    case AARCH64_OP(xr):
      return kTargetRALocation;
    case AARCH64_OP(bl):
    case AARCH64_OP(blr):
      // A call's own register holds its result.
      return kTargetRAValue;
    case AARCH64_OP(literal):
      // Printed as a label by the instruction that reads it.
      return kTargetRANone;
    default:
      return AARCH64IsExpression(inst) ? kTargetRAValue : kTargetRANone;
  }
}

static int LocationKey(TargetInstruction* inst) {
  AARCH64Opcode op = (AARCH64Opcode)inst->opcode;
  if (op >= AARCH64_OP(r0) && op <= AARCH64_OP(r9)) {
    return (int)(op - AARCH64_OP(r0));
  }
  if (op >= AARCH64_OP(d0) && op <= AARCH64_OP(d7)) {
    return kFloatKeyBase + (int)(op - AARCH64_OP(d0)) + AARCH64_FP_ARG_START;
  }
  switch (op) {
    case AARCH64_OP(xr):
      return AARCH64_XR_REG;
    case AARCH64_OP(fp):
      return AARCH64_FP_REG;
    case AARCH64_OP(lr):
      return AARCH64_LR_REG;
    case AARCH64_OP(sp):
      return AARCH64_SP_REG;
    case AARCH64_OP(zr):
      return AARCH64_INT_ZERO_REG;
    case AARCH64_OP(resulti):
      return AARCH64_INT_RETURN_REG;
    case AARCH64_OP(resultf):
    case AARCH64_OP(resultd):
      return kFloatKeyBase + AARCH64_FLOAT_RETURN_REG;
    default:
      return -1;
  }
}

// `mov loc, src` with no dest writes its first operand.
static int WrittenOperand(TargetInstruction* inst) {
  AARCH64Opcode op = (AARCH64Opcode)inst->opcode;
  if ((op == AARCH64_OP(mov) || op == AARCH64_OP(fmov)) &&
      inst->dest == NULL && inst->operand[0] != NULL &&
      inst->operand[1] != NULL && Kind(inst->operand[0]) == kTargetRALocation) {
    return 0;
  }
  return -1;
}

static int Clobbers(TargetInstruction* inst, int* keys, int max_keys) {
  if (!AARCH64IsCall(inst)) {
    return 0;
  }
  int n = 0;
  for (int r = 0; r <= 18 && n < max_keys; r++) {
    keys[n++] = r;
  }
  if (n < max_keys) {
    keys[n++] = AARCH64_LR_REG;
  }
  for (int r = 0; r <= 7 && n < max_keys; r++) {
    keys[n++] = kFloatKeyBase + r;
  }
  for (int r = AARCH64_FP_TEMP_START; r <= AARCH64_FP_TEMP_END && n < max_keys;
       r++) {
    keys[n++] = kFloatKeyBase + r;
  }
  return n;
}

static bool ReadsThrough(TargetInstruction* operand) {
  return (AARCH64Opcode)operand->opcode == AARCH64_OP(regarg);
}

static TargetRACopyKind CopyKind(TargetInstruction* inst);

static const TargetRAInfo kInfo = {
    .kind = Kind,
    .location_key = LocationKey,
    .written_operand = WrittenOperand,
    .clobbers = Clobbers,
    .reads_through = ReadsThrough,
    .copy_kind = CopyKind,
};

const TargetRAInfo* AARCH64RAInfo(void) { return &kInfo; }

// ---------------------------------------------------------------------------
// Coloring allocator.

typedef struct {
  AARCH64RegisterAllocator* allocator;
  bool returns_int;
  bool returns_float;
  int spill_size;
} ColoringContext;

// The atomic pseudos keep their size and memory orders in the flag bits that
// other instructions use for their register constraints.  Their results are
// all integers.
static bool IsAtomic(TargetInstruction* inst) {
  AARCH64Opcode op = (AARCH64Opcode)inst->opcode;
  return op >= AARCH64_OP(atomic_load) && op <= AARCH64_OP(atomic_fence);
}

static bool AvoidsArgRegs(TargetInstruction* inst) {
  return !IsAtomic(inst) && (inst->flags & AARCH64_INST_AVOID_ARG_REGS) != 0;
}

static int RegClass(TargetInstruction* inst) {
  if ((AARCH64Opcode)inst->opcode == AARCH64_OP(reload)) {
    return RegClass(inst->operand[0]->operand[0]);
  }
  if (AARCH64IsCall(inst)) {
    return (inst->flags & AARCH64_INST_FP_RETURN) != 0 ? kFloatClass
                                                       : kIntClass;
  }
  if (IsAtomic(inst)) {
    return kIntClass;
  }
  return AARCH64HasFloatResult(inst) ? kFloatClass : kIntClass;
}

static int FixedValueKey(TargetInstruction* inst) {
  if (!AARCH64IsCall(inst)) {
    return -1;
  }
  return (inst->flags & AARCH64_INST_FP_RETURN) != 0
             ? kFloatKeyBase + AARCH64_FLOAT_RETURN_REG
             : AARCH64_INT_RETURN_REG;
}

static bool DefinesRegister(TargetInstruction* inst) {
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(bl):
    case AARCH64_OP(blr):
      return true;
    case AARCH64_OP(cmp):
    case AARCH64_OP(cmn):
    case AARCH64_OP(tst):
    case AARCH64_OP(ccmp):
    case AARCH64_OP(ccmn):
    case AARCH64_OP(fcmp):
    case AARCH64_OP(fstr):
    case AARCH64_OP(prfm):
    case AARCH64_OP(nop):
    case AARCH64_OP(dmb):
    case AARCH64_OP(clrex):
      return false;
    default:
      return AARCH64GeneratesOutput(inst);
  }
}

static int OperandSize(TargetInstruction* inst) {
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(fmv_s):
      return kSize32Bit;
    case AARCH64_OP(fmv_d):
      return kSize64Bit;
    default: {
      int size = GetRegisterSize(inst);
      return size == 0 ? kSize64Bit : size;
    }
  }
}

static bool IsMoveOpcode(AARCH64Opcode op) {
  return op == AARCH64_OP(mov) || op == AARCH64_OP(mv) ||
         op == AARCH64_OP(fmov) || op == AARCH64_OP(fmv_s) ||
         op == AARCH64_OP(fmv_d);
}

// Whether |inst| leaves the upper half of its 64-bit register clear, as
// every instruction that writes a w register does.
static bool ClearsUpperHalf(TargetInstruction* inst) {
  AARCH64Opcode op = (AARCH64Opcode)inst->opcode;
  if (Kind(inst) != kTargetRAValue || AARCH64IsCall(inst) || IsMoveOpcode(op) ||
      GetRegisterSize(inst) != kSize32Bit) {
    return false;
  }
  switch (op) {
    case AARCH64_OP(reload):
    case AARCH64_OP(sxtw):
    case AARCH64_OP(ldursw):
      return false;
    default:
      return true;
  }
}

static TargetRACopyKind CopyKind(TargetInstruction* inst) {
  AARCH64Opcode op = (AARCH64Opcode)inst->opcode;
  TargetInstruction* src = inst->operand[0];
  if (!IsMoveOpcode(op) || src == NULL || inst->operand[1] != NULL ||
      Kind(src) == kTargetRANone || RegClass(inst) != RegClass(src)) {
    return kTargetRANotCopy;
  }
  int size = OperandSize(inst);
  if (RegClass(inst) == kFloatClass) {
    return size == OperandSize(src) ? kTargetRACopy : kTargetRANotCopy;
  }
  // A 32-bit move clears the upper half, so it is only a plain copy when the
  // source already has.
  if (size == kSize64Bit || ClearsUpperHalf(src)) {
    return kTargetRACopy;
  }
  return kTargetRAConvertingCopy;
}

static int ArgumentKeys(int* keys, int max_keys) {
  int n = 0;
  for (int r = 0; r < AARCH64_NUM_INT_ARGS && n < max_keys; r++) {
    keys[n++] = AARCH64_INT_ARG_START + r;
  }
  for (int r = 0; r < AARCH64_NUM_FP_ARGS && n < max_keys; r++) {
    keys[n++] = kFloatKeyBase + AARCH64_FP_ARG_START + r;
  }
  return n;
}

static int ImplicitUses(void* ctx, TargetInstruction* inst, int* keys,
                        int max_keys) {
  ColoringContext* c = ctx;
  int n = 0;
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(save):
      // The prologue stores incoming argument registers to the frame.
      return ArgumentKeys(keys, max_keys);
    case AARCH64_OP(ret):
      if (c->returns_int && n < max_keys) {
        keys[n++] = AARCH64_INT_RETURN_REG;
      }
      if (c->returns_float && n < max_keys) {
        keys[n++] = kFloatKeyBase + AARCH64_FLOAT_RETURN_REG;
      }
      return n;
    default:
      return 0;
  }
}

static bool IsVector128(TargetInstruction* inst) {
  return RegClass(inst) == kFloatClass && OperandSize(inst) == kSize128Bit;
}

// A call preserves only the low 64 bits of v8-v15, so a 128-bit value stays
// out of them, and one live across a call spills.
static int ForbiddenKeys(TargetInstruction* inst, int* keys, int max_keys) {
  int n = AvoidsArgRegs(inst) ? ArgumentKeys(keys, max_keys) : 0;
  if (IsVector128(inst)) {
    for (int r = AARCH64_FP_SAVED_START;
         r <= AARCH64_FP_SAVED_END && n < max_keys; r++) {
      keys[n++] = kFloatKeyBase + r;
    }
  }
  return n;
}

// These expand to a retry loop that loads the result before the store that
// reads the operands again.
static bool EarlyClobber(TargetInstruction* inst) {
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(atomic_fetch_add):
    case AARCH64_OP(atomic_fetch_sub):
    case AARCH64_OP(atomic_add_fetch):
    case AARCH64_OP(atomic_sub_fetch):
    case AARCH64_OP(atomic_compare_exchange_bool):
    case AARCH64_OP(atomic_compare_exchange_val):
    case AARCH64_OP(atomic_compare_exchange_n):
      return true;
    default:
      return false;
  }
}

static TargetInstruction* CreateCopy(void* ctx, TargetInstruction* value,
                                     TargetInstruction* dest) {
  (void)ctx;
  bool fp = RegClass(value) == kFloatClass;
  TargetInstruction* copy = TargetNewInstruction1(
      (TargetOpcode)(fp ? AARCH64_OP(fmov) : AARCH64_OP(mov)), value);
  SetInstructionSize(copy, OperandSize(value));
  if (fp) {
    copy->flags |= AARCH64_INST_FP;
  }
  copy->dest = dest;
  // The emitter requires fmv_s and fmv_d to keep a dest; it writes their own
  // register either way.
  AARCH64Opcode op = (AARCH64Opcode)value->opcode;
  if (dest != NULL && value->dest == dest && op != AARCH64_OP(fmv_s) &&
      op != AARCH64_OP(fmv_d)) {
    value->dest = NULL;
  }
  return copy;
}

static int SlotSize(TargetInstruction* value) {
  return IsVector128(value) ? kSize128Bit : kSize64Bit;
}

// The register variable whose location is |inst|, or NULL.
static RegisterVariable* FindVariable(ColoringContext* c,
                                      TargetInstruction* inst) {
  if (Kind(inst) != kTargetRALocation) {
    return NULL;
  }
  Vector* var_regs = &c->allocator->g->var_regs;
  for (size_t i = 0; i < var_regs->length; i++) {
    RegisterVariable* var = var_regs->value.p[i];
    if (var->inst == inst) {
      return var;
    }
  }
  return NULL;
}

// The emitter addresses slot |s| 8 + |s| bytes below the top of the spill
// region, so slot |s| is the 8 bytes at depth |s| and a 16-byte slot at depth
// |d| is numbered |d| + 8.
static int NewSlot(void* ctx, TargetInstruction* value) {
  ColoringContext* c = ctx;
  int slot;
  if (SlotSize(value) != kSize128Bit) {
    slot = c->spill_size;
    c->spill_size += 8;
  } else {
    c->spill_size = (c->spill_size + 15) & ~15;
    slot = c->spill_size + 8;
    c->spill_size += 16;
  }
  RegisterVariable* var = FindVariable(c, value);
  if (var != NULL) {
    var->spill_slot = var->spilled && var->spill_slot != slot ? -1 : slot;
    var->spilled = true;
  }
  return slot;
}

static TargetInstruction* CreateSpill(void* ctx, TargetInstruction* value,
                                      int slot) {
  ColoringContext* c = ctx;
  TargetInstruction* spill = TargetNewInstruction2(
      (TargetOpcode)AARCH64_OP(spill), value,
      TargetGetIntConstant(&c->allocator->g->base, NULL, kTargetType32Bit,
                           slot));
  SetInstructionSize(spill, SlotSize(value));
  return spill;
}

static TargetInstruction* CreateReload(void* ctx, TargetInstruction* spill) {
  (void)ctx;
  TargetInstruction* reload =
      TargetNewInstruction1((TargetOpcode)AARCH64_OP(reload), spill);
  SetInstructionSize(reload, GetRegisterSize(spill));
  if (RegClass(spill->operand[0]) == kFloatClass) {
    reload->flags |= AARCH64_INST_FP;
  }
  return reload;
}

// Variable registers are TargetSymbols; this one stands for no variable.
static TargetInstruction* CreateLocation(void* ctx, TargetInstruction* like) {
  ColoringContext* c = ctx;
  int size = OperandSize(like);
  if (size == kSize128Bit) {
    return NULL;
  }
  bool fp = RegClass(like) == kFloatClass;
  TargetSymbol* location = calloc(1, sizeof(TargetSymbol));
  TargetInitInstruction(
      &location->base,
      (TargetOpcode)(fp ? AARCH64_OP(fvarreg) : AARCH64_OP(ivarreg)));
  SetInstructionSize(&location->base, size);
  if (AvoidsArgRegs(like)) {
    location->base.flags |= AARCH64_INST_AVOID_ARG_REGS;
  }
  TargetTrackOrphanInstruction(&c->allocator->g->base, &location->base);
  RegisterVariable* var = FindVariable(c, like);
  if (var != NULL) {
    var->split_location = &location->base;
  }
  return &location->base;
}

static TargetInstruction* CreateLabel(void* ctx) {
  (void)ctx;
  return TargetNewInstruction((TargetOpcode)AARCH64_OP(label));
}

static TargetInstruction* CreateJump(void* ctx, TargetInstruction* label) {
  ColoringContext* c = ctx;
  return TargetNewInstruction2((TargetOpcode)AARCH64_OP(b),
                               AARCH64AlwaysCondition(c->allocator->g), label);
}

static void Assign(void* ctx, TargetInstruction* inst, int key) {
  ColoringContext* c = ctx;
  AARCH64RegisterAllocator* allocator = c->allocator;
  if (key >= kFloatKeyBase) {
    int num = key - kFloatKeyBase;
    inst->reg = &allocator->float_regs[num].base;
    if (num >= AARCH64_FP_SAVED_START && num <= AARCH64_FP_SAVED_END) {
      BitSetInsert(&allocator->used_float_regs, (size_t)num);
    }
    return;
  }
  inst->reg = &allocator->int_regs[key].base;
  if (key >= AARCH64_INT_SAVED_START && key <= AARCH64_INT_SAVED_END) {
    BitSetInsert(&allocator->used_int_regs, (size_t)key);
  }
}

static TargetInstruction* NextInBlock(TargetBasicBlock* block,
                                      TargetInstruction* inst) {
  return inst == block->end_code ? NULL : TargetNext(inst);
}

#define FOR_EACH_INSTRUCTION(gen, inst)                                     \
  for (size_t b_ = 0; b_ < (gen)->basic_blocks.length; b_++)               \
    for (TargetBasicBlock* block_ = (gen)->basic_blocks.value.p[b_];        \
         block_ != NULL && block_->code != NULL &&                          \
         block_->end_code != NULL;                                          \
         block_ = NULL)                                                     \
      for (TargetInstruction* inst = block_->code; inst != NULL;            \
           inst = NextInBlock(block_, inst))

static const char* Unsupported(AARCH64Generator* g) {
  FOR_EACH_INSTRUCTION(&g->base, inst) {
    AARCH64Opcode op = (AARCH64Opcode)inst->opcode;
    switch (op) {
      case AARCH64_OP(asm):
      case AARCH64_OP(tlsgd):
      case AARCH64_OP(ldp):
      case AARCH64_OP(ldpsw):
      case AARCH64_OP(movk):
      case AARCH64_OP(bfi):
      case AARCH64_OP(bfxil):
      case AARCH64_OP(ldxr):
      case AARCH64_OP(ldaxr):
      case AARCH64_OP(stxr):
      case AARCH64_OP(stlxr):
      case AARCH64_OP(ldar):
      case AARCH64_OP(stlr):
      case AARCH64_OP(spill):
      case AARCH64_OP(reload):
        return AARCH64OpcodeName((int)op);
      default:
        break;
    }
    // Neither emitter copies all 128 bits of a register, so a 128-bit value
    // must be one that never needs a copy.
    if (Kind(inst) != kTargetRANone && GetRegisterSize(inst) == kSize128Bit &&
        (!IsVector128(inst) || Kind(inst) != kTargetRAValue ||
         inst->dest != NULL || IsMoveOpcode(op) || AARCH64IsCall(inst))) {
      return "128-bit";
    }
  }
  return NULL;
}

bool AARCH64AllocateRegistersByColoring(AARCH64RegisterAllocator* allocator) {
  AARCH64Generator* g = allocator->g;
  TargetGenerator* gen = &g->base;
  const char* reason = Unsupported(g);
  if (reason != NULL) {
    if (getenv("DAVECC_NEW_RA_STATS") != NULL) {
      fprintf(stderr, "new-ra-fallback\t%s\t%s\n", gen->function_name.value,
              reason);
    }
    return false;
  }

  ColoringContext ctx = {.allocator = allocator};
  FOR_EACH_INSTRUCTION(gen, inst) {
    TargetInstruction* dest = inst->dest;
    if (dest == NULL) {
      continue;
    }
    AARCH64Opcode op = (AARCH64Opcode)dest->opcode;
    if (op == AARCH64_OP(resulti)) {
      ctx.returns_int = true;
    } else if (op == AARCH64_OP(resultf) || op == AARCH64_OP(resultd)) {
      ctx.returns_float = true;
    }
  }

  TargetRegAllocDesc desc = {
      .info = &kInfo,
      .ctx = &ctx,
      .num_classes = 2,
      .reg_class = RegClass,
      .fixed_value_key = FixedValueKey,
      .defines_register = DefinesRegister,
      .implicit_uses = ImplicitUses,
      .forbidden_keys = ForbiddenKeys,
      .early_clobber = EarlyClobber,
      .create_copy = CreateCopy,
      .new_slot = NewSlot,
      .create_spill = CreateSpill,
      .create_reload = CreateReload,
      .assign = Assign,
  };
  // DAVECC_NEW_RA_NO_SPLIT turns splitting off, for comparison.
  if (getenv("DAVECC_NEW_RA_NO_SPLIT") == NULL) {
    desc.create_location = CreateLocation;
    desc.create_label = CreateLabel;
    desc.create_jump = CreateJump;
  }
  // Caller-saved registers first, so that only values live across a call
  // cost a save in the prologue.  x8 and x9 are emitter scratch registers,
  // x16 and x17 the linker's and x18 the platform's.
  TargetRAClass* ints = &desc.classes[kIntClass];
  for (int r = AARCH64_INT_TEMP_START + 1; r <= AARCH64_INT_TEMP_END; r++) {
    ints->regs[ints->num_regs++] = r;
  }
  for (int r = 0; r < AARCH64_NUM_INT_ARGS; r++) {
    ints->regs[ints->num_regs++] = AARCH64_INT_ARG_START + r;
  }
  for (int r = AARCH64_INT_SAVED_START; r <= AARCH64_INT_SAVED_END; r++) {
    ints->regs[ints->num_regs++] = r;
  }
  TargetRAClass* floats = &desc.classes[kFloatClass];
  for (int r = AARCH64_FP_TEMP_START; r <= AARCH64_FP_TEMP_END; r++) {
    floats->regs[floats->num_regs++] = kFloatKeyBase + r;
  }
  for (int r = 0; r < AARCH64_NUM_FP_ARGS; r++) {
    floats->regs[floats->num_regs++] = kFloatKeyBase + AARCH64_FP_ARG_START + r;
  }
  for (int r = AARCH64_FP_SAVED_START; r <= AARCH64_FP_SAVED_END; r++) {
    floats->regs[floats->num_regs++] = kFloatKeyBase + r;
  }

  if (!TargetAllocateRegisters(gen, &desc, NULL)) {
    return false;
  }
  FOR_EACH_INSTRUCTION(gen, inst) {
    if ((AARCH64Opcode)inst->opcode == AARCH64_OP(spill)) {
      inst->reg = inst->operand[0]->reg;
    }
  }
  allocator->current_spilled_region_size = ctx.spill_size;
  allocator->max_spilled_region_size = ctx.spill_size;
  g->colored = true;
  return true;
}
