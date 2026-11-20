//
//  risc_v32_ra_info.c
//  c_compiler
//
//  How the shared register allocator code reads RISC-V32 instructions, and the
//  RISC-V32 side of the coloring allocator.  See target_ra_ir.h and
//  target_reg_alloc.h.
//

#include <stdlib.h>
#include <string.h>
#include "risc_v32_reg_alloc.h"
#include "risc_v32_codegen.h"
#include "risc_v32_machine.h"
#include "target_basic_block.h"
#include "target_reg_alloc.h"

// Physical register keys: x0..x31 are 0..31, f0..f31 are 64..95.
enum { kFloatKeyBase = 64 };

enum { kIntClass, kFloatClass };

enum { kThreadPointerReg = 4 };

static TargetRAKind KindOfOpcode(TargetInstruction* inst) {
  switch ((RV32Opcode)inst->opcode) {
    case RV32_OP(ivarreg):
    case RV32_OP(fvarreg):
    case RV32_OP(tmp):
    case RV32_OP(structreturn):
    case RV32_OP(resulti):
    case RV32_OP(resultf):
    case RV32_OP(resultd):
    case RV32_OP(a0):
    case RV32_OP(a1):
    case RV32_OP(a2):
    case RV32_OP(a3):
    case RV32_OP(a4):
    case RV32_OP(a5):
    case RV32_OP(a6):
    case RV32_OP(a7):
    case RV32_OP(fa0):
    case RV32_OP(fa1):
    case RV32_OP(fa2):
    case RV32_OP(fa3):
    case RV32_OP(fa4):
    case RV32_OP(fa5):
    case RV32_OP(fa6):
    case RV32_OP(fa7):
    case RV32_OP(fp):
    case RV32_OP(sp):
    case RV32_OP(tp):
    case RV32_OP(x0):
    case RV32_OP(t0):
    case RV32_OP(t1):
    case RV32_OP(t2):
      return kTargetRALocation;
    case RV32_OP(call):
    case RV32_OP(rcall):
    case RV32_OP(callf):
    case RV32_OP(rcallf):
      // A call's own register holds its result.
      return kTargetRAValue;
    case RV32_OP(literal):
      return kTargetRANone;
    default:
      return RV32IsExpression(inst) ? kTargetRAValue : kTargetRANone;
  }
}

// The allocator asks these of every instruction or operand it meets, and they
// depend only on the opcode, so they are kept by opcode once found: each is
// the answer plus one, or 0 if not yet known.
static signed char opcode_kinds[RV32_OP(reload) + 1];
static signed char opcode_defines[RV32_OP(reload) + 1];

static TargetRAKind Kind(TargetInstruction* inst) {
  unsigned op = (unsigned)inst->opcode;
  if (op > (unsigned)RV32_OP(reload)) {
    return KindOfOpcode(inst);
  }
  if (opcode_kinds[op] == 0) {
    opcode_kinds[op] = (signed char)(KindOfOpcode(inst) + 1);
  }
  return (TargetRAKind)(opcode_kinds[op] - 1);
}

static int LocationKey(TargetInstruction* inst) {
  RV32Opcode op = (RV32Opcode)inst->opcode;
  if (op >= RV32_OP(a0) && op <= RV32_OP(a7)) {
    return RV32_INT_ARG_START + (int)(op - RV32_OP(a0));
  }
  if (op >= RV32_OP(fa0) && op <= RV32_OP(fa7)) {
    return kFloatKeyBase + RV32_FP_ARG_START + (int)(op - RV32_OP(fa0));
  }
  switch (op) {
    case RV32_OP(x0):
      return RV32_INT_ZERO_REG;
    case RV32_OP(sp):
      return RV32_SP_REG;
    case RV32_OP(tp):
      return kThreadPointerReg;
    case RV32_OP(fp):
      return RV32_FP_REG;
    case RV32_OP(t0):
    case RV32_OP(t1):
    case RV32_OP(t2):
      return RV32_INT_TEMP_START_1 + (int)(op - RV32_OP(t0));
    case RV32_OP(resulti):
      return RV32_INT_RETURN_REG;
    case RV32_OP(resultf):
    case RV32_OP(resultd):
      return kFloatKeyBase + RV32_FLOAT_RETURN_REG;
    default:
      return -1;
  }
}

static bool IsMoveOpcode(RV32Opcode op) {
  return op == RV32_OP(mv) || op == RV32_OP(fmv_s) || op == RV32_OP(fmv_d);
}

// `mv loc, src` with no dest writes its first operand.
static int WrittenOperand(TargetInstruction* inst) {
  if (IsMoveOpcode((RV32Opcode)inst->opcode) && inst->dest == NULL &&
      inst->operand[0] != NULL && inst->operand[1] != NULL &&
      Kind(inst->operand[0]) == kTargetRALocation) {
    return 0;
  }
  return -1;
}

static bool IsSavedInt(int r) {
  return r == RV32_INT_SAVED_END_1 ||
         (r >= RV32_INT_SAVED_START_2 && r <= RV32_INT_SAVED_END_2);
}

static bool IsSavedFloat(int r) {
  return (r >= RV32_FP_SAVED_START_1 && r <= RV32_FP_SAVED_END_1) ||
         (r >= RV32_FP_SAVED_START_2 && r <= RV32_FP_SAVED_END_2);
}

// A call overwrites ra and the temporary and argument registers.  The
// epilogue reloads ra and the callee-saved registers, so a value read after
// it, such as the target of an indirect tail call, must be in neither.
static int Clobbers(TargetInstruction* inst, int* keys, int max_keys) {
  int n = 0;
  bool restore = (RV32Opcode)inst->opcode == RV32_OP(restore);
  if ((RV32IsCall(inst) || restore) && n < max_keys) {
    keys[n++] = RV32_RET_REG;
  }
  if (RV32IsCall(inst)) {
    for (int r = 0; r < RV32_NUM_INT_REGS && n < max_keys; r++) {
      if ((r >= RV32_INT_TEMP_START_1 && r <= RV32_INT_TEMP_END_1) ||
          (r >= RV32_INT_ARG_START && r <= RV32_INT_ARG_END) ||
          (r >= RV32_INT_TEMP_START_2 && r <= RV32_INT_TEMP_END_2)) {
        keys[n++] = r;
      }
    }
    for (int r = 0; r < RV32_NUM_FLOAT_REGS && n < max_keys; r++) {
      if (!IsSavedFloat(r)) {
        keys[n++] = kFloatKeyBase + r;
      }
    }
    return n;
  }
  if (restore) {
    for (int r = 0; r < RV32_NUM_INT_REGS && n < max_keys; r++) {
      if (IsSavedInt(r)) {
        keys[n++] = r;
      }
    }
    for (int r = 0; r < RV32_NUM_FLOAT_REGS && n < max_keys; r++) {
      if (IsSavedFloat(r)) {
        keys[n++] = kFloatKeyBase + r;
      }
    }
  }
  return n;
}

static bool ReadsThrough(TargetInstruction* operand) {
  return (RV32Opcode)operand->opcode == RV32_OP(regarg);
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

const TargetRAInfo* RV32RAInfo(void) { return &kInfo; }

// ---------------------------------------------------------------------------
// Coloring allocator.

typedef struct {
  RV32RegisterAllocator* allocator;
  bool returns_int;
  bool returns_float;
  int spill_size;
} ColoringContext;

// The atomic pseudos keep their size and memory orders in flag bits that
// other instructions use for other things.  Their results are all integers.
static bool IsAtomic(TargetInstruction* inst) {
  RV32Opcode op = (RV32Opcode)inst->opcode;
  return op >= RV32_OP(atomic_load) && op <= RV32_OP(atomic_fence);
}

static int RegClass(TargetInstruction* inst) {
  switch ((RV32Opcode)inst->opcode) {
    case RV32_OP(reload):
      return RegClass(inst->operand[0]->operand[0]);
    case RV32_OP(call):
    case RV32_OP(rcall):
    case RV32_OP(feq_s):
    case RV32_OP(flt_s):
    case RV32_OP(fle_s):
    case RV32_OP(feq_d):
    case RV32_OP(flt_d):
    case RV32_OP(fle_d):
      return kIntClass;
    case RV32_OP(callf):
    case RV32_OP(rcallf):
    case RV32_OP(resultf):
    case RV32_OP(resultd):
      return kFloatClass;
    case RV32_OP(mv):
    case RV32_OP(fmv_s):
    case RV32_OP(fmv_d):
      // A move into a float temporary is still `mv`.  Its register is the
      // destination's, which is what the dominator walk assigns it.
      if (inst->dest != NULL) {
        return RegClass(inst->dest);
      }
      if (inst->operand[0] != NULL && inst->operand[1] == NULL) {
        return RegClass(inst->operand[0]);
      }
      return RV32ResultRegisterType(inst) == kRV32RegTypeFloat ? kFloatClass
                                                               : kIntClass;
    default:
      if (IsAtomic(inst)) {
        return kIntClass;
      }
      return RV32ResultRegisterType(inst) == kRV32RegTypeFloat ? kFloatClass
                                                           : kIntClass;
  }
}

static int FixedValueKey(TargetInstruction* inst) {
  switch ((RV32Opcode)inst->opcode) {
    case RV32_OP(call):
    case RV32_OP(rcall):
      return RV32_INT_RETURN_REG;
    case RV32_OP(callf):
    case RV32_OP(rcallf):
      return kFloatKeyBase + RV32_FLOAT_RETURN_REG;
    default:
      return -1;
  }
}

static bool OpcodeDefinesRegister(TargetInstruction* inst) {
  switch ((RV32Opcode)inst->opcode) {
    case RV32_OP(call):
    case RV32_OP(rcall):
    case RV32_OP(callf):
    case RV32_OP(rcallf):
      return true;
    case RV32_OP(fence):
    case RV32_OP(fence_i):
    case RV32_OP(ecall):
    case RV32_OP(ebreak):
    case RV32_OP(nop):
      return false;
    default:
      return RV32GeneratesOutput(inst);
  }
}

static bool DefinesRegister(TargetInstruction* inst) {
  unsigned op = (unsigned)inst->opcode;
  if (op > (unsigned)RV32_OP(reload)) {
    return OpcodeDefinesRegister(inst);
  }
  if (opcode_defines[op] == 0) {
    opcode_defines[op] = (signed char)(OpcodeDefinesRegister(inst) + 1);
  }
  return opcode_defines[op] - 1;
}

// A move copies its whole register: 32 bits for an integer, the whole F
// register for a float.
static TargetRACopyKind CopyKind(TargetInstruction* inst) {
  TargetInstruction* src = inst->operand[0];
  if (!IsMoveOpcode((RV32Opcode)inst->opcode) || src == NULL ||
      inst->operand[1] != NULL || Kind(src) == kTargetRANone ||
      RegClass(inst) != RegClass(src)) {
    return kTargetRANotCopy;
  }
  return kTargetRACopy;
}

static int ArgumentKeys(int* keys, int max_keys) {
  int n = 0;
  for (int r = 0; r < RV32_NUM_INT_ARGS && n < max_keys; r++) {
    keys[n++] = RV32_INT_ARG_START + r;
  }
  for (int r = 0; r < RV32_NUM_FP_ARGS && n < max_keys; r++) {
    keys[n++] = kFloatKeyBase + RV32_FP_ARG_START + r;
  }
  return n;
}

static int ImplicitUses(void* ctx, TargetInstruction* inst, int* keys,
                        int max_keys) {
  ColoringContext* c = ctx;
  int n = 0;
  switch ((RV32Opcode)inst->opcode) {
    case RV32_OP(save):
      // The prologue stores incoming argument registers to the frame.
      return ArgumentKeys(keys, max_keys);
    case RV32_OP(jr):
      // Only tail calls jump through a register, and their arguments are not
      // operands.
      return ArgumentKeys(keys, max_keys);
    case RV32_OP(j):
      if (inst->operand[0] != NULL &&
          (RV32Opcode)inst->operand[0]->opcode == RV32_OP(symbol)) {
        return ArgumentKeys(keys, max_keys);
      }
      return 0;
    case RV32_OP(ret):
      if (c->returns_int && n < max_keys) {
        keys[n++] = RV32_INT_RETURN_REG;
      }
      if (c->returns_float && n < max_keys) {
        keys[n++] = kFloatKeyBase + RV32_FLOAT_RETURN_REG;
      }
      return n;
    default:
      return 0;
  }
}

static int ForbiddenKeys(TargetInstruction* inst, int* keys, int max_keys) {
  (void)inst;
  (void)keys;
  (void)max_keys;
  return 0;
}

// On a mismatch this stores the loaded value through `expected` after the
// result register holds it.
static bool EarlyClobber(TargetInstruction* inst) {
  return (RV32Opcode)inst->opcode == RV32_OP(atomic_compare_exchange_n);
}

static TargetInstruction* CreateCopy(void* ctx, TargetInstruction* value,
                                     TargetInstruction* dest) {
  (void)ctx;
  bool fp = RegClass(value) == kFloatClass;
  TargetInstruction* copy = TargetNewInstruction1(
      (TargetOpcode)(fp ? RV32_OP(fmv_d) : RV32_OP(mv)), value);
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
  Vector* var_regs = &c->allocator->rv->var_regs;
  for (size_t i = 0; i < var_regs->length; i++) {
    RegisterVariable* var = var_regs->value.p[i];
    if (var->inst == inst) {
      return var;
    }
  }
  return NULL;
}

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
  return TargetNewInstruction2(
      (TargetOpcode)RV32_OP(spill), value,
      TargetGetIntConstant(&c->allocator->rv->base, NULL, kTargetType32Bit,
                           slot));
}

static TargetInstruction* CreateReload(void* ctx, TargetInstruction* spill) {
  (void)ctx;
  return TargetNewInstruction1((TargetOpcode)RV32_OP(reload), spill);
}

// Variable registers are TargetSymbols; this one stands for no variable.
static TargetInstruction* CreateLocation(void* ctx, TargetInstruction* like) {
  ColoringContext* c = ctx;
  bool fp = RegClass(like) == kFloatClass;
  TargetSymbol* location = calloc(1, sizeof(TargetSymbol));
  TargetInitInstruction(
      &location->base,
      (TargetOpcode)(fp ? RV32_OP(fvarreg) : RV32_OP(ivarreg)));
  TargetTrackOrphanInstruction(&c->allocator->rv->base, &location->base);
  RegisterVariable* var = FindVariable(c, like);
  if (var != NULL) {
    var->split_location = &location->base;
  }
  return &location->base;
}

static TargetInstruction* CreateLabel(void* ctx) {
  (void)ctx;
  return TargetNewInstruction((TargetOpcode)RV32_OP(label));
}

static TargetInstruction* CreateJump(void* ctx, TargetInstruction* label) {
  (void)ctx;
  return TargetNewInstruction1((TargetOpcode)RV32_OP(j), label);
}

static void Assign(void* ctx, TargetInstruction* inst, int key) {
  ColoringContext* c = ctx;
  RV32RegisterAllocator* allocator = c->allocator;
  if (key >= kFloatKeyBase) {
    int num = key - kFloatKeyBase;
    inst->reg = &allocator->float_regs[num].base;
    if (IsSavedFloat(num)) {
      BitSetInsert(&allocator->used_float_regs, (size_t)num);
    }
    return;
  }
  inst->reg = &allocator->int_regs[key].base;
  if (IsSavedInt(key)) {
    BitSetInsert(&allocator->used_int_regs, (size_t)key);
  }
  if (key == RV32_RET_REG) {
    // The prologue saves ra only outside leaf functions.
    allocator->rv->not_leaf = true;
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

static const char* Unsupported(RV32Generator* rv) {
  FOR_EACH_INSTRUCTION(&rv->base, inst) {
    RV32Opcode op = (RV32Opcode)inst->opcode;
    switch (op) {
      case RV32_OP(asm):
      case RV32_OP(spill):
      case RV32_OP(reload):
        return RV32OpcodeName((int)op);
      default:
        break;
    }
  }
  return NULL;
}

// The allocator reads `mv loc, src` as `mv src -> loc`.  t2 only keeps a call
// target away from the argument registers for the dominator walk, so a move
// into it becomes a plain copy that the allocator places itself.  Records
// each move and the location it wrote in |moves| for RestoreWrittenMoves.
static void RewriteWrittenMoves(TargetGenerator* gen, Vector* moves) {
  FOR_EACH_INSTRUCTION(gen, inst) {
    if (WrittenOperand(inst) != 0) {
      continue;
    }
    TargetInstruction* location = inst->operand[0];
    VectorAppend(moves, inst);
    VectorAppend(moves, location);
    TargetReplaceOperand(inst, 0, inst->operand[1]);
    TargetReplaceOperand(inst, 1, NULL);
    if ((RV32Opcode)location->opcode != RV32_OP(t2)) {
      inst->dest = location;
    }
  }
}

static void RestoreWrittenMoves(Vector* moves) {
  for (size_t i = 0; i + 1 < moves->length; i += 2) {
    TargetInstruction* inst = moves->value.p[i];
    TargetInstruction* location = moves->value.p[i + 1];
    TargetReplaceOperand(inst, 1, inst->operand[0]);
    TargetReplaceOperand(inst, 0, location);
    inst->dest = NULL;
  }
}

static bool HasAtomics(TargetGenerator* gen) {
  FOR_EACH_INSTRUCTION(gen, inst) {
    if (IsAtomic(inst)) {
      return true;
    }
  }
  return false;
}

bool RV32AllocateRegistersByColoring(RV32RegisterAllocator* allocator) {
  RV32Generator* rv = allocator->rv;
  TargetGenerator* gen = &rv->base;
  const char* reason = Unsupported(rv);
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
    RV32Opcode op = (RV32Opcode)dest->opcode;
    if (op == RV32_OP(resulti)) {
      ctx.returns_int = true;
    } else if (op == RV32_OP(resultf) || op == RV32_OP(resultd)) {
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
  // cost a save in the prologue.  t0 and gp are emitter scratch registers,
  // and the atomic pseudos expand with t1-t6 as scratch.  ra costs a save in
  // a leaf function, like a callee-saved register.
  TargetRAClass* ints = &desc.classes[kIntClass];
  if (!HasAtomics(gen)) {
    for (int r = RV32_INT_TEMP_START_1 + 1; r <= RV32_INT_TEMP_END_1; r++) {
      ints->regs[ints->num_regs++] = r;
    }
    for (int r = RV32_INT_TEMP_START_2; r <= RV32_INT_TEMP_END_2; r++) {
      ints->regs[ints->num_regs++] = r;
    }
  }
  for (int r = RV32_INT_ARG_START; r <= RV32_INT_ARG_END; r++) {
    ints->regs[ints->num_regs++] = r;
  }
  ints->regs[ints->num_regs++] = RV32_RET_REG;
  for (int r = 0; r < RV32_NUM_INT_REGS; r++) {
    if (IsSavedInt(r)) {
      ints->regs[ints->num_regs++] = r;
    }
  }
  TargetRAClass* floats = &desc.classes[kFloatClass];
  for (int r = RV32_FP_TEMP_START_1; r <= RV32_FP_TEMP_END_1; r++) {
    floats->regs[floats->num_regs++] = kFloatKeyBase + r;
  }
  for (int r = RV32_FP_TEMP_START_2; r <= RV32_FP_TEMP_END_2; r++) {
    floats->regs[floats->num_regs++] = kFloatKeyBase + r;
  }
  for (int r = RV32_FP_ARG_START; r <= RV32_FP_ARG_END; r++) {
    floats->regs[floats->num_regs++] = kFloatKeyBase + r;
  }
  for (int r = 0; r < RV32_NUM_FLOAT_REGS; r++) {
    if (IsSavedFloat(r)) {
      floats->regs[floats->num_regs++] = kFloatKeyBase + r;
    }
  }

  Vector moves;
  VectorInit(&moves);
  RewriteWrittenMoves(gen, &moves);
  if (getenv("DAVECC_RA_CHECK") != NULL) {
    TargetRACheck(gen, &kInfo, NULL, stderr, 3);
  }
  if (!TargetAllocateRegisters(gen, &desc, NULL)) {
    RestoreWrittenMoves(&moves);
    VectorDestruct(&moves);
    return false;
  }
  VectorDestruct(&moves);
  FOR_EACH_INSTRUCTION(gen, inst) {
    if ((RV32Opcode)inst->opcode == RV32_OP(spill)) {
      inst->reg = inst->operand[0]->reg;
    }
  }
  allocator->current_spilled_region_size = ctx.spill_size;
  allocator->max_spilled_region_size = ctx.spill_size;
  rv->colored = true;
  return true;
}
