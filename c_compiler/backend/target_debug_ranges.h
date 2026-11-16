//
//  target_debug_ranges.h
//  c_compiler_library
//
//  Where variables are in registers over a function's final code, for the
//  location lists of the debug information.  After register allocation a
//  location holds its variable wherever the location is live under the rules
//  in target_ra_ir.h: from each write to the last read of the value written,
//  the allocator keeps the location's register for it.  So does a value that
//  copies the location into the same register, while the variable can get no
//  new value in another register.
//

#ifndef target_debug_ranges_h
#define target_debug_ranges_h

#include <stddef.h>
#include "target_generator.h"
#include "target_ra_ir.h"

typedef struct {
  Symbol* symbol;
  TargetInstruction* locations[2];  // Unused ones are NULL.
  bool holds_address;               // The locations hold its address.
} TargetDebugVariable;

// Gives each of the |num_vars| variables in |vars| a location list: the
// registers of its locations, over the code of |gen|'s current function where
// they hold it.  |dwarf_register| returns a register's DWARF number, or -1 for
// a register that a location does not stay in.  |emits_code| returns false
// only for an instruction that changes no register a variable can be in, as
// when the emitter prints no machine code for it.  Gives the instructions
// where the stretches of code begin and end debug labels.
void TargetSetDebugLocationLists(
    TargetGenerator* gen, const TargetRAInfo* info,
    const TargetDebugVariable* vars, int num_vars,
    int (*dwarf_register)(TargetGenerator* gen, TargetRegister* reg),
    bool (*emits_code)(TargetInstruction* inst));

// The debug label to emit before |inst|, or -1.
static inline int TargetDebugLabel(const TargetGenerator* gen,
                                   const TargetInstruction* inst) {
  return gen->debug_labels == NULL
             ? -1
             : (int)HashMapFind(gen->debug_labels, (uint64_t)(uintptr_t)inst,
                                -1);
}

// Writes the name of debug label |label| to |buf|.
void TargetDebugLabelName(int label, char* buf, size_t size);

#endif /* target_debug_ranges_h */
