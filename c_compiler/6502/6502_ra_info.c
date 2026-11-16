//
//  6502_ra_info.c
//  c_compiler
//
//  How the shared register allocator reads 6502 instructions, and the 6502
//  side of the coloring allocator.  See target_ra_ir.h and target_reg_alloc.h.
//
//  A zero-page register cell is filled and read a byte at a time, or by a
//  runtime helper given its address, so every instruction that names a cell
//  counts as a read of it, writes included.  A cell is then live from its
//  expr or variable register instruction to the last instruction naming it,
//  which is the range the old allocator gives it.  Spilling follows the old
//  allocator too: see RewriteSpill.
//

#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "6502_codegen.h"
#include "6502_machine.h"
#include "6502_reg_alloc.h"
#include "target_basic_block.h"
#include "target_reg_alloc.h"

// Physical register keys, by register type: b0..b5 are 0..5, i0..i7 8..15,
// l0..l3 16..19, x0..x2 20..22 and f0..f3 24..27.  Each type is a class.
static const int kKeyBase[] = {0, 8, 16, 20, 24};
static const int kNumRegs[] = {W65C02_NUM_B_REGS, W65C02_NUM_I_REGS,
                               W65C02_NUM_L_REGS, W65C02_NUM_X_REGS,
                               W65C02_NUM_F_REGS};
static const int kNumTemps[] = {W65C02_NUM_TEMP_B_REGS, W65C02_NUM_TEMP_I_REGS,
                                W65C02_NUM_TEMP_L_REGS, W65C02_NUM_TEMP_X_REGS,
                                W65C02_NUM_TEMP_F_REGS};
static const int kRegBytes[] = {1, 2, 4, 8, 4};
enum { kNumClasses = 5 };

static bool IsCell(TargetInstruction* inst) {
  switch ((W65C02Opcode)inst->opcode) {
    case W65C02_OP(expr1):
    case W65C02_OP(expr2):
    case W65C02_OP(expr4):
    case W65C02_OP(expr8):
    case W65C02_OP(exprf):
    case W65C02_OP(exprd):
      return true;
    default:
      return false;
  }
}

static bool IsRegVar(TargetInstruction* inst) {
  switch ((W65C02Opcode)inst->opcode) {
    case W65C02_OP(ivarreg):
    case W65C02_OP(bvarreg):
    case W65C02_OP(lvarreg):
    case W65C02_OP(xvarreg):
    case W65C02_OP(fvarreg):
    case W65C02_OP(dvarreg):
      return true;
    default:
      return false;
  }
}

static bool IsSpill(TargetInstruction* inst) {
  W65C02Opcode op = (W65C02Opcode)inst->opcode;
  return op >= W65C02_OP(spill1) && op <= W65C02_OP(spill8);
}

static bool IsReload(TargetInstruction* inst) {
  W65C02Opcode op = (W65C02Opcode)inst->opcode;
  return op >= W65C02_OP(reload1) && op <= W65C02_OP(reload8);
}

// A structreturn loads the address of the caller's result buffer into its
// own cell.
static TargetRAKind Kind(TargetInstruction* inst) {
  return IsCell(inst) || IsRegVar(inst) || IsReload(inst) ||
                 TargetOpcodeEq(inst->opcode, W65C02_OP(structreturn))
             ? kTargetRAValue
             : kTargetRANone;
}

static int LocationKey(TargetInstruction* inst) {
  (void)inst;
  return -1;
}

static int WrittenOperand(TargetInstruction* inst) {
  (void)inst;
  return -1;
}

// A jsr to a C function, or the jmp the optimizer made of one before an rts.
static bool IsProcedureCall(TargetInstruction* inst) {
  return (TargetOpcodeEq(inst->opcode, W65C02_OP(jsr)) ||
          TargetOpcodeEq(inst->opcode, W65C02_OP(jmp))) &&
         (inst->flags & k6502ProcedureCall) != 0;
}

static int TempKeys(int reg_class, int* keys, int max_keys) {
  int n = 0;
  for (int r = 0; r < kNumTemps[reg_class] && n < max_keys; r++) {
    keys[n++] = kKeyBase[reg_class] + r;
  }
  return n;
}

static int PreservedKeys(int reg_class, int* keys, int max_keys) {
  int n = 0;
  for (int r = kNumTemps[reg_class]; r < kNumRegs[reg_class] && n < max_keys;
       r++) {
    keys[n++] = kKeyBase[reg_class] + r;
  }
  return n;
}

// A C function may use the temp registers without saving them.  The runtime
// helpers write only the cells they are given.
static int Clobbers(TargetInstruction* inst, int* keys, int max_keys) {
  if (!IsProcedureCall(inst)) {
    return 0;
  }
  int n = 0;
  for (int c = 0; c < kNumClasses; c++) {
    n += TempKeys(c, keys + n, max_keys - n);
  }
  return n;
}

static bool ReadsThrough(TargetInstruction* operand) {
  (void)operand;
  return false;
}

static TargetRACopyKind CopyKind(TargetInstruction* inst) {
  return W65C02CopySource(inst) != NULL ? kTargetRACopy : kTargetRANotCopy;
}

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

static int RegClass(TargetInstruction* inst) {
  if (IsReload(inst)) {
    inst = inst->operand[0]->operand[0];  // The cell its spill stores.
  }
  return (int)W65C02InstructionRegisterType(inst);
}

static int FixedValueKey(TargetInstruction* inst) {
  (void)inst;
  return -1;
}

static bool DefinesRegister(TargetInstruction* inst) {
  (void)inst;
  return true;
}

static int ImplicitUses(void* ctx, TargetInstruction* inst, int* keys,
                        int max_keys) {
  (void)ctx;
  (void)inst;
  (void)keys;
  (void)max_keys;
  return 0;
}

static int ForbiddenKeys(TargetInstruction* inst, int* keys, int max_keys) {
  // A callee stores its result after saving the registers it uses, so a
  // result in one of those would be lost when it restores them.
  if (W65C02ResultCall(inst) != NULL) {
    return PreservedKeys(RegClass(inst), keys, max_keys);
  }
  // Variables stay out of the temps, as under the old allocator, for the
  // runtime helpers that write fixed zero-page cells.
  if (IsRegVar(inst)) {
    return TempKeys(RegClass(inst), keys, max_keys);
  }
  return 0;
}

// The 6502 IR has no dests, fixed values or splits, so the allocator never
// asks for a copy.
static TargetInstruction* CreateCopy(void* ctx, TargetInstruction* value,
                                     TargetInstruction* dest) {
  (void)ctx;
  (void)value;
  (void)dest;
  fprintf(stderr, "unexpected register copy on the 6502\n");
  abort();
}

static W65C02Register* RegisterOfKey(W65C02RegisterAllocator* allocator,
                                     int key, BitSet** used) {
  int c = kNumClasses - 1;
  while (key < kKeyBase[c]) {
    c--;
  }
  int r = key - kKeyBase[c];
  switch ((W65C02RegisterType)c) {
    case k6502RegTypeB:
      *used = &allocator->used_b_regs;
      return &allocator->b_regs[r];
    case k6502RegTypeI:
      *used = &allocator->used_i_regs;
      return &allocator->i_regs[r];
    case k6502RegTypeL:
      *used = &allocator->used_l_regs;
      return &allocator->l_regs[r];
    case k6502RegTypeX:
      *used = &allocator->used_x_regs;
      return &allocator->x_regs[r];
    case k6502RegTypeF:
      *used = &allocator->used_f_regs;
      return &allocator->f_regs[r];
  }
  abort();
}

static void Assign(void* ctx, TargetInstruction* inst, int key) {
  BitSet* used;
  W65C02Register* reg = RegisterOfKey(ctx, key, &used);
  inst->reg = &reg->base;
  // The prologue saves the preserved registers a function uses.
  if (!reg->temp) {
    BitSetInsert(used, (size_t)reg->base.num);
  }
}

static bool WritesZeroPage(TargetInstruction* inst) {
  switch ((W65C02Opcode)inst->opcode) {
    case W65C02_OP(sta):
    case W65C02_OP(stx):
    case W65C02_OP(sty):
    case W65C02_OP(stz):
    case W65C02_OP(inc):
    case W65C02_OP(dec):
    case W65C02_OP(asl):
    case W65C02_OP(lsr):
    case W65C02_OP(rol):
    case W65C02_OP(ror):
    case W65C02_OP(tsb):
    case W65C02_OP(trb):
      break;
    default:
      return false;
  }
  AddressingMode mode = (AddressingMode)((inst->flags >> 16) & 0x1f);
  return mode == kAddrModeZeroPage;
}

static int CellBytes(TargetInstruction* cell) {
  return kRegBytes[RegClass(cell)];
}

// Whether |inst| loads the address of a cell for a runtime helper, which
// reads and writes the cell when the next jsr runs.
static bool LoadsCellAddress(TargetInstruction* inst) {
  AddressingMode mode = (AddressingMode)((inst->flags >> 16) & 0x1f);
  return mode == kAddrModeZeroPageImmediate && inst->operand[0] != NULL &&
         Kind(inst->operand[0]) == kTargetRAValue;
}

// Whether |inst| runs the helper that cell addresses were loaded for, or
// leaves the code that does.
static bool EndsHelperWindow(TargetInstruction* inst) {
  W65C02Opcode op = (W65C02Opcode)inst->opcode;
  return op == W65C02_OP(jsr) || op == W65C02_OP(jmp) ||
         op == W65C02_OP(label) || W65C02IsBranch(inst);
}

// ---------------------------------------------------------------------------
// Spilling.
//
// A cell is complete only at its spill point, the end of the code that fills
// it, so that is where it is stored.  Each block the store dominates reloads
// it once, after the reload point the code generator put before the first
// instruction there that reads it, where a runtime helper may run, and reads
// the reload from then on.

static TargetInstruction* SpillPoint(W65C02RegisterAllocator* allocator,
                                     TargetInstruction* cell) {
  MapKeyType key = {.w = cell->id};
  return MapFind(&allocator->spill_points, key);
}

static bool Names(TargetInstruction* inst, TargetInstruction* cell) {
  for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
    if (inst->operand[i] == cell) {
      return true;
    }
  }
  return false;
}

static bool IsReloadPoint(TargetInstruction* inst) {
  return TargetOpcodeEq(inst->opcode, W65C02_OP(reloadpoint));
}

// Whether |inst| is in |block| before |end|, or anywhere when |end| is NULL.
static bool InBlockBefore(TargetBasicBlock* block, TargetInstruction* inst,
                          TargetInstruction* end) {
  for (TargetInstruction* p = block->code; p != NULL && p != end;
       p = TargetNext(p)) {
    if (p == inst) {
      return true;
    }
    if (p == block->end_code) {
      break;
    }
  }
  return false;
}

// Whether a runtime helper has been given the address of a cell at |inst| and
// has yet to run.
static bool InHelperWindow(TargetInstruction* inst) {
  for (TargetInstruction* p = inst; p != NULL; p = TargetPrev(p)) {
    if (LoadsCellAddress(p)) {
      return true;
    }
    if (EndsHelperWindow(p)) {
      return false;
    }
  }
  return false;
}

// A block's reload is spilled by reloading again after each later reload
// point of its spill that a read follows.  Returns how many reloads that
// adds, only counting them when |split| is false.
static int SplitReload(TargetGenerator* gen, TargetInstruction* reload,
                       bool split) {
  TargetInstruction* spill = reload->operand[0];
  TargetBasicBlock* block = reload->block;
  TargetInstruction* current = reload;
  TargetInstruction* point = NULL;
  int added = 0;
  for (TargetInstruction* p = TargetNext(reload);
       p != NULL && TargetPrev(p) != block->end_code; p = TargetNext(p)) {
    if (IsReloadPoint(p) && p->operand[0] == spill && !InHelperWindow(p)) {
      point = p;
      continue;
    }
    if (!Names(p, reload)) {
      continue;
    }
    if (point != NULL) {
      added++;
      if (!split) {
        return added;
      }
      current = TargetNewInstruction1(reload->opcode, spill);
      TargetBasicBlockEmitAfter(gen, block, current, point);
      point = NULL;
    }
    for (int op = 0; op < TARGET_MAX_OPERANDS && current != reload; op++) {
      if (p->operand[op] == reload) {
        TargetReplaceOperand(p, op, current);
      }
    }
  }
  return added;
}

static bool CanSpill(void* ctx, TargetInstruction* value) {
  W65C02RegisterAllocator* allocator = ctx;
  if (IsReload(value)) {
    return SplitReload(&allocator->g->base, value, false) > 0;
  }
  return IsCell(value) && SpillPoint(allocator, value) != NULL;
}

// Spilled, a cell stays in a register up to its spill, and is back in one
// from each reload point to the reads that follow.  A reload is split only at
// the reload points of its spill.
static bool HeldSincePrevious(void* ctx, TargetInstruction* user,
                              TargetInstruction* value) {
  W65C02RegisterAllocator* allocator = ctx;
  TargetBasicBlock* block = user->block;
  if (block == NULL) {
    return false;
  }
  if (IsReload(value)) {
    TargetInstruction* p = user;
    while (p != block->code && (p = TargetPrev(p)) != NULL) {
      if (p == value || Names(p, value)) {
        return true;
      }
      if (IsReloadPoint(p) && p->operand[0] == value->operand[0] &&
          !InHelperWindow(p)) {
        return false;
      }
    }
    return false;
  }
  if (!IsReloadPoint(user) || InHelperWindow(user)) {
    return true;
  }
  TargetInstruction* point = SpillPoint(allocator, value);
  for (TargetInstruction* p = user; p != NULL; p = TargetNext(p)) {
    if (p == point) {
      return true;
    }
    if (p == block->end_code) {
      break;
    }
  }
  return false;
}

typedef struct {
  TargetBasicBlock* block;
  TargetInstruction* reload_point;  // NULL when only reload points read it.
  TargetInstruction* reload;
} BlockReload;

static BlockReload* FindBlockReload(BlockReload* reloads, int num_reloads,
                                    TargetBasicBlock* block) {
  for (int r = 0; r < num_reloads; r++) {
    if (reloads[r].block == block) {
      return &reloads[r];
    }
  }
  return NULL;
}

static bool CannotSpill(TargetGenerator* gen, TargetInstruction* cell,
                        const char* reason) {
  if (getenv("DAVECC_NEW_RA_STATS") != NULL) {
    fprintf(stderr, "new-ra-nospill\t%s\t@%d\t%s\n", gen->function_name.value,
            cell->id, reason);
  }
  return false;
}

// Finds where each block the store of |cell| after |point| dominates reloads
// it, filling |reloads|.  |late| holds the instructions there naming |cell|.
static const char* PlaceReloads(TargetInstruction* cell,
                                TargetInstruction* point, const Vector* late,
                                BlockReload* reloads, int* num_reloads) {
  for (size_t i = 0; i < late->length; i++) {
    TargetInstruction* user = late->value.p[i];
    if (WritesZeroPage(user) && user->operand[0] == cell) {
      return "late-write";
    }
    if (FindBlockReload(reloads, *num_reloads, user->block) == NULL) {
      reloads[(*num_reloads)++] = (BlockReload){.block = user->block};
    }
  }
  for (int r = 0; r < *num_reloads; r++) {
    BlockReload* reload = &reloads[r];
    TargetBasicBlock* block = reload->block;
    TargetInstruction* first = NULL;
    for (TargetInstruction* p =
             block == point->block ? TargetNext(point) : block->code;
         p != NULL && TargetPrev(p) != block->end_code; p = TargetNext(p)) {
      if (!Names(p, cell)) {
        continue;
      }
      if (!IsReloadPoint(p)) {
        first = p;
        break;
      }
      reload->reload_point = p;
    }
    if (first == NULL) {
      reload->reload_point = NULL;
    } else if (reload->reload_point == NULL) {
      return "no-reload-point";
    } else if (InHelperWindow(reload->reload_point)) {
      return "helper-window";
    }
  }
  return NULL;
}

static bool RewriteSpill(void* ctx, TargetInstruction* cell) {
  W65C02RegisterAllocator* allocator = ctx;
  TargetGenerator* gen = &allocator->g->base;
  if (IsReload(cell)) {
    return SplitReload(gen, cell, true) > 0;
  }
  TargetInstruction* point = SpillPoint(allocator, cell);
  TargetBasicBlock* block = point->block;
  // A call ends its block too, but carries on to the next instruction.
  if (block == NULL || !InBlockBefore(block, point, NULL) ||
      (W65C02IsBranch(point) && !W65C02IsCall(point))) {
    return CannotSpill(gen, cell, "spill-point");
  }
  // A copy is its own spill point: it expands in place.
  TargetBasicBlock* def_block = cell->block;
  if (def_block == NULL ||
      !BitSetContains(&block->dominators, def_block->block_id) ||
      (def_block == block && cell != point &&
       !InBlockBefore(block, cell, point))) {
    return CannotSpill(gen, cell, "spill-point-first");
  }

  // The instructions the store will dominate: those after it in its block,
  // and all of those in the blocks its block dominates.
  Vector late;
  VectorInit(&late);
  for (TargetInstruction* p = TargetNext(point);
       p != NULL && TargetPrev(p) != block->end_code; p = TargetNext(p)) {
    if (Names(p, cell)) {
      VectorAppend(&late, p);
    }
  }
  for (size_t i = 0; i < cell->users.length; i++) {
    TargetInstruction* user = cell->users.value.p[i];
    TargetBasicBlock* b = user->block;
    if (b != NULL && b != block &&
        BitSetContains(&b->dominators, block->block_id)) {
      VectorAppend(&late, user);
    }
  }
  BlockReload* reloads = calloc(late.length + 1, sizeof(BlockReload));
  int num_reloads = 0;
  const char* reason =
      PlaceReloads(cell, point, &late, reloads, &num_reloads);
  if (reason != NULL) {
    free(reloads);
    VectorDestruct(&late);
    return CannotSpill(gen, cell, reason);
  }

  int bytes = CellBytes(cell);
  int index = __builtin_ctz((unsigned)bytes);
  allocator->current_spilled_region_size += bytes;
  if (allocator->current_spilled_region_size >
      allocator->max_spilled_region_size) {
    allocator->max_spilled_region_size =
        allocator->current_spilled_region_size;
  }
  TargetInstruction* offset = TargetGetIntConstant(
      gen, NULL, kTargetType32Bit, allocator->current_spilled_region_size);
  TargetInstruction* spill = TargetNewInstruction2(
      (TargetOpcode)(W65C02_OP(spill1) + index), cell, offset);
  TargetBasicBlockEmitAfter(gen, block, spill, point);
  for (int r = 0; r < num_reloads; r++) {
    if (reloads[r].reload_point != NULL) {
      reloads[r].reload = TargetNewInstruction1(
          (TargetOpcode)(W65C02_OP(reload1) + index), spill);
      TargetBasicBlockEmitAfter(gen, reloads[r].block, reloads[r].reload,
                                reloads[r].reload_point);
    }
  }
  // Reload points move to the spill, as under the old allocator.
  for (size_t i = 0; i < late.length; i++) {
    TargetInstruction* user = late.value.p[i];
    TargetInstruction* to =
        IsReloadPoint(user)
            ? spill
            : FindBlockReload(reloads, num_reloads, user->block)->reload;
    for (int op = 0; op < TARGET_MAX_OPERANDS; op++) {
      if (user->operand[op] == cell) {
        TargetReplaceOperand(user, op, to);
      }
    }
  }
  free(reloads);
  VectorDestruct(&late);
  return true;
}

static const char* Unsupported(TargetGenerator* gen) {
  if (gen->exception_edges.length != 0) {
    return "exceptions";
  }
  // A cell defined after a helper is given the address of another, and before
  // the helper runs, could share that cell's register although the helper
  // reads one and writes the other.
  bool loading_addresses = false;
  for (TargetInstruction* inst = TargetFirstInstruction(gen); inst != NULL;
       inst = TargetNext(inst)) {
    W65C02Opcode op = (W65C02Opcode)inst->opcode;
    switch (op) {
      case W65C02_OP(asm):
      case W65C02_OP(ssavar):
      case W65C02_OP(phi):
      case W65C02_OP(tmp):
      case W65C02_OP(ap):
      case W65C02_OP(tp):
      case W65C02_OP(resulti):
      case W65C02_OP(resultf):
      case W65C02_OP(resultd):
      case W65C02_OP(spill1):
      case W65C02_OP(spill2):
      case W65C02_OP(spill4):
      case W65C02_OP(spill8):
      case W65C02_OP(reload1):
      case W65C02_OP(reload2):
      case W65C02_OP(reload4):
      case W65C02_OP(reload8):
        return W65C02OpcodeName((int)op);
      default:
        break;
    }
    if ((inst->flags & TARGET_INST_EXCEPTION_LANDING) != 0) {
      return "landing-pad";
    }
    if (Kind(inst) == kTargetRAValue && loading_addresses) {
      return "def-before-helper";
    }
    if (LoadsCellAddress(inst)) {
      loading_addresses = true;
    } else if (EndsHelperWindow(inst)) {
      loading_addresses = false;
    }
    if (WritesZeroPage(inst) && inst->operand[0] != NULL &&
        Kind(inst->operand[0]) == kTargetRAValue && inst->operand[1] != NULL &&
        TargetIntValue(inst->operand[1]) >= CellBytes(inst->operand[0])) {
      return "write-past-cell";
    }
  }
  return NULL;
}

bool W65C02UseColoringAllocator(W65C02Generator* g) {
  if (getenv("DAVECC_OLD_RA") != NULL ||
      (!OptLevel1() && getenv("DAVECC_NEW_RA") == NULL)) {
    return false;
  }
  const char* only = getenv("DAVECC_NEW_RA_FUNC");
  if (only != NULL && strcmp(only, g->base.function_name.value) != 0) {
    return false;
  }
  const char* limit = getenv("DAVECC_NEW_RA_LIMIT");
  if (limit != NULL) {
    static int count;
    return count++ < atoi(limit);
  }
  return true;
}

bool W65C02AllocateRegistersByColoring(W65C02RegisterAllocator* allocator) {
  TargetGenerator* gen = &allocator->g->base;
  const char* reason = Unsupported(gen);
  if (reason != NULL) {
    if (getenv("DAVECC_NEW_RA_STATS") != NULL) {
      fprintf(stderr, "new-ra-fallback\t%s\t%s\n", gen->function_name.value,
              reason);
    }
    return false;
  }

  TargetRegAllocDesc desc = {
      .info = &kInfo,
      .ctx = allocator,
      .num_classes = kNumClasses,
      .reg_class = RegClass,
      .fixed_value_key = FixedValueKey,
      .defines_register = DefinesRegister,
      .implicit_uses = ImplicitUses,
      .forbidden_keys = ForbiddenKeys,
      .create_copy = CreateCopy,
      .rewrite_spill = RewriteSpill,
      .can_spill = CanSpill,
      .held_since_previous = HeldSincePrevious,
      .assign = Assign,
  };
  // The temps first, then the preserved registers in the order __enter saves
  // them, so that it saves few.
  for (int c = 0; c < kNumClasses; c++) {
    for (int r = 0; r < kNumRegs[c]; r++) {
      desc.classes[c].regs[desc.classes[c].num_regs++] = kKeyBase[c] + r;
    }
  }

  if (getenv("DAVECC_RA_CHECK") != NULL) {
    TargetRACheck(gen, &kInfo, NULL, stderr, 3);
  }
  if (!TargetAllocateRegisters(gen, &desc, NULL)) {
    return false;
  }
  for (TargetInstruction* inst = TargetFirstInstruction(gen); inst != NULL;
       inst = TargetNext(inst)) {
    if (TargetOpcodeEq(inst->opcode, W65C02_OP(fp))) {
      inst->reg = &allocator->fp_reg.base;
    } else if (TargetOpcodeEq(inst->opcode, W65C02_OP(sp))) {
      inst->reg = &allocator->sp_reg.base;
    } else if (IsSpill(inst)) {
      inst->reg = inst->operand[0]->reg;
    }
  }
  return true;
}
