//
//  target_reg_alloc.h
//  c_compiler_library
//
//  Graph-coloring register allocator shared between backends.
//
//  It reads the IR under the rules in target_ra_ir.h, computes its own
//  liveness, builds an interference graph over values and locations with the
//  physical registers precolored, and colors it with iterated register
//  coalescing (George and Appel).  Values it cannot color are spilled around
//  every definition and use, and the graph is rebuilt until everything
//  colors.  Before coloring, values live both on the call-free path from the
//  entry and past it are split in two, so that the call-free path can run
//  without a frame.  A backend describes its registers and supplies the
//  instructions the allocator inserts through TargetRegAllocDesc.
//

#ifndef target_reg_alloc_h
#define target_reg_alloc_h

#include <stdbool.h>
#include "target_generator.h"
#include "target_ra_ir.h"

#define TARGET_RA_MAX_CLASSES 2
#define TARGET_RA_MAX_CLASS_REGS 64

typedef struct {
  // Allocatable physical register keys (see TargetRAInfo::location_key), in
  // the order the allocator prefers them.
  int num_regs;
  int regs[TARGET_RA_MAX_CLASS_REGS];
} TargetRAClass;

typedef enum {
  kTargetRANotCopy,
  // A copy the allocator may delete by giving both sides one register.
  kTargetRACopy,
  // A copy that also changes the value, such as a 32-bit move that clears
  // the upper half.  Both sides must stay in different registers.
  kTargetRAConvertingCopy,
} TargetRACopyKind;

typedef struct TargetRegAllocDesc {
  const TargetRAInfo* info;
  void* ctx;
  int num_classes;
  TargetRAClass classes[TARGET_RA_MAX_CLASSES];

  // Register class of a value or location.
  int (*reg_class)(TargetInstruction* inst);
  // Physical key a value's own register is tied to (a call's result), or -1.
  int (*fixed_value_key)(TargetInstruction* inst);
  // False for a value that only sets flags or memory.
  bool (*defines_register)(TargetInstruction* inst);
  // Whether |inst| copies operand 0 into its result, and how.
  TargetRACopyKind (*copy_kind)(TargetInstruction* inst);
  // Whether reading |operand| reads its own operands, as a chain of
  // argument holders does at the call.
  bool (*reads_through)(TargetInstruction* operand);
  // Physical keys |inst| reads without naming them, such as the result
  // registers at a return.  Returns how many it stored.
  int (*implicit_uses)(void* ctx, TargetInstruction* inst, int* keys,
                       int max_keys);
  // Physical keys |inst|'s register must avoid.  Returns how many it stored.
  int (*forbidden_keys)(TargetInstruction* inst, int* keys, int max_keys);

  // A new copy of |value| that also writes |dest| when it is not NULL.  The
  // allocator inserts it, possibly between a compare and the branch that
  // reads its flags, so it must leave the flags alone.  When |dest| is
  // |value|'s dest the copy takes over writing it, and |value| may drop its
  // dest.  |value| may also be a location.
  TargetInstruction* (*create_copy)(void* ctx, TargetInstruction* value,
                                    TargetInstruction* dest);
  // A new stack slot for a value of |reg_class|.
  int (*new_slot)(void* ctx, int reg_class);
  // A new store of |value| to |slot|.  The allocator inserts it.
  TargetInstruction* (*create_spill)(void* ctx, TargetInstruction* value,
                                     int slot);
  // A new load from the slot |spill| stores to.  The allocator inserts it.
  TargetInstruction* (*create_reload)(void* ctx, TargetInstruction* spill);
  // Gives |inst| the physical register |key|.
  void (*assign)(void* ctx, TargetInstruction* inst, int key);

  // For splitting; the allocator does not split when these are NULL.
  // A new location for a value or location like |like|: same register class,
  // size and register constraints.  NULL when |like| cannot be split.
  TargetInstruction* (*create_location)(void* ctx, TargetInstruction* like);
  // A new label, and a new unconditional branch to |label|, for the blocks
  // the allocator places on edges.
  TargetInstruction* (*create_label)(void* ctx);
  TargetInstruction* (*create_jump)(void* ctx, TargetInstruction* label);
} TargetRegAllocDesc;

typedef struct {
  int rounds;
  int nodes;
  int moves;
  int coalesced;
  int spilled;
  int copies;  // Copies inserted where a value and its dest differ.
  int split;   // Values and locations split at the fast path.
} TargetRegAllocStats;

// Allocates registers for |gen|'s current function.  Leaves every operand
// read, location and value with a register, and every fixed location named
// by an operand or a dest assigned through |desc|.  Returns false, with the
// IR untouched, if the allocator could not handle the function.
bool TargetAllocateRegisters(TargetGenerator* gen,
                             const TargetRegAllocDesc* desc,
                             TargetRegAllocStats* stats);

#endif /* target_reg_alloc_h */
