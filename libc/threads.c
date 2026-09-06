//
//  threads.c
//  libc
//
//  Host-backed guest threads via DaveCC syscalls.
//

#include <threads.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <syscall.h>
#include <stdlib.h>

#if defined(__DAVECC_HAS_NATIVE_THREADS__)
#define __DAVECC_HAS_GUEST_THREADS__ 1
#endif

#define DAVECC_TSS_KEYS_MAX 64

typedef struct DaveCCTssKey {
  tss_dtor_t destructor;
  unsigned int generation;
  int active;
} DaveCCTssKey;

static DaveCCTssKey tss_keys[DAVECC_TSS_KEYS_MAX];
static unsigned int tss_registry_lock;
#if defined(__DAVECC_HAS_GUEST_THREADS__)
static __thread void* tss_values[DAVECC_TSS_KEYS_MAX];
static __thread unsigned int tss_generations[DAVECC_TSS_KEYS_MAX];

typedef struct DaveCCThreadExitJob {
  void (*fn)(void*);
  void* arg;
  struct DaveCCThreadExitJob* next;
} DaveCCThreadExitJob;

static __thread DaveCCThreadExitJob* thread_exit_jobs;
#else
static void* tss_values[DAVECC_TSS_KEYS_MAX];
static unsigned int tss_generations[DAVECC_TSS_KEYS_MAX];
#endif

#if defined(__DAVECC_HAS_NATIVE_THREADS__)
void __davecc_tls_thread_init(void);
void __davecc_tls_thread_fini(void);
long __davecc_linux_clone(void* stack_top, unsigned long flags, void* argument,
                          int (*entry)(void*), void* tls, int* tid);
void __davecc_linux_tls_copy(void* thread_pointer);
#if defined(__arm__)
#define DAVE_THREAD_MMAP SYS_mmap2
#else
#define DAVE_THREAD_MMAP SYS_mmap
#endif

typedef struct DaveCCConditionAtExit {
  cnd_t* condition;
  mtx_t* mutex;
  struct DaveCCConditionAtExit* next;
} DaveCCConditionAtExit;

static __thread DaveCCConditionAtExit* condition_at_exit;

typedef struct LinuxThread {
  volatile int tid;
  int result;
  int detached;
  struct LinuxThread* next_detached;
  thrd_start_t function;
  void* argument;
  void* stack;
  size_t stack_size;
  void* tls;
  size_t tls_size;
} LinuxThread;

__thread LinuxThread* __davecc_linux_current_thread;
static LinuxThread* detached_threads;
static unsigned int detached_threads_lock;

static void LockDetachedThreads(void) {
  for (;;) {
    unsigned int expected = 0;
    if (__atomic_compare_exchange_n(&detached_threads_lock, &expected, 1, 0, 2,
                                    0)) {
      return;
    }
    syscall(SYS_futex, &detached_threads_lock, 0, 1, 0, 0, 0);
  }
}

static void UnlockDetachedThreads(void) {
  __atomic_store_n(&detached_threads_lock, 0, 3);
  syscall(SYS_futex, &detached_threads_lock, 1, 1, 0, 0, 0);
}

static void ReapDetachedThreads(void) {
  LinuxThread* ready = NULL;
  LockDetachedThreads();
  LinuxThread** link = &detached_threads;
  while (*link != NULL) {
    LinuxThread* thread = *link;
    if (__atomic_load_n(&thread->tid, 2) == 0) {
      *link = thread->next_detached;
      thread->next_detached = ready;
      ready = thread;
    } else {
      link = &thread->next_detached;
    }
  }
  UnlockDetachedThreads();

  while (ready != NULL) {
    LinuxThread* thread = ready;
    ready = thread->next_detached;
    syscall(SYS_munmap, thread->stack, thread->stack_size);
    syscall(SYS_munmap, thread->tls, thread->tls_size);
    free(thread);
  }
}

int __davecc_linux_thread_entry(void* value) {
  LinuxThread* thread = (LinuxThread*)value;
  __davecc_linux_current_thread = thread;
  __davecc_tls_thread_init();
  int result = thread->function(thread->argument);
  thread->result = result;
  ReapDetachedThreads();
  __davecc_tls_thread_fini();
  return result;
}

int thrd_create(thrd_t* thr, thrd_start_t func, void* arg) {
  if (thr == NULL || func == NULL) return thrd_error;
  ReapDetachedThreads();
  LinuxThread* thread = (LinuxThread*)calloc(1, sizeof(LinuxThread));
  if (thread == NULL) return thrd_nomem;
  thread->stack_size = 1024 * 1024;
  thread->tls_size = 64 * 1024;
  thread->stack = (void*)syscall(DAVE_THREAD_MMAP, 0, thread->stack_size, 3,
                                 0x22, -1, 0);
  thread->tls =
      (void*)syscall(DAVE_THREAD_MMAP, 0, thread->tls_size, 3, 0x22, -1, 0);
  if (thread->stack == (void*)-1 || thread->tls == (void*)-1) {
    if (thread->stack != (void*)-1)
      syscall(SYS_munmap, thread->stack, thread->stack_size);
    if (thread->tls != (void*)-1)
      syscall(SYS_munmap, thread->tls, thread->tls_size);
    free(thread);
    return thrd_nomem;
  }
  thread->function = func;
  thread->argument = arg;
  __davecc_linux_tls_copy(thread->tls);
  unsigned long flags = 0x3d0f00;
  long child = __davecc_linux_clone(
      (char*)thread->stack + thread->stack_size, flags, thread,
      __davecc_linux_thread_entry, thread->tls, (int*)&thread->tid);
  if (child < 0) {
    syscall(SYS_munmap, thread->stack, thread->stack_size);
    syscall(SYS_munmap, thread->tls, thread->tls_size);
    free(thread);
    return child == -ENOMEM || child == -EAGAIN ? thrd_nomem : thrd_error;
  }
  *thr = (thrd_t)(uintptr_t)thread;
  return thrd_success;
}

int thrd_detach(thrd_t thr) {
  LinuxThread* thread = (LinuxThread*)(uintptr_t)thr;
  if (thread == NULL) return thrd_error;
  LockDetachedThreads();
  if (thread->detached) {
    UnlockDetachedThreads();
    return thrd_error;
  }
  thread->detached = 1;
  thread->next_detached = detached_threads;
  detached_threads = thread;
  UnlockDetachedThreads();
  ReapDetachedThreads();
  return thrd_success;
}

int thrd_join(thrd_t thr, int* res) {
  LinuxThread* thread = (LinuxThread*)(uintptr_t)thr;
  if (thread == NULL || thread->detached) return thrd_error;
  while (__atomic_load_n(&thread->tid, 2) != 0) {
    int expected = __atomic_load_n(&thread->tid, 0);
    if (expected != 0)
      syscall(SYS_futex, &thread->tid, 0, expected, 0, 0, 0);
  }
  if (res != NULL) *res = thread->result;
  syscall(SYS_munmap, thread->stack, thread->stack_size);
  syscall(SYS_munmap, thread->tls, thread->tls_size);
  free(thread);
  ReapDetachedThreads();
  return thrd_success;
}

int thrd_sleep(const struct timespec* duration, struct timespec* remaining) {
  if (duration == NULL || duration->tv_sec < 0 || duration->tv_nsec < 0 ||
      duration->tv_nsec >= 1000000000)
    return -1;
  long result = syscall(SYS_nanosleep, duration, remaining);
  return result == 0 ? 0 : (errno == EINTR ? -2 : -1);
}

thrd_t thrd_current(void) {
  if (__davecc_linux_current_thread != NULL) {
    return (thrd_t)(uintptr_t)__davecc_linux_current_thread;
  }
  return (thrd_t)syscall(SYS_gettid);
}

int thrd_equal(thrd_t a, thrd_t b) {
  return a == b;
}

void thrd_exit(int res) {
  if (__davecc_linux_current_thread != NULL) {
    __davecc_linux_current_thread->result = res;
    ReapDetachedThreads();
    __davecc_tls_thread_fini();
  }
  syscall(SYS_exit, res);
  for (;;) {}
}

#elif defined(__DAVECC_HAS_GUEST_THREADS__)
void __davecc_tls_thread_init(void);
void __davecc_tls_thread_fini(void);

typedef struct DaveCCConditionAtExit {
  cnd_t* condition;
  mtx_t* mutex;
  struct DaveCCConditionAtExit* next;
} DaveCCConditionAtExit;

static __thread DaveCCConditionAtExit* condition_at_exit;

int thrd_create(thrd_t* thr, thrd_start_t func, void* arg) {
  if (thr == NULL || func == NULL) {
    return thrd_error;
  }
  long tid = syscall(SYS_THREAD_CREATE, func, arg, __davecc_tls_thread_init,
                     __davecc_tls_thread_fini);
  if (tid <= 0) {
    return tid == -ENOMEM || tid == -EAGAIN ? thrd_nomem : thrd_error;
  }
  *thr = (thrd_t)tid;
  return thrd_success;
}

int thrd_detach(thrd_t thr) {
  if (thr == 0) {
    return thrd_error;
  }
  return syscall(SYS_THREAD_DETACH, thr) == 0 ? thrd_success : thrd_error;
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

int thrd_sleep(const struct timespec* duration, struct timespec* remaining) {
  if (duration == NULL || duration->tv_sec < 0 || duration->tv_nsec < 0 ||
      duration->tv_nsec >= 1000000000) {
    return -1;
  }
  long result = syscall(SYS_THREAD_SLEEP, duration, remaining);
  return result == 0 ? 0 : (result == -EINTR ? -2 : -1);
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

int thrd_detach(thrd_t thr) {
  (void)thr;
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

int thrd_sleep(const struct timespec* duration, struct timespec* remaining) {
  (void)duration;
  (void)remaining;
  return -1;
}
#endif

int __davecc_addr_wait(const volatile void* address, const void* expected,
                       size_t size, long long timeout_us) {
#if defined(__DAVECC_HAS_NATIVE_THREADS__)
  if (size != sizeof(unsigned int)) return thrd_error;
  unsigned int expected_value = *(const unsigned int*)expected;
  struct timespec timeout;
  struct timespec* timeout_pointer = NULL;
  if (timeout_us >= 0) {
    timeout.tv_sec = timeout_us / 1000000;
    timeout.tv_nsec = (timeout_us % 1000000) * 1000;
    timeout_pointer = &timeout;
  }
  long result =
      syscall(SYS_futex, address, 0, expected_value, timeout_pointer, 0, 0);
  if (result == 0 || errno == EAGAIN) return thrd_success;
  return errno == ETIMEDOUT ? thrd_timedout : thrd_error;
#elif defined(__DAVECC_HAS_GUEST_THREADS__)
  long result = syscall(SYS_ADDR_WAIT, address, expected, size, timeout_us);
  if (result == 0) {
    return thrd_success;
  }
  return result == 1 ? thrd_timedout : thrd_error;
#else
  (void)address;
  (void)expected;
  (void)size;
  (void)timeout_us;
  return thrd_error;
#endif
}

int __davecc_addr_wake(const volatile void* address, int wake_all) {
#if defined(__DAVECC_HAS_NATIVE_THREADS__)
  return syscall(SYS_futex, address, 1, wake_all ? INT_MAX : 1, 0, 0, 0) >= 0
             ? thrd_success
             : thrd_error;
#elif defined(__DAVECC_HAS_GUEST_THREADS__)
  return syscall(SYS_ADDR_WAKE, address, wake_all) >= 0 ? thrd_success
                                                        : thrd_error;
#else
  (void)address;
  (void)wake_all;
  return thrd_error;
#endif
}

unsigned int __davecc_hardware_concurrency(void) {
#if defined(__DAVECC_HAS_NATIVE_THREADS__)
  unsigned long mask[16] = {0};
  long bytes = syscall(SYS_sched_getaffinity, 0, sizeof(mask), mask);
  if (bytes < 0) return 0;
  unsigned int count = 0;
  for (size_t i = 0; i < sizeof(mask) / sizeof(mask[0]); ++i) {
    unsigned long value = mask[i];
    while (value != 0) {
      value &= value - 1;
      ++count;
    }
  }
  return count;
#elif defined(__DAVECC_HAS_GUEST_THREADS__)
  long result = syscall(SYS_HARDWARE_CONCURRENCY);
  return result > 0 ? (unsigned int)result : 0;
#else
  return 0;
#endif
}

long long __davecc_monotonic_time_us(void) {
#if defined(__DAVECC_HAS_NATIVE_THREADS__)
  struct timespec value;
  if (syscall(SYS_clock_gettime, 1, &value) == 0)
    return (long long)value.tv_sec * 1000000 + value.tv_nsec / 1000;
  return (long long)clock();
#elif defined(__DAVECC_HAS_HOST_CLOCK__)
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

long long __davecc_realtime_time_us(void) {
#if defined(__DAVECC_HAS_NATIVE_THREADS__)
  struct timespec value;
  if (syscall(SYS_clock_gettime, 0, &value) == 0)
    return (long long)value.tv_sec * 1000000 + value.tv_nsec / 1000;
#elif defined(__DAVECC_HAS_HOST_CLOCK__)
  long long result = 0;
  if (syscall(SYS_REALTIME_TIME, &result) == 0) {
    return result;
  }
#endif
  return (long long)time(NULL) * 1000000;
}

void thrd_yield(void) {
#if defined(__DAVECC_HAS_NATIVE_THREADS__)
  (void)syscall(SYS_sched_yield);
#elif defined(__DAVECC_HAS_GUEST_THREADS__)
  (void)syscall(SYS_THREAD_YIELD);
#endif
}

static void TssRegistryLock(void) {
#if defined(__DAVECC_HAS_GUEST_THREADS__)
  for (;;) {
    unsigned int expected = 0;
    if (__atomic_compare_exchange_n(&tss_registry_lock, &expected, 1, 0, 2,
                                    0)) {
      return;
    }
    thrd_yield();
  }
#endif
}

static void TssRegistryUnlock(void) {
#if defined(__DAVECC_HAS_GUEST_THREADS__)
  __atomic_store_n(&tss_registry_lock, 0, 3);
#endif
}

void __davecc_tss_thread_init(void) {
  for (tss_t key = 0; key < DAVECC_TSS_KEYS_MAX; ++key) {
    tss_values[key] = NULL;
    tss_generations[key] = 0;
  }
}

int tss_create(tss_t* key, tss_dtor_t destructor) {
  if (key == NULL) return thrd_error;
  TssRegistryLock();
  for (tss_t i = 0; i < DAVECC_TSS_KEYS_MAX; ++i) {
    if (!tss_keys[i].active) {
      unsigned int generation = tss_keys[i].generation + 1;
      if (generation == 0) generation = 1;
      tss_keys[i].generation = generation;
      tss_keys[i].destructor = destructor;
      tss_keys[i].active = 1;
      *key = i;
      TssRegistryUnlock();
      return thrd_success;
    }
  }
  TssRegistryUnlock();
  return thrd_error;
}

void tss_delete(tss_t key) {
  if (key >= DAVECC_TSS_KEYS_MAX) return;
  TssRegistryLock();
  tss_keys[key].active = 0;
  tss_keys[key].destructor = NULL;
  TssRegistryUnlock();
}

void* tss_get(tss_t key) {
  if (key >= DAVECC_TSS_KEYS_MAX) return NULL;
  TssRegistryLock();
  int valid = tss_keys[key].active &&
              tss_generations[key] == tss_keys[key].generation;
  void* value = valid ? tss_values[key] : NULL;
  TssRegistryUnlock();
  return value;
}

int tss_set(tss_t key, void* value) {
  if (key >= DAVECC_TSS_KEYS_MAX) return thrd_error;
  TssRegistryLock();
  if (!tss_keys[key].active) {
    TssRegistryUnlock();
    return thrd_error;
  }
  tss_values[key] = value;
  tss_generations[key] = tss_keys[key].generation;
  TssRegistryUnlock();
  return thrd_success;
}

static void RunTssDestructors(void) {
  for (int pass = 0; pass < TSS_DTOR_ITERATIONS; ++pass) {
    int called_destructor = 0;
    for (tss_t key = 0; key < DAVECC_TSS_KEYS_MAX; ++key) {
      TssRegistryLock();
      int valid = tss_keys[key].active &&
                  tss_generations[key] == tss_keys[key].generation;
      void* value = valid ? tss_values[key] : NULL;
      tss_dtor_t destructor = valid ? tss_keys[key].destructor : NULL;
      tss_values[key] = NULL;
      TssRegistryUnlock();
      if (value != NULL && destructor != NULL) {
        called_destructor = 1;
        destructor(value);
      }
    }
    if (!called_destructor) return;
  }
}

static thrd_t MutexOwnerToken(void) {
  thrd_t owner = thrd_current();
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

static thrd_t MutexOwnerLoad(const mtx_t* mutex) {
#if defined(__DAVECC_HAS_GUEST_THREADS__)
  return __atomic_load_n(&mutex->owner, 2);
#else
  return mutex->owner;
#endif
}

static void MutexOwnerStore(mtx_t* mutex, thrd_t owner) {
#if defined(__DAVECC_HAS_GUEST_THREADS__)
  __atomic_store_n(&mutex->owner, owner, 3);
#else
  mutex->owner = owner;
#endif
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
  thrd_t self = MutexOwnerToken();
  if ((mutex->type & mtx_recursive) != 0 && MutexLoad(&mutex->state) != 0 &&
      MutexOwnerLoad(mutex) == self) {
    ++mutex->recursion;
    return thrd_success;
  }
  unsigned int expected = 0;
  if (!MutexCompareExchange(&mutex->state, &expected, 1)) {
    return thrd_busy;
  }
  MutexOwnerStore(mutex, self);
  mutex->recursion = 1;
  return thrd_success;
}

int mtx_lock(mtx_t* mutex) {
  for (;;) {
    int result = mtx_trylock(mutex);
    if (result == thrd_success || result == thrd_error) {
      return result;
    }
    unsigned int locked = 1;
#if defined(__DAVECC_HAS_GUEST_THREADS__)
    if (__davecc_addr_wait(&mutex->state, &locked, sizeof(locked), -1) ==
        thrd_error) {
      return thrd_error;
    }
#else
    (void)locked;
    thrd_yield();
#endif
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
    long long remaining = deadline - __davecc_monotonic_time_us();
    unsigned int locked = 1;
    int wait_result = __davecc_addr_wait(
        &mutex->state, &locked, sizeof(locked), remaining > 0 ? remaining : 0);
    if (wait_result == thrd_error) {
      return thrd_error;
    }
  }
#endif
}

static long long TimespecToMicroseconds(const struct timespec* time_point) {
  if ((long long)time_point->tv_sec > LLONG_MAX / 1000000) {
    return LLONG_MAX;
  }
  return (long long)time_point->tv_sec * 1000000 +
         time_point->tv_nsec / 1000;
}

int mtx_timedlock(mtx_t* mutex, const struct timespec* time_point) {
  if (time_point == NULL || time_point->tv_sec < 0 ||
      time_point->tv_nsec < 0 || time_point->tv_nsec >= 1000000000) {
    return thrd_error;
  }
  long long now = __davecc_realtime_time_us();
  long long deadline = TimespecToMicroseconds(time_point);
  return __davecc_mtx_timedlock_for(mutex, deadline - now);
}

int mtx_unlock(mtx_t* mutex) {
  if (mutex == NULL || MutexLoad(&mutex->state) == 0 ||
      MutexOwnerLoad(mutex) != MutexOwnerToken()) {
    return thrd_error;
  }
  if ((mutex->type & mtx_recursive) != 0 && mutex->recursion > 1) {
    --mutex->recursion;
    return thrd_success;
  }
  mutex->recursion = 0;
  MutexOwnerStore(mutex, 0);
  MutexStore(&mutex->state, 0);
  (void)__davecc_addr_wake(&mutex->state, 0);
  return thrd_success;
}

static unsigned int ConditionLoad(const cnd_t* condition) {
#if defined(__DAVECC_HAS_GUEST_THREADS__)
  return __atomic_load_n(&condition->generation, 2);
#else
  return condition->generation;
#endif
}

static void ConditionAdvance(cnd_t* condition) {
#if defined(__DAVECC_HAS_GUEST_THREADS__)
  (void)__atomic_fetch_add(&condition->generation, 1, 3);
#else
  condition->generation++;
#endif
}

int cnd_init(cnd_t* condition) {
  if (condition == NULL) {
    return thrd_error;
  }
  condition->generation = 0;
  return thrd_success;
}

void cnd_destroy(cnd_t* condition) {
  if (condition != NULL) {
    condition->generation = 0;
  }
}

int cnd_signal(cnd_t* condition) {
  if (condition == NULL) {
    return thrd_error;
  }
  ConditionAdvance(condition);
  return __davecc_addr_wake(&condition->generation, 0);
}

int cnd_broadcast(cnd_t* condition) {
  if (condition == NULL) {
    return thrd_error;
  }
  ConditionAdvance(condition);
  return __davecc_addr_wake(&condition->generation, 1);
}

static int ConditionWaitFor(cnd_t* condition, mtx_t* mutex,
                            long long timeout_us) {
  if (condition == NULL || mutex == NULL) {
    return thrd_error;
  }
  unsigned int generation = ConditionLoad(condition);
  int unlock_result = mtx_unlock(mutex);
  if (unlock_result != thrd_success) {
    return unlock_result;
  }
  int wait_result = __davecc_addr_wait(
      &condition->generation, &generation, sizeof(generation), timeout_us);
  int lock_result = mtx_lock(mutex);
  return lock_result == thrd_success ? wait_result : lock_result;
}

int cnd_wait(cnd_t* condition, mtx_t* mutex) {
  return ConditionWaitFor(condition, mutex, -1);
}

int __davecc_cnd_timedwait_for(cnd_t* condition, mtx_t* mutex,
                               long long timeout_us) {
  return ConditionWaitFor(condition, mutex, timeout_us > 0 ? timeout_us : 0);
}

int __davecc_cnd_notify_all_at_thread_exit(cnd_t* condition, mtx_t* mutex) {
#if defined(__DAVECC_HAS_GUEST_THREADS__)
  if (condition == NULL || mutex == NULL) {
    return thrd_error;
  }
  DaveCCConditionAtExit* entry = malloc(sizeof(*entry));
  if (entry == NULL) {
    return thrd_nomem;
  }
  entry->condition = condition;
  entry->mutex = mutex;
  entry->next = condition_at_exit;
  condition_at_exit = entry;
  return thrd_success;
#else
  (void)condition;
  (void)mutex;
  return thrd_error;
#endif
}

void __davecc_thread_exit_callbacks(void) {
#if defined(__DAVECC_HAS_GUEST_THREADS__)
  RunTssDestructors();
  while (thread_exit_jobs != NULL) {
    DaveCCThreadExitJob* job = thread_exit_jobs;
    thread_exit_jobs = job->next;
    void (*fn)(void*) = job->fn;
    void* arg = job->arg;
    free(job);
    if (fn != NULL) {
      fn(arg);
    }
  }
  while (condition_at_exit != NULL) {
    DaveCCConditionAtExit* entry = condition_at_exit;
    condition_at_exit = entry->next;
    (void)mtx_unlock(entry->mutex);
    (void)cnd_broadcast(entry->condition);
    free(entry);
  }
#endif
}

int __davecc_register_thread_exit(void (*fn)(void*), void* arg) {
#if defined(__DAVECC_HAS_GUEST_THREADS__)
  if (fn == NULL) {
    return thrd_error;
  }
  DaveCCThreadExitJob* job = malloc(sizeof(*job));
  if (job == NULL) {
    return thrd_nomem;
  }
  job->fn = fn;
  job->arg = arg;
  job->next = thread_exit_jobs;
  thread_exit_jobs = job;
  return thrd_success;
#else
  (void)fn;
  (void)arg;
  return thrd_error;
#endif
}

int cnd_timedwait(cnd_t* condition, mtx_t* mutex,
                  const struct timespec* time_point) {
  if (time_point == NULL || time_point->tv_sec < 0 ||
      time_point->tv_nsec < 0 ||
      time_point->tv_nsec >= 1000000000) {
    return thrd_error;
  }
  long long deadline = TimespecToMicroseconds(time_point);
  long long now = __davecc_realtime_time_us();
  return ConditionWaitFor(condition, mutex,
                          deadline > now ? deadline - now : 0);
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
    unsigned int initializing = 1;
#if defined(__DAVECC_HAS_GUEST_THREADS__)
    if (__davecc_addr_wait(&flag->state, &initializing,
                           sizeof(initializing), -1) == thrd_error) {
      return 0;
    }
#else
    (void)initializing;
    thrd_yield();
#endif
  }
}

void __davecc_once_complete(dave_once_flag_t* flag) {
  if (flag != NULL) {
    MutexStore(&flag->state, 2);
    (void)__davecc_addr_wake(&flag->state, 1);
  }
}

void __davecc_once_abort(dave_once_flag_t* flag) {
  if (flag != NULL) {
    MutexStore(&flag->state, 0);
    (void)__davecc_addr_wake(&flag->state, 1);
  }
}

void call_once(dave_once_flag_t* flag, void (*func)(void)) {
  if (func != NULL && __davecc_once_begin(flag)) {
    func();
    __davecc_once_complete(flag);
  }
}
