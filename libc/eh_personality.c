#include <stdint.h>

// Migration-stage ARM EHABI personality stubs.  x86_64/AArch64/RISC-V use the
// full Itanium implementation in gxx_personality.c.

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
