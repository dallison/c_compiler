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

long long __davecc_monotonic_time_us(void) {
#if defined(__DAVECC_HAS_GUEST_THREADS__)
  long long result = 0;
  if (syscall(SYS_MONOTONIC_TIME, &result) == 0) {
    return result;
  }
  return (long long)clock();
#else
  static long long deterministic_time;
  return deterministic_time++;
#endif
}

void thrd_yield(void) {
#if defined(__DAVECC_HAS_GUEST_THREADS__)
  (void)syscall(SYS_THREAD_YIELD);
#endif
}

static unsigned int MutexOwnerToken(void) {
  unsigned int owner = (unsigned int)thrd_current();
  return owner == 0 ? 1 : owner;
}

static unsigned int MutexLoad(const unsigned int* value) {
#if defined(__DAVECC_HAS_GUEST_THREADS__)
  return __atomic_load_n(value, 2);
#else
  return *value;
#endif
}

static void MutexStore(unsigned int* value, unsigned int desired) {
#if defined(__DAVECC_HAS_GUEST_THREADS__)
  __atomic_store_n(value, desired, 3);
#else
  *value = desired;
#endif
}

static int MutexCompareExchange(unsigned int* value, unsigned int* expected,
                                unsigned int desired) {
#if defined(__DAVECC_HAS_GUEST_THREADS__)
  return __atomic_compare_exchange_n(value, expected, desired, 0, 2, 0);
#else
  if (*value != *expected) {
    *expected = *value;
    return 0;
  }
  *value = desired;
  return 1;
#endif
}

static unsigned int* MutexOwnerSlot(mtx_t* mutex) {
  return (unsigned int*)&mutex->owner;
}

int mtx_init(mtx_t* mutex, int type) {
  if (mutex == NULL ||
      (type & ~(mtx_recursive | mtx_timed)) != 0) {
    return thrd_error;
  }
  mutex->state = 0;
  mutex->owner = 0;
  mutex->recursion = 0;
  mutex->type = type;
  return thrd_success;
}

void mtx_destroy(mtx_t* mutex) {
  if (mutex != NULL) {
    mutex->state = 0;
    mutex->owner = 0;
    mutex->recursion = 0;
  }
}

int mtx_trylock(mtx_t* mutex) {
  if (mutex == NULL) {
    return thrd_error;
  }
  unsigned int self = MutexOwnerToken();
  if ((mutex->type & mtx_recursive) != 0 && MutexLoad(&mutex->state) != 0 &&
      MutexLoad(MutexOwnerSlot(mutex)) == self) {
    ++mutex->recursion;
    return thrd_success;
  }
  unsigned int expected = 0;
  if (!MutexCompareExchange(&mutex->state, &expected, 1)) {
    return thrd_busy;
  }
  MutexStore(MutexOwnerSlot(mutex), self);
  mutex->recursion = 1;
  return thrd_success;
}

int mtx_lock(mtx_t* mutex) {
  for (;;) {
    int result = mtx_trylock(mutex);
    if (result == thrd_success || result == thrd_error) {
      return result;
    }
    thrd_yield();
  }
}

int __davecc_mtx_timedlock_for(mtx_t* mutex, long long timeout_us) {
#if !defined(__DAVECC_HAS_GUEST_THREADS__)
  (void)timeout_us;
  int result = mtx_trylock(mutex);
  return result == thrd_busy ? thrd_timedout : result;
#else
  if (timeout_us < 0) {
    timeout_us = 0;
  }
  long long start = __davecc_monotonic_time_us();
  long long deadline =
      timeout_us > 0x7fffffffffffffffLL - start
          ? 0x7fffffffffffffffLL
          : start + timeout_us;
  for (;;) {
    int result = mtx_trylock(mutex);
    if (result == thrd_success || result == thrd_error) {
      return result;
    }
    if (__davecc_monotonic_time_us() >= deadline) {
      return thrd_timedout;
    }
    thrd_yield();
  }
#endif
}

int mtx_timedlock(mtx_t* mutex, const struct timespec* time_point) {
  if (time_point == NULL) {
    return thrd_error;
  }
  long long now = (long long)time(NULL) * 1000000;
  long long deadline = (long long)time_point->tv_sec * 1000000 +
                       time_point->tv_nsec / 1000;
  return __davecc_mtx_timedlock_for(mutex, deadline - now);
}

int mtx_unlock(mtx_t* mutex) {
  if (mutex == NULL || MutexLoad(&mutex->state) == 0 ||
      MutexLoad(MutexOwnerSlot(mutex)) != MutexOwnerToken()) {
    return thrd_error;
  }
  if ((mutex->type & mtx_recursive) != 0 && mutex->recursion > 1) {
    --mutex->recursion;
    return thrd_success;
  }
  mutex->recursion = 0;
  MutexStore(MutexOwnerSlot(mutex), 0);
  MutexStore(&mutex->state, 0);
  return thrd_success;
}

int __davecc_once_begin(dave_once_flag_t* flag) {
  if (flag == NULL) {
    return 0;
  }
  for (;;) {
    unsigned int state = MutexLoad(&flag->state);
    if (state == 2) {
      return 0;
    }
    if (state == 0) {
      unsigned int expected = 0;
      if (MutexCompareExchange(&flag->state, &expected, 1)) {
        return 1;
      }
    }
    thrd_yield();
  }
}

void __davecc_once_complete(dave_once_flag_t* flag) {
  if (flag != NULL) {
    MutexStore(&flag->state, 2);
  }
}

void __davecc_once_abort(dave_once_flag_t* flag) {
  if (flag != NULL) {
    MutexStore(&flag->state, 0);
  }
}

void call_once(dave_once_flag_t* flag, void (*func)(void)) {
  if (func != NULL && __davecc_once_begin(flag)) {
    func();
    __davecc_once_complete(flag);
  }
}
