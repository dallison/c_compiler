//
//  threads.c
//  libc
//
//  Host-backed guest threads via DaveCC syscalls.
//

#include <threads.h>
#include <syscall.h>
#include <stdlib.h>

#if defined(__DAVECC_HAS_GUEST_THREADS__)
void __davecc_tls_thread_init(void);
void __davecc_tls_thread_fini(void);

int thrd_create(thrd_t* thr, thrd_start_t func, void* arg) {
  if (thr == NULL || func == NULL) {
    return thrd_error;
  }
  long tid = syscall(SYS_THREAD_CREATE, func, arg, __davecc_tls_thread_init,
                     __davecc_tls_thread_fini);
  if (tid <= 0) {
    return thrd_nomem;
  }
  *thr = (thrd_t)tid;
  return thrd_success;
}

int thrd_join(thrd_t thr, int* res) {
  if (thr == 0) {
    return thrd_error;
  }
  long rc = syscall(SYS_THREAD_JOIN, thr, res);
  if (rc != 0) {
    return thrd_error;
  }
  return thrd_success;
}

thrd_t thrd_current(void) {
  return (thrd_t)syscall(SYS_THREAD_SELF);
}

int thrd_equal(thrd_t a, thrd_t b) {
  return a == b;
}

void thrd_exit(int res) {
  syscall(SYS_THREAD_EXIT, res);
}
#else
int thrd_create(thrd_t* thr, thrd_start_t func, void* arg) {
  (void)thr;
  (void)func;
  (void)arg;
  return thrd_error;
}

int thrd_join(thrd_t thr, int* res) {
  (void)thr;
  (void)res;
  return thrd_error;
}

thrd_t thrd_current(void) {
  return 0;
}

int thrd_equal(thrd_t a, thrd_t b) {
  return a == b;
}

void thrd_exit(int res) {
  exit(res);
}
#endif
