//
//  x86_ra_info.c
//  c_compiler
//
//  How the shared register allocator code reads x86-64 instructions, and the
//  x86-64 side of the coloring allocator.  See target_ra_ir.h and
//  target_reg_alloc.h.
//

#include <stdio.h>
#include <stdlib.h>
#include "x86_reg_alloc.h"
#include "x86_codegen.h"
#include "x86_machine.h"
#include "target_basic_block.h"
#include "target_reg_alloc.h"

// Physical register keys: the integer registers by hardware number (rax=0,
// rcx=1, ... r15=15) and xmm0..xmm15 as 64..79.
enum { kFloatKeyBase = 64 };

enum { kIntClass, kFloatClass };

static TargetRAKind KindOfOpcode(TargetInstruction* inst) {
  switch ((X86Opcode)inst->opcode) {
    case X86_OP(ivarreg):
    case X86_OP(fvarreg):
    case X86_OP(tmp):
    case X86_OP(structreturn):
    case X86_OP(resulti):
    case X86_OP(resulth):
    case X86_OP(resultf):
    case X86_OP(resultd):
    case X86_OP(resultv):
    case X86_OP(a0):
    case X86_OP(a1):
    case X86_OP(a2):
    case X86_OP(a3):
    case X86_OP(a4):
    case X86_OP(a5):
    case X86_OP(a6):
    case X86_OP(a7):
    case X86_OP(fa0):
    case X86_OP(fa1):
    case X86_OP(fa2):
    case X86_OP(fa3):
    case X86_OP(fa4):
    case X86_OP(fa5):
    case X86_OP(fa6):
    case X86_OP(fa7):
    case X86_OP(fp):
    case X86_OP(sp):
    case X86_OP(t0):
    case X86_OP(asm_bx):
      return kTargetRALocation;
    case X86_OP(call):
    case X86_OP(rcall):
    case X86_OP(callf):
    case X86_OP(rcallf):
      // A call's own register holds its result.
      return kTargetRAValue;
    case X86_OP(x0):
    case X86_OP(literal):
    case X86_OP(nrvoval):
      return kTargetRANone;
    default:
      return X86IsExpression(inst) ? kTargetRAValue : kTargetRANone;
  }
}

// The allocator asks these of every instruction or operand it meets, and they
// depend only on the opcode, so they are kept by opcode once found: each is
// the answer plus one, or 0 if not yet known.
static signed char opcode_kinds[X86_OP(reload) + 1];
static signed char opcode_defines[X86_OP(reload) + 1];

static TargetRAKind Kind(TargetInstruction* inst) {
  unsigned op = (unsigned)inst->opcode;
  if (op > (unsigned)X86_OP(reload)) {
    return KindOfOpcode(inst);
  }
  if (opcode_kinds[op] == 0) {
    opcode_kinds[op] = (signed char)(KindOfOpcode(inst) + 1);
  }
  return (TargetRAKind)(opcode_kinds[op] - 1);
}

static const int kIntArgKeys[] = {X86_REG_RDI, X86_REG_RSI, X86_REG_RDX,
                                  X86_REG_RCX, X86_REG_R8,  X86_REG_R9};
enum { kNumIntArgKeys = sizeof(kIntArgKeys) / sizeof(kIntArgKeys[0]) };
enum { kNumFloatArgKeys = 8 };

static int LocationKey(TargetInstruction* inst) {
  X86Opcode op = (X86Opcode)inst->opcode;
  if (op >= X86_OP(a0) && op <= X86_OP(a5)) {
    return kIntArgKeys[op - X86_OP(a0)];
  }
  if (op >= X86_OP(fa0) && op <= X86_OP(fa7)) {
    return kFloatKeyBase + (int)(op - X86_OP(fa0));
  }
  switch (op) {
    case X86_OP(sp):
      return X86_REG_RSP;
    case X86_OP(fp):
      return X86_REG_RBP;
    case X86_OP(t0):
      return X86_REG_R10;
    case X86_OP(asm_bx):
      return X86_REG_RBX;
    case X86_OP(resulti):
      return X86_REG_RAX;
    case X86_OP(resulth):
      return X86_REG_RDX;
    case X86_OP(resultf):
    case X86_OP(resultd):
    case X86_OP(resultv):
      return kFloatKeyBase;
    default:
      return -1;
  }
}

static bool IsMoveOpcode(X86Opcode op) {
  return op == X86_OP(mv) || op == X86_OP(fmv_s) || op == X86_OP(fmv_d);
}

// `mv loc, src` with no dest writes its first operand.
static int WrittenOperand(TargetInstruction* inst) {
  if (IsMoveOpcode((X86Opcode)inst->opcode) && inst->dest == NULL &&
      inst->operand[0] != NULL && inst->operand[1] != NULL &&
      Kind(inst->operand[0]) == kTargetRALocation) {
    return 0;
  }
  return -1;
}

static bool IsSavedInt(int r) {
  return r == X86_REG_RBX || (r >= X86_REG_R12 && r <= X86_REG_R15);
}

// A call overwrites the SysV caller-saved registers, which are all of the xmm
// registers.  The divide family leaves rax and rdx holding its quotient and
// remainder.  The epilogue reloads the callee-saved registers, so a value read
// after it, such as the target of an indirect tail call, must be in none of
// them.
static int Clobbers(TargetInstruction* inst, int* keys, int max_keys) {
  int n = 0;
  switch ((X86Opcode)inst->opcode) {
    case X86_OP(call):
    case X86_OP(rcall):
    case X86_OP(callf):
    case X86_OP(rcallf):
      for (int r = 0; r < 16 && n < max_keys; r++) {
        if (!IsSavedInt(r) && r != X86_REG_RSP && r != X86_REG_RBP) {
          keys[n++] = r;
        }
      }
      for (int r = 0; r < 16 && n < max_keys; r++) {
        keys[n++] = kFloatKeyBase + r;
      }
      return n;
    case X86_OP(idiv):
    case X86_OP(div):
    case X86_OP(mod):
      keys[n++] = X86_REG_RAX;
      keys[n++] = X86_REG_RDX;
      return n;
    case X86_OP(restore):
      for (int r = 0; r < 16 && n < max_keys; r++) {
        if (IsSavedInt(r)) {
          keys[n++] = r;
        }
      }
      return n;
    default:
      return 0;
  }
}

static bool ReadsThrough(TargetInstruction* operand) {
  return (X86Opcode)operand->opcode == X86_OP(regarg);
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

const TargetRAInfo* X86RAInfo(void) { return &kInfo; }

// ---------------------------------------------------------------------------
// Coloring allocator.

typedef struct {
  X86RegisterAllocator* allocator;
  bool returns_int;
  bool returns_float;
  int spill_size;
} ColoringContext;

static int RegClass(TargetInstruction* inst) {
  switch ((X86Opcode)inst->opcode) {
    case X86_OP(reload):
      return RegClass(inst->operand[0]->operand[0]);
    case X86_OP(movq_xmm):
      // Moves a double's bits into an xmm register.
      return kFloatClass;
    default:
      return X86HasFloatResult(inst) ? kFloatClass : kIntClass;
  }
}

static int FixedValueKey(TargetInstruction* inst) {
  switch ((X86Opcode)inst->opcode) {
    case X86_OP(call):
    case X86_OP(rcall):
      return X86_REG_RAX;
    case X86_OP(callf):
    case X86_OP(rcallf):
      return kFloatKeyBase;
    default:
      return -1;
  }
}

static bool OpcodeDefinesRegister(TargetInstruction* inst) {
  switch ((X86Opcode)inst->opcode) {
    case X86_OP(call):
    case X86_OP(rcall):
    case X86_OP(callf):
    case X86_OP(rcallf):
      return true;
    case X86_OP(cmp):
    case X86_OP(test):
    case X86_OP(ucomiss):
    case X86_OP(ucomisd):
    case X86_OP(nop):
    case X86_OP(prefetchnta):
    case X86_OP(prefetcht0):
    case X86_OP(prefetcht1):
    case X86_OP(prefetcht2):
    case X86_OP(prefetchw):
      return false;
    default:
      return X86GeneratesOutput(inst);
  }
}

static bool DefinesRegister(TargetInstruction* inst) {
  unsigned op = (unsigned)inst->opcode;
  if (op > (unsigned)X86_OP(reload)) {
    return OpcodeDefinesRegister(inst);
  }
  if (opcode_defines[op] == 0) {
    opcode_defines[op] = (signed char)(OpcodeDefinesRegister(inst) + 1);
  }
  return opcode_defines[op] - 1;
}

// Every register is 64 bits and the moves copy all that a value uses.
static TargetRACopyKind CopyKind(TargetInstruction* inst) {
  TargetInstruction* src = inst->operand[0];
  if (!IsMoveOpcode((X86Opcode)inst->opcode) || src == NULL ||
      inst->operand[1] != NULL || Kind(src) == kTargetRANone ||
      RegClass(inst) != RegClass(src)) {
    return kTargetRANotCopy;
  }
  return kTargetRACopy;
}

static int ArgumentKeys(int* keys, int max_keys) {
  int n = 0;
  for (int r = 0; r < kNumIntArgKeys && n < max_keys; r++) {
    keys[n++] = kIntArgKeys[r];
  }
  for (int r = 0; r < kNumFloatArgKeys && n < max_keys; r++) {
    keys[n++] = kFloatKeyBase + r;
  }
  return n;
}

static int ImplicitUses(void* ctx, TargetInstruction* inst, int* keys,
                        int max_keys) {
  ColoringContext* c = ctx;
  int n = 0;
  switch ((X86Opcode)inst->opcode) {
    case X86_OP(save):
      // The prologue stores incoming argument registers to the frame.
      return ArgumentKeys(keys, max_keys);
    case X86_OP(jmp): {
      // A tail call's arguments are not operands.  It jumps to a symbol, or
      // through t0 to an address computed in the body.
      X86Opcode target = inst->operand[0] == NULL
                             ? X86_OP(label)
                             : (X86Opcode)inst->operand[0]->opcode;
      if (target == X86_OP(symbol) || target == X86_OP(t0)) {
        return ArgumentKeys(keys, max_keys);
      }
      return 0;
    }
    case X86_OP(ret):
      if (c->returns_int && n < max_keys) {
        keys[n++] = X86_REG_RAX;
      }
      if (c->returns_float && n < max_keys) {
        keys[n++] = kFloatKeyBase;
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

static bool EarlyClobber(TargetInstruction* inst) {
  (void)inst;
  return false;
}

// The copy is in the class of the register it writes.  Both moves print as
// whichever instruction takes the value between the register files.
static TargetInstruction* CreateCopy(void* ctx, TargetInstruction* value,
                                     TargetInstruction* dest) {
  (void)ctx;
  bool fp = RegClass(dest != NULL ? dest : value) == kFloatClass;
  TargetInstruction* copy = TargetNewInstruction1(
      (TargetOpcode)(fp ? X86_OP(fmv_d) : X86_OP(mv)), value);
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
      (TargetOpcode)X86_OP(spill), value,
      TargetGetIntConstant(&c->allocator->rv->base, NULL, kTargetType32Bit,
                           slot));
}

static TargetInstruction* CreateReload(void* ctx, TargetInstruction* spill) {
  (void)ctx;
  return TargetNewInstruction1((TargetOpcode)X86_OP(reload), spill);
}

// Variable registers are TargetSymbols; this one stands for no variable.
static TargetInstruction* CreateLocation(void* ctx, TargetInstruction* like) {
  ColoringContext* c = ctx;
  bool fp = RegClass(like) == kFloatClass;
  TargetSymbol* location = calloc(1, sizeof(TargetSymbol));
  TargetInitInstruction(
      &location->base,
      (TargetOpcode)(fp ? X86_OP(fvarreg) : X86_OP(ivarreg)));
  TargetTrackOrphanInstruction(&c->allocator->rv->base, &location->base);
  RegisterVariable* var = FindVariable(c, like);
  if (var != NULL) {
    var->split_location = &location->base;
  }
  return &location->base;
}

static TargetInstruction* CreateLabel(void* ctx) {
  (void)ctx;
  return TargetNewInstruction((TargetOpcode)X86_OP(label));
}

static TargetInstruction* CreateJump(void* ctx, TargetInstruction* label) {
  (void)ctx;
  return TargetNewInstruction1((TargetOpcode)X86_OP(jmp), label);
}

// The logical register slot that names each integer register.  The logical
// file aliases the 16 registers, and the emitter compares registers by slot.
static const int kIntSlot[16] = {
    1,  // rax
    13, // rcx
    12, // rdx
    9,  // rbx
    2,  // rsp
    8,  // rbp
    11, // rsi
    10, // rdi
    14, // r8
    15, // r9
    5,  // r10
    3,  // r11
    18, // r12
    19, // r13
    20, // r14
    21, // r15
};

// The dominator walk keeps values in xmm8-xmm15 across calls, so those are
// saved in the prologue here too, for its callers.
enum { kFirstSavedFloat = 8 };

static void Assign(void* ctx, TargetInstruction* inst, int key) {
  ColoringContext* c = ctx;
  X86RegisterAllocator* allocator = c->allocator;
  if (key >= kFloatKeyBase) {
    int num = key - kFloatKeyBase;
    inst->reg = &allocator->float_regs[num].base;
    if (num >= kFirstSavedFloat) {
      BitSetInsert(&allocator->used_float_regs, (size_t)num);
    }
    return;
  }
  int slot = kIntSlot[key];
  inst->reg = &allocator->int_regs[slot].base;
  if (IsSavedInt(key)) {
    BitSetInsert(&allocator->used_int_regs, (size_t)slot);
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

// The emitter prints some addresses with the operands of an add that forms
// them, so it reads those where the add is read, not where the add is: an
// offset that adds a constant to a register is printed as that register and
// constant, and a TLS offset from a base that adds one as an %fs-relative
// address.
static bool IsAdd(TargetInstruction* op) {
  return op != NULL && (X86Opcode)op->opcode == X86_OP(add) &&
         op->operand[0] != NULL && op->operand[1] != NULL;
}

static bool IsTlsOffset(TargetInstruction* op) {
  return op != NULL && (X86Opcode)op->opcode == X86_OP(movxc) &&
         (op->flags & X86_TLS_RELOC) != 0;
}

static bool IsPrintedThroughOffset(TargetInstruction* offset) {
  return IsAdd(offset) && TargetIsConst(offset->operand[1]) &&
         !TargetIsConst(offset->operand[0]) &&
         (X86Opcode)offset->operand[0]->opcode != X86_OP(x0);
}

// Loads and lea take their base and offset as operands 0 and 1; stores take
// them after the value.
static bool HasPrintedThroughAddress(TargetInstruction* inst) {
  int base;
  if (X86IsLoad(inst) || (X86Opcode)inst->opcode == X86_OP(lea)) {
    base = 0;
  } else if (X86IsStore(inst)) {
    base = 1;
  } else {
    return false;
  }
  TargetInstruction* offset = inst->operand[base + 1];
  return IsPrintedThroughOffset(offset) ||
         (IsAdd(inst->operand[base]) && IsTlsOffset(offset));
}

static const char* Unsupported(X86Generator* rv) {
  FOR_EACH_INSTRUCTION(&rv->base, inst) {
    X86Opcode op = (X86Opcode)inst->opcode;
    switch (op) {
      case X86_OP(asm):
      case X86_OP(spill):
      case X86_OP(reload):
      case X86_OP(atomic_compare_exchange_bool):
      case X86_OP(atomic_compare_exchange_val):
      case X86_OP(atomic_compare_exchange_n):
      case X86_OP(atomic_fetch_add_sub):
      case X86_OP(atomic64):
      case X86_OP(a6):
      case X86_OP(a7):
      case X86_OP(resulth):
      case X86_OP(movslq):
        return X86OpcodeName((int)op);
      default:
        break;
    }
    if ((op >= X86_OP(paddb) && op <= X86_OP(divpd)) ||
        op == X86_OP(loadv) || op == X86_OP(storev) ||
        op == X86_OP(resultv) || (inst->flags & X86_VECTOR_VALUE) != 0) {
      return "vector";
    }
    if (HasPrintedThroughAddress(inst)) {
      return "folded-address";
    }
  }
  return NULL;
}

// The allocator reads `mv loc, src` as `mv src -> loc`.  Records each move and
// the location it wrote in |moves| for RestoreWrittenMoves.
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
    inst->dest = location;
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

bool X86AllocateRegistersByColoring(X86RegisterAllocator* allocator) {
  X86Generator* rv = allocator->rv;
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
    X86Opcode op = (X86Opcode)dest->opcode;
    if (op == X86_OP(resulti)) {
      ctx.returns_int = true;
    } else if (op == X86_OP(resultf) || op == X86_OP(resultd)) {
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
  // cost a save in the prologue, and the argument registers last among those,
  // since calls need them.  rsp and rbp hold the frame, and the emitter uses
  // r11, xmm14 and xmm15 as scratch registers.
  static const int kIntOrder[] = {
      X86_REG_RAX, X86_REG_R10, X86_REG_R9,  X86_REG_R8,  X86_REG_RCX,
      X86_REG_RDX, X86_REG_RSI, X86_REG_RDI, X86_REG_RBX, X86_REG_R12,
      X86_REG_R13, X86_REG_R14, X86_REG_R15,
  };
  TargetRAClass* ints = &desc.classes[kIntClass];
  for (size_t i = 0; i < sizeof(kIntOrder) / sizeof(kIntOrder[0]); i++) {
    ints->regs[ints->num_regs++] = kIntOrder[i];
  }
  TargetRAClass* floats = &desc.classes[kFloatClass];
  for (int r = 0; r < 14; r++) {
    floats->regs[floats->num_regs++] = kFloatKeyBase + r;
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
    if ((X86Opcode)inst->opcode == X86_OP(spill)) {
      inst->reg = inst->operand[0]->reg;
    }
  }
  allocator->current_spilled_region_size = ctx.spill_size;
  allocator->max_spilled_region_size = ctx.spill_size;
  if (ctx.spill_size > 0) {
    // The spill slots are in the frame, which a leaf need not have.
    rv->not_leaf = true;
  }
  rv->colored = true;
  return true;
}
