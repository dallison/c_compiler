#include <stdint.h>

// Migration-stage personality stubs.  Later stages replace these with full
// Itanium/ARM EHABI dispatch while keeping the canonical symbol names.

__attribute__((weak)) int __gxx_personality_v0(int version, int actions,
                                              uint64_t exception_class,
                                              void* exception_object,
                                              void* context) {
  (void)version;
  (void)actions;
  (void)exception_class;
  (void)exception_object;
  (void)context;
  return 1;  // _URC_FATAL_PHASE1_ERROR
}

#if defined(__arm__)
__attribute__((weak)) int __aeabi_unwind_cpp_pr0(int state, int reason,
                                                  void* unwind_data) {
  (void)state;
  (void)reason;
  (void)unwind_data;
  return 9;  // _URC_FAILURE
}

__attribute__((weak)) int __aeabi_unwind_cpp_pr1(int state, int reason,
                                                  void* unwind_data) {
  return __aeabi_unwind_cpp_pr0(state, reason, unwind_data);
}

__attribute__((weak)) int __aeabi_unwind_cpp_pr2(int state, int reason,
                                                  void* unwind_data) {
  return __aeabi_unwind_cpp_pr0(state, reason, unwind_data);
}
#endif
