//
//  cxx_guard.c
//  c_compiler
//
//  Portable, single-threaded fallback for the Itanium C++ local-static guard
//  ABI.  x86_64 supplies synchronized implementations in cxx_guard.s.
//

#include <stdlib.h>
#include <syscall.h>

#if !defined(__DAVECC_HAS_THREAD_SAFE_GUARDS__)

int __cxa_guard_acquire(unsigned long long* guard) {
  unsigned char* state = (unsigned char*)guard;
  if (state[0] != 0) {
    return 0;
  }
  if (state[1] != 0) {
    abort();
  }
  state[1] = 1;
  return 1;
}

void __cxa_guard_release(unsigned long long* guard) {
  unsigned char* state = (unsigned char*)guard;
  state[0] = 1;
  state[1] = 0;
}

void __cxa_guard_abort(unsigned long long* guard) {
  ((unsigned char*)guard)[1] = 0;
}

#endif
