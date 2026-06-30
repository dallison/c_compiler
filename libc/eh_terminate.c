#include <stdlib.h>

// Default std::terminate handler: the standard's default calls abort().
//
// This lives in its own translation unit -- separate from the x86_64-only
// exception-unwinding runtime in eh_throw.c -- because the compiler emits a
// reference to it for *any* noexcept function that contains a potentially
// throwing call (a per-function landing pad that calls terminate when an
// exception would escape).  That lowering is backend-independent, so the symbol
// must be available on every target, including those whose full exception
// machinery is not yet implemented.
void __davecc_terminate(void) {
  abort();
}
