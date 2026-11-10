// ESP32 has no frame unwinder.  C++ still emits EH calls (including from
// coroutines and the standard library), so the link needs these symbols.
// Reaching one means an exception was thrown or cleaned up; that path is
// not implemented on this target, so abort.
#include <stddef.h>
#include <stdlib.h>
#include <unwind.h>

struct type_info;

void* __cxa_allocate_exception(size_t thrown_size) {
  (void)thrown_size;
  abort();
  return 0;
}

void __cxa_free_exception(void* thrown_exception) {
  (void)thrown_exception;
  abort();
}

void __cxa_throw(void* thrown_exception, struct type_info* tinfo,
                 void (*dest)(void*)) {
  (void)thrown_exception;
  (void)tinfo;
  (void)dest;
  abort();
}

void __cxa_rethrow(void) { abort(); }

void* __cxa_begin_catch(void* exception_object) {
  (void)exception_object;
  abort();
  return 0;
}

void __cxa_end_catch(void) { abort(); }

void* __cxa_get_exception_ptr(void* exception_object) {
  (void)exception_object;
  abort();
  return 0;
}

void __cxa_call_unexpected(void* exception_object) {
  (void)exception_object;
  abort();
}

void __davecc_terminate(void) { abort(); }

void* __davecc_eh_landing_pad_unwind_header(void) {
  abort();
  return 0;
}

long __davecc_eh_landing_pad_selector(void) {
  abort();
  return 0;
}

void* __davecc_eh_landing_pad_exception_object(void) {
  abort();
  return 0;
}

void _Unwind_Resume(_Unwind_Exception* exception) {
  (void)exception;
  abort();
}
