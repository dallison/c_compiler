//
//  arm_reg_alloc.c
//  c_compiler
//
//  Created by David Allison on 7/20/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include "arm_reg_alloc.h"
#include <assert.h>
#include <limits.h>
#include "arm_codegen.h"
#include "arm_machine.h"
#include "target_basic_block.h"
#include "compiler.h"

static void AllocateRegister(ARMRegisterAllocator* allocator,
                             TargetInstruction* inst);
static ARMRegisterType RegisterTypeFromInstruction(TargetInstruction* inst);

static const char* ARMRegisterNameFromNum1(int num, ARMRegisterType type, int size,
                                    bool allow_no_size,
                                   char* buf,
                                           size_t len);

static const char* ARMRegisterNameNoSize(ARMRegister* reg, int size, char* buf, size_t len);

static void Trap() {}

static void TrapInstruction(TargetInstruction* inst) {
  if (inst->id == 16) {
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

static void InitializeRegister(ARMRegister* reg, int num, ARMRegisterType type) {
  TargetRegisterInit(&reg->base, num);
  reg->type = type;
}

void ARMRegisterAllocatorInit(ARMRegisterAllocator* allocator,
                             struct ARMGenerator* g) {
  allocator->g = g;

  for (int i = 0; i < ARM_NUM_INT_REGS; i++) {
    InitializeRegister(&allocator->int_regs[i], i, kARMRegTypeInt);
  }

  for (int i = 0; i < ARM_NUM_FLOAT_REGS; i++) {
    InitializeRegister(&allocator->float_regs[i], i, kARMRegTypeFloat);
  }

  // Reserve some registers.
  allocator->int_regs[ARM_FP_REG].base.reserved = true;
  allocator->int_regs[ARM_LR_REG].base.reserved = true;
  allocator->int_regs[ARM_SPILL_ADDR].base.reserved = true;
  // r9 is a dedicated scratch register (see Tmp() in arm_codegen.c) used to
  // stage indirect-call targets and other temporaries.  Keep it out of the
  // general allocatable pool so it is never clobbered by argument setup.
  allocator->int_regs[ARM_TMP_REG].base.reserved = true;

  BitSetInit(&allocator->used_int_regs);
  BitSetInit(&allocator->used_float_regs);
  
  allocator->current_spilled_region_size = 0;
  allocator->max_spilled_region_size = 0;
  BitSetInit(&allocator->preserved_instructions);
  MapInitForPointerKeys(&allocator->reassignable_spills);
  allocator->allocating_depth = 0;
}

ARMRegisterAllocator* NewARMRegisterAllocator(struct ARMGenerator* g) {
  ARMRegisterAllocator* reg_alloc = malloc(sizeof(ARMRegisterAllocator));
  ARMRegisterAllocatorInit(reg_alloc, g);
  return reg_alloc;
}

void ARMRegisterAllocatorDestruct(ARMRegisterAllocator* allocator) {
  BitSetDestruct(&allocator->used_int_regs);
  BitSetDestruct(&allocator->used_float_regs);
  BitSetDestruct(&allocator->preserved_instructions);
  MapDestruct(&allocator->reassignable_spills);
}

void ARMRegisterAllocatorDelete(ARMRegisterAllocator* alloc) {
  ARMRegisterAllocatorDestruct(alloc);
  free(alloc);
}

// Size for registers.
#define kSize32Bit 1
#define kSize64Bit 2

// The arm ABI divides registers into various ranges, some of which are
// temporary and some preserved across calls.  We use this array to
// search for registers.
static struct {
  ARMRegisterType type;  // Register type.
  int start;            // Start of range.
  int end;              // End of range.
  bool temp;
} register_ranges[] = {
  // General temporaries prefer non-argument registers.  On ARM the only
  // caller-saved scratch registers are the argument registers (r0..r3), so a
  // general temporary searched out of that pool can grab a register that is
  // about to be needed for an outgoing call's register argument.  When the
  // argument is then set up it either clobbers the still-live temporary, or
  // forces an impossible two-instruction register swap (e.g. r0<->r1).  Search
  // the callee-saved range (r4..r11) first and fall back to the argument
  // registers last, mirroring aarch64's allocation order.  r9 (ARM_TMP_REG)
  // and r11 (frame pointer) are reserved and skipped by FindFreeRegister.
    {kARMRegTypeInt, ARM_INT_SAVED_START, ARM_INT_SAVED_END, false},
  {kARMRegTypeInt, ARM_INT_ARG_START, ARM_INT_ARG_END, true},
    {kARMRegTypeFloat, ARM_FP_SAVED_START, ARM_FP_SAVED_END,false},
    {kARMRegTypeFloat, ARM_FP_TEMP_START, ARM_FP_TEMP_END, true},
    {kARMRegTypeFloat, ARM_FP_ARG_START, ARM_FP_ARG_END, true},
};

#define NUM_REG_RANGES (sizeof(register_ranges) / sizeof(register_ranges[0]))

static void DumpRegisters(ARMRegisterAllocator* allocator) {
  char buf[32];
  for (ARMRegisterType type = kARMRegTypeInt; type <= kARMRegTypeFloat; type++) {
    ARMRegister* regs =
        type == kARMRegTypeInt ? allocator->int_regs : allocator->float_regs;
    for (int i = 0; i < NUM_REG_RANGES; i++) {
      if (register_ranges[i].type == type) {
        for (int j = register_ranges[i].start; j <= register_ranges[i].end; j++) {
          if (regs[j].base.owner == NULL) {
            printf("%s(r%d): free\n", ARMRegisterNameNoSize(&regs[j], 0, buf, sizeof(buf)), regs[j].base.num);
          } else {
            TargetInstruction* owner = regs[j].base.owner;
            printf("%s(r%d): owner: @%d %s\n", ARMRegisterNameNoSize(&regs[j], 0, buf, sizeof(buf)), regs[j].base.num, owner->id,
                   ARMOpcodeName(owner->opcode));
          }
        }
      }
    }
  }
}

static void AssignRegister(ARMRegister* reg, TargetInstruction* inst) {
  assert(inst->reg == NULL);
  inst->reg = &reg->base;
  reg->base.owner = inst;
  inst->uses = (int)inst->users.length;
  inst->flags |= TARGET_INST_PROCESSED;
}

static bool IsFrameFreeStructReturnLeaf(ARMRegisterAllocator* allocator) {
  return OptLevel1() && allocator->g->base.num_calls == 0 &&
         !allocator->g->not_leaf && !allocator->g->base.varargs &&
         !allocator->g->uses_dynamic_stack && !allocator->g->has_stack_args &&
         allocator->g->exception_ranges.length == 0 &&
         allocator->g->base.stack_frame_size == 0 &&
         allocator->g->saved_regs.length == 0 &&
         allocator->g->struct_return_reg >= 0;
}

static ARMRegister* FindFreeRegister(ARMRegisterAllocator* allocator,
                                    ARMRegisterType type, bool can_use_temp) {
  ARMRegister* regs =
      type == kARMRegTypeInt ? allocator->int_regs : allocator->float_regs;
  bool frame_free_leaf = type == kARMRegTypeInt && can_use_temp &&
                         IsFrameFreeStructReturnLeaf(allocator);
  int passes = frame_free_leaf ? 2 : 1;
  for (int pass = 0; pass < passes; pass++) {
    bool want_temp = frame_free_leaf && pass == 0;
    for (size_t i = 0; i < NUM_REG_RANGES; i++) {
      if (register_ranges[i].type != type) {
        continue;
      }
      if (frame_free_leaf && register_ranges[i].temp != want_temp) {
        continue;
      }
      if (!can_use_temp && register_ranges[i].temp) {
        continue;
      }
      // Float registers are allocated in double-precision (d-register) units:
      // each allocation consumes an even/odd s-register pair, named by its even
      // low half.  This keeps a `float` and a `double` from aliasing the same
      // physical storage.  Integer registers step by one.
      int step = (type == kARMRegTypeFloat) ? 2 : 1;
      for (int j = register_ranges[i].start; j <= register_ranges[i].end; j += step) {
        if (!regs[j].base.reserved && regs[j].base.owner == NULL) {
          return &regs[j];
        }
      }
    }
  }

  // No registers available.
  return NULL;
}

static void FreeRegister(ARMRegisterAllocator* allocator, ARMRegister* reg) {
  reg->base.owner = NULL;
}


// Free up any registers that are no longer needed by the instruction.  This
// frees up all now-unused operands and destination.
static void FreeRegisters(ARMRegisterAllocator* allocator,
                          TargetInstruction* inst) {
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    if (inst->operand[i] != NULL) {
      TargetInstruction* op = inst->operand[i];
      // The use counter is the number of distinct user instructions (the users
      // list is deduplicated), so an instruction that references the same value
      // in several operand slots -- e.g. `vmov Dn, Rt, Rt` materializing the
      // double 0.0, or `cmp a, a` -- must only decrement it once.  Skip an
      // operand already seen in an earlier slot; otherwise the count underflows
      // early and the value's register is freed while it is still live, letting
      // a later instruction reuse and clobber it.
      bool duplicate = false;
      for (size_t j = 0; j < i; j++) {
        if (inst->operand[j] == op) {
          duplicate = true;
          break;
        }
      }
      if (duplicate) {
        continue;
      }
      if (ARMIsFixedRegister(op)) {
        continue;
      }
      // A variable register is a dedicated callee-saved register that holds a C
      // variable for its whole live range.  Its (linear) last use inside a loop
      // body is not really the last use -- the value is read again on the next
      // iteration via the back edge -- so it must not be freed by the use
      // counter, or a later temp in the same block would reuse the register and
      // clobber the still-live variable.  These registers are re-owned at each
      // block entry from the live-in set, so they never need explicit freeing.
      if (ARMIsVarRegister(op)) {
        continue;
      }
      // Inside a loop, a value that is live-out of the block is read again on a
      // later iteration through the back edge, so its linear "last use" in this
      // block is not really its last use.  Freeing its register here would let a
      // subsequent temp reuse it and clobber the still-live value.  Restrict
      // this to loop blocks so straight-line code keeps freeing registers
      // promptly (avoiding needless register pressure).
      if (inst->block != NULL && inst->block->loop_nesting > 0 &&
          BitSetContains(&inst->block->output_ids, op->id)) {
        continue;
      }
      TargetRegister* reg = op->reg;
      if (reg != NULL && !reg->reserved && reg->owner != NULL && op->uses > 0) {
        op->uses--;
        assert(op->uses >= 0);
        if (op->uses == 0) {
          if (reg->owner == op) {
            FreeRegister(allocator, (ARMRegister*)reg);
          }
        }
      }
    }
  }
}

// Is the register meant to be saved by the callee?
static bool IsSavedReg(ARMRegister* reg) {
  int num = reg->base.num;
  if ((num >= ARM_INT_SAVED_START && num <= ARM_INT_SAVED_END) ||
      (num >= ARM_FP_SAVED_START && num <= ARM_FP_SAVED_END)) {
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

static bool InstructionHasExternalDefs(ARMRegisterAllocator* allocator,
                                       TargetInstruction* target);

static TargetInstruction* FindSpillVictim(ARMRegisterAllocator* allocator,
                                          ARMRegisterType type,
                                          bool can_use_temp) {
  ARMRegister* regs =
      type == kARMRegTypeInt ? allocator->int_regs : allocator->float_regs;
  int min_cost = INT_MAX;
  TargetInstruction* victim = NULL;
  // Variable registers hold a C local across its whole (often loop-spanning)
  // live range and may be redefined in place (e.g. a loop counter `i++`).  The
  // spill machinery only emits a single store/reload for the value's first
  // definition, which cannot model a value that is reassigned or whose register
  // is reused between definition and use -- spilling such a register silently
  // corrupts the variable (the store can capture a later, unrelated value, and
  // in-loop reassignments are never written back).  Only fall back to spilling
  // a variable register when nothing else is available.
  int min_var_cost = INT_MAX;
  TargetInstruction* var_victim = NULL;
  // Find the instruction with the lowest spill cost.
  for (size_t i = 0; i < NUM_REG_RANGES; i++) {
    if (register_ranges[i].type == type) {
      if (!can_use_temp && register_ranges[i].temp) {
        continue;
      }
      int step = (type == kARMRegTypeFloat) ? 2 : 1;
      for (int j = register_ranges[i].start; j <= register_ranges[i].end; j += step) {
        if (!regs[j].base.reserved && regs[j].base.owner != NULL) {
          TargetInstruction* owner = regs[j].base.owner;
          // A staged definition such as movw/movt may own the physical
          // register while its destination is the logical variable that
          // remains live. Spill and retarget the variable, not the final
          // definition instruction.
          if (owner->dest != NULL && ARMIsVarRegister(owner->dest)) {
            owner = owner->dest;
          }
          if ((owner->flags & TARGET_INST_SPILLED) != 0 ||
              owner->opcode == (TargetOpcode)ARM_OP(spill) ||
              owner->opcode == (TargetOpcode)ARM_OP(reload)) {
            // The value is already in its spill slot; stale ownership must
            // not cause the same instruction to be spilled a second time.
            regs[j].base.owner = NULL;
            continue;
          }
          // Destination routing can reserve a register for an instruction
          // before that instruction has produced its value.  Spilling such an
          // owner emits a store of the register's previous contents and later
          // reloads that garbage as the routed result.
          if ((owner->flags & TARGET_INST_PROCESSED) == 0) {
            continue;
          }
          // A fixed-register holder (an incoming argument register r0..r3, the
          // call-result register, etc.) is pinned to a physical register and is
          // typically defined at function entry, before the prologue establishes
          // fp.  Spilling it would emit the store there (referencing fp before
          // it is valid, at a bogus offset), so never choose one as a victim.
          if (ARMIsFixedRegister(owner) || ARMIsResult(owner)) {
            continue;
          }
          // VFP spills preserve a double with vstr/vldr dN. Integer spills,
          // however, currently store one 32-bit register, so a reassignable
          // 64-bit integer merge cannot be spilled without losing its high
          // half.
          if (RegisterTypeFromInstruction(owner) == kARMRegTypeInt &&
              ARMGetRegisterSize(owner) == kSize64Bit &&
              InstructionHasExternalDefs(allocator, owner)) {
            continue;
          }
          if (ARMIsVarRegister(owner)) {
            // A variable register with no users cannot be spilled (there is no
            // use site to reload it at, see SpillInstruction).  Such a register
            // holds a dead value, but skip it as a victim so we pick a register
            // we can actually spill.
            if (owner->users.length == 0) {
              continue;
            }
            int cost = SpillCost(owner);
            if (cost < min_var_cost) {
              min_var_cost = cost;
              var_victim = owner;
            }
            continue;
          }
          int cost = SpillCost(owner);
          if (cost < min_cost) {
            min_cost = cost;
            victim = owner;
          }
        }
      }
    }
  }
  if (victim == NULL) {
    // No ordinary spill candidate; reluctantly spill a variable register.
    victim = var_victim;
  }
  if (victim == NULL) {
    return NULL;
  }
  return victim;
}

static bool IsReassignableDefinition(TargetInstruction* inst,
                                     TargetInstruction* target) {
  return inst->dest == target;
}

static bool InstructionHasExternalDefs(ARMRegisterAllocator* allocator,
                                       TargetInstruction* target) {
  TargetGenerator* gen = &allocator->g->base;
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    TargetBasicBlock* block = gen->basic_blocks.value.p[i];
    for (TargetInstruction* inst = block->code; inst != NULL;
         inst = TargetNext(inst)) {
      bool is_end = inst == block->end_code;
      if (inst != target && IsReassignableDefinition(inst, target)) {
        return true;
      }
      if (is_end) {
        break;
      }
    }
  }
  return false;
}

// True if |inst|'s own allocation is in progress.  Its operands have already
// had reloads inserted for any of them that were spilled, so pointing one of
// them at a spill slot now would do nothing but leave it naming the physical
// register: the reload pass for this instruction has been and gone.
static bool IsBeingAllocated(ARMRegisterAllocator* allocator,
                             TargetInstruction* inst) {
  for (size_t d = 0; d < allocator->allocating_depth; d++) {
    if (allocator->allocating[d] == inst) {
      return true;
    }
  }
  return false;
}

static bool NotProcessed(TargetInstruction* inst, void* data) {
  ARMRegisterAllocator* allocator = data;
  return (inst->flags & TARGET_INST_PROCESSED) == 0 &&
         !IsBeingAllocated(allocator, inst);
}

// A read that has already been allocated names the physical register the value
// was in, and spilling hands that register to something else.  Stage the slot
// through the dedicated scratch register immediately before the read instead.
// Returns false when the read cannot be repaired, which leaves it as it was.
static bool RepairProcessedRead(ARMRegisterAllocator* allocator,
                                TargetInstruction* user,
                                TargetInstruction* value,
                                TargetInstruction* spill) {
  if (user->block == NULL || value->reg == NULL) {
    return false;
  }
  if ((int)user->opcode == (int)ARM_OP(spill) ||
      (int)user->opcode == (int)ARM_OP(reload)) {
    // Spill stores and reloads name their slot's register directly rather than
    // reading an operand's, so there is nothing to redirect.
    return true;
  }
  bool reads_value = false;
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    if (user->operand[i] == value) {
      reads_value = true;
    }
  }
  if (!reads_value) {
    // A duplicate entry in the user list, already dealt with.
    return true;
  }
  if (RegisterTypeFromInstruction(value) != kARMRegTypeInt) {
    // There is no scratch register in the VFP file.
    return false;
  }
  ARMRegister* scratch = &allocator->int_regs[ARM_TMP_REG];
  // There is one scratch register, so a read that already stages something else
  // through it cannot stage this slot too.
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    TargetInstruction* op = user->operand[i];
    if (op != NULL && op != value && op->reg == &scratch->base) {
      return false;
    }
  }
  TargetInstruction* reload =
      TargetNewInstruction1((TargetOpcode)ARM_OP(reload), spill);
  TrapReload(reload);
  TargetBasicBlockEmitBefore(&allocator->g->base, user->block, reload, user);
  reload->reg = &scratch->base;
  reload->flags |= TARGET_INST_PROCESSED;
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    if (user->operand[i] == value) {
      user->operand[i] = reload;
    }
  }
  TargetAddUser(reload, user);
  // This reload is for a single instruction, however many of its operands read
  // it.
  reload->uses = 1;
  return true;
}

// Where the store back for a definition has to go.  Normally that is right
// after the definition itself, but ABI and symbol pseudos sit ahead of the
// prologue: their spill slots are frame-pointer-relative, so a store there
// writes through the caller's fp.  They also generate no code of their own --
// the register is written by a later instruction targeting them.  Anchor after
// that writer instead; if it has not been allocated yet, return NULL and let
// SyncReassignableSpill store the slot when it is.
static TargetInstruction* StoreBackAnchor(ARMRegisterAllocator* allocator,
                                          TargetInstruction* definition) {
  TargetBasicBlock* entry = allocator->g->base.entry_block;
  if (definition->block != entry) {
    return definition;
  }
  TargetInstruction* save = NULL;
  bool before_save = false;
  for (TargetInstruction* current = entry->code; current != NULL;
       current = TargetNext(current)) {
    if (current == definition) {
      before_save = true;
    }
    if ((ARMOpcode)current->opcode == ARM_OP(save)) {
      save = current;
      break;
    }
    if (current == entry->end_code) {
      break;
    }
  }
  if (!before_save || save == NULL) {
    return definition;
  }
  TargetInstruction* anchor = NULL;
  bool has_writer = false;
  for (TargetInstruction* current = TargetNext(save); current != NULL;
       current = TargetNext(current)) {
    if (current->dest == definition) {
      has_writer = true;
      if ((current->flags & TARGET_INST_PROCESSED) != 0) {
        anchor = current;
      }
    }
    if (current == entry->end_code) {
      break;
    }
  }
  if (anchor != NULL || has_writer) {
    return anchor;
  }
  return save;
}

static void InsertReassignableStoreBacks(
    ARMRegisterAllocator* allocator, TargetInstruction* target,
    TargetInstruction* spill, TargetInstruction* initial_definition) {
  TargetGenerator* gen = &allocator->g->base;
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    TargetBasicBlock* block = gen->basic_blocks.value.p[i];
    for (TargetInstruction* inst = block->code; inst != NULL;
         inst = TargetNext(inst)) {
      bool is_end = inst == block->end_code;
      if (inst != initial_definition &&
          (initial_definition == NULL || inst->id > initial_definition->id) &&
          (inst->flags & TARGET_INST_PROCESSED) != 0 && inst->reg != NULL &&
          IsReassignableDefinition(inst, target)) {
        TargetInstruction* anchor = StoreBackAnchor(allocator, inst);
        if (anchor != NULL) {
          TargetInstruction* store = TargetNewInstruction2(
              (TargetOpcode)ARM_OP(spill), target, spill->operand[1]);
          store->reg = inst->reg;
          store->flags |= TARGET_INST_PROCESSED;
          TargetBasicBlockEmitAfter(gen, anchor->block, store, anchor);
          if (anchor == inst) {
            inst = store;
          }
        }
      }
      if (is_end) {
        break;
      }
    }
  }
}

static void SyncReassignableSpill(ARMRegisterAllocator* allocator,
                                  TargetInstruction* definition,
                                  TargetInstruction* target) {
  if (target == NULL || definition->reg == NULL) {
    return;
  }
  // A definition can write a pseudo that is itself routed into a register
  // variable (structreturn -> ivarreg), so follow the chain of destinations
  // looking for the value that owns the spill slot.
  TargetInstruction* spill = NULL;
  for (TargetInstruction* current = target; current != NULL;
       current = current->dest) {
    spill = MapFindPointerKey(&allocator->reassignable_spills, current);
    if (spill != NULL) {
      target = current;
      break;
    }
  }
  if (spill == NULL) {
    return;
  }
  TargetInstruction* store = TargetNewInstruction2(
      (TargetOpcode)ARM_OP(spill), target, spill->operand[1]);
  store->reg = definition->reg;
  store->flags |= TARGET_INST_PROCESSED;
  TargetBasicBlockEmitAfter(&allocator->g->base, definition->block, store,
                            definition);
}

static ARMRegister* SpillInstruction(ARMRegisterAllocator* allocator, TargetInstruction* inst) {
  ARMRegister* reg = (ARMRegister*)inst->reg;    // Current register.
  
  // Generate a spill instruction with 2 operands:
  // 1. Instruction to spill (not set yet)
  // 2. Offset into spill region.
  // We don't set the spilled instruction yet because TargetRetargetInstruction
  // will see it and retarget it to the spill.
  TrapSpill(inst);
  TargetInstruction* spill = TargetNewInstruction2((TargetOpcode)ARM_OP(spill), NULL,
                                                   TargetGetIntConstant(&allocator->g->base,
                                                                        NULL,
                                                                        kTargetType32Bit,
                                                                        allocator->current_spilled_region_size));
  allocator->current_spilled_region_size += 8;    // Space for one register.
  if (allocator->current_spilled_region_size > allocator->max_spilled_region_size) {
    allocator->max_spilled_region_size = allocator->current_spilled_region_size;
  }

  TargetInstruction* save = NULL;
  bool defined_before_save = false;
  if (inst->block == allocator->g->base.entry_block) {
    for (TargetInstruction* current = inst->block->code; current != NULL;
         current = TargetNext(current)) {
      if (current == inst) {
        defined_before_save = true;
      }
      if ((ARMOpcode)current->opcode == ARM_OP(save)) {
        save = current;
        break;
      }
    }
  }
  if (ARMIsVarRegister(inst)) {
    // A varreg is a pseudo whose executable definitions are instructions with
    // `dest == inst`.  Keep this spill only as a slot handle and store back
    // after every real definition; emitting it after users[0] is unsafe because
    // user order is not instruction order and the first user is not the
    // defining assignment.
    TargetTrackOrphanInstruction(&allocator->g->base, spill);
    MapKeyValue kv = {.key.p = inst, .value.p = spill};
    MapInsert(&allocator->reassignable_spills, kv);
    InsertReassignableStoreBacks(allocator, inst, spill, NULL);
  } else if (InstructionHasExternalDefs(allocator, inst)) {
    MapKeyValue kv = {.key.p = inst, .value.p = spill};
    MapInsert(&allocator->reassignable_spills, kv);
    if (ARMGeneratesOutput(inst) &&
        inst->opcode != (TargetOpcode)ARM_OP(tmp)) {
      // An instruction can be built in stages by earlier instructions that
      // target its destination (for example movw -> movt).  Spill only after
      // the final instruction, then synchronize later redefinitions; storing
      // after an earlier partial definition loses the remaining bytes.
      TargetBasicBlockEmitAfter(&allocator->g->base, inst->block, spill, inst);
      InsertReassignableStoreBacks(allocator, inst, spill, inst);
    } else {
      TargetTrackOrphanInstruction(&allocator->g->base, spill);
      InsertReassignableStoreBacks(allocator, inst, spill, NULL);
    }
  } else if (defined_before_save && save != NULL) {
    // ABI argument and symbol pseudos are defined before the prologue.  Their
    // spill slots are frame-pointer-relative, so defer the store until save has
    // established fp.
    TargetBasicBlockEmitAfter(&allocator->g->base, save->block, spill, save);
  } else {
    // Emit spill instruction just after spilled instruction.
    TargetBasicBlockEmitAfter(&allocator->g->base, inst->block, spill, inst);
  }

  // Retarget all uses of the original instruction to the spill.  If the
  // user has already been processed this will have no effect.
  // NOTE: this will transfer all uses of the inst to the spill, leaving
  // the users of inst empty and its uses count 0.
  TargetRetargetInstructionIf(inst, spill, NotProcessed, allocator);
  spill->uses = inst->uses;
  spill->operand[0] = inst;
  spill->reg = inst->reg;
  // Whatever is left over reads the register directly and has already been
  // allocated, so it has to be repaired in place.  The user list is left as it
  // is; nothing consults it once the value is marked spilled.
  for (size_t i = 0; i < inst->users.length; i++) {
    RepairProcessedRead(allocator, inst->users.value.p[i], inst, spill);
  }
  reg->base.owner = NULL;
  inst->flags |= TARGET_INST_SPILLED;
  return reg;
}

static ARMRegister* AllocateRegisterWithType(ARMRegisterAllocator* allocator,
                                            TargetBasicBlock* block,
                                            TargetInstruction* inst,
                                            ARMRegisterType type,
                                            bool can_use_temp) {
  ARMRegister* reg = FindFreeRegister(allocator, type, can_use_temp);

  if (reg == NULL) {
    TargetInstruction* victim =
        FindSpillVictim(allocator, type, can_use_temp);
    // FindSpillVictim may have released a stale owner from an earlier spill.
    reg = FindFreeRegister(allocator, type, can_use_temp);
    if (reg == NULL && victim != NULL) {
      reg = SpillInstruction(allocator, victim);
    }
  }
  if (reg == NULL) {
    DumpRegisters(allocator);
    abort();
  }

  if (reg->base.reserved) {
    return reg;
  }
  if (!IsSavedReg(reg)) {
    return reg;
  }
  switch (type) {
    case kARMRegTypeInt:
      BitSetInsert(&allocator->used_int_regs, reg->base.num);
      break;
    case kARMRegTypeFloat:
      BitSetInsert(&allocator->used_float_regs, reg->base.num);
      break;
  }
  return reg;
}

static ARMRegisterType RegisterTypeFromInstruction(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(constf):
    case ARM_OP(constd):
    case ARM_OP(fmv_s):
    case ARM_OP(fmv_d):
    case ARM_OP(d0):
    case ARM_OP(d1):
    case ARM_OP(d2):
    case ARM_OP(d3):
    case ARM_OP(d4):
    case ARM_OP(d5):
    case ARM_OP(d6):
    case ARM_OP(d7):
    case ARM_OP(fldr):
    case  ARM_OP(fstr):
    case  ARM_OP(fadd):
    case  ARM_OP(fsub):
    case  ARM_OP(fmul):
    case  ARM_OP(fdiv):
    case  ARM_OP(fsqrt):
    case  ARM_OP(fmin):
    case  ARM_OP(fmax):
    case  ARM_OP(fcvtsd):     // Single to double
    case  ARM_OP(fcvtds):     // Double to single.
    case  ARM_OP(fcvt):     // Copy from int reg (no conversion)
    case  ARM_OP(fmov):
    case  ARM_OP(fcmp):
    case  ARM_OP(scvtf):
    case  ARM_OP(ucvtf):
    case  ARM_OP(fneg):
    case ARM_OP(fvarreg):
      return kARMRegTypeFloat;

    case  ARM_OP(fcvtns):
    case  ARM_OP(fcvtnu):
      return kARMRegTypeInt;

    case ARM_OP(tmp):
      // A bare temporary defaults to an integer register unless it was tagged
      // as holding a floating-point value (e.g. a `double` ?: merge slot).
      return (inst->flags & kARMFloatValue) ? kARMRegTypeFloat : kARMRegTypeInt;

    default:
      if ((ARMOpcode)inst->opcode >= ARM_OP(fldr) &&
          (ARMOpcode)inst->opcode <= ARM_OP(fneg)) {
        return kARMRegTypeFloat;
      }
      return kARMRegTypeInt;
  }
}

// Can we use a temp register?  If not we will have to use a saved one and
// those are more expensive since they need to be saved on entry and reloaded
// on exit.
static bool CanUseTemp(ARMRegisterAllocator* allocator, TargetInstruction* inst) {
  return !BitSetContains(&allocator->preserved_instructions, inst->id);
}

static void AllocateVariableRegister(ARMRegisterAllocator* allocator,
                                     TargetInstruction* inst) {
  ARMRegisterType reg_type = RegisterTypeFromInstruction(inst);
  for (size_t i = 0; i < allocator->g->var_regs.length; i++) {
    RegisterVariable* var = allocator->g->var_regs.value.p[i];
    if (var->inst == inst && !var->is_fp &&
        var->varnum == allocator->g->struct_return_reg) {
      bool is_leaf =
          allocator->g->base.num_calls == 0 && compiler->optimize;
      ARMRegister* reg =
          &allocator->int_regs[(is_leaf ? ARM_FIRST_LEAF_INT_REG_VAR
                                       : ARM_FIRST_INT_REG_VAR) +
                               allocator->g->struct_return_reg];
      AssignRegister(reg, inst);
      return;
    }
  }

  ARMRegister* reg = AllocateRegisterWithType(allocator, inst->block, inst,
                                 reg_type, CanUseTemp(allocator, inst));
  AssignRegister(reg, inst);
}

static COMPILER_UNUSED void AllocateForRmov(ARMRegisterAllocator* allocator,
                            TargetInstruction* inst) {
  assert(((int)inst->opcode == (int)ARM_OP(mv)) || ((int)inst->opcode == (int)ARM_OP(fmv_s)) ||
         ((int)inst->opcode == (int)ARM_OP(fmv_d)));
  TargetInstruction* dest = inst->operand[0];
  TargetInstruction* src = inst->operand[1];

  if (ARMIsVarRegister(dest) && dest->reg == NULL) {
    // Delayed allocation of variable register.
    AllocateVariableRegister(allocator, dest);
  }
  ARMRegister* reg = (ARMRegister*)dest->reg;
  assert(reg != NULL);
  
  if (((int)src->opcode == (int)ARM_OP(spill))) {
    // If we are rmoving a spill we can just load it directly into the
    // destination register.  To do this, we convert the rmov
    // into a reload instruction
    inst->opcode = (TargetOpcode)ARM_OP(reload);
    inst->operand[0] = src;
    inst->operand[1] = NULL;
    TrapReload(inst);
  } else {
    if (ARMIsVarRegister(src) && src->reg == NULL) {
      // Delayed allocation of variable register.
      AllocateVariableRegister(allocator, src);
    }
    inst->operand[0]->uses++;
    FreeRegisters(allocator, inst);
  }
  
  inst->reg = &reg->base;
  inst->flags |= TARGET_INST_PROCESSED;
  SyncReassignableSpill(allocator, inst, inst->dest);
}


static void ReloadSpills(ARMRegisterAllocator* allocator,
                         TargetInstruction* inst) {
  // All operands of `inst` are simultaneously live when `inst` executes, so a
  // register allocated for reloading one operand must not be reused for another
  // operand's reload (nor stolen as a spill victim).  Temporarily reserve each
  // operand's register -- both those already holding a value and those we
  // allocate here for reloads -- so FindFreeRegister/FindSpillVictim skip them,
  // then release the reservations once every operand has its register.  Without
  // this, reloading a second spilled operand could spill the reload just made
  // for the first operand and reuse its register, leaving the first operand
  // reading the wrong value (e.g. `add r8, r8, r8` where the base pointer and
  // index collapsed onto the same register).
  ARMRegister* protect[TARGET_MAX_OPERANDS];
  int num_protect = 0;
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    TargetInstruction* op = inst->operand[i];
    if (op != NULL && op->reg != NULL) {
      ARMRegister* r = (ARMRegister*)op->reg;
      if (!r->base.reserved) {
        r->base.reserved = true;
        protect[num_protect++] = r;
      }
    }
  }
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    TargetInstruction* op = inst->operand[i];
    TargetInstruction* spill = NULL;
    if (op != NULL && (int)op->opcode == (int)ARM_OP(spill)) {
      spill = op;
    } else if (op != NULL) {
      // A pseudo that generates no code of its own -- a variable register, or a
      // tmp staging an indirect call target -- is written by the instructions
      // that target it, so a read can just as well name the writer as the
      // pseudo.  Either way the value is in the pseudo's slot once it has been
      // spilled, and the slot is what has to be read.
      for (TargetInstruction* value = op; value != NULL && spill == NULL;
           value = value->dest) {
        spill = MapFindPointerKey(&allocator->reassignable_spills, value);
      }
    }
    if (spill != NULL) {
      TargetInstruction* reload = TargetNewInstruction1((TargetOpcode)ARM_OP(reload),
                                                        spill);
      TrapReload(reload);
      TargetBasicBlockEmitBefore(&allocator->g->base, inst->block, reload, inst);
      inst->operand[i] = reload;
      // The reload must land in a register of the spilled VALUE's class, not
      // the using instruction's class.  These differ for `fcvt`, which lowers a
      // double constant / variadic-double argument to `vmov Dn, Rlo, Rhi`: the
      // instruction is float-typed but its two operands are the int halves.
      // Using the user's (float) type here would reload an int half into a VFP
      // register and emit an invalid `vmov d, d, r`.  spill->operand[0] is the
      // original spilled instruction (see SpillInstruction).
      TargetInstruction* spilled_value =
          (spill->operand[0] != NULL) ? spill->operand[0] : inst;
      ARMRegisterType reg_type = RegisterTypeFromInstruction(spilled_value);
      ARMRegister *reg = AllocateRegisterWithType(allocator, reload->block, reload,
                                     reg_type, CanUseTemp(allocator, reload));
      AssignRegister(reg, reload);
      // This reload is for a single instruction.
      reload->uses = 1;
      // Protect this reload's register from being chosen to satisfy a later
      // operand's reload of the same instruction.
      if (!reg->base.reserved) {
        reg->base.reserved = true;
        protect[num_protect++] = reg;
      }
    }
  }
  for (int i = 0; i < num_protect; i++) {
    protect[i]->base.reserved = false;
  }
}

static void AllocateRegisterOnce(ARMRegisterAllocator* allocator,
                                 TargetInstruction* inst);

// Record that this instruction is mid-allocation while it runs, so that a spill
// triggered from inside knows its reads have already been resolved.
static void AllocateRegister(ARMRegisterAllocator* allocator,
                             TargetInstruction* inst) {
  bool pushed = allocator->allocating_depth < ARM_MAX_ALLOCATION_DEPTH;
  if (pushed) {
    allocator->allocating[allocator->allocating_depth++] = inst;
  }
  AllocateRegisterOnce(allocator, inst);
  if (pushed) {
    allocator->allocating_depth--;
  }
}

static void AllocateRegisterOnce(ARMRegisterAllocator* allocator,
                                 TargetInstruction* inst) {
   bool is_leaf = allocator->g->base.num_calls == 0 &&
      compiler->optimize;

  ARMOpcode opcode = (ARMOpcode)inst->opcode;
  
  TrapInstruction(inst);

  // If we already have a register allocated (as can be the case
  // for an ivarreg that is the dest of another instruction) don't
  // reallocate register.
  if (inst->reg != NULL) {
    return;
  }
  
  
  // rmov instructions use the register allocated to their first
  // operand as their own register.
  if ((opcode == ARM_OP(mv) || opcode == ARM_OP(fmv_s) ||
      opcode == ARM_OP(fmv_d)) && inst->dest != NULL) {
    AllocateForRmov(allocator, inst);
    return;
  }

  if (ARMIsVarRegister(inst)) {
    // Variable regsiter.  Delay allocation until it's assigned to.
    // It will be assigned to by an rmov or from a destination
    // assignemnt.
    return;
  }

  // Ensure any variable-register operand has a physical register.  A variable
  // that is read but never written (e.g. used while uninitialized) never goes
  // through the "allocate on assignment" path, so its register would otherwise
  // stay NULL and the emitter would dereference it.  Give it one on first use.
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    TargetInstruction* op = inst->operand[i];
    if (op != NULL && ARMIsVarRegister(op) && op->reg == NULL) {
      AllocateVariableRegister(allocator, op);
    }
  }

  // Reload any spilled expressions.
  ReloadSpills(allocator, inst);

  ARMRegister* reg;

  // A register-passed value copied into a dedicated variable in a frame-free
  // struct-return leaf can remain in its incoming caller-saved register. This
  // coalesces the entry copy (for example r1 -> reference variable) and leaves
  // the other caller-saved registers available for short-lived expressions.
  if (opcode == ARM_OP(mov) && inst->dest != NULL &&
      ARMIsVarRegister(inst->dest) && inst->dest->reg == NULL &&
      inst->operand[0] != NULL && ARMIsFixedRegister(inst->operand[0]) &&
      inst->operand[0]->reg != NULL &&
      inst->operand[0]->users.length == 1 &&
      RegisterTypeFromInstruction(inst->dest) == kARMRegTypeInt &&
      IsFrameFreeStructReturnLeaf(allocator)) {
    ARMRegister* source_reg = (ARMRegister*)inst->operand[0]->reg;
    if (source_reg->base.num >= ARM_INT_ARG_START &&
        source_reg->base.num <= ARM_INT_ARG_END) {
      AssignRegister(source_reg, inst->dest);
    }
  }

  if (inst->dest == NULL &&
      (opcode == ARM_OP(atomic_load) ||
       opcode == ARM_OP(atomic_fetch_add) ||
       opcode == ARM_OP(atomic_fetch_sub) ||
       opcode == ARM_OP(atomic_add_fetch) ||
       opcode == ARM_OP(atomic_sub_fetch) ||
       opcode == ARM_OP(atomic_compare_exchange_bool) ||
       opcode == ARM_OP(atomic_compare_exchange_val) ||
       opcode == ARM_OP(atomic_compare_exchange_n))) {
    reg = AllocateRegisterWithType(allocator, inst->block, inst,
                                   kARMRegTypeInt,
                                   CanUseTemp(allocator, inst));
    AssignRegister(reg, inst);
    FreeRegisters(allocator, inst);
    inst->flags |= TARGET_INST_PROCESSED;
    SyncReassignableSpill(allocator, inst, inst->dest);
    return;
  }

  if (inst->dest != NULL) {
    if (inst->dest->reg == NULL) {
      if (ARMIsVarRegister(inst->dest)) {
        // Assignment to a variable register,
        AllocateVariableRegister(allocator, inst->dest);
      } else {
        AllocateRegister(allocator, inst->dest);
      }
    }
    assert(inst->dest->reg != NULL);
    reg = (ARMRegister*)inst->dest->reg;
    inst->reg = inst->dest->reg;
    // A fixed destination can clobber an ordinary value that the allocator
    // placed in the same physical register earlier in the block. Preserve that
    // value when it still has a later use (for example, a chained call result
    // held in r1 while the next argument setup writes r1).
    TargetInstruction* displaced = reg->base.owner;
    if (displaced != NULL && displaced != inst->dest && displaced != inst &&
        displaced->uses > 0 && !ARMIsFixedRegister(displaced) &&
        !ARMIsResult(displaced) &&
        (displaced->flags & TARGET_INST_SPILLED) == 0 &&
        displaced->opcode != (TargetOpcode)ARM_OP(spill) &&
        displaced->opcode != (TargetOpcode)ARM_OP(reload) &&
        (displaced->flags & TARGET_INST_PROCESSED) != 0) {
      SpillInstruction(allocator, displaced);
    }
    // Re-establish ownership of the destination register.  Argument registers
    // (and other fixed-register holders) are shared singletons whose `reg`
    // field persists across calls, so when the holder already has a register
    // the allocation path above is skipped and AssignRegister never runs.
    // InitializeBasicBlockRegisters clears all owners at block entry, so
    // without this the register appears free for the rest of the block.  A
    // reload inserted for a later operand in the same argument setup (e.g. the
    // high half of a 64-bit argument whose source was spilled) would then grab
    // this very register and clobber the value we just moved in.  Claim it for
    // the destination so it stays live until legitimately freed.
    if (!reg->base.reserved) {
      reg->base.owner = inst->dest;
    }
    FreeRegisters(allocator, inst);
    inst->flags |= TARGET_INST_PROCESSED;
    SyncReassignableSpill(allocator, inst, inst->dest);
    return;
  }

  // Free up any registers we can.
  FreeRegisters(allocator, inst);
  
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(const8):
    case ARM_OP(const16):
    case ARM_OP(const32):
    case ARM_OP(const64):
    case ARM_OP(constf):
    case ARM_OP(constd):
    case ARM_OP(symbol):
    case ARM_OP(b):
    case ARM_OP(br):
    case ARM_OP(label):
    case ARM_OP(ret):
    case ARM_OP(save):
    case ARM_OP(restore):
    case ARM_OP(literal):
    case ARM_OP(asm):
    case ARM_OP(loc):
    case  ARM_OP(eq):
    case  ARM_OP(ne):
    case  ARM_OP(cs):
    case  ARM_OP(hs):
    case  ARM_OP(cc):
    case  ARM_OP(lo):
    case  ARM_OP(mi):
    case   ARM_OP(pl):
    case   ARM_OP(vs):
    case   ARM_OP(vc):
    case   ARM_OP(hi):
    case   ARM_OP(ls):
    case   ARM_OP(ge):
    case   ARM_OP(lt):
    case   ARM_OP(gt):
    case   ARM_OP(le):
    case   ARM_OP(al):
    case   ARM_OP(cmp):
    case   ARM_OP(fcmp):
    case ARM_OP(atomic_store):
    case ARM_OP(atomic_fence):
    case   ARM_OP(oplsl):
      // These instructions do not have registers allocated to them.
      return;

    case ARM_OP(regarg):
      // Always refers to fixed register so no allocation necessry.
      return;
      
    case ARM_OP(fp):
      reg = &allocator->int_regs[ARM_FP_REG];
      break;

    case ARM_OP(sp):
      reg = &allocator->int_regs[ARM_SP_REG];
      break;

    case ARM_OP(lr):
      reg = &allocator->int_regs[ARM_LR_REG];
      break;

    case ARM_OP(xr):
      reg = &allocator->int_regs[ARM_XR_REG];
      break;

    case ARM_OP(r0):
    case ARM_OP(r1):
    case ARM_OP(r2):
    case ARM_OP(r3):
    case ARM_OP(r4):
    case ARM_OP(r5):
    case ARM_OP(r6):
    case ARM_OP(r7):
    case ARM_OP(r9):  // Dedicated scratch/temp register (see Tmp()).
      reg = &allocator->int_regs[(int)inst->opcode - ARM_OP(r0) + ARM_INT_ARG_START];
      break;

    case ARM_OP(ivarreg):
    case ARM_OP(fvarreg):
      assert(false);
      COMPILER_UNREACHABLE();
      
    case ARM_OP(d0):
    case ARM_OP(d1):
    case ARM_OP(d2):
    case ARM_OP(d3):
    case ARM_OP(d4):
    case ARM_OP(d5):
    case ARM_OP(d6):
    case ARM_OP(d7):
      // Floating-point argument registers are double-precision: d{n} maps to
      // the even/odd s-register pair starting at s{2n}.
      reg = &allocator
                 ->float_regs[((int)inst->opcode - ARM_OP(d0) + ARM_FP_ARG_START) * 2];
      break;

    case ARM_OP(structreturn):
      reg = &allocator->int_regs[(is_leaf ? ARM_FIRST_LEAF_INT_REG_VAR
                                          : ARM_FIRST_INT_REG_VAR) +
                                 allocator->g->struct_return_reg];
      if (IsSavedReg(reg)) {
        BitSetInsert(&allocator->used_int_regs, reg->base.num);
      }
      break;

    case ARM_OP(resulti):
      reg = &allocator->int_regs[ARM_INT_RETURN_REG];
      break;

    case ARM_OP(resultf):
    case ARM_OP(resultd):
      reg = &allocator->float_regs[ARM_FLOAT_RETURN_REG];
      break;

    case ARM_OP(bl):
    case ARM_OP(blr):
      if ((inst->flags & kARMFpReturn) != 0) {
        reg = &allocator->float_regs[ARM_FLOAT_RETURN_REG];
      } else {
        reg = &allocator->int_regs[ARM_INT_RETURN_REG];
      }
      break;
#if 0
    case ARM_OP(callf):
    case ARM_OP(rcallf):
      reg = &allocator->float_regs[ARM_FLOAT_RETURN_REG];
#endif
      break;
      

    default: {
      ARMRegisterType reg_type = RegisterTypeFromInstruction(inst);
      reg = AllocateRegisterWithType(allocator, inst->block, inst,
                                     reg_type, CanUseTemp(allocator, inst));
    }
  }

  AssignRegister(reg, inst);

  // If nobody is using this register free it up immediately.
  // TODO: argument registers are not used explicitly but can't be freed here.
  if (inst->uses == 0 && !reg->base.reserved) {
    FreeRegister(allocator, reg);
  }
}

static void InitializeBasicBlockRegisters(ARMRegisterAllocator* allocator,
                                          TargetBasicBlock* block) {
  for (int i = 0; i < ARM_NUM_INT_REGS; i++) {
    ARMRegister* reg = &allocator->int_regs[i];
    if (reg->base.reserved) {
      continue;
    }
    reg->base.owner = NULL;
  }
  for (int i = 0; i < ARM_NUM_FLOAT_REGS; i++) {
     ARMRegister* reg = &allocator->float_regs[i];
     if (reg->base.reserved) {
       continue;
     }
     reg->base.owner = NULL;
  }

  // Promoted variables own function-wide registers. Reclaim every register
  // already assigned to one before allocating this block: the approximate
  // live-in sets can omit a variable in the middle of a switch chain even
  // though a loop backedge needs it again.
  //
  // A spilled variable is the exception.  Its value lives in its spill slot and
  // every use reloads into a fresh register, so it no longer needs the register
  // it was assigned before the spill -- `inst->reg` merely still names it.
  // Reclaiming it squats on a register the variable will never read, and the
  // squatting is not harmless: the live-in loop below refuses to reserve a
  // register that another value already owns, so the value genuinely holding it
  // across this block loses its reservation.  The victim search then finds the
  // register owned by a spilled instruction, clears that stale ownership as it
  // is entitled to, and hands the register to the next value needing one while
  // the real owner is still live in it.
  for (size_t i = 0; i < allocator->g->var_regs.length; i++) {
    RegisterVariable* var = allocator->g->var_regs.value.p[i];
    TargetInstruction* inst = var != NULL ? var->inst : NULL;
    if (inst != NULL && inst->reg != NULL && !inst->reg->reserved &&
        (inst->flags & TARGET_INST_SPILLED) == 0) {
      assert(inst->reg->owner == NULL || inst->reg->owner == inst);
      inst->reg->owner = inst;
    }
  }

  // A value defined before a loop and live across its back edge must not remain
  // solely in a physical register.  Dominator-order allocation can otherwise
  // spill it only after an early loop use has already been processed, leaving
  // that processed use tied to a register which later loop instructions reuse.
  // Spill such invariants before allocating the first loop instruction so all
  // loop uses are retargeted to reload from a stable stack slot.
  if (block->loop_nesting > 0) {
    for (size_t i = 0; i < block->inputs.length; i++) {
      TargetInstruction* inst = block->inputs.value.p[i];
      if (inst->block == block || inst->reg == NULL ||
          ARMIsFixedRegister(inst) || ARMIsVarRegister(inst) ||
          (inst->flags & TARGET_INST_SPILLED) != 0 ||
          !BitSetContains(&block->output_ids, inst->id) ||
          InstructionHasExternalDefs(allocator, inst)) {
        continue;
      }
      SpillInstruction(allocator, inst);
    }
  }
    
  // Now allocate the registers to the inputs.
  for (size_t i = 0; i < block->inputs.length; i++) {
    TargetInstruction* inst = block->inputs.value.p[i];
    if (inst->reg == NULL) {
      continue;
    }
    if (((int)inst->opcode == (int)ARM_OP(spill)) ||
        (inst->flags & TARGET_INST_SPILLED) != 0) {
      continue;
    }
    // A value listed in block->inputs is live on entry to this block.  Reserve
    // its register even when the linear use counter has already reached zero:
    // that happens for a value read again across a loop back edge after its
    // single static use was processed in an earlier (dominator-order) block,
    // e.g. a hoisted loop-invariant constant used by one switch arm.  Without
    // reserving it, a temp in this block could reuse and clobber the still-live
    // value.  Only claim the register if it is not already owned by another
    // live-in value processed above.
    assert(inst->reg != NULL);
    if (inst->reg->owner == NULL || inst->reg->owner == inst ||
        (ARMIsVarRegister(inst) &&
         !ARMIsVarRegister(inst->reg->owner))) {
      inst->reg->owner = inst;
    }
  }
}

static void ProcessBlock(TargetBasicBlock* block, void* data) {
  TrapBlock(block);
  
  // printf("Allocating registers for block %zd\n", block->block_id);
  ARMRegisterAllocator* allocator = data;

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

static void ProcessBasicBlock(ARMRegisterAllocator* allocator,
                              TargetBasicBlock* block) {
  TargetTraverseDominatorTree(&allocator->g->base, ProcessBlock,
                          kTraversePreOrder, allocator);
}


static bool IsCallInstruction(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)ARM_OP(bl) ||
         inst->opcode == (TargetOpcode)ARM_OP(blr);
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
// register.  Block outputs must survive every call in the block.  Values used
// later in the same block must also be preserved when a call lies between
// their definition and use.
static void BuildPreservedInstructionsSet(TargetBasicBlock* block, void* data) {
  ARMRegisterAllocator* allocator = data;
  if (block->contains_call) {
    BitSetUnionInPlace(&allocator->preserved_instructions,
                       &block->output_ids);
  }

  for (TargetInstruction* inst = block->code;
       inst != NULL && inst != block->end_code; inst = TargetNext(inst)) {
    if ((inst->flags & kARMIndirectCallTarget) != 0) {
      BitSetInsert(&allocator->preserved_instructions, inst->id);
    }
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

// Maximum number of register-argument moves we resolve in a single run.  ARM
// has only r0..r3 for integer arguments, so a run never needs more than a
// handful of slots; the extra headroom covers any self-moves.
#define ARM_MAX_ARG_MOVES 16

// Build a throwaway instruction that merely carries a physical register
// reference for the emitter (which only reads ->reg / ->dest->reg).
static TargetInstruction* RegRef(ARMRegisterAllocator* allocator, int num) {
  TargetInstruction* ref = TargetNewInstruction((TargetOpcode)ARM_OP(r9));
  ref->reg = &allocator->int_regs[num].base;
  ref->flags |= TARGET_INST_PROCESSED;
  // This register-reference is used only as an operand of resolved moves; it is
  // never emitted into the code list, so track it for cleanup at teardown.
  TargetTrackOrphanInstruction(&allocator->g->base, ref);
  return ref;
}

// Emit a re-sequenced integer register move ("mov dst, src") into the block
// just before `pos`, mirroring the form produced by SetDestOrMoveToArgReg.
static void EmitResolvedMove(ARMRegisterAllocator* allocator,
                             TargetBasicBlock* block, TargetInstruction* pos,
                             TargetInstruction* dst_ref, TargetInstruction* src_ref,
                             int size_flags) {
  TargetInstruction* mov = TargetNewInstruction1((TargetOpcode)ARM_OP(mov), src_ref);
  mov->dest = dst_ref;
  mov->reg = dst_ref->reg;
  // Carry the source value's size but never re-tag as an argument move (so the
  // resolver does not revisit it).
  mov->flags |= (size_flags & (kARMInstructionSize | (kARMInstructionSize << 1)));
  TargetBasicBlockEmitBefore(&allocator->g->base, block, mov, pos);
}

// A scalar constant is first materialized as `mov tmp, #imm` during lowering.
// If that value then participates in a parallel argument move, coalescing can
// remove the materialization while leaving its uninitialized allocator register
// as the apparent source. Preserve the immediate itself when re-sequencing.
static TargetInstruction* ArgumentMoveSource(TargetInstruction* move) {
  TargetInstruction* source = move != NULL ? move->operand[0] : NULL;
  if (source != NULL && (ARMOpcode)source->opcode == ARM_OP(mov) &&
      source->operand[0] != NULL && ARMIsConst(source->operand[0])) {
    return source->operand[0];
  }
  return source;
}

// Re-sequence a contiguous run of tagged argument moves (moves[0..count-1])
// into a valid parallel-move order, breaking register cycles through r9.  The
// original move instructions are disabled and replaced with the resolved
// sequence inserted before the run.
static void ResolveArgMoveRun(ARMRegisterAllocator* allocator,
                              TargetInstruction** moves, int count) {
  if (count < 2) {
    return;  // A single move cannot form a cycle.
  }
  TargetBasicBlock* block = moves[0]->block;
  TargetInstruction* pos = moves[0];
  int size_flags = moves[0]->flags;

  int dst[ARM_MAX_ARG_MOVES];
  int src[ARM_MAX_ARG_MOVES];
  bool src_reads_register[ARM_MAX_ARG_MOVES];
  TargetInstruction* src_ref[ARM_MAX_ARG_MOVES];
  TargetInstruction* dst_ref[ARM_MAX_ARG_MOVES];
  bool done[ARM_MAX_ARG_MOVES];

  for (int i = 0; i < count; i++) {
    TargetInstruction* m = moves[i];
    // Bail out if anything is unexpected; leave the run untouched.
    if (m->dest == NULL || m->dest->reg == NULL || m->operand[0] == NULL) {
      return;
    }
    dst[i] = m->dest->reg->num;
    TargetInstruction* source = ArgumentMoveSource(m);
    src_reads_register[i] = !ARMIsConst(source);
    if (src_reads_register[i] && source->reg == NULL) {
      return;
    }
    src[i] =
        src_reads_register[i] ? source->reg->num : -1;
    src_ref[i] = source;
    dst_ref[i] = m->dest;
    // A constant assigned to its destination register still needs an emitted
    // materialization; its allocator register does not contain the value yet.
    done[i] = src_reads_register[i] && dst[i] == src[i];
  }

  TargetInstruction* r9ref = RegRef(allocator, ARM_TMP_REG);

  int remaining = 0;
  for (int i = 0; i < count; i++) {
    if (!done[i]) remaining++;
  }

  int guard = 0;
  while (remaining > 0 && guard++ < ARM_MAX_ARG_MOVES * 4) {
    bool progressed = false;
    for (int i = 0; i < count; i++) {
      if (done[i]) continue;
      // The move is safe to emit now if no other pending move still reads the
      // register we are about to overwrite.
      bool safe = true;
      for (int j = 0; j < count; j++) {
        if (j == i || done[j]) continue;
        if (src_reads_register[j] && src[j] == dst[i]) {
          safe = false;
          break;
        }
      }
      if (safe) {
        EmitResolvedMove(allocator, block, pos, dst_ref[i], src_ref[i],
                         size_flags);
        done[i] = true;
        remaining--;
        progressed = true;
      }
    }
    if (!progressed) {
      // Every remaining move is part of a cycle.  Pick one, save the register
      // it targets into r9, and redirect that register's readers to r9.  This
      // turns the cycle into a chain that the next iterations can drain.
      int c = -1;
      for (int i = 0; i < count; i++) {
        if (!done[i]) {
          c = i;
          break;
        }
      }
      EmitResolvedMove(allocator, block, pos, r9ref, dst_ref[c], size_flags);
      for (int j = 0; j < count; j++) {
        if (!done[j] && src_reads_register[j] && src[j] == dst[c]) {
          src[j] = ARM_TMP_REG;
          src_ref[j] = r9ref;
        }
      }
    }
  }

  // Disable the original moves; the resolved sequence replaces them.
  for (int i = 0; i < count; i++) {
    moves[i]->block = NULL;
  }
}

static bool IsIntegerArgumentMove(TargetInstruction* inst) {
  if (inst == NULL || (ARMOpcode)inst->opcode != ARM_OP(mov) ||
      inst->dest == NULL || inst->dest->reg == NULL) {
    return false;
  }
  ARMRegister* dest = (ARMRegister*)inst->dest->reg;
  return dest->type == kARMRegTypeInt &&
         dest->base.num >= ARM_INT_ARG_START &&
         dest->base.num <= ARM_INT_ARG_END;
}

static bool ArgumentMoveRunHasDependency(TargetInstruction** moves, int count) {
  for (int i = 0; i < count; i++) {
    TargetInstruction* source = ArgumentMoveSource(moves[i]);
    if (source == NULL || ARMIsConst(source) || source->reg == NULL) {
      continue;
    }
    for (int j = 0; j < count; j++) {
      if (i == j || moves[j]->dest == NULL ||
          moves[j]->dest->reg == NULL) {
        continue;
      }
      if (source->reg->num == moves[j]->dest->reg->num) {
        return true;
      }
    }
  }
  return false;
}

// Scan the instruction stream for contiguous runs of tagged integer argument
// moves and resolve each run as a parallel move.  Do not absorb adjacent
// untagged moves: those can be the function-entry copies from r0-r3 into local
// variable registers and are sequentially ordered before the call setup.
static void ResolveArgumentMoves(ARMRegisterAllocator* allocator) {
  TargetInstruction* inst = TargetFirstInstruction(&allocator->g->base);
  while (inst != NULL) {
    if (IsIntegerArgumentMove(inst) &&
        (inst->flags & kARMArgMove) != 0) {
      TargetInstruction* moves[ARM_MAX_ARG_MOVES];
      int count = 0;
      TargetInstruction* run = inst;
      while (IsIntegerArgumentMove(run) &&
             (run->flags & kARMArgMove) != 0 &&
             count < ARM_MAX_ARG_MOVES) {
        moves[count++] = run;
        run = TargetNext(run);
      }
      if (ArgumentMoveRunHasDependency(moves, count)) {
        ResolveArgMoveRun(allocator, moves, count);
      }
      inst = run;  // Continue after the run (resolved moves were inserted
                   // before it and are not re-tagged).
    } else {
      inst = TargetNext(inst);
    }
  }
}

void ARMAllocateRegisters(ARMRegisterAllocator* allocator) {
  if (allocator->g->struct_return_reg >= 0) {
    bool is_leaf = allocator->g->base.num_calls == 0 && compiler->optimize;
    int reg_num = (is_leaf ? ARM_FIRST_LEAF_INT_REG_VAR
                           : ARM_FIRST_INT_REG_VAR) +
                  allocator->g->struct_return_reg;
    // The hidden result pointer is live for the whole function, including
    // every loop back edge and early return.  It is represented by a fixed
    // structreturn pseudo rather than ordinary SSA liveness, so reserve its
    // physical register from temporary allocation.
    allocator->int_regs[reg_num].base.reserved = true;
    if (IsSavedReg(&allocator->int_regs[reg_num])) {
      // Reserving it is also what stops AllocateRegisterWithType recording it
      // as used, and the structreturn pseudo that would otherwise record it has
      // no readers of its own, so the optimizer can delete it.  The function
      // writes the register either way -- the prologue homes the pointer there
      // and every call reloads it from that home -- so it has to be preserved
      // for the caller.
      BitSetInsert(&allocator->used_int_regs, reg_num);
    }
  }

  TargetTraverseDominatorTree(&allocator->g->base, BuildPreservedInstructionsSet,
                          kTraversePreOrder, allocator);

  // Process all basic blocks in the ARM generator by traversing the
  // dominator tree.
  ProcessBasicBlock(allocator, allocator->g->base.entry_block);

  // Fix up any argument-register move cycles the linear-scan allocator left
  // behind (e.g. an r2<->r3 swap).
  ResolveArgumentMoves(allocator);
}

static const char* ARMRegisterNameNoSize(ARMRegister* reg, int size, char* buf, size_t len) {
  return ARMRegisterNameFromNum1(reg->base.num, reg->type, size, true, buf, len);
}

const char* ARMRegisterName(ARMRegister* reg, int size, char* buf, size_t len) {
  return ARMRegisterNameFromNum(reg->base.num, reg->type, size, buf, len);
}

static const char* ARMRegisterNameFromNum1(int num, ARMRegisterType type, int size,
                                    bool allow_no_size,
                                   char* buf,
                                           size_t len) {
  (void)allow_no_size;
  switch (type) {
    case kARMRegTypeInt:
      switch (num) {
        case ARM_FP_REG:
          snprintf(buf, len, "fp");
          return buf;
        case ARM_SP_REG:
          snprintf(buf, len, "sp");
          return buf;
        case ARM_LR_REG:
          snprintf(buf, len, "lr");
          return buf;
        case ARM_IP_REG:
          snprintf(buf, len, "ip");
          return buf;
        case ARM_PC_REG:
          snprintf(buf, len, "pc");
          return buf;
        default:
          snprintf(buf, len, "r%d", num);
          return buf;
      }

    case kARMRegTypeFloat:
      if (size == kSize32Bit || size == 0) {
        snprintf(buf, len, "s%d", num);
      } else {
        snprintf(buf, len, "d%d", num / 2);
      }
      return buf;
  }

  return buf;
}

const char* ARMRegisterNameFromNum(int num, ARMRegisterType type, int size,
                                   char* buf,
                                   size_t len) {
  return ARMRegisterNameFromNum1(num, type, size, false, buf, len);
}

#undef NUM_REG_RANGES

