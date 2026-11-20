//
//  target_shrink_wrap.h
//  c_compiler
//
//  Moves a function's prologue off paths that return without needing the
//  frame.  The region walk is shared.  A backend supplies the opcodes,
//  register classes, and the instructions the pass inserts.
//
//  The prologue is one save in the entry block and one restore plus return
//  in a single epilogue.  Blocks that can run without the frame and still
//  reach that epilogue form the fast region.  A block reached from both the
//  fast region and the framed region is cloned.  Callee-saved registers the
//  fast region uses are remapped onto caller-saved registers the backend
//  offers; the prologue copies them back where the framed region begins.
//

#ifndef target_shrink_wrap_h
#define target_shrink_wrap_h

#include <stdbool.h>
#include <stdint.h>
#include "target_generator.h"

struct TargetBasicBlock;

// How one instruction affects a block's eligibility for the fast region.
typedef enum {
  kTargetSWBad = -1,     // The block needs the frame.
  kTargetSWMeta = 0,     // No code (label, location, constant).
  kTargetSWExec = 1,     // Real code.  The pass records the registers it names.
  kTargetSWSpill = 2,    // A spill of a callee-saved register.  Masks updated.
  kTargetSWReload = 3,   // A reload.  Masks updated, and it counts as code.
} TargetSWKind;

typedef struct TargetShrinkWrapOps {
  void* ctx;

  // At most this many framed blocks the fast region may exit to.  Zero means
  // 16, which is the most the pass can dispatch.
  int max_slow_targets;
  // Callee-saved registers are bits 0 .. num_saved_bits-1 of the saved mask.
  int num_saved_bits;

  // False skips the function.  Opt level, varargs, and exceptions live here.
  bool (*function_ok)(void* ctx, TargetGenerator* gen);

  bool (*is_save)(TargetInstruction* inst);
  bool (*is_restore)(TargetInstruction* inst);
  bool (*is_return)(TargetInstruction* inst);
  bool (*is_spill)(TargetInstruction* inst);
  bool (*is_reload)(TargetInstruction* inst);
  bool (*is_label)(TargetInstruction* inst);
  bool (*is_meta)(void* ctx, TargetInstruction* inst);
  bool (*no_fallthrough)(void* ctx, TargetInstruction* inst);
  // The operand that names a branch target, or NULL when |inst| is not a branch.
  TargetInstruction** (*branch_target)(void* ctx, TargetInstruction* inst);

  TargetSWKind (*classify)(void* ctx, TargetInstruction* inst, bool allow_save,
                           bool allow_reload, uint64_t* saved, uint64_t* temp,
                           uint64_t* arg);
  // False when |reg| makes the block ineligible (frame pointer, a saved
  // register the backend will not remap).  NULL is eligible and records nothing.
  bool (*record_reg)(void* ctx, TargetRegister* reg, uint64_t* saved,
                     uint64_t* temp, uint64_t* arg);

  uint64_t (*incoming_args)(void* ctx);
  // How many caller-saved registers exist before extras, and how many extra
  // caller-saved registers this function may use.  Together with coalesced
  // argument copies they bound the callee-saved registers the fast region
  // may hold.
  int (*scratch_base)(void* ctx);
  int (*extra_scratch)(void* ctx, uint64_t incoming_args);

  // |inst| copies incoming argument |*arg_index| into saved bit |*saved_bit|.
  // NULL when the backend does not coalesce those copies.
  bool (*coalesce_copy)(void* ctx, TargetInstruction* inst,
                        uint64_t written_args, int* saved_bit, int* arg_index);
  // Marks caller-saved registers the fast region already uses.  NULL when
  // nothing is remapped.
  void (*take_temps)(void* ctx, uint64_t temps, uint64_t* taken);
  // Hardware register of saved bit |bit|.
  int (*saved_reg_id)(void* ctx, int bit);
  // Caller-saved registers still available, most preferred first.
  int (*scratch_pool)(void* ctx, int* out, int cap, uint64_t taken,
                      uint64_t total_arg, uint64_t incoming_args);

  // Spill of a callee-saved register: its saved bit, hardware register, and
  // slot offset.  False rejects the whole function.  NULL rejects any spill
  // in the fast region.
  bool (*spill_info)(void* ctx, TargetInstruction* spill, int* saved_bit,
                     int* reg_id, int64_t* offset);
  bool (*reload_offset)(void* ctx, TargetInstruction* reload, int64_t* offset);
  // True when |inst| writes the callee-saved register |proto| spilled.
  bool (*clobbers_saved)(void* ctx, TargetInstruction* proto,
                         TargetInstruction* inst);

  // Entry splits into the fast return and the framed body on a cold null or
  // zero test.  The pass then leaves the prologue at the entry.
  bool (*cold_entry_split)(void* ctx, struct TargetBasicBlock* entry);
  // True when |inst| is work worth running before the prologue.  A fast
  // region of only null and zero guards is left alone.
  bool (*is_nontrivial)(void* ctx, TargetInstruction* inst);

  bool (*omit_from_clone)(void* ctx, TargetInstruction* inst);
  // Hardware register of |reg| when the fast region should rename it, or
  // false.  NULL when the fast region never uses a callee-saved register.
  bool (*remap_id)(void* ctx, TargetRegister* reg, int* reg_id);
  // Zero for the narrow move, one for the wide move.
  int (*value_width)(void* ctx, TargetInstruction* inst);
  bool (*is_var)(TargetInstruction* inst);

  TargetInstruction* (*new_label)(void* ctx);
  TargetInstruction* (*new_branch)(void* ctx, TargetInstruction* label);
  TargetInstruction* (*new_return)(void* ctx);
  // A stand-in for |reg_id| at |width| (0 narrow, 1 wide), not inserted.
  TargetInstruction* (*proxy)(void* ctx, TargetGenerator* gen, int reg_id,
                              int width);
  // Copy that replaces a reload of the slot whose register is |src_reg_id|.
  TargetInstruction* (*clone_reload)(void* ctx, TargetGenerator* gen,
                                    TargetInstruction* reload, int src_reg_id);
  // Move from the fast register back to the callee-saved register.
  TargetInstruction* (*create_move)(void* ctx, int saved_reg_id,
                                   TargetInstruction* fast_proxy,
                                   TargetInstruction* slow_proxy);
  TargetInstruction* (*clone_spill)(void* ctx, TargetInstruction* proto,
                                   int reg_id);
  // The fast region held |reg_id|, so debug locations must not say the
  // prologue saved it on every path.
  void (*note_wrapped)(void* ctx, int reg_id);

  // More than one framed successor.  NULL with max_slow_targets above 1
  // rejects the function.  |emit_trampolines| appends one block per successor
  // and fills |tramp_labels|; the last block falls into the prologue the pass
  // appends next.  |emit_dispatch| runs after the prologue and the restoring
  // moves.
  TargetInstruction* (*emit_trampolines)(void* ctx, TargetGenerator* gen,
                                        TargetInstruction* cursor,
                                        TargetInstruction* save_label,
                                        int num_slow,
                                        TargetInstruction** tramp_labels);
  TargetInstruction* (*emit_dispatch)(void* ctx, TargetGenerator* gen,
                                     struct TargetBasicBlock* save_block,
                                     TargetInstruction* cursor,
                                     TargetInstruction** slow_labels,
                                     int num_slow);
} TargetShrinkWrapOps;

void TargetShrinkWrap(TargetGenerator* gen, const TargetShrinkWrapOps* ops);

// Inserts |inst| after |cursor| and makes it the end of |block|.
TargetInstruction* TargetShrinkWrapAppend(TargetGenerator* gen,
                                          struct TargetBasicBlock* block,
                                          TargetInstruction* inst,
                                          TargetInstruction* cursor);

#endif /* target_shrink_wrap_h */
