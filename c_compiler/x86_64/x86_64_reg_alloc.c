//
//  x86_64_reg_alloc.c
//  c_compiler_library
//
//  Created by David Allison on 2/21/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "x86_64_reg_alloc.h"
#include <assert.h>
#include <limits.h>
#include "map.h"
#include "x86_64_codegen.h"
#include "x86_64_machine.h"
#include "target_basic_block.h"
#include "compiler.h"


static void AllocateRegister(X86_64RegisterAllocator* allocator,
                             TargetInstruction* inst);

static void Trap() {}

static void TrapInstruction(TargetInstruction* inst) {
  if (inst->id == 63) {
    Trap();
  }
}

static void TrapSpill(TargetInstruction* inst) {
  if (inst->id == 63) {
    Trap();
  }
}

static void TrapReload(TargetInstruction* inst) {
  if (inst->id == 781) {
    Trap();
  }
}

static void TrapBlock(TargetBasicBlock* block) {
  if (block->block_id == 82) {
    Trap();
  }
}

static void InitializeRegister(X86_64Register* reg, int num, X86_64RegisterType type) {
  TargetRegisterInit(&reg->base, num);
  reg->type = type;
}

void X86_64RegisterAllocatorInit(X86_64RegisterAllocator* allocator, X86_64Generator* rv) {
  allocator->rv = rv;

  for (int i = 0; i < X86_64_NUM_INT_REGS; i++) {
    InitializeRegister(&allocator->int_regs[i], i, kX86_64RegTypeInt);
  }

  for (int i = 0; i < X86_64_NUM_FLOAT_REGS; i++) {
    InitializeRegister(&allocator->float_regs[i], i, kX86_64RegTypeFloat);
  }

  // Reserve some registers.
  allocator->int_regs[X86_64_INT_ZERO_REG].base.reserved = true;
  allocator->int_regs[X86_64_FP_REG].base.reserved = true;
  allocator->int_regs[X86_64_SP_REG].base.reserved = true;
  allocator->int_regs[X86_64_SPILL_ADDR].base.reserved = true;

  // The 32-slot logical register file aliases x86-64's 16 physical registers,
  // and some slots place a register in the wrong ABI class.  Reserve those so
  // the allocator never uses a physical register inconsistently:
  //
  //  * Slot 7 is r12 living in the temp range.  r12 is callee-saved and is not
  //    recognised by IsSavedReg() at this slot, so it would never be preserved
  //    in the prologue and a leaf function would clobber the caller's r12.  Use
  //    r12 only through its saved-range slot (18).
  //  * Slots 22-25 are duplicate r12/r13/r14/r15 entries that alias slots
  //    18-21, and slots 26-27 place caller-saved r10/r11 in the saved range
  //    (they would be clobbered by any callee, so they cannot hold values that
  //    must survive a call).
  allocator->int_regs[7].base.reserved = true;
  for (int i = 22; i <= 27; i++) {
    allocator->int_regs[i].base.reserved = true;
  }

  BitSetInit(&allocator->used_int_regs);
  BitSetInit(&allocator->used_float_regs);
  
  allocator->current_spilled_region_size = 0;
  allocator->max_spilled_region_size = 0;
  BitSetInit(&allocator->preserved_instructions);
  MapInitForPointerKeys(&allocator->varreg_spills);
  allocator->spill_after_definition = false;
  allocator->pinned_int_phys = 0;
  allocator->pinned_float_phys = 0;
}

X86_64RegisterAllocator* NewX86_64RegisterAllocator(X86_64Generator* pcode) {
  X86_64RegisterAllocator* reg_alloc = malloc(sizeof(X86_64RegisterAllocator));
  X86_64RegisterAllocatorInit(reg_alloc, pcode);
  return reg_alloc;
}

void X86_64RegisterAllocatorDestruct(X86_64RegisterAllocator* allocator) {
  BitSetDestruct(&allocator->used_int_regs);
  BitSetDestruct(&allocator->used_float_regs);
  BitSetDestruct(&allocator->preserved_instructions);
}

void X86_64RegisterAllocatorDelete(X86_64RegisterAllocator* alloc) {
  X86_64RegisterAllocatorDestruct(alloc);
  free(alloc);
}

// x86-64's ABI divides registers into various ranges, some of which are
// temporary and some preserved across calls.  We use this array to
// search for registers.
static struct {
  X86_64RegisterType type;  // Register type.
  int start;            // Start of range.
  int end;              // End of range.
  const char* prefix;   // Register name prefix.
  int base;
  bool temp;
} register_ranges[] = {
    {kX86_64RegTypeInt, X86_64_INT_TEMP_START_1, X86_64_INT_TEMP_END_1, "t", 0, true},
    {kX86_64RegTypeInt, X86_64_INT_TEMP_START_2, X86_64_INT_TEMP_END_2, "t", 3, true},
    {kX86_64RegTypeInt, X86_64_INT_ARG_START, X86_64_INT_ARG_END, "a", 0, true},
    {kX86_64RegTypeInt, X86_64_RET_REG, X86_64_RET_REG, "ra", 0, true},
    {kX86_64RegTypeInt, X86_64_INT_SAVED_START_1, X86_64_INT_SAVED_END_1, "s", 0, false},
    {kX86_64RegTypeInt, X86_64_INT_SAVED_START_2, X86_64_INT_SAVED_END_2, "s", 2, false},
    {kX86_64RegTypeFloat, X86_64_FP_TEMP_START_1, X86_64_FP_TEMP_END_1, "ft", 0, true},
    {kX86_64RegTypeFloat, X86_64_FP_TEMP_START_2, X86_64_FP_TEMP_END_2, "ft", 2, true},
    {kX86_64RegTypeFloat, X86_64_FP_ARG_START, X86_64_FP_ARG_END, "fa", 0, true},
    {kX86_64RegTypeFloat, X86_64_FP_SAVED_START_1, X86_64_FP_SAVED_END_1, "fs", 0, false},
    {kX86_64RegTypeFloat, X86_64_FP_SAVED_START_2, X86_64_FP_SAVED_END_2, "fs", 2, false},
};

#define NUM_REG_RANGES (sizeof(register_ranges) / sizeof(register_ranges[0]))

static void DumpRegisters(X86_64RegisterAllocator* allocator) {
  char buf[32];
  for (X86_64RegisterType type = kX86_64RegTypeInt; type <= kX86_64RegTypeFloat; type++) {
    X86_64Register* regs =
        type == kX86_64RegTypeInt ? allocator->int_regs : allocator->float_regs;
    for (int i = 0; i < NUM_REG_RANGES; i++) {
      if (register_ranges[i].type == type) {
        for (int j = register_ranges[i].start; j <= register_ranges[i].end; j++) {
          if (regs[j].base.owner == NULL) {
            printf("%s(x%d): free\n", X86_64RegisterName(&regs[j], buf, sizeof(buf)), regs[j].base.num);
          } else {
            TargetInstruction* owner = regs[j].base.owner;
            printf("%s(x%d): owner: @%d %s\n", X86_64RegisterName(&regs[j], buf, sizeof(buf)), regs[j].base.num, owner->id,
                   X86_64OpcodeName(owner->opcode));
          }
        }
      }
    }
  }
}

static void AssignRegister(X86_64Register* reg, TargetInstruction* inst) {
  assert(inst->reg == NULL);
  inst->reg = &reg->base;
  reg->base.owner = inst;
  inst->uses = (int)inst->users.length;
  inst->flags |= TARGET_INST_PROCESSED;
}

// The logical register file has 32 slots but x86-64 only has 16 integer and 16
// xmm physical registers, so several slots alias the same physical register
// (e.g. logical slots 7, 18 and 22 all denote r12).  Map a logical slot to its
// canonical physical register number so the allocator can avoid handing the
// same physical register to two simultaneously-live values.
static int X86_64IntPhysical(int slot) {
  // Canonical x86-64 numbering: rax=0 rcx=1 rdx=2 rbx=3 rsp=4 rbp=5 rsi=6 rdi=7
  // r8=8 r9=9 r10=10 r11=11 r12=12 r13=13 r14=14 r15=15.  This mirrors the
  // kIntRegNames table used for printing.
  static const int kPhys[X86_64_NUM_INT_REGS] = {
      0,  0,  4,  11, 10, 10, 11, 12, 5,  3,  7,  6,  2,  1,  8,  9,
      10, 11, 12, 13, 14, 15, 12, 13, 14, 15, 10, 11, 8,  9,  10, 11,
  };
  return (slot >= 0 && slot < X86_64_NUM_INT_REGS) ? kPhys[slot] : -1;
}

static int X86_64FloatPhysical(int slot) {
  // The xmm name table is xmm0..xmm15 repeated, so the physical register is the
  // slot modulo 16.
  return slot & 15;
}

// Physical registers the emitter uses implicitly as scratch and which therefore
// must never hold an allocator-managed value: r11 (spill-address / general
// scratch in several emit paths) and xmm15 (used to shuttle integer values into
// the SSE unit for ucomiss etc.).
static bool X86_64PhysicalReserved(X86_64RegisterType type, int phys) {
  if (type == kX86_64RegTypeInt) {
    // r11: spill-address register and general scratch in several emit paths.
    return phys == 11;
  }
  return phys == 15;  // xmm15: scratch for moving integers into the SSE unit.
}

// Is the physical register denoted by logical slot |slot| available, i.e. not
// implicitly reserved and not already owned by a value in any aliasing slot?
static bool X86_64PhysicalAvailable(X86_64RegisterAllocator* allocator,
                                    X86_64RegisterType type, int slot) {
  X86_64Register* regs =
      type == kX86_64RegTypeInt ? allocator->int_regs : allocator->float_regs;
  int num_regs =
      type == kX86_64RegTypeInt ? X86_64_NUM_INT_REGS : X86_64_NUM_FLOAT_REGS;
  int phys = type == kX86_64RegTypeInt ? X86_64IntPhysical(slot)
                                       : X86_64FloatPhysical(slot);
  if (X86_64PhysicalReserved(type, phys)) {
    return false;
  }
  unsigned pinned = type == kX86_64RegTypeInt ? allocator->pinned_int_phys
                                              : allocator->pinned_float_phys;
  if ((pinned & (1u << phys)) != 0 && !regs[slot].base.reserved) {
    // A register variable holds this physical register for the whole function
    // through the slot pinned to it.  Reaching it through any other slot hands
    // the same register to a second value: the register variable's slot looks
    // untouched, so nothing stops the variable being assigned over the top of
    // a value that is still live, or the other way about.
    return false;
  }
  for (int k = 0; k < num_regs; k++) {
    if (k == slot) {
      continue;
    }
    int other = type == kX86_64RegTypeInt ? X86_64IntPhysical(k)
                                          : X86_64FloatPhysical(k);
    if (other == phys && regs[k].base.owner != NULL) {
      return false;
    }
  }
  return true;
}

static X86_64Register* FindFreeRegister(X86_64RegisterAllocator* allocator,
                                    X86_64RegisterType type, bool can_use_temp) {
  X86_64Register* regs =
      type == kX86_64RegTypeInt ? allocator->int_regs : allocator->float_regs;
  for (size_t i = 0; i < NUM_REG_RANGES; i++) {
    if (register_ranges[i].type == type) {
      // If we are told not to use a temp register, ignore any that are
      // marked as temp.
      if (!can_use_temp && register_ranges[i].temp) {
        continue;
      }
      for (int j = register_ranges[i].start; j <= register_ranges[i].end; j++) {
        if (!regs[j].base.reserved && regs[j].base.owner == NULL &&
            X86_64PhysicalAvailable(allocator, type, j)) {
          // If we use the return address register, we must save it and
          // therefore we are not a leaf procedure.
          if (regs[j].base.num == X86_64_RET_REG) {
            allocator->rv->not_leaf = true;
          }
          return &regs[j];
        }
      }
    }
  }

  // No registers available.
  return NULL;
}

static void FreeRegister(X86_64RegisterAllocator* allocator, X86_64Register* reg) {
  reg->base.owner = NULL;
}

// Free up any registers that are no longer needed by the instruction.  This
// frees up all now-unused operands and destination.
static void FreeRegisters(X86_64RegisterAllocator* allocator,
                          TargetInstruction* inst) {
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    if (inst->operand[i] != NULL) {
      TargetInstruction* op = inst->operand[i];
      if (X86_64IsFixedRegister(op)) {
        continue;
      }
      TargetRegister* reg = op->reg;
      if (reg != NULL && !reg->reserved && reg->owner != NULL && op->uses > 0) {
        op->uses--;
        assert(op->uses >= 0);
        // A value that is live out of this block can be consumed again after a
        // back edge even when its single static user has just reduced the
        // global use counter to zero.  Keep its register through the block;
        // otherwise a destructive two-address instruction may reuse and
        // overwrite a loop invariant that the next iteration still needs.
        if (op->uses == 0 &&
            !TargetBasicBlockOutputs(inst->block, op)) {
          if (reg->owner == op) {
            FreeRegister(allocator, (X86_64Register*)reg);
          }
        }
      }
    }
  }
  
}

// Is the register meant to be saved by the callee?
static bool IsSavedReg(X86_64Register* reg) {
  int num = reg->base.num;
  if ((num >= X86_64_INT_SAVED_START_1 && num <= X86_64_INT_SAVED_END_1) ||
      (num >= X86_64_INT_SAVED_START_2 && num <= X86_64_INT_SAVED_END_2) ||
      (num >= X86_64_FP_SAVED_START_1 && num <= X86_64_FP_SAVED_END_1) ||
      (num >= X86_64_FP_SAVED_START_2 && num <= X86_64_FP_SAVED_END_2)) {
    return true;
  }
  return false;
}

// Calculate the cost of spilling the instruction.  This
// takes the number of uses and the loop nesting level into
// account.  The cost starts out as the number of
// uses left for the instruction.  It is then multiplied
// by the loop nesting level for all its users that have not yet
// got a register (+1) all the way to the top of the dominator tree.
//
// Blocks outside loops will have no effect on the score but
// blocks with a loop_nesting value of 1 will double the cost.  A
// loop_nesting value of 2 will quadruple the cost, and so on.
static int SpillCost(TargetInstruction* inst) {
  int cost = inst->uses;
  for (size_t i = 0; i < inst->users.length; i++) {
    TargetInstruction* user = inst->users.value.p[i];
    if (user->reg == NULL) {
      TargetBasicBlock* block = user->block;
      while (block != NULL) {
        cost *= block->loop_nesting + 1;
        block = block->idom;
      }
    }
  }

  return cost;
}

// True when spilling this register's owner is unsound for this allocator's
// "store once / reload for reads" spill model.  A register variable can be
// reassigned (e.g. a loop induction variable `p = p + 1`); the spill slot
// would not be updated on those writes, leaving back-edge reloads stale.  So
// such values must be kept in a register and never chosen as a spill victim
// unless there is no alternative.
//
// Spilling also leaves already-processed reads of the value pointing at the
// physical register rather than the slot, which only works while every such
// read happens before the register is handed to something else.  Down a
// straight line it does, because the allocator processes blocks in dominator
// order; around a loop it does not, so a read in any block control can come
// back to is a read of whatever the new owner left there.  |reentered| holds
// those blocks.
static bool IsUnsafeSpillVictim(TargetInstruction* owner, BitSet* reentered) {
  if (X86_64IsVarRegister(owner)) {
    return true;
  }
  if (owner->dest != NULL && X86_64IsVarRegister(owner->dest)) {
    return true;
  }
  for (size_t i = 0; i < owner->users.length; i++) {
    TargetInstruction* user = owner->users.value.p[i];
    if ((user->flags & TARGET_INST_PROCESSED) != 0 && user->block != NULL &&
        BitSetContains(reentered, user->block->block_id)) {
      return true;
    }
  }
  return false;
}

// Returns the cheapest value whose register can be taken without breaking a
// read that has already been handed that register, or NULL when there is none.
// |unsafe|, when not NULL, receives the cheapest value that could be taken if
// there were no such reads, for a caller that has no way to proceed without
// taking a register from something.
static TargetInstruction* FindSpillVictim(X86_64RegisterAllocator* allocator,
                                           X86_64RegisterType type,
                                           bool can_use_temp,
                                           TargetBasicBlock* block,
                                           TargetInstruction** unsafe) {
  X86_64Register* regs =
      type == kX86_64RegTypeInt ? allocator->int_regs : allocator->float_regs;
  BitSet reentered;
  BitSetInit(&reentered);
  TargetBasicBlockReachableAfter(&allocator->rv->base, block, &reentered);
  int min_cost = INT_MAX;
  TargetInstruction* victim = NULL;
  TargetInstruction* fallback_victim = NULL;
  int fallback_cost = INT_MAX;
  // Find the instruction with the lowest spill cost.  Prefer values that are
  // safe to spill (not reassignable register variables); only fall back to an
  // unsafe victim if no safe register is available.
  for (size_t i = 0; i < NUM_REG_RANGES; i++) {
    if (register_ranges[i].type == type) {
      if (!can_use_temp && register_ranges[i].temp) {
        continue;
      }
      for (int j = register_ranges[i].start; j <= register_ranges[i].end; j++) {
        if (!regs[j].base.reserved && regs[j].base.owner != NULL) {
          TargetInstruction* owner = regs[j].base.owner;
          assert((owner->flags & TARGET_INST_SPILLED) == 0);
          // A fixed-register value (an argument register a0..a7 / fa0..fa7, a
          // call result, ...) lives in one specific physical register because
          // its consumer (the call, the return) reads that exact register.
          // The spill model stores the value and reloads it into some *other*
          // temporary, so spilling a fixed register silently delivers the
          // argument/result in the wrong place.  Never choose one as a victim.
          if (X86_64IsFixedRegister(owner)) {
            continue;
          }
          int cost = SpillCost(owner);
          if (IsUnsafeSpillVictim(owner, &reentered)) {
            if (cost < fallback_cost) {
              fallback_cost = cost;
              fallback_victim = owner;
            }
            continue;
          }
          if (cost < min_cost) {
            min_cost = cost;
            victim = owner;
          }
        }
      }
    }
  }
  BitSetDestruct(&reentered);
  if (victim == NULL && fallback_victim == NULL) {
    DumpRegisters(allocator);
    abort();
  }
  if (unsafe != NULL) {
    *unsafe = fallback_victim;
    return victim;
  }
  // The caller cannot deal with there being nothing safe to take, so give it
  // the cheapest unsafe candidate as this always did.
  return victim != NULL ? victim : fallback_victim;
}

static bool NotProcessed(TargetInstruction* inst, void* data) {
  return (inst->flags & TARGET_INST_PROCESSED) == 0;
}

// Is inst a definition (write) of the given variable register?  A definition
// either targets it via inst->dest or is an rmov that copies a value into it.
static bool IsVarRegDef(TargetInstruction* inst, TargetInstruction* varreg) {
  if (inst->dest == varreg) {
    return true;
  }
  X86_64Opcode op = (X86_64Opcode)inst->opcode;
  if ((op == X86_64_OP(mv) || op == X86_64_OP(fmv_s) || op == X86_64_OP(fmv_d)) &&
      inst->dest == NULL && inst->operand[0] == varreg &&
      inst->operand[1] != NULL) {
    return true;
  }
  return false;
}

// When a variable register is spilled, its stack slot is initialised once (by
// `spill`, placed after `first_use`).  But the variable may have already been
// redefined before the spill decision was made (it shares one physical
// register, so earlier redefinitions only updated that register).  Insert a
// store after every already-processed redefinition so the slot stays current;
// future redefinitions are handled by SyncSpilledVarReg.
static void InsertVarRegStoreBacks(X86_64RegisterAllocator* allocator,
                                   TargetInstruction* varreg,
                                   TargetInstruction* spill,
                                   TargetInstruction* first_use) {
  TargetGenerator* gen = &allocator->rv->base;
  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    TargetBasicBlock* block = gen->basic_blocks.value.p[b];
    for (TargetInstruction* inst = block->code;
         inst != NULL && inst != block->end_code;
         inst = TargetNext(inst)) {
      if (inst == first_use) {
        continue;
      }
      if ((inst->flags & TARGET_INST_PROCESSED) == 0 || inst->reg == NULL) {
        continue;
      }
      if ((int)inst->opcode == (int)X86_64_OP(spill) ||
          (int)inst->opcode == (int)X86_64_OP(reload)) {
        continue;
      }
      if (!IsVarRegDef(inst, varreg)) {
        continue;
      }
      TargetInstruction* store = TargetNewInstruction2(
          (TargetOpcode)X86_64_OP(spill), varreg, spill->operand[1]);
      store->reg = inst->reg;
      store->flags |= TARGET_INST_PROCESSED;
      TargetBasicBlockEmitAfter(gen, block, store, inst);
      // Skip past the store we just inserted.
      inst = store;
    }
  }
}

// True when |target| is produced not by itself but by one or more *other*
// instructions writing it via ->dest (or an rmov into it).  The canonical
// example is the result temporary of a conditional (?:) expression: a bare
// `tmp` node is emitted at the top of the function and each arm assigns it
// with `arm->dest = tmp`.  Such a value can have several definitions in
// mutually-exclusive blocks, so - exactly like a reassignable variable
// register - it must use the "store back after every definition, reload for
// every read" spill model.  Spilling it with the single-definition model
// (store once, right after the `tmp` node itself) captures the register's
// undefined value at the declaration point and never observes the real
// per-branch definitions, so every reload reads garbage.
static bool InstructionHasExternalDefs(X86_64RegisterAllocator* allocator,
                                       TargetInstruction* target) {
  TargetGenerator* gen = &allocator->rv->base;
  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    TargetBasicBlock* block = gen->basic_blocks.value.p[b];
    for (TargetInstruction* inst = block->code;
         inst != NULL && inst != block->end_code;
         inst = TargetNext(inst)) {
      if (inst == target) {
        continue;
      }
      if ((int)inst->opcode == (int)X86_64_OP(spill) ||
          (int)inst->opcode == (int)X86_64_OP(reload)) {
        continue;
      }
      if (IsVarRegDef(inst, target)) {
        return true;
      }
    }
  }
  return false;
}

static X86_64Register* SpillInstruction(X86_64RegisterAllocator* allocator, TargetInstruction* inst) {
  X86_64Register* reg = (X86_64Register*)inst->reg;    // Current register.

  // Spilled values are addressed relative to the frame pointer (rbp).  A leaf
  // function that emits no frame never establishes rbp, so its spill slots
  // would alias the caller's frame.  Force a real stack frame whenever we
  // spill.  (The allocator's leaf test keys off num_calls only, so this does
  // not perturb register-variable slot selection -- only frame emission.)
  allocator->rv->not_leaf = true;
  
  // Generate a spill instruction with 2 operands:
  // 1. Instruction to spill (not set yet)
  // 2. Offset into spill region.
  // We don't set the spilled instruction yet because TargetRetargetInstruction
  // will see it and retarget it to the spill.
  TrapSpill(inst);
  TargetInstruction* spill = TargetNewInstruction2((TargetOpcode)X86_64_OP(spill), NULL,
                                                   TargetGetIntConstant(&allocator->rv->base,
                                                                        NULL,
                                                                        kTargetType32Bit,
                                                                        allocator->current_spilled_region_size));
  allocator->current_spilled_region_size += 8;    // Space for one register.
  if (allocator->current_spilled_region_size > allocator->max_spilled_region_size) {
    allocator->max_spilled_region_size = allocator->current_spilled_region_size;
  }
  if (X86_64IsVarRegister(inst)) {
    // Spilling a varreg.  This instruction is in the entry block but
    // it can't be spilled there.  It needs to be spilled at its first
    // use (the assignment to it).  This is going to be the first user
    // of the instruction.
    assert(inst->users.length > 0);
    TargetInstruction* first_use = inst->users.value.p[0];
    TargetBasicBlockEmitAfter(&allocator->rv->base, first_use->block, spill, first_use);
    // Remember the slot so redefinitions of this variable register can store
    // their new value back into it (see SyncSpilledVarReg).
    MapKeyValue kv = {.key.p = inst, .value.p = spill};
    MapInsert(&allocator->varreg_spills, kv);
    // Catch up any redefinitions that were already processed before this spill.
    InsertVarRegStoreBacks(allocator, inst, spill, first_use);
  } else if (InstructionHasExternalDefs(allocator, inst)) {
    // A value written by separate instructions via ->dest (e.g. the result
    // temporary of a `?:` expression, assigned once per arm).  Use the
    // variable-register store-back model.  The value node itself (a bare `tmp`
    // emitted at function entry) carries no meaningful value at its declaration
    // point, so - unlike a varreg - we must not emit an initialising store
    // there.  Instead the `spill` handle stays out of the instruction stream
    // (it only supplies the slot offset for reloads and store-backs), a store
    // is inserted after every already-processed definition, and definitions
    // processed after this spill are handled by SyncSpilledVarReg.
    TargetTrackOrphanInstruction(&allocator->rv->base, spill);
    MapKeyValue kv = {.key.p = inst, .value.p = spill};
    MapInsert(&allocator->varreg_spills, kv);
    InsertVarRegStoreBacks(allocator, inst, spill, /*first_use=*/NULL);
  } else {
    // Emit spill instruction just after spilled instruction.
    TargetBasicBlockEmitAfter(&allocator->rv->base, inst->block, spill, inst);
    // Keep the slot discoverable if an already-tagged future user was not
    // retargeted by TargetRetargetInstructionIf.
    MapKeyValue kv = {.key.p = inst, .value.p = spill};
    MapInsert(&allocator->varreg_spills, kv);
  }
  
  // Retarget all uses of the original instruction to the spill.  If the
  // user has already been processed this will have no effect.
  // NOTE: this will transfer all uses of the inst to the spill, leaving
  // the users of inst empty and its uses count 0.
  TargetRetargetInstructionIf(inst, spill, NotProcessed, NULL);
  spill->uses = inst->uses;
  spill->operand[0] = inst;
  spill->reg = inst->reg;
  reg->base.owner = NULL;
  inst->flags |= TARGET_INST_SPILLED;
  return reg;
}

// Force the physical register denoted by logical slot |slot| (and any aliasing
// logical slot) to be free, spilling any live value currently occupying it.
// This is used when an instruction must land in a specific ABI register (an
// argument register, a return register, ...): a general temporary may already
// have been handed that register by the dynamic allocator, and simply
// reassigning it would silently clobber the still-live temporary (the classic
// symptom: a stack-passed argument's address computed into rdi/r8/r9 and then
// overwritten by the register-argument move before its store executes).  The
// spilled value's pending uses are reloaded on demand, mirroring
// ReserveIdivRegisters which evicts rax/rdx the same way.
static void EvictPhysicalRegister(X86_64RegisterAllocator* allocator,
                                  X86_64RegisterType type, int slot,
                                  TargetInstruction* keep) {
  X86_64Register* regs =
      type == kX86_64RegTypeInt ? allocator->int_regs : allocator->float_regs;
  int num_regs =
      type == kX86_64RegTypeInt ? X86_64_NUM_INT_REGS : X86_64_NUM_FLOAT_REGS;
  int phys = type == kX86_64RegTypeInt ? X86_64IntPhysical(slot)
                                       : X86_64FloatPhysical(slot);
  for (int k = 0; k < num_regs; k++) {
    int other = type == kX86_64RegTypeInt ? X86_64IntPhysical(k)
                                          : X86_64FloatPhysical(k);
    if (other != phys) {
      continue;
    }
    TargetInstruction* owner = regs[k].base.owner;
    if (owner == NULL || owner == keep) {
      continue;
    }
    if (regs[k].base.reserved) {
      continue;
    }
    if ((owner->flags & TARGET_INST_SPILLED) != 0) {
      continue;
    }
    if (owner->uses <= 0) {
      // Stale ownership of a value with no remaining uses; just release it.
      regs[k].base.owner = NULL;
      continue;
    }
    SpillInstruction(allocator, owner);
  }
}

// A variable register that has been spilled keeps a working copy in its
// register, but every redefinition must also be written back to its stack
// slot.  Otherwise a later reload (for example on a loop back-edge that tests
// the variable) would observe a stale value.  def_inst is the instruction that
// just produced the new value (in def_inst->reg); varreg is the variable
// register it assigns.
static void SyncSpilledVarReg(X86_64RegisterAllocator* allocator,
                              TargetInstruction* def_inst,
                              TargetInstruction* varreg) {
  if (varreg == NULL) {
    return;
  }
  if ((varreg->flags & TARGET_INST_SPILLED) == 0) {
    return;
  }
  // The store-back model applies to any spilled value that keeps its slot fresh
  // across multiple definitions: variable registers and externally-defined
  // temporaries (see InstructionHasExternalDefs).  Both are recorded in
  // varreg_spills when spilled; a value absent from the map uses the plain
  // single-definition model and needs no store-back here.
  TargetInstruction* orig_spill =
      MapFindPointerKey(&allocator->varreg_spills, varreg);
  if (orig_spill == NULL || def_inst->reg == NULL) {
    return;
  }
  // Reuse the original slot's offset operand so the store and all reloads
  // reference the same stack location.
  TargetInstruction* store = TargetNewInstruction2(
      (TargetOpcode)X86_64_OP(spill), varreg, orig_spill->operand[1]);
  store->reg = def_inst->reg;
  store->flags |= TARGET_INST_PROCESSED;
  TargetBasicBlockEmitAfter(&allocator->rv->base, def_inst->block, store,
                            def_inst);
}

// r11, which the allocator never hands out and the emitter borrows for the odd
// job that needs a register of its own, such as addressing a spill slot.  A
// value put here has to be consumed or stored before the next instruction.
// There is no counterpart in the xmm file: xmm15 is the emitter's scratch there
// but it is not marked reserved, so handing it out would put it in reach of the
// spill victim search.
static X86_64Register* ScratchRegister(X86_64RegisterAllocator* allocator,
                                       X86_64RegisterType type) {
  return type == kX86_64RegTypeInt ? &allocator->int_regs[X86_64_SPILL_ADDR]
                                   : NULL;
}

// A value whose register a read has already been handed cannot be spilled: that
// read keeps naming the register, and once control comes back round a loop the
// register's next owner is what it finds there.  When every register holds such
// a value there is no victim to take, and the way out is to spill the value
// being defined here instead: it is computed into the scratch register and
// written straight to a slot, and every read of it is still ahead of us and so
// gets redirected to that slot.  A value written by other instructions than
// itself takes the same route: the node carries nothing at its own position, so
// the slot is only registered here and each definition stores into it as it is
// reached.
//
// A variable register is the one thing this cannot do, because its slot is
// initialized at its first use rather than at the node itself, which sits in
// the entry block; likewise a value bound to a fixed register, which its
// consumer reads by name.
static bool CanSpillAfterDefinition(TargetInstruction* inst) {
  return inst != NULL && inst->users.length > 0 && inst->block != NULL &&
         !X86_64IsVarRegister(inst) && !X86_64IsFixedRegister(inst) &&
         (inst->dest == NULL || !X86_64IsVarRegister(inst->dest));
}

static X86_64Register* AllocateRegisterWithType(X86_64RegisterAllocator* allocator,
                                            TargetBasicBlock* block,
                                            TargetInstruction* inst,
                                            X86_64RegisterType type,
                                            bool can_use_temp) {
  X86_64Register* reg = FindFreeRegister(allocator, type, can_use_temp);

  if (reg == NULL) {
    X86_64Register* scratch = ScratchRegister(allocator, type);
    TargetInstruction* unsafe = NULL;
    TargetInstruction* victim = FindSpillVictim(
        allocator, type, can_use_temp, block,
        scratch != NULL && CanSpillAfterDefinition(inst) ? &unsafe : NULL);
    if (victim == NULL && unsafe != NULL) {
      allocator->spill_after_definition = true;
      return scratch;
    }
    reg = SpillInstruction(allocator, victim);
  }
  assert(reg != NULL);

  if (reg->base.reserved) {
    return reg;
  }
  if (!IsSavedReg(reg)) {
    return reg;
  }
  switch (type) {
    case kX86_64RegTypeInt:
      BitSetInsert(&allocator->used_int_regs, reg->base.num);
      break;
    case kX86_64RegTypeFloat:
      BitSetInsert(&allocator->used_float_regs, reg->base.num);
      break;
  }
  return reg;
}

static void ReserveIdivRegisters(X86_64RegisterAllocator* allocator) {
  static const int kIdivRegs[] = {X86_64_RET_REG, X86_64_INT_ARG_START + 2};
  for (size_t i = 0; i < sizeof(kIdivRegs) / sizeof(kIdivRegs[0]); i++) {
    X86_64Register* reg = &allocator->int_regs[kIdivRegs[i]];
    if (reg->base.owner != NULL) {
      SpillInstruction(allocator, reg->base.owner);
    }
  }
}

static X86_64RegisterType RegisterTypeFromInstruction(TargetInstruction* inst) {
  switch ((X86_64Opcode)inst->opcode) {
    case X86_64_OP(constf):
    case X86_64_OP(constd):
    case X86_64_OP(fmv_s):
    case X86_64_OP(fmv_d):
    case X86_64_OP(fa0):
    case X86_64_OP(fa1):
    case X86_64_OP(fa2):
    case X86_64_OP(fa3):
    case X86_64_OP(fa4):
    case X86_64_OP(fa5):
    case X86_64_OP(fa6):
    case X86_64_OP(fa7):
    case X86_64_OP(fvarreg):
    case X86_64_OP(loadss):
    case X86_64_OP(loadsd):
    case X86_64_OP(storess):
    case X86_64_OP(storesd):
    case X86_64_OP(addss):
    case X86_64_OP(addsd):
    case X86_64_OP(subss):
    case X86_64_OP(subsd):
    case X86_64_OP(mulss):
    case X86_64_OP(mulsd):
    case X86_64_OP(divss):
    case X86_64_OP(divsd):
    case X86_64_OP(sqrtss):
    case X86_64_OP(sqrtsd):
    case X86_64_OP(ucomiss):
    case X86_64_OP(ucomisd):
    case X86_64_OP(cvtsi2ss):
    case X86_64_OP(cvtsi2sd):
    case X86_64_OP(cvtss2sd):
    case X86_64_OP(cvtsd2ss):
    case X86_64_OP(movss):
    case X86_64_OP(movsd):
    case X86_64_OP(movd):
    case X86_64_OP(fneg_ss):
    case X86_64_OP(fneg_sd):
    case X86_64_OP(callf):
    case X86_64_OP(rcallf):
    case X86_64_OP(resultf):
    case X86_64_OP(resultd):
      return kX86_64RegTypeFloat;

    case X86_64_OP(cvttsd2si):
    case X86_64_OP(cvttss2si):
    case X86_64_OP(movq_xmm):
      return kX86_64RegTypeInt;

    default:
      return kX86_64RegTypeInt;
  }
}

// Can we use a temp register?  If not we will have to use a saved one and
// those are more expensive since they need to be saved on entry and reloaded
// on exit.
static bool CanUseTemp(X86_64RegisterAllocator* allocator, TargetInstruction* inst) {
  return !BitSetContains(&allocator->preserved_instructions, inst->id);
}

// Map a register variable's index to its fixed logical slot, or -1 if the
// variable should instead be allocated dynamically.  Register variables are
// pinned to a fixed physical register by index (FIRST_*_REG_VAR + varnum), the
// same way the structreturn opcode picks its register, so that values which are
// meant to share a register (e.g. an NRVO result and the struct-return pointer)
// land in the same place and so two distinct register variables never collide
// on one physical register.  x86-64's 32-slot logical file aliases its 16
// physical registers and some slots are reserved (scratch / wrong ABI class),
// so a slot that is reserved or maps to a reserved physical register cannot be
// used; such variables fall back to the dynamic allocator.
// Logical int slots that X86_64RegisterAllocatorInit() reserves because they
// alias another slot or place a register in the wrong ABI class.  Replicated
// here as a pure predicate so it is independent of the runtime `.reserved`
// flag, which ReserveVariableRegisters() also sets on register-variable slots.
static bool X86_64IntSlotStructurallyReserved(int slot) {
  if (slot == X86_64_INT_ZERO_REG || slot == X86_64_FP_REG ||
      slot == X86_64_SP_REG || slot == X86_64_SPILL_ADDR || slot == 7) {
    return true;
  }
  // Slots 22-25 duplicate r12-r15 (slots 18-21) and 26-27 place caller-saved
  // r10/r11 in the saved range, so they cannot hold call-surviving values.
  if (slot >= 22 && slot <= 27) {
    return true;
  }
  return false;
}

static int X86_64VarRegSlot(X86_64RegisterAllocator* allocator,
                            bool is_fp, bool is_leaf, int varnum) {
  (void)allocator;
  // Only integer register variables use fixed slots.  Floating-point variables
  // are allocated dynamically: x86-64 has no callee-saved xmm registers and the
  // logical "saved" fp slots are a fiction, so pinning them is unsound.
  if (is_fp) {
    return -1;
  }
  int first = is_leaf ? X86_64_FIRST_LEAF_INT_REG_VAR : X86_64_FIRST_INT_REG_VAR;
  int last = is_leaf ? X86_64_LAST_LEAF_INT_REG_VAR : X86_64_LAST_INT_REG_VAR;
  int slot = first + varnum;
  if (varnum < 0 || slot > last) {
    return -1;
  }
  if (X86_64PhysicalReserved(kX86_64RegTypeInt, X86_64IntPhysical(slot))) {
    return -1;
  }
  if (X86_64IntSlotStructurallyReserved(slot)) {
    return -1;
  }
  return slot;
}

static void AllocateVariableRegister(X86_64RegisterAllocator* allocator,
                                     TargetInstruction* inst) {
  bool is_leaf = allocator->rv->base.num_calls == 0 && compiler->optimize;

  for (size_t i = 0; i < allocator->rv->var_regs.length; i++) {
    RegisterVariable* var = allocator->rv->var_regs.value.p[i];
    if (var->inst != inst) {
      continue;
    }
    int slot = X86_64VarRegSlot(allocator, var->is_fp, is_leaf, var->varnum);
    if (slot < 0) {
      break;  // Fall back to dynamic allocation.
    }
    X86_64Register* regs =
        var->is_fp ? allocator->float_regs : allocator->int_regs;
    X86_64Register* reg = &regs[slot];
    AssignRegister(reg, inst);
    if (IsSavedReg(reg)) {
      if (var->is_fp) {
        BitSetInsert(&allocator->used_float_regs, reg->base.num);
      } else {
        BitSetInsert(&allocator->used_int_regs, reg->base.num);
      }
    }
    return;
  }

  // No fixed slot was available; allocate dynamically.
  X86_64RegisterType reg_type = RegisterTypeFromInstruction(inst);
  X86_64Register* reg = AllocateRegisterWithType(allocator, inst->block, inst,
                                 reg_type, CanUseTemp(allocator, inst));
  AssignRegister(reg, inst);
  if (IsSavedReg(reg)) {
    if (reg_type == kX86_64RegTypeFloat) {
      BitSetInsert(&allocator->used_float_regs, reg->base.num);
    } else {
      BitSetInsert(&allocator->used_int_regs, reg->base.num);
    }
  }
}

static COMPILER_UNUSED void AllocateForRmov(X86_64RegisterAllocator* allocator,
                            TargetInstruction* inst) {
  assert(((int)inst->opcode == (int)X86_64_OP(mv)) ||
         ((int)inst->opcode == (int)X86_64_OP(fmv_s)) ||
         ((int)inst->opcode == (int)X86_64_OP(fmv_d)));
  TargetInstruction* dest = inst->operand[0];
  TargetInstruction* src = inst->operand[1];

  if (X86_64IsVarRegister(dest) && dest->reg == NULL) {
    // Delayed allocation of variable register.
    AllocateVariableRegister(allocator, dest);
  }
  X86_64Register* reg = (X86_64Register*)dest->reg;
  assert(reg != NULL);
  
  if (((int)src->opcode == (int)X86_64_OP(spill))) {
    // If we are rmoving a spill we can just load it directly into the
    // destination register.  To do this, we convert the rmov
    // into a reload instruction
    inst->opcode = (TargetOpcode)X86_64_OP(reload);
    inst->operand[0] = src;
    inst->operand[1] = NULL;
    TrapReload(inst);
  } else {
    if (X86_64IsVarRegister(src) && src->reg == NULL) {
      // Delayed allocation of variable register.
      AllocateVariableRegister(allocator, src);
    }
    inst->operand[0]->uses++;
    FreeRegisters(allocator, inst);
  }
  
  inst->reg = &reg->base;
  inst->flags |= TARGET_INST_PROCESSED;

  // If this rmov redefines a spilled variable register, write the new value
  // back to its stack slot.
  SyncSpilledVarReg(allocator, inst, dest);
}


static void ReloadSpills(X86_64RegisterAllocator* allocator,
                         TargetInstruction* inst) {
  // The scratch register is only free for the gap between the reload and the
  // instruction that consumes it, so at most one of this instruction's operands
  // can come from there.
  bool scratch_taken = false;
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    TargetInstruction* op = inst->operand[i];
    TargetInstruction* spill = NULL;
    if (op != NULL && (int)op->opcode == (int)X86_64_OP(spill)) {
      spill = op;
    } else if (op != NULL && (op->flags & TARGET_INST_SPILLED) != 0) {
      spill = MapFindPointerKey(&allocator->varreg_spills, op);
    }
    if (spill != NULL) {
      TargetInstruction* reload = TargetNewInstruction1((TargetOpcode)X86_64_OP(reload),
                                                        spill);
      TrapReload(reload);
      TargetBasicBlockEmitBefore(&allocator->rv->base, inst->block, reload, inst);
      inst->operand[i] = reload;
      TargetInstruction* spilled_value =
          spill->operand[0] != NULL ? spill->operand[0] : op;
      X86_64RegisterType reg_type =
          RegisterTypeFromInstruction(spilled_value);
      X86_64Register* reg = FindFreeRegister(
          allocator, reg_type, CanUseTemp(allocator, reload));
      if (reg == NULL && !scratch_taken && X86_64IsStore(inst) && i == 1 &&
          reg_type == kX86_64RegTypeInt) {
        // A store needs its value and address simultaneously.  Under high
        // pressure, allocating the address reload can otherwise spill the
        // value reload that operand[0] already references; the address then
        // reuses that register and the store writes an address byte instead of
        // the value.  r11 is reserved as the spill-address scratch register,
        // so it is safe to use for this final address reload immediately before
        // the store.
        reg = ScratchRegister(allocator, reg_type);
        scratch_taken = true;
      }
      if (reg == NULL) {
        X86_64Register* scratch =
            scratch_taken ? NULL : ScratchRegister(allocator, reg_type);
        TargetInstruction* unsafe = NULL;
        TargetInstruction* victim = FindSpillVictim(
            allocator, reg_type, CanUseTemp(allocator, reload), inst->block,
            scratch != NULL ? &unsafe : NULL);
        if (victim == NULL && unsafe != NULL) {
          // Nothing can give up its register without breaking a read that
          // already has one.  This reload dies at the instruction it feeds, so
          // the scratch register carries it across that one gap.
          reg = scratch;
          scratch_taken = true;
        } else {
          reg = SpillInstruction(allocator, victim);
        }
      }
      AssignRegister(reg, reload);
      // ReloadSpills bypasses AllocateRegisterWithType, so record a saved
      // register here as well. Otherwise the generated function may use (for
      // example) rbx without preserving the caller's value.
      if (IsSavedReg(reg)) {
        if (reg_type == kX86_64RegTypeFloat) {
          BitSetInsert(&allocator->used_float_regs, reg->base.num);
        } else {
          BitSetInsert(&allocator->used_int_regs, reg->base.num);
        }
      }
      // This reload is for a single instruction.
      reload->uses = 1;
    }
  }
}

// True if |inst| is an argument-register pseudo-instruction (a0..a7 / fa0..fa7)
// that the call lowering uses as a forced destination.  These instructions are
// shared across all calls in a function, so binding a value to one must first
// evict any unrelated value occupying the physical register (see the call site
// in AllocateUsingDest).  Sets *type to the register file the destination uses.
static bool X86_64FixedArgDestType(TargetInstruction* inst,
                                   X86_64RegisterType* type) {
  switch ((X86_64Opcode)inst->opcode) {
    case X86_64_OP(a0):
    case X86_64_OP(a1):
    case X86_64_OP(a2):
    case X86_64_OP(a3):
    case X86_64_OP(a4):
    case X86_64_OP(a5):
    case X86_64_OP(a6):
    case X86_64_OP(a7):
      *type = kX86_64RegTypeInt;
      return true;
    case X86_64_OP(fa0):
    case X86_64_OP(fa1):
    case X86_64_OP(fa2):
    case X86_64_OP(fa3):
    case X86_64_OP(fa4):
    case X86_64_OP(fa5):
    case X86_64_OP(fa6):
    case X86_64_OP(fa7):
      *type = kX86_64RegTypeFloat;
      return true;
    default:
      return false;
  }
}

static bool AllocateUsingDest(X86_64RegisterAllocator* allocator,
                              TargetInstruction* inst) {
  if (inst->dest == NULL) {
    return false;
  }
  if (inst->dest->reg == NULL) {
    if (X86_64IsVarRegister(inst->dest)) {
      AllocateVariableRegister(allocator, inst->dest);
    } else {
      AllocateRegister(allocator, inst->dest);
    }
  }
  assert(inst->dest->reg != NULL);
  inst->reg = inst->dest->reg;
  // When the destination is a fixed ABI register (an argument register, a
  // return register, ...), the argument-register pseudo-instruction is shared
  // across every call in the function, so its physical register may already
  // have been handed to an unrelated live temporary by the dynamic allocator
  // (argument registers are caller-saved and sit in the temp search range).
  // Writing this instruction's result would silently clobber that temporary
  // (e.g. a stack-passed argument address parked in rdi/r8/r9 and overwritten
  // by the register-argument move before its store executes).  Spill any such
  // occupant first; its pending reads reload on demand.
  X86_64RegisterType fixed_dest_type;
  if (X86_64FixedArgDestType(inst->dest, &fixed_dest_type)) {
    EvictPhysicalRegister(allocator, fixed_dest_type, inst->dest->reg->num,
                          inst->dest);
    // The argument-register pseudo-instruction is shared, so an earlier call's
    // value in it may have been spilled (reserving rax/rdx for an idiv, for
    // example).  That spill retargeted the reads still pending to the spill
    // slot, and this instruction now puts a live value back in the register, so
    // the value no longer lives only in memory.  Leaving the flag set would
    // reload this call's argument from the previous call's slot, and would also
    // offer the register up as a spill victim while it is live.
    inst->dest->flags &= ~TARGET_INST_SPILLED;
    MapRemove(&allocator->varreg_spills, (MapKeyType){.p = inst->dest});
    inst->dest->reg->owner = inst->dest;
  }
  if ((inst->dest->flags & TARGET_INST_SPILLED) != 0 &&
      inst->reg->owner != NULL && inst->reg->owner != inst->dest) {
    // The destination is in a spill slot and the register it used to hold has
    // since been handed to another value, so writing it here would clobber that
    // value.  Route this assignment through the scratch register; the store-back
    // below writes the slot from there.
    X86_64Register* scratch = ScratchRegister(
        allocator, RegisterTypeFromInstruction(inst->dest));
    if (scratch != NULL) {
      inst->reg = &scratch->base;
    }
  }
  if (inst->operand[0] != NULL) {
    inst->operand[0]->uses++;
  }
  FreeRegisters(allocator, inst);
  inst->flags |= TARGET_INST_PROCESSED;

  // The result now lives in the destination's register, so this instruction is
  // what keeps that register occupied.  FreeRegisters goes by the
  // destination's own use count, which this instruction has just brought to
  // zero, and would otherwise offer the register to the next value while reads
  // of this result still expect to find it there.
  if (inst->uses > 0 && inst->reg != NULL && !inst->reg->reserved &&
      inst->reg->owner == NULL) {
    inst->reg->owner = inst;
  }

  // If this instruction redefines a spilled variable register, write the new
  // value back to its stack slot.
  SyncSpilledVarReg(allocator, inst, inst->dest);
  return true;
}

static void AllocateRegister(X86_64RegisterAllocator* allocator,
                             TargetInstruction* inst) {
   bool is_leaf = allocator->rv->base.num_calls == 0 &&
      compiler->optimize;

  X86_64Opcode opcode = (X86_64Opcode)inst->opcode;
  
  TrapInstruction(inst);

  // If we already have a register allocated (as can be the case
  // for an ivarreg that is the dest of another instruction) don't
  // reallocate register.
  if (inst->reg != NULL) {
    return;
  }

  // rmov instructions use the register allocated to their first
  // operand as their own register.
  if ((opcode == X86_64_OP(mv) || opcode == X86_64_OP(fmv_s) ||
      opcode == X86_64_OP(fmv_d)) && inst->dest == NULL &&
      inst->operand[1] != NULL) {
    AllocateForRmov(allocator, inst);
    return;
  }

  if (X86_64IsVarRegister(inst)) {
    // Variable regsiter.  Delay allocation until it's assigned to.
    // It will be assigned to by an rmov or from a destination
    // assignemnt.
    return;
  }

  // Destination-coalesced instructions return early from allocation, but
  // their source operands still need to be restored from spill slots first.
  ReloadSpills(allocator, inst);

  if (AllocateUsingDest(allocator, inst)) {
    return;
  }

  X86_64Register* reg;

  // Free up any registers we can.
  FreeRegisters(allocator, inst);

  switch ((X86_64Opcode)inst->opcode) {
    case X86_64_OP(const8):
    case X86_64_OP(const16):
    case X86_64_OP(const32):
    case X86_64_OP(const64):
    case X86_64_OP(constf):
    case X86_64_OP(constd):
    case X86_64_OP(symbol):
    case X86_64_OP(cmp):
    case X86_64_OP(test):
    case X86_64_OP(je):
    case X86_64_OP(jne):
    case X86_64_OP(jl):
    case X86_64_OP(jb):
    case X86_64_OP(jge):
    case X86_64_OP(jae):
    case X86_64_OP(jmp):
    case X86_64_OP(label):
    case X86_64_OP(ret):
    case X86_64_OP(save):
    case X86_64_OP(restore):
    case X86_64_OP(literal):
    case X86_64_OP(asm):
    case X86_64_OP(loc):
      // These instructions do not have registers allocated to them.  They are
      // finished all the same: ReloadSpills above has already given them their
      // final operands, so a later spill must not treat them as a read it can
      // still redirect to a stack slot.  A compare against a value that is
      // spilled afterwards is the case that matters -- it keeps reading the
      // register, and only the flag says so.
      inst->flags |= TARGET_INST_PROCESSED;
      return;

    case X86_64_OP(regarg):
      // Always refers to fixed register so no allocation necessry.
      return;
      
    case X86_64_OP(x0):
      reg = &allocator->int_regs[X86_64_INT_ZERO_REG];
      break;

    case X86_64_OP(fp):
      reg = &allocator->int_regs[X86_64_FP_REG];
      break;

    case X86_64_OP(sp):
      reg = &allocator->int_regs[X86_64_SP_REG];
      break;

    case X86_64_OP(t0):
      reg = &allocator->int_regs[X86_64_INT_TEMP_START_1];
      break;

    case X86_64_OP(a0):
    case X86_64_OP(a1):
    case X86_64_OP(a2):
    case X86_64_OP(a3):
    case X86_64_OP(a4):
    case X86_64_OP(a5):
    case X86_64_OP(a6):
    case X86_64_OP(a7):
      reg = &allocator->int_regs[(int)inst->opcode - X86_64_OP(a0) + X86_64_INT_ARG_START];
      EvictPhysicalRegister(allocator, kX86_64RegTypeInt, reg->base.num, inst);
      break;

    case X86_64_OP(ivarreg):
    case X86_64_OP(fvarreg):
      assert(false);
      COMPILER_UNREACHABLE();
      
    case X86_64_OP(fa0):
    case X86_64_OP(fa1):
    case X86_64_OP(fa2):
    case X86_64_OP(fa3):
    case X86_64_OP(fa4):
    case X86_64_OP(fa5):
    case X86_64_OP(fa6):
    case X86_64_OP(fa7):
      reg = &allocator
                 ->float_regs[(int)inst->opcode - X86_64_OP(fa0) + X86_64_FP_ARG_START];
      EvictPhysicalRegister(allocator, kX86_64RegTypeFloat, reg->base.num, inst);
      break;

    case X86_64_OP(structreturn):
      reg = &allocator->int_regs[(is_leaf ? X86_64_FIRST_LEAF_INT_REG_VAR
                                          : X86_64_FIRST_INT_REG_VAR) +
                                 allocator->rv->struct_return_reg];
      BitSetInsert(&allocator->used_int_regs, reg->base.num);
      break;

    case X86_64_OP(resulti):
      reg = &allocator->int_regs[X86_64_INT_RETURN_REG];
      break;

    case X86_64_OP(resultf):
    case X86_64_OP(resultd):
      reg = &allocator->float_regs[X86_64_FLOAT_RETURN_REG];
      break;

    case X86_64_OP(call):
    case X86_64_OP(rcall):
      reg = &allocator->int_regs[X86_64_INT_RETURN_REG];
      break;
    case X86_64_OP(callf):
    case X86_64_OP(rcallf):
      reg = &allocator->float_regs[X86_64_FLOAT_RETURN_REG];
      break;
      
    case X86_64_OP(ucomiss):
    case X86_64_OP(ucomisd):
      reg = AllocateRegisterWithType(allocator, inst->block, inst,
                                     kX86_64RegTypeInt, CanUseTemp(allocator, inst));
      break;

    case X86_64_OP(idiv):
    case X86_64_OP(div):
    case X86_64_OP(mod):
      ReserveIdivRegisters(allocator);
      reg = AllocateRegisterWithType(allocator, inst->block, inst,
                                     kX86_64RegTypeInt, CanUseTemp(allocator, inst));
      break;

    default: {
      X86_64RegisterType reg_type = RegisterTypeFromInstruction(inst);
      reg = AllocateRegisterWithType(allocator, inst->block, inst,
                                     reg_type, CanUseTemp(allocator, inst));
    }
  }

  AssignRegister(reg, inst);

  if (allocator->spill_after_definition) {
    // No register could be freed, so this value went into the scratch register
    // and has to leave it again before the next instruction.
    allocator->spill_after_definition = false;
    SpillInstruction(allocator, inst);
    return;
  }

  // If nobody is using this register free it up immediately.
  // TODO: argument registers are not used explicitly but can't be freed here.
  if (inst->uses == 0 && !reg->base.reserved) {
    FreeRegister(allocator, reg);
  }
}

// Give |inst|, a live-in value of |block|, ownership of the register it was
// allocated.
//
// A block input is live-in.  It must retain its register when it is live-out of
// this block too (i.e. it appears in the block's output set), because such a
// value is live *through* the block and its register may not be reused for
// another value defined here.  The running `uses` counter cannot be trusted for
// this test: it is decremented globally as the dominator-tree traversal
// descends into sibling subtrees and is never restored per subtree, so a value
// that is still live on a later path can read as `uses == 0`.  Relying on it
// alone let a value defined after a call (e.g. the call result) steal the
// register of a parameter that is still needed further down the block's own
// subtree.  Keep ownership whenever the value is live-out; only drop it when it
// is neither live-out nor has any remaining recorded use.
//
// Because liveness is over-approximated that way, two inputs can name the same
// physical register: one whose reads are all done -- so the allocator handed its
// register to a value defined in a later block -- and the value that took it
// over.  Both are recorded live-in, so ownership must not simply go to whichever
// input the loop visits last.  Handing it to the exhausted value loses the live
// one: a later spill of the exhausted value releases the register
// (SpillInstruction clears its owner) and the next definition in this block is
// then handed the same register, clobbering the live value.  Resolve such a
// clash the way the value flowed at run time instead: a value with reads still
// pending outranks an exhausted one, and between two equal claimants the later
// definition -- the one that took the register over -- outranks the earlier.
static bool BetterInputClaim(TargetInstruction* candidate,
                             TargetInstruction* owner) {
  if (owner == NULL || owner == candidate) {
    return owner == NULL;
  }
  if ((candidate->uses > 0) != (owner->uses > 0)) {
    return candidate->uses > 0;
  }
  return candidate->id > owner->id;
}

// The value holding the physical register denoted by logical slot |slot|, which
// may have been allocated through any of the logical slots aliasing it.  Returns
// NULL when the register is free; *holder is the slot recording the ownership.
static TargetInstruction* X86_64PhysicalOwner(X86_64RegisterAllocator* allocator,
                                              X86_64RegisterType type, int slot,
                                              X86_64Register** holder) {
  X86_64Register* regs =
      type == kX86_64RegTypeInt ? allocator->int_regs : allocator->float_regs;
  int num_regs =
      type == kX86_64RegTypeInt ? X86_64_NUM_INT_REGS : X86_64_NUM_FLOAT_REGS;
  int phys = type == kX86_64RegTypeInt ? X86_64IntPhysical(slot)
                                       : X86_64FloatPhysical(slot);
  *holder = NULL;
  for (int k = 0; k < num_regs; k++) {
    int other = type == kX86_64RegTypeInt ? X86_64IntPhysical(k)
                                          : X86_64FloatPhysical(k);
    if (other != phys || regs[k].base.owner == NULL) {
      continue;
    }
    *holder = &regs[k];
    return regs[k].base.owner;
  }
  return NULL;
}

static void ClaimInputRegister(X86_64RegisterAllocator* allocator,
                               TargetBasicBlock* block,
                               TargetInstruction* inst) {
  if (inst->reg == NULL) {
    return;
  }
  if (((int)inst->opcode == (int)X86_64_OP(spill)) ||
      (inst->flags & TARGET_INST_SPILLED) != 0) {
    return;
  }
  if (inst->uses == 0 && !TargetBasicBlockOutputs(block, inst)) {
    return;
  }
  X86_64RegisterType type = ((X86_64Register*)inst->reg)->type;
  X86_64Register* holder = NULL;
  TargetInstruction* owner =
      X86_64PhysicalOwner(allocator, type, inst->reg->num, &holder);
  if (owner == inst) {
    return;
  }
  // A reserved slot's ownership survives block entry (a register variable, the
  // struct-return pointer): it is pinned there for the whole function and is
  // never up for grabs.
  if (holder != NULL && holder->base.reserved) {
    return;
  }
  if (!BetterInputClaim(inst, owner)) {
    return;
  }
  if (holder != NULL) {
    holder->base.owner = NULL;
  }
  inst->reg->owner = inst;
}

static void InitializeBasicBlockRegisters(X86_64RegisterAllocator* allocator,
                                          TargetBasicBlock* block) {
  for (int i = 0; i < X86_64_NUM_INT_REGS; i++) {
    X86_64Register* reg = &allocator->int_regs[i];
    if (reg->base.reserved) {
      continue;
    }
    reg->base.owner = NULL;
  }
  for (int i = 0; i < X86_64_NUM_FLOAT_REGS; i++) {
     X86_64Register* reg = &allocator->float_regs[i];
     if (reg->base.reserved) {
       continue;
     }
     reg->base.owner = NULL;
  }
    
  // Now allocate the registers to the inputs.
  for (size_t i = 0; i < block->inputs.length; i++) {
    ClaimInputRegister(allocator, block, block->inputs.value.p[i]);
  }
}

static void ProcessBlock(TargetBasicBlock* block, void* data) {
  TrapBlock(block);
  
  // printf("Allocating registers for block %zd\n", block->block_id);
  X86_64RegisterAllocator* allocator = data;

  // For a basic block, the inputs specify what instructions are alive
  // on entry.  An alive instruction has a register allocated to it.  All
  // other registers should be free at this point.
  InitializeBasicBlockRegisters(allocator, block);
  
  for (TargetInstruction* inst = block->code;
       inst != NULL && inst != block->end_code;
       inst = TargetNext(inst)) {
    AllocateRegister(allocator, inst);
  }
  if (block->end_code != NULL) {
    AllocateRegister(allocator, block->end_code);
  }
}

static void ProcessBasicBlock(X86_64RegisterAllocator* allocator,
                              TargetBasicBlock* block) {
  TargetBasicBlockTraverseDominatorTree(&allocator->rv->base, block,
                                        ProcessBlock, kTraversePreOrder,
                                        allocator);
}


static bool IsCallInstruction(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)X86_64_OP(call) ||
         inst->opcode == (TargetOpcode)X86_64_OP(rcall) ||
         inst->opcode == (TargetOpcode)X86_64_OP(callf) ||
         inst->opcode == (TargetOpcode)X86_64_OP(rcallf);
}

static bool IsUser(TargetInstruction* inst, TargetInstruction* candidate) {
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    if (candidate->operand[i] == inst) {
      return true;
    }
  }
  return false;
}

// Build the preserved_instructions set, instructions that need a callee-saved
// register. Block outputs must survive every call in the block. Values used
// later in the same block must also be preserved when a call lies between
// their definition and use.
static void BuildPreservedInstructionsSet(TargetBasicBlock* block, void* data) {
  X86_64RegisterAllocator* allocator = data;
  bool contains_call =
      block->end_code != NULL && IsCallInstruction(block->end_code);
  for (TargetInstruction* inst = block->code;
       inst != NULL; inst = TargetNext(inst)) {
    if (IsCallInstruction(inst)) {
      contains_call = true;
      break;
    }
    if (inst == block->end_code) {
      break;
    }
  }
  if (contains_call) {
    BitSetUnionInPlace(&allocator->preserved_instructions, &block->output_ids);
  }

  for (TargetInstruction* inst = block->code;
       inst != NULL && inst != block->end_code; inst = TargetNext(inst)) {
    bool crossed_call = false;
    for (TargetInstruction* next = TargetNext(inst);
         next != NULL; next = TargetNext(next)) {
      if (IsCallInstruction(next)) {
        crossed_call = true;
      } else if (crossed_call && IsUser(inst, next)) {
        BitSetInsert(&allocator->preserved_instructions, inst->id);
        break;
      }
      if (next == block->end_code) {
        break;
      }
    }
  }
}

// Register variables are pinned to a fixed physical register by index and are
// allocated lazily at their first definition.  Those physical registers are
// drawn from the same range the dynamic allocator searches, so without
// reserving them the dynamic allocator could hand a variable's register to an
// unrelated value (e.g. a pooled loop-bound constant) before the variable is
// allocated, and the variable's later assignment would then clobber that
// still-live value.  Reserve them up front.
static void ReserveVariableRegisters(X86_64RegisterAllocator* allocator) {
  bool is_leaf = allocator->rv->base.num_calls == 0 && compiler->optimize;
  for (size_t i = 0; i < allocator->rv->var_regs.length; i++) {
    RegisterVariable* var = allocator->rv->var_regs.value.p[i];
    int slot = X86_64VarRegSlot(allocator, var->is_fp, is_leaf, var->varnum);
    if (slot < 0) {
      continue;
    }
    if (var->is_fp) {
      allocator->float_regs[slot].base.reserved = true;
      allocator->pinned_float_phys |= 1u << X86_64FloatPhysical(slot);
    } else {
      allocator->int_regs[slot].base.reserved = true;
      allocator->pinned_int_phys |= 1u << X86_64IntPhysical(slot);
    }
  }
}

void X86_64AllocateRegisters(X86_64RegisterAllocator* allocator) {
  TargetTraverseDominatorTree(&allocator->rv->base, BuildPreservedInstructionsSet,
                          kTraversePreOrder, allocator);
  for (size_t i = 0; i < allocator->rv->base.basic_blocks.length; i++) {
    TargetBasicBlock* block =
        allocator->rv->base.basic_blocks.value.p[i];
    if (block != allocator->rv->base.entry_block && block->idom == NULL) {
      TargetBasicBlockTraverseDominatorTree(
          &allocator->rv->base, block, BuildPreservedInstructionsSet,
          kTraversePreOrder, allocator);
    }
  }

  ReserveVariableRegisters(allocator);

  // Process the normal-entry dominator tree, then any disconnected roots such
  // as exception landing pads. They are emitted too and therefore need the
  // same register-allocation pass.
  ProcessBasicBlock(allocator, allocator->rv->base.entry_block);
  for (size_t i = 0; i < allocator->rv->base.basic_blocks.length; i++) {
    TargetBasicBlock* block =
        allocator->rv->base.basic_blocks.value.p[i];
    if (block != allocator->rv->base.entry_block && block->idom == NULL) {
      ProcessBasicBlock(allocator, block);
    }
  }
}

const char* X86_64RegisterName(X86_64Register* reg, char* buf, size_t len) {
  return X86_64RegisterNameFromNum(reg->base.num, reg->type, buf, len);
}

const char* X86_64RegisterNameFromNum(int num, X86_64RegisterType type, char* buf,
                                  size_t len) {
  static const char* kIntRegNames[X86_64_NUM_INT_REGS] = {
      "rax",  "rax",  "rsp",  "r11",  "r10",  "r10",  "r11",  "r12",
      "rbp",  "rbx",  "rdi",  "rsi",  "rdx",  "rcx",  "r8",   "r9",
      "r10",  "r11",  "r12",  "r13",  "r14",  "r15",  "r12",  "r13",
      "r14",  "r15",  "r10",  "r11",  "r8",   "r9",   "r10",  "r11",
  };
  static const char* kFloatRegNames[X86_64_NUM_FLOAT_REGS] = {
      "xmm0",  "xmm1",  "xmm2",  "xmm3",  "xmm4",  "xmm5",  "xmm6",  "xmm7",
      "xmm8",  "xmm9",  "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15",
      "xmm0",  "xmm1",  "xmm2",  "xmm3",  "xmm4",  "xmm5",  "xmm6",  "xmm7",
      "xmm8",  "xmm9",  "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15",
  };

  if (type == kX86_64RegTypeInt) {
    if (num >= 0 && num < X86_64_NUM_INT_REGS) {
      snprintf(buf, len, "%s", kIntRegNames[num]);
      return buf;
    }
    snprintf(buf, len, "r%d", num);
    return buf;
  }

  if (num >= 0 && num < X86_64_NUM_FLOAT_REGS) {
    snprintf(buf, len, "%s", kFloatRegNames[num]);
    return buf;
  }
  snprintf(buf, len, "xmm%d", num);
  return buf;
}

#undef NUM_REG_RANGES
