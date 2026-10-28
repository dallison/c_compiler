// ESP32 has no frame unwinder.  C++ still emits landing-pad calls, including
// from coroutines, so the link needs these symbols.  Reaching one means an
// exception was thrown; that path is not implemented on this target.
#include <stdlib.h>
#include <unwind.h>

void* __cxa_begin_catch(void* exception_object) {
  (void)exception_object;
  abort();
  return 0;
}

void __cxa_end_catch(void) { abort(); }

void* __davecc_eh_landing_pad_unwind_header(void) {
  abort();
  return 0;
}

long __davecc_eh_landing_pad_selector(void) {
  abort();
  return 0;
}

void _Unwind_Resume(_Unwind_Exception* exception) {
  (void)exception;
  abort();
}
