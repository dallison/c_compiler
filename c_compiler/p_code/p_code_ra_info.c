//
//  p_code_ra_info.c
//  c_compiler
//
//  How the shared register allocator reads P-Code instructions, and the P-Code
//  side of the coloring allocator.  See target_ra_ir.h and target_reg_alloc.h.
//

#include <stdlib.h>
#include <string.h>
#include "p_code_codegen.h"
#include "p_code_machine.h"
#include "p_code_reg_alloc.h"
#include "target_basic_block.h"
#include "target_reg_alloc.h"

// Physical register keys: r0..r63 are 0..63, f0..f31 are 64..95 and d0..d31
// are 96..127.  The machine has 256 of each, but the keys must stay below
// TARGET_RA_FIXED_KEYS, and a function that needs more spills.
enum {
  kNumIntKeys = 64,
  kFloatKeyBase = 64,
  kDoubleKeyBase = 96,
  kNumFpKeys = 32,  // Of floats, and of doubles.
};

enum { kIntClass, kFloatClass, kDoubleClass };

static TargetRAKind KindOfOpcode(TargetInstruction* inst) {
  switch ((PCodeOpcode)inst->opcode) {
    case P_OP(tmp):
    case P_OP(fp):
    case P_OP(sp):
    case P_OP(ap):
    case P_OP(tp):
    case P_OP(resulti):
    case P_OP(resultf):
    case P_OP(resultd):
    case P_OP(structreturn):
      return kTargetRALocation;
    case P_OP(save):
    case P_OP(restore):
    case P_OP(symbol):
    case P_OP(literal):
    case P_OP(const8):
    case P_OP(const16):
    case P_OP(const32):
    case P_OP(const64):
    case P_OP(constf):
    case P_OP(constd):
    case P_OP(ret):
    case P_OP(label):
    case P_OP(asm):
    case P_OP(loc):
    case P_OP(named_label):
    case P_OP(ivarreg):
    case P_OP(fvarreg):
    case P_OP(decsp):
    case P_OP(incsp):
    case P_OP(push):
    case P_OP(pushf):
    case P_OP(pushd):
    case P_OP(pushx):
    case P_OP(pop):
    case P_OP(popf):
    case P_OP(popd):
    case P_OP(popx):
    case P_OP(stw):
    case P_OP(sth):
    case P_OP(stx):
    case P_OP(stf):
    case P_OP(std):
    case P_OP(stb):
    case P_OP(bnz):
    case P_OP(bz):
    case P_OP(bra):
    case P_OP(cbra):
    case P_OP(jmp):
    case P_OP(cjmp):
    case P_OP(spill):
      return kTargetRANone;
    default:
      return kTargetRAValue;
  }
}

// The allocator asks this of every instruction or operand it meets, and it
// depends only on the opcode, so it is kept by opcode once found: the answer
// plus one, or 0 if not yet known.
static signed char opcode_kinds[P_OP(reload) + 1];

static TargetRAKind Kind(TargetInstruction* inst) {
  unsigned op = (unsigned)inst->opcode;
  if (op > (unsigned)P_OP(reload)) {
    return KindOfOpcode(inst);
  }
  if (opcode_kinds[op] == 0) {
    opcode_kinds[op] = (signed char)(KindOfOpcode(inst) + 1);
  }
  return (TargetRAKind)(opcode_kinds[op] - 1);
}

static int LocationKey(TargetInstruction* inst) {
  switch ((PCodeOpcode)inst->opcode) {
    case P_OP(fp):
      return PCODE_FP_REG;
    case P_OP(sp):
      return PCODE_SP_REG;
    case P_OP(ap):
      return PCODE_AP_REG;
    case P_OP(tp):
      return PCODE_TP_REG;
    case P_OP(resulti):
    case P_OP(structreturn):
      return PCODE_INT_RETURN_REG;
    case P_OP(resultf):
      return kFloatKeyBase + PCODE_FLOAT_RETURN_REG;
    case P_OP(resultd):
      return kDoubleKeyBase + PCODE_DOUBLE_RETURN_REG;
    default:
      return -1;
  }
}

static int WrittenOperand(TargetInstruction* inst) {
  (void)inst;
  return -1;
}

static bool IsCall(PCodeOpcode op) {
  return op == P_OP(call) || op == P_OP(callf) || op == P_OP(calld) ||
         op == P_OP(rcall) || op == P_OP(rcallf) || op == P_OP(rcalld);
}

// r26..r34 are the unsaved temporaries, sp, fp, pc, ap and tp.
static bool IsAllocatableInt(int r) {
  return r < PCODE_TMP1_REG || (r > PCODE_TP_REG && r < kNumIntKeys);
}

static bool IsReturnKey(int key) {
  return key == PCODE_INT_RETURN_REG ||
         key == kFloatKeyBase + PCODE_FLOAT_RETURN_REG ||
         key == kDoubleKeyBase + PCODE_DOUBLE_RETURN_REG;
}

// Every allocatable key, or every one but the return registers.
static int AllocatableKeys(bool with_return_regs, int* keys, int max_keys) {
  int n = 0;
  for (int r = 0; r < kNumIntKeys && n < max_keys; r++) {
    if (IsAllocatableInt(r) && (with_return_regs || !IsReturnKey(r))) {
      keys[n++] = r;
    }
  }
  for (int r = 0; r < kNumFpKeys; r++) {
    if (r == PCODE_FLOAT_RETURN_REG && !with_return_regs) {
      continue;
    }
    if (n + 2 > max_keys) {
      break;
    }
    keys[n++] = kFloatKeyBase + r;
    keys[n++] = kDoubleKeyBase + r;
  }
  return n;
}

// The prologue saves every register a function uses but the return
// registers, so a call overwrites only those, and the epilogue reloads the
// others.  A landing pad reloads the saved registers from the prologue's
// slots, so it overwrites them all.
static int Clobbers(TargetInstruction* inst, int* keys, int max_keys) {
  PCodeOpcode op = (PCodeOpcode)inst->opcode;
  if (IsCall(op)) {
    static const int kReturnKeys[] = {
        PCODE_INT_RETURN_REG, kFloatKeyBase + PCODE_FLOAT_RETURN_REG,
        kDoubleKeyBase + PCODE_DOUBLE_RETURN_REG};
    int n = 0;
    for (int i = 0; i < 3 && n < max_keys; i++) {
      keys[n++] = kReturnKeys[i];
    }
    return n;
  }
  if (op == P_OP(restore)) {
    return AllocatableKeys(false, keys, max_keys);
  }
  if (op == P_OP(label) && (inst->flags & TARGET_INST_EXCEPTION_LANDING) != 0) {
    return AllocatableKeys(true, keys, max_keys);
  }
  return 0;
}

static bool ReadsThrough(TargetInstruction* operand) {
  (void)operand;
  return false;
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

// ---------------------------------------------------------------------------
// Coloring allocator.

typedef struct {
  PCodeRegisterAllocator* allocator;
  // The class of each merge tmp, from the instructions that write it.
  TargetRAIndex tmp_classes;
  bool returns_int;
  bool returns_float;
  bool returns_double;
  int frame_size;  // Before the spill slots, which go below it.
  int spill_size;
} ColoringContext;

// The function being allocated, for RegClass, which has no context.
static ColoringContext* current;

static int ClassOfType(PCodeRegisterType type) {
  switch (type) {
    case kPCodeRegTypeFloat:
      return kFloatClass;
    case kPCodeRegTypeDouble:
      return kDoubleClass;
    default:
      return kIntClass;
  }
}

static int RegClass(TargetInstruction* inst) {
  switch ((PCodeOpcode)inst->opcode) {
    case P_OP(reload):
      return RegClass(inst->operand[0]->operand[0]);
    case P_OP(tmp): {
      int c = current == NULL ? -1
                              : TargetRAIndexFind(&current->tmp_classes, inst);
      if (c >= 0) {
        return c;
      }
      break;
    }
    default:
      break;
  }
  return ClassOfType(PCodeInstructionRegisterType(inst));
}

static int FixedValueKey(TargetInstruction* inst) {
  switch ((PCodeOpcode)inst->opcode) {
    case P_OP(call):
    case P_OP(rcall):
      return PCODE_INT_RETURN_REG;
    case P_OP(callf):
    case P_OP(rcallf):
      return kFloatKeyBase + PCODE_FLOAT_RETURN_REG;
    case P_OP(calld):
    case P_OP(rcalld):
      return kDoubleKeyBase + PCODE_DOUBLE_RETURN_REG;
    default:
      return -1;
  }
}

static bool DefinesRegister(TargetInstruction* inst) {
  (void)inst;
  return true;
}

static bool IsMoveOpcode(PCodeOpcode op) {
  return op == P_OP(mov) || op == P_OP(movf) || op == P_OP(movd);
}

// The moves copy the whole register.
static TargetRACopyKind CopyKind(TargetInstruction* inst) {
  TargetInstruction* src = inst->operand[0];
  if (!IsMoveOpcode((PCodeOpcode)inst->opcode) || src == NULL ||
      inst->operand[1] != NULL || Kind(src) == kTargetRANone ||
      RegClass(inst) != RegClass(src)) {
    return kTargetRANotCopy;
  }
  return kTargetRACopy;
}

static int ImplicitUses(void* ctx, TargetInstruction* inst, int* keys,
                        int max_keys) {
  ColoringContext* c = ctx;
  int n = 0;
  if ((PCodeOpcode)inst->opcode != P_OP(ret)) {
    return 0;
  }
  if (c->returns_int && n < max_keys) {
    keys[n++] = PCODE_INT_RETURN_REG;
  }
  if (c->returns_float && n < max_keys) {
    keys[n++] = kFloatKeyBase + PCODE_FLOAT_RETURN_REG;
  }
  if (c->returns_double && n < max_keys) {
    keys[n++] = kDoubleKeyBase + PCODE_DOUBLE_RETURN_REG;
  }
  return n;
}

static int ForbiddenKeys(TargetInstruction* inst, int* keys, int max_keys) {
  (void)inst;
  (void)keys;
  (void)max_keys;
  return 0;
}

static TargetInstruction* CreateCopy(void* ctx, TargetInstruction* value,
                                     TargetInstruction* dest) {
  (void)ctx;
  static const PCodeOpcode kMoves[] = {P_OP(mov), P_OP(movf), P_OP(movd)};
  TargetInstruction* copy =
      TargetNewInstruction1((TargetOpcode)kMoves[RegClass(value)], value);
  copy->dest = dest;
  if (dest != NULL && value->dest == dest) {
    value->dest = NULL;
  }
  return copy;
}

static int NewSlot(void* ctx, TargetInstruction* value) {
  (void)value;
  ColoringContext* c = ctx;
  int slot = c->spill_size;
  c->spill_size += 8;
  return slot;
}

static TargetInstruction* CreateSpill(void* ctx, TargetInstruction* value,
                                      int slot) {
  ColoringContext* c = ctx;
  int offset = -(c->frame_size + 8 + slot);
  return TargetNewInstruction2(
      (TargetOpcode)P_OP(spill), value,
      TargetGetIntConstant(&c->allocator->pcode->base, NULL, kTargetType32Bit,
                           offset));
}

static TargetInstruction* CreateReload(void* ctx, TargetInstruction* spill) {
  (void)ctx;
  return TargetNewInstruction1((TargetOpcode)P_OP(reload), spill);
}

static void Assign(void* ctx, TargetInstruction* inst, int key) {
  ColoringContext* c = ctx;
  PCodeRegisterAllocator* allocator = c->allocator;
  PCodeRegister* reg;
  BitSet* used;
  if (key >= kDoubleKeyBase) {
    reg = &allocator->double_regs[key - kDoubleKeyBase];
    used = &allocator->used_double_regs;
  } else if (key >= kFloatKeyBase) {
    reg = &allocator->float_regs[key - kFloatKeyBase];
    used = &allocator->used_float_regs;
  } else {
    reg = &allocator->int_regs[key];
    used = &allocator->used_int_regs;
  }
  inst->reg = &reg->base;
  // The return registers and the fixed ones are not saved.
  if (!reg->base.reserved) {
    BitSetInsert(used, (size_t)reg->base.num);
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

// These only come from allocation.
static const char* Unsupported(PCodeGenerator* pcode) {
  FOR_EACH_INSTRUCTION(&pcode->base, inst) {
    PCodeOpcode op = (PCodeOpcode)inst->opcode;
    switch (op) {
      case P_OP(spill):
      case P_OP(reload):
        return PCodeOpcodeName((int)op);
      default:
        break;
    }
  }
  return NULL;
}

// Makes room for the spill slots below the frame the prologue allocates.
static void GrowFrame(PCodeGenerator* pcode, int spill_size) {
  TargetGenerator* gen = &pcode->base;
  int32_t old_size = gen->stack_frame_size;
  gen->stack_frame_size = old_size + spill_size;
  TargetInstruction* setup = pcode->frame_setup;
  TargetInstruction* next = TargetNext(setup);
  if (old_size > 0 && next != NULL &&
      (PCodeOpcode)next->opcode == P_OP(decsp)) {
    TargetReplaceOperand(next, 0,
                         TargetGetIntConstant(gen, NULL, kTargetType32Bit,
                                              gen->stack_frame_size));
    return;
  }
  TargetInstruction* decsp = TargetNewInstruction1(
      (TargetOpcode)P_OP(decsp),
      TargetGetIntConstant(gen, NULL, kTargetType32Bit, spill_size));
  TargetBasicBlockEmitAfter(gen, setup->block, decsp, setup);
}

static void CannotAllocate(TargetGenerator* gen, const char* reason) {
  fprintf(stderr, "Internal error: cannot allocate registers for %s: %s\n",
          gen->function_name.value, reason);
  abort();
}

void PCodeAllocateRegistersByColoring(PCodeRegisterAllocator* allocator) {
  PCodeGenerator* pcode = allocator->pcode;
  TargetGenerator* gen = &pcode->base;
  const char* reason = Unsupported(pcode);
  if (reason != NULL) {
    CannotAllocate(gen, reason);
  }

  ColoringContext ctx = {.allocator = allocator,
                         .frame_size = gen->stack_frame_size};
  TargetRAIndexInit(&ctx.tmp_classes, 16);
  FOR_EACH_INSTRUCTION(gen, inst) {
    TargetInstruction* dest = inst->dest;
    if (dest == NULL) {
      continue;
    }
    switch ((PCodeOpcode)dest->opcode) {
      case P_OP(resulti):
        ctx.returns_int = true;
        break;
      case P_OP(resultf):
        ctx.returns_float = true;
        break;
      case P_OP(resultd):
        ctx.returns_double = true;
        break;
      case P_OP(tmp):
        if (TargetRAIndexFind(&ctx.tmp_classes, dest) < 0) {
          TargetRAIndexInsert(&ctx.tmp_classes, dest, RegClass(inst));
        }
        break;
      default:
        break;
    }
  }
  ColoringContext* saved = current;
  current = &ctx;

  TargetRegAllocDesc desc = {
      .info = &kInfo,
      .ctx = &ctx,
      .num_classes = 3,
      .reg_class = RegClass,
      .fixed_value_key = FixedValueKey,
      .defines_register = DefinesRegister,
      .implicit_uses = ImplicitUses,
      .forbidden_keys = ForbiddenKeys,
      .create_copy = CreateCopy,
      .new_slot = NewSlot,
      .create_spill = CreateSpill,
      .create_reload = CreateReload,
      .assign = Assign,
  };
  // The return registers first, since the prologue saves every other register
  // a function uses, then the rest in order so that few are used.
  TargetRAClass* ints = &desc.classes[kIntClass];
  for (int r = 0; r < kNumIntKeys; r++) {
    if (IsAllocatableInt(r)) {
      ints->regs[ints->num_regs++] = r;
    }
  }
  TargetRAClass* floats = &desc.classes[kFloatClass];
  TargetRAClass* doubles = &desc.classes[kDoubleClass];
  for (int r = 0; r < kNumFpKeys; r++) {
    floats->regs[floats->num_regs++] = kFloatKeyBase + r;
    doubles->regs[doubles->num_regs++] = kDoubleKeyBase + r;
  }

  if (getenv("DAVECC_RA_CHECK") != NULL) {
    TargetRACheck(gen, &kInfo, NULL, stderr, 3);
  }
  bool colored = TargetAllocateRegisters(gen, &desc, NULL);
  current = saved;
  TargetRAIndexDestruct(&ctx.tmp_classes);
  if (!colored) {
    CannotAllocate(gen, "the shared allocator gave up");
  }
  // A write to a spilled location goes to the value's own register, which the
  // allocator stores after it.
  FOR_EACH_INSTRUCTION(gen, inst) {
    if (inst->dest != NULL && inst->dest->reg == NULL) {
      inst->dest = NULL;
    }
  }
  if (ctx.spill_size > 0) {
    // Spill slots are addressed from fp.
    if (pcode->frame_setup == NULL || pcode->frame_setup->block == NULL) {
      CannotAllocate(gen, "spills without a frame");
    }
    GrowFrame(pcode, ctx.spill_size);
  }
}
