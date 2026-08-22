//
//  errno.c
//  libc
//

#include <syscall.h>

#if !defined(__6502__)
#if defined(__DAVECC_HAS_TLS_THREAD_ERRNO__)
__thread int errno;

int* __davecc_errno_location(void) {
  return &errno;
}
#else
int errno;

int* __davecc_errno_location(void) {
  return &errno;
}
#endif
#endif
