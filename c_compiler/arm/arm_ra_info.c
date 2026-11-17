//
//  arm_ra_info.c
//  c_compiler
//
//  How the shared register allocator code reads ARM instructions, and the
//  ARM side of the coloring allocator.  See target_ra_ir.h and
//  target_reg_alloc.h.
//

#include <stdlib.h>
#include <string.h>
#include "arm_reg_alloc.h"
#include "arm_codegen.h"
#include "arm_machine.h"
#include "target_basic_block.h"
#include "target_reg_alloc.h"

// Physical register keys: r0..r15 are 0..15 and s0..s31 are 64..95.  A double
// occupies an even s-register and the odd one above it, so floats are only
// allocated to the even ones.
enum { kFloatKeyBase = 64 };

enum { kIntClass, kFloatClass };

static bool IsCondition(ARMOpcode op) {
  return op >= ARM_OP(eq) && op <= ARM_OP(al);
}

static TargetRAKind Kind(TargetInstruction* inst) {
  ARMOpcode op = (ARMOpcode)inst->opcode;
  switch (op) {
    case ARM_OP(ivarreg):
    case ARM_OP(fvarreg):
    case ARM_OP(tmp):
    case ARM_OP(structreturn):
    case ARM_OP(resulti):
    case ARM_OP(resultf):
    case ARM_OP(resultd):
    case ARM_OP(r0):
    case ARM_OP(r1):
    case ARM_OP(r2):
    case ARM_OP(r3):
    case ARM_OP(r4):
    case ARM_OP(r5):
    case ARM_OP(r6):
    case ARM_OP(r7):
    case ARM_OP(r8):
    case ARM_OP(r9):
    case ARM_OP(d0):
    case ARM_OP(d1):
    case ARM_OP(d2):
    case ARM_OP(d3):
    case ARM_OP(d4):
    case ARM_OP(d5):
    case ARM_OP(d6):
    case ARM_OP(d7):
    case ARM_OP(fp):
    case ARM_OP(sp):
    case ARM_OP(lr):
    case ARM_OP(xr):
      return kTargetRALocation;
    case ARM_OP(bl):
    case ARM_OP(blr):
      // A call's own register holds its result.
      return kTargetRAValue;
    case ARM_OP(literal):
    case ARM_OP(zr):
    case ARM_OP(oplsl):
      // Printed by the instruction that reads it.
      return kTargetRANone;
    default:
      if (IsCondition(op)) {
        return kTargetRANone;
      }
      return ARMIsExpression(inst) ? kTargetRAValue : kTargetRANone;
  }
}

static int LocationKey(TargetInstruction* inst) {
  ARMOpcode op = (ARMOpcode)inst->opcode;
  if (op >= ARM_OP(r0) && op <= ARM_OP(r9)) {
    return (int)(op - ARM_OP(r0)) + ARM_INT_ARG_START;
  }
  if (op >= ARM_OP(d0) && op <= ARM_OP(d7)) {
    return kFloatKeyBase + 2 * ((int)(op - ARM_OP(d0)) + ARM_FP_ARG_START);
  }
  switch (op) {
    case ARM_OP(xr):
      return ARM_XR_REG;
    case ARM_OP(fp):
      return ARM_FP_REG;
    case ARM_OP(lr):
      return ARM_LR_REG;
    case ARM_OP(sp):
      return ARM_SP_REG;
    case ARM_OP(resulti):
      return ARM_INT_RETURN_REG;
    case ARM_OP(resultf):
    case ARM_OP(resultd):
      return kFloatKeyBase + ARM_FLOAT_RETURN_REG;
    default:
      return -1;
  }
}

// Every ARM move writes through its dest.
static int WrittenOperand(TargetInstruction* inst) {
  (void)inst;
  return -1;
}

// `vmov rlo, rhi, dm` passes a variadic double in a pair of core registers.
// It is an fcvt whose dest is the low register and whose first operand is the
// high one, which it writes rather than reads.
static bool MovesDoubleToCorePair(TargetInstruction* inst) {
  if ((ARMOpcode)inst->opcode != ARM_OP(fcvt) || inst->operand[0] == NULL ||
      inst->operand[1] == NULL || Kind(inst->operand[0]) != kTargetRALocation) {
    return false;
  }
  int key = LocationKey(inst->operand[0]);
  return key >= 0 && key < kFloatKeyBase;
}

static int Clobbers(TargetInstruction* inst, int* keys, int max_keys) {
  if (MovesDoubleToCorePair(inst)) {
    keys[0] = LocationKey(inst->operand[0]);
    return 1;
  }
  if (!ARMIsCall(inst)) {
    return 0;
  }
  int n = 0;
  for (int r = ARM_INT_TEMP_START; r <= ARM_INT_TEMP_END && n < max_keys; r++) {
    keys[n++] = r;
  }
  if (n < max_keys) {
    keys[n++] = ARM_IP_REG;
  }
  if (n < max_keys) {
    keys[n++] = ARM_LR_REG;
  }
  for (int r = ARM_FP_TEMP_START; r <= ARM_FP_TEMP_END && n < max_keys; r++) {
    keys[n++] = kFloatKeyBase + r;
  }
  return n;
}

static bool ReadsThrough(TargetInstruction* operand) {
  return (ARMOpcode)operand->opcode == ARM_OP(regarg);
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

const TargetRAInfo* ARMRAInfo(void) { return &kInfo; }

// ---------------------------------------------------------------------------
// Coloring allocator.

typedef struct {
  ARMRegisterAllocator* allocator;
  bool returns_int;
  bool returns_float;
  int spill_size;
} ColoringContext;

// The atomic pseudos keep their size and memory orders in flag bits that
// other instructions use for other things.  Their results are all integers.
static bool IsAtomic(TargetInstruction* inst) {
  ARMOpcode op = (ARMOpcode)inst->opcode;
  return op >= ARM_OP(atomic_load) && op <= ARM_OP(atomic_fence);
}

static int RegClass(TargetInstruction* inst) {
  if ((ARMOpcode)inst->opcode == ARM_OP(reload)) {
    return RegClass(inst->operand[0]->operand[0]);
  }
  if (ARMIsCall(inst)) {
    return (inst->flags & kARMFpReturn) != 0 ? kFloatClass : kIntClass;
  }
  if (IsAtomic(inst) || MovesDoubleToCorePair(inst)) {
    return kIntClass;
  }
  if (Kind(inst) == kTargetRALocation) {
    int key = LocationKey(inst);
    if (key >= 0) {
      return key >= kFloatKeyBase ? kFloatClass : kIntClass;
    }
  }
  return ARMHasFloatResult(inst) ? kFloatClass : kIntClass;
}

static int FixedValueKey(TargetInstruction* inst) {
  if (!ARMIsCall(inst)) {
    return -1;
  }
  return (inst->flags & kARMFpReturn) != 0
             ? kFloatKeyBase + ARM_FLOAT_RETURN_REG
             : ARM_INT_RETURN_REG;
}

static bool DefinesRegister(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(bl):
    case ARM_OP(blr):
      return true;
    case ARM_OP(cmp):
    case ARM_OP(cmn):
    case ARM_OP(tst):
    case ARM_OP(ccmp):
    case ARM_OP(ccmn):
    case ARM_OP(fcmp):
    case ARM_OP(prfm):
    case ARM_OP(nop):
      return false;
    default:
      return ARMGeneratesOutput(inst);
  }
}

static TargetInstruction* SetSize(TargetInstruction* inst, int size) {
  inst->flags = (inst->flags & ~(3u << 16)) | ((uint32_t)size << 16);
  return inst;
}

// The width of the register |inst| uses.  Variable registers carry no size,
// so a float one is as wide as its variable.
static int OperandSize(TargetInstruction* inst) {
  int size = ARMGetRegisterSize(inst);
  if (size != 0) {
    return size;
  }
  if ((ARMOpcode)inst->opcode == ARM_OP(fvarreg)) {
    Symbol* symbol = ((TargetSymbol*)inst)->symbol;
    if (symbol != NULL && symbol->type != NULL &&
        TypeUsesFloat64Representation(symbol->type)) {
      return kSize64Bit;
    }
  }
  return kSize32Bit;
}

static TargetRACopyKind CopyKind(TargetInstruction* inst) {
  ARMOpcode op = (ARMOpcode)inst->opcode;
  TargetInstruction* src = inst->operand[0];
  if ((op != ARM_OP(mov) && op != ARM_OP(fmov)) || src == NULL ||
      inst->operand[1] != NULL || Kind(src) == kTargetRANone ||
      RegClass(inst) != RegClass(src)) {
    return kTargetRANotCopy;
  }
  // A single-precision move of a double keeps only its low half.  Locations
  // hold whatever was last written to them.
  if (RegClass(inst) == kFloatClass && Kind(src) == kTargetRAValue &&
      OperandSize(inst) != OperandSize(src)) {
    return kTargetRANotCopy;
  }
  return kTargetRACopy;
}

static int ArgumentKeys(int* keys, int max_keys) {
  int n = 0;
  for (int r = 0; r < ARM_NUM_INT_ARGS && n < max_keys; r++) {
    keys[n++] = ARM_INT_ARG_START + r;
  }
  for (int r = 0; r < ARM_NUM_FP_ARGS && n < max_keys; r++) {
    keys[n++] = kFloatKeyBase + 2 * (ARM_FP_ARG_START + r);
  }
  return n;
}

// A tail call branches through r9 to a computed target, or to a symbol.
static bool IsTailCall(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(br):
      return inst->operand[0] != NULL &&
             (ARMOpcode)inst->operand[0]->opcode == ARM_OP(r9);
    case ARM_OP(b):
      return inst->operand[1] != NULL &&
             (ARMOpcode)inst->operand[1]->opcode == ARM_OP(symbol);
    default:
      return false;
  }
}

static int ImplicitUses(void* ctx, TargetInstruction* inst, int* keys,
                        int max_keys) {
  ColoringContext* c = ctx;
  int n = 0;
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(save):
      // The prologue stores incoming argument registers to the frame.
      return ArgumentKeys(keys, max_keys);
    case ARM_OP(ret):
      if (c->returns_int && n < max_keys) {
        keys[n++] = ARM_INT_RETURN_REG;
      }
      if (c->allocator->g->wide_result && n < max_keys) {
        keys[n++] = ARM_INT_RETURN_VALUE_1;
      }
      if (c->returns_float && n < max_keys) {
        keys[n++] = kFloatKeyBase + ARM_FLOAT_RETURN_REG;
      }
      return n;
    default:
      // The callee reads the argument registers set up before the epilogue.
      return IsTailCall(inst) ? ArgumentKeys(keys, max_keys) : 0;
  }
}

static int ForbiddenKeys(TargetInstruction* inst, int* keys, int max_keys) {
  (void)inst;
  (void)keys;
  (void)max_keys;
  return 0;
}

// The atomics expand to a retry loop that loads the result before the store
// that reads the operands again, and csel to `mov rd, b; mov<cond> rd, a`.
static bool EarlyClobber(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(atomic_fetch_add):
    case ARM_OP(atomic_fetch_sub):
    case ARM_OP(atomic_add_fetch):
    case ARM_OP(atomic_sub_fetch):
    case ARM_OP(atomic_compare_exchange_bool):
    case ARM_OP(atomic_compare_exchange_val):
    case ARM_OP(atomic_compare_exchange_n):
    case ARM_OP(csel):
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
      (TargetOpcode)(fp ? ARM_OP(fmov) : ARM_OP(mov)), value);
  SetSize(copy, OperandSize(value));
  copy->dest = dest;
  if (dest != NULL && value->dest == dest) {
    value->dest = NULL;
  }
  return copy;
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

// The emitter addresses slot |s| 8 + |s| bytes below the local variables, so
// slot |s| is the 8 bytes at depth |s|.
static int NewSlot(void* ctx, TargetInstruction* value) {
  ColoringContext* c = ctx;
  int slot = c->spill_size;
  c->spill_size += 8;
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
      (TargetOpcode)ARM_OP(spill), value,
      TargetGetIntConstant(&c->allocator->g->base, NULL, kTargetType32Bit,
                           slot));
  return SetSize(spill, OperandSize(value));
}

static TargetInstruction* CreateReload(void* ctx, TargetInstruction* spill) {
  (void)ctx;
  TargetInstruction* reload =
      TargetNewInstruction1((TargetOpcode)ARM_OP(reload), spill);
  return SetSize(reload, ARMGetRegisterSize(spill));
}

// Variable registers are TargetSymbols; this one stands for no variable.
static TargetInstruction* CreateLocation(void* ctx, TargetInstruction* like) {
  ColoringContext* c = ctx;
  bool fp = RegClass(like) == kFloatClass;
  TargetSymbol* location = calloc(1, sizeof(TargetSymbol));
  TargetInitInstruction(
      &location->base,
      (TargetOpcode)(fp ? ARM_OP(fvarreg) : ARM_OP(ivarreg)));
  SetSize(&location->base, OperandSize(like));
  TargetTrackOrphanInstruction(&c->allocator->g->base, &location->base);
  RegisterVariable* var = FindVariable(c, like);
  if (var != NULL) {
    var->split_location = &location->base;
  }
  return &location->base;
}

static TargetInstruction* CreateLabel(void* ctx) {
  (void)ctx;
  return TargetNewInstruction((TargetOpcode)ARM_OP(label));
}

static TargetInstruction* CreateJump(void* ctx, TargetInstruction* label) {
  ColoringContext* c = ctx;
  return TargetNewInstruction2((TargetOpcode)ARM_OP(b),
                               ARMAlwaysCondition(c->allocator->g), label);
}

// r9 is the codegen's scratch register and r11 the frame pointer; the
// prologue saves neither as a callee-saved register.
static bool IsSavedAllocatable(int key) {
  return key >= ARM_INT_SAVED_START && key <= ARM_INT_SAVED_END &&
         key != ARM_TMP_REG && key != ARM_FP_REG;
}

static void Assign(void* ctx, TargetInstruction* inst, int key) {
  ColoringContext* c = ctx;
  ARMRegisterAllocator* allocator = c->allocator;
  if (key >= kFloatKeyBase) {
    int num = key - kFloatKeyBase;
    inst->reg = &allocator->float_regs[num].base;
    if (num >= ARM_FP_SAVED_START && num <= ARM_FP_SAVED_END) {
      BitSetInsert(&allocator->used_float_regs, (size_t)num);
    }
    return;
  }
  inst->reg = &allocator->int_regs[key].base;
  if (IsSavedAllocatable(key)) {
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

static int AtomicSizeLog2(TargetInstruction* inst) {
  return (int)((inst->flags & ARM_ATOMIC_SIZE_MASK) >> ARM_ATOMIC_SIZE_SHIFT);
}

static const char* Unsupported(ARMGenerator* g) {
  FOR_EACH_INSTRUCTION(&g->base, inst) {
    ARMOpcode op = (ARMOpcode)inst->opcode;
    switch (op) {
      case ARM_OP(asm):
      case ARM_OP(ldp):
      case ARM_OP(ldpsw):
      case ARM_OP(stp):
      case ARM_OP(movk):
      case ARM_OP(bfi):
      case ARM_OP(bfxil):
      case ARM_OP(spill):
      case ARM_OP(reload):
        return ARMOpcodeName((int)op);
      default:
        break;
    }
    // A 64-bit atomic finds the high halves of its operands and result
    // through their users rather than its operands.
    if (IsAtomic(inst) && AtomicSizeLog2(inst) == 3) {
      return "64-bit atomic";
    }
    // A quad register is four s-registers.
    if (Kind(inst) != kTargetRANone && ARMGetRegisterSize(inst) == kSize128Bit) {
      return "128-bit";
    }
  }
  return NULL;
}

// movt replaces the high half of the register movw wrote, and codegen ties
// the two together by making movt movw's dest.  The allocator gives them
// their own registers instead, and the emitter repeats the movw in movt's
// register when they differ.  Records each movw in |undo| for Restore.
static void UntieMovw(TargetGenerator* gen, Vector* undo) {
  FOR_EACH_INSTRUCTION(gen, inst) {
    if ((ARMOpcode)inst->opcode != ARM_OP(movw) || inst->dest == NULL ||
        (ARMOpcode)inst->dest->opcode != ARM_OP(movt)) {
      continue;
    }
    VectorAppend(undo, inst);
    VectorAppend(undo, inst->dest);
    inst->dest = NULL;
  }
}

// An argument list keeps the argument registers live up to the call.  Where
// it names the value moved into one rather than the register itself, it
// names the register.  Records each change in |undo| for Restore.
static void ReadArgumentRegisters(TargetGenerator* gen, Vector* undo) {
  FOR_EACH_INSTRUCTION(gen, inst) {
    if ((ARMOpcode)inst->opcode != ARM_OP(regarg)) {
      continue;
    }
    TargetInstruction* arg = inst->operand[1];
    if (arg == NULL || Kind(arg) != kTargetRAValue || arg->dest == NULL ||
        LocationKey(arg->dest) < 0) {
      continue;
    }
    VectorAppend(undo, inst);
    VectorAppend(undo, arg);
    TargetReplaceOperand(inst, 1, arg->dest);
  }
}

static void Restore(Vector* undo) {
  for (size_t i = 0; i + 1 < undo->length; i += 2) {
    TargetInstruction* inst = undo->value.p[i];
    TargetInstruction* was = undo->value.p[i + 1];
    if ((ARMOpcode)inst->opcode == ARM_OP(movw)) {
      inst->dest = was;
    } else {
      TargetReplaceOperand(inst, 1, was);
    }
  }
}

bool ARMAllocateRegistersByColoring(ARMRegisterAllocator* allocator) {
  ARMGenerator* g = allocator->g;
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
    ARMOpcode op = (ARMOpcode)dest->opcode;
    if (op == ARM_OP(resulti)) {
      ctx.returns_int = true;
    } else if (op == ARM_OP(resultf) || op == ARM_OP(resultd)) {
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
  // cost a save in the prologue.  r9 and r12 are scratch registers for the
  // codegen and emitter.
  TargetRAClass* ints = &desc.classes[kIntClass];
  for (int r = ARM_INT_TEMP_START; r <= ARM_INT_TEMP_END; r++) {
    ints->regs[ints->num_regs++] = r;
  }
  for (int r = ARM_INT_SAVED_START; r <= ARM_INT_SAVED_END; r++) {
    if (IsSavedAllocatable(r)) {
      ints->regs[ints->num_regs++] = r;
    }
  }
  TargetRAClass* floats = &desc.classes[kFloatClass];
  for (int r = 2 * ARM_NUM_FP_ARGS; r <= ARM_FP_TEMP_END; r += 2) {
    floats->regs[floats->num_regs++] = kFloatKeyBase + r;
  }
  for (int r = 0; r < ARM_NUM_FP_ARGS; r++) {
    floats->regs[floats->num_regs++] =
        kFloatKeyBase + 2 * (ARM_FP_ARG_START + r);
  }
  for (int r = ARM_FP_SAVED_START; r <= ARM_FP_SAVED_END; r += 2) {
    floats->regs[floats->num_regs++] = kFloatKeyBase + r;
  }

  Vector undo;
  VectorInit(&undo);
  UntieMovw(gen, &undo);
  ReadArgumentRegisters(gen, &undo);
  if (getenv("DAVECC_RA_CHECK") != NULL) {
    TargetRACheck(gen, &kInfo, NULL, stderr, 3);
  }
  if (!TargetAllocateRegisters(gen, &desc, NULL)) {
    Restore(&undo);
    VectorDestruct(&undo);
    return false;
  }
  VectorDestruct(&undo);
  FOR_EACH_INSTRUCTION(gen, inst) {
    if ((ARMOpcode)inst->opcode == ARM_OP(spill)) {
      inst->reg = inst->operand[0]->reg;
    }
  }
  allocator->current_spilled_region_size = ctx.spill_size;
  allocator->max_spilled_region_size = ctx.spill_size;
  g->colored = true;
  return true;
}
