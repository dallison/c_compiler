#ifndef x86_64_machine_h
#define x86_64_machine_h
#include "../x86/x86_machine.h"

// Compatibility for the x86_64 interpreter's physical register file.  The
// shared backend exposes the larger logical allocator limit separately.
#define X86_NUM_INT_REGS 16

#endif /* x86_64_machine_h */
