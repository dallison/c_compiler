//
//  x86_ra_info.c
//  c_compiler
//
//  How the shared register allocator code reads x86-64 instructions, and the
//  x86-64 side of the coloring allocator.  See target_ra_ir.h and
//  target_reg_alloc.h.
//

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "x86_reg_alloc.h"
#include "x86_codegen.h"
#include "x86_machine.h"
#include "target_basic_block.h"
#include "target_reg_alloc.h"

// Physical register keys: the integer registers by hardware number (rax=0,
// rcx=1, ... r15=15) and xmm0..xmm15 as 64..79.  On i386 the integer keys are
// eax..edi (0..7) and the xmm keys are xmm0..xmm7 (64..71).
enum { kFloatKeyBase = 64 };

// The allocator's callbacks take no context.  A run compiles for one target,
// and the allocation sets the register-name profile to that target's.
static bool Is64Bit(void) { return X86CurrentRegisterNameProfile()->is_64bit; }
static int NumIntKeys(void) { return Is64Bit() ? 16 : 8; }
static int NumFloatKeys(void) { return Is64Bit() ? 16 : 8; }

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
  // i386 passes every argument on the stack, so its argument pseudo-registers
  // name no register.  The codegen still writes them for the copy helpers,
  // which then pass the values on the stack.
  if (op >= X86_OP(a0) && op <= X86_OP(a5)) {
    return Is64Bit() ? kIntArgKeys[op - X86_OP(a0)] : -1;
  }
  if (op >= X86_OP(fa0) && op <= X86_OP(fa7)) {
    return Is64Bit() ? kFloatKeyBase + (int)(op - X86_OP(fa0)) : -1;
  }
  switch (op) {
    case X86_OP(sp):
      return X86_REG_RSP;
    case X86_OP(fp):
      return X86_REG_RBP;
    case X86_OP(t0):
      // The emitter's temporary: r10, or eax on i386.
      return Is64Bit() ? X86_REG_R10 : X86_REG_EAX;
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
  if (!Is64Bit()) {
    return r == X86_REG_EBX || r == X86_REG_ESI || r == X86_REG_EDI;
  }
  return r == X86_REG_RBX || (r >= X86_REG_R12 && r <= X86_REG_R15);
}

static bool IsCompareExchange(TargetInstruction* inst) {
  switch ((X86Opcode)inst->opcode) {
    case X86_OP(atomic_compare_exchange_bool):
    case X86_OP(atomic_compare_exchange_val):
    case X86_OP(atomic_compare_exchange_n):
      return true;
    default:
      return false;
  }
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

// The integer register that logical slot |slot| names.  The i386 logical file
// is the hardware file.
static int SlotKey(int slot) {
  if (!Is64Bit()) {
    return slot >= 0 && slot < 8 ? slot : -1;
  }
  for (int r = 0; r < 16; r++) {
    if (kIntSlot[r] == slot) {
      return r;
    }
  }
  return -1;
}

// The logical slot that integer register key |key| is.
static int IntSlotOf(int key) { return Is64Bit() ? kIntSlot[key] : key; }

static bool IsExtendedAsm(TargetInstruction* inst) {
  return (X86Opcode)inst->opcode == X86_OP(asm) &&
         (inst->flags & X86_INST_EXTENDED_ASM) != 0;
}

// The callee-saved registers named in an asm clobber list.
static int CalleeSavedClobbers(TargetInstruction* inst, int* keys,
                               int max_keys) {
  if (!IsExtendedAsm(inst)) {
    return 0;
  }
  static const struct {
    const char* name;
    int key;
  } kSaved[] = {{"rbx", X86_REG_RBX}, {"ebx", X86_REG_EBX},
                {"esi", X86_REG_ESI}, {"edi", X86_REG_EDI},
                {"r12", X86_REG_R12}, {"r13", X86_REG_R13},
                {"r14", X86_REG_R14}, {"r15", X86_REG_R15}};
  Vector* clobbers = &((X86AsmInstruction*)inst)->asm_node->clobbers;
  int n = 0;
  for (size_t i = 0; i < clobbers->length && n < max_keys; i++) {
    const char* name = ((String*)clobbers->value.p[i])->value;
    for (size_t r = 0; r < sizeof(kSaved) / sizeof(kSaved[0]); r++) {
      // A name the target does not save, such as esi on x86-64, is not one.
      if (IsSavedInt(kSaved[r].key) && strcmp(name, kSaved[r].name) == 0) {
        keys[n++] = kSaved[r].key;
      }
    }
  }
  return n;
}

// The registers codegen put an asm's operands in.  The outputs come first,
// and an immediate input has no register.
static int AsmOperandKeys(TargetInstruction* inst, bool outputs, int* keys,
                          int max_keys) {
  if (!IsExtendedAsm(inst)) {
    return 0;
  }
  X86AsmInstruction* asm_inst = (X86AsmInstruction*)inst;
  Vector* asm_outputs = &asm_inst->asm_node->outputs;
  int n = 0;
  for (int i = 0; i < asm_inst->num_operands && n < max_keys; i++) {
    bool is_output = (size_t)i < asm_outputs->length;
    if (outputs ? !is_output
                : is_output &&
                      !((AsmOperand*)asm_outputs->value.p[i])->is_readwrite) {
      continue;
    }
    int key = asm_inst->reg_nums[i] == INT_MIN
                  ? -1
                  : SlotKey(asm_inst->reg_nums[i]);
    if (key >= 0) {
      keys[n++] = key;
    }
  }
  return n;
}

// A call overwrites the SysV caller-saved registers, which are all of the xmm
// registers.  The divide family leaves rax and rdx holding its quotient and
// remainder, and cmpxchg leaves rax holding the value it found.  The epilogue
// reloads the callee-saved registers, so a value read after it, such as the
// target of an indirect tail call, must be in none of them.  Inline asm is
// lowered with its operands in fixed registers, and the registers its text
// uses are unknown, so it clobbers what a call does, its outputs and whatever
// callee-saved registers it names.
static int Clobbers(TargetInstruction* inst, int* keys, int max_keys) {
  int n = 0;
  if (IsCompareExchange(inst)) {
    keys[n++] = X86_REG_RAX;
    return n;
  }
  switch ((X86Opcode)inst->opcode) {
    case X86_OP(asm):
    case X86_OP(call):
    case X86_OP(rcall):
    case X86_OP(callf):
    case X86_OP(rcallf):
      for (int r = 0; r < NumIntKeys() && n < max_keys; r++) {
        if (!IsSavedInt(r) && r != X86_REG_RSP && r != X86_REG_RBP) {
          keys[n++] = r;
        }
      }
      for (int r = 0; r < NumFloatKeys() && n < max_keys; r++) {
        keys[n++] = kFloatKeyBase + r;
      }
      n += CalleeSavedClobbers(inst, keys + n, max_keys - n);
      return n + AsmOperandKeys(inst, true, keys + n, max_keys - n);
    case X86_OP(idiv):
    case X86_OP(div):
    case X86_OP(mod):
      keys[n++] = X86_REG_RAX;
      keys[n++] = X86_REG_RDX;
      return n;
    case X86_OP(restore):
      for (int r = 0; r < NumIntKeys() && n < max_keys; r++) {
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
  bool returns_hi;  // A 64-bit result's high half, in edx.
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

// A vector in an xmm register, which is 16 bytes of it unless its width flags
// say 8 or 4.  A reload is a vector if its spill is.
static bool IsVector(TargetInstruction* inst) {
  if ((X86Opcode)inst->opcode == X86_OP(reload)) {
    inst = inst->operand[0];
  }
  return (inst->flags & X86_VECTOR_VALUE) != 0 ||
         (X86Opcode)inst->opcode == X86_OP(loadv) ||
         (X86Opcode)inst->opcode == X86_OP(resultv);
}

// The flags that make the emitter move all of a vector, for an instruction
// that moves |inst|.
static int VectorFlags(TargetInstruction* inst) {
  if (!IsVector(inst)) {
    return 0;
  }
  if ((X86Opcode)inst->opcode == X86_OP(reload)) {
    inst = inst->operand[0];
  }
  return X86_VECTOR_VALUE | (inst->flags & X86_VECTOR_WIDTH_MASK);
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
  // i386 passes every argument on the stack.
  if (!Is64Bit()) {
    return 0;
  }
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
  if (IsCompareExchange(inst)) {
    // The lowering moves the expected value into rax, which cmpxchg compares
    // with memory.
    keys[n++] = X86_REG_RAX;
    return n;
  }
  switch ((X86Opcode)inst->opcode) {
    case X86_OP(asm):
      return AsmOperandKeys(inst, false, keys, max_keys);
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
      if (c->returns_hi && n < max_keys) {
        keys[n++] = X86_REG_RDX;
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
// whichever instruction takes the value between the register files, and a
// vector's copy moves the whole register.
static TargetInstruction* CreateCopy(void* ctx, TargetInstruction* value,
                                     TargetInstruction* dest) {
  (void)ctx;
  bool fp = RegClass(dest != NULL ? dest : value) == kFloatClass;
  TargetInstruction* copy = TargetNewInstruction1(
      (TargetOpcode)(fp ? X86_OP(fmv_d) : X86_OP(mv)), value);
  copy->flags |= VectorFlags(value);
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

// The emitter addresses slot |s| 8 + |s| bytes below the top of the spill
// region, so slot |s| is the 8 bytes at depth |s| and a 16-byte slot at depth
// |d| is numbered |d| + 8.
static int NewSlot(void* ctx, TargetInstruction* value) {
  ColoringContext* c = ctx;
  int vector = VectorFlags(value);
  int slot;
  if (vector != 0 && (vector & X86_VECTOR_WIDTH_MASK) == 0) {
    c->spill_size = (c->spill_size + 15) & ~15;
    slot = c->spill_size + 8;
    c->spill_size += 16;
  } else {
    slot = c->spill_size;
    c->spill_size += 8;
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
      (TargetOpcode)X86_OP(spill), value,
      TargetGetIntConstant(&c->allocator->rv->base, NULL, kTargetType32Bit,
                           slot));
  // The emitter stores a vector's spill and reloads it at its width.
  spill->flags |= VectorFlags(value);
  return spill;
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
  location->base.flags |= VectorFlags(like);
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
  int slot = IntSlotOf(key);
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
  bool is64 = rv->profile->is_64bit;
  FOR_EACH_INSTRUCTION(&rv->base, inst) {
    X86Opcode op = (X86Opcode)inst->opcode;
    switch (op) {
      case X86_OP(spill):
      case X86_OP(reload):
      case X86_OP(movslq):
      case X86_OP(a6):
      case X86_OP(a7):
        return X86OpcodeName((int)op);
      case X86_OP(atomic64):
      case X86_OP(resulth):
        // i386 has these, which saves the pair's registers itself.
        if (is64) {
          return X86OpcodeName((int)op);
        }
        break;
      default:
        break;
    }
    if (HasPrintedThroughAddress(inst)) {
      return "folded-address";
    }
  }
  return NULL;
}

// The allocator reads `mv loc, src` as `mv src -> loc`.
static void RewriteWrittenMoves(TargetGenerator* gen) {
  FOR_EACH_INSTRUCTION(gen, inst) {
    if (WrittenOperand(inst) != 0) {
      continue;
    }
    TargetInstruction* location = inst->operand[0];
    TargetReplaceOperand(inst, 0, inst->operand[1]);
    TargetReplaceOperand(inst, 1, NULL);
    inst->dest = location;
  }
}

// xadd leaves its result in its delta's register, which the lowering says by
// making the delta its dest, although the delta is a value.  Here the result
// gets a register of its own, and the emitter copies the delta into it.
static void DetachXaddDests(TargetGenerator* gen) {
  FOR_EACH_INSTRUCTION(gen, inst) {
    if ((X86Opcode)inst->opcode == X86_OP(atomic_fetch_add_sub) &&
        inst->dest != NULL && inst->dest == inst->operand[1]) {
      inst->dest = NULL;
    }
  }
}

// Notes which result registers the function returns values in.  The lowering
// also moves a compare-exchange's expected value and an asm's `a` operand into
// rax, so a write to rax that one of those follows in its block is not a
// result.
static void FindResults(TargetGenerator* gen, ColoringContext* c) {
  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    TargetBasicBlock* block = gen->basic_blocks.value.p[b];
    if (block->code == NULL || block->end_code == NULL) {
      continue;
    }
    bool writes_int = false;
    bool writes_hi = false;
    for (TargetInstruction* inst = block->code; inst != NULL;
         inst = NextInBlock(block, inst)) {
      if (IsCompareExchange(inst) || IsExtendedAsm(inst)) {
        writes_int = false;
        writes_hi = false;
        continue;
      }
      if (inst->dest == NULL) {
        continue;
      }
      X86Opcode op = (X86Opcode)inst->dest->opcode;
      if (op == X86_OP(resulti)) {
        writes_int = true;
      } else if (op == X86_OP(resulth)) {
        writes_hi = true;
      } else if (op == X86_OP(resultf) || op == X86_OP(resultd) ||
                 op == X86_OP(resultv)) {
        c->returns_float = true;
      }
    }
    c->returns_int |= writes_int;
    c->returns_hi |= writes_hi;
  }
}

static void CannotAllocate(TargetGenerator* gen, const char* reason) {
  fprintf(stderr, "Internal error: cannot allocate registers for %s: %s\n",
          gen->function_name.value, reason);
  abort();
}

void X86AllocateRegistersByColoring(X86RegisterAllocator* allocator) {
  X86Generator* rv = allocator->rv;
  TargetGenerator* gen = &rv->base;
  const char* reason = Unsupported(rv);
  if (reason != NULL) {
    CannotAllocate(gen, reason);
  }

  ColoringContext ctx = {.allocator = allocator};
  FindResults(gen, &ctx);

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
  desc.create_location = CreateLocation;
  desc.create_label = CreateLabel;
  desc.create_jump = CreateJump;
  // Caller-saved registers first, so that only values live across a call
  // cost a save in the prologue, and the argument registers last among those,
  // since calls need them.  rsp and rbp hold the frame, and the emitter uses
  // r11, xmm14 and xmm15 as scratch registers.
  static const int kIntOrder[] = {
      X86_REG_RAX, X86_REG_R10, X86_REG_R9,  X86_REG_R8,  X86_REG_RCX,
      X86_REG_RDX, X86_REG_RSI, X86_REG_RDI, X86_REG_RBX, X86_REG_R12,
      X86_REG_R13, X86_REG_R14, X86_REG_R15,
  };
  // i386: ecx is the emitter's address register, so it is not allocated.  The
  // xmm registers 6 and 7 are the emitter's scratch.
  static const int kIntOrder32[] = {
      X86_REG_EAX, X86_REG_EDX, X86_REG_EBX, X86_REG_ESI, X86_REG_EDI,
  };
  const int* int_order = Is64Bit() ? kIntOrder : kIntOrder32;
  size_t num_int_order = Is64Bit() ? sizeof(kIntOrder) / sizeof(kIntOrder[0])
                                   : sizeof(kIntOrder32) / sizeof(kIntOrder32[0]);
  TargetRAClass* ints = &desc.classes[kIntClass];
  for (size_t i = 0; i < num_int_order; i++) {
    ints->regs[ints->num_regs++] = int_order[i];
  }
  TargetRAClass* floats = &desc.classes[kFloatClass];
  int num_float_regs = Is64Bit() ? 14 : 6;
  for (int r = 0; r < num_float_regs; r++) {
    floats->regs[floats->num_regs++] = kFloatKeyBase + r;
  }

  RewriteWrittenMoves(gen);
  DetachXaddDests(gen);
  if (!TargetAllocateRegisters(gen, &desc, NULL)) {
    CannotAllocate(gen, "the shared allocator gave up");
  }
  FOR_EACH_INSTRUCTION(gen, inst) {
    if ((X86Opcode)inst->opcode == X86_OP(spill)) {
      inst->reg = inst->operand[0]->reg;
    }
    int keys[16];
    int num_keys = CalleeSavedClobbers(inst, keys, 16);
    for (int k = 0; k < num_keys; k++) {
      BitSetInsert(&allocator->used_int_regs, (size_t)IntSlotOf(keys[k]));
    }
  }
  allocator->current_spilled_region_size = ctx.spill_size;
  allocator->max_spilled_region_size = ctx.spill_size;
  if (ctx.spill_size > 0) {
    // The spill slots are in the frame, which a leaf need not have.
    rv->not_leaf = true;
  }
  if (!Is64Bit()) {
    // i386 passes arguments on the stack, which the body reads through ebp,
    // and its prologue saves the callee-saved registers.  Both need a frame.
    rv->not_leaf = true;
  }
}
