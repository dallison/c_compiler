#include <threads.h>

enum { kThreads = 4, kIterations = 200 };

static mtx_t counter_mutex;
static int counter;
static once_flag once = ONCE_FLAG_INIT;
static int once_count;

static int increment_worker(void* unused) {
  (void)unused;
  for (int i = 0; i < kIterations; ++i) {
    if (mtx_lock(&counter_mutex) != thrd_success) return 1;
    ++counter;
    if (mtx_unlock(&counter_mutex) != thrd_success) return 2;
  }
  return 0;
}

static void initialize_once(void) {
  ++once_count;
}

static int once_worker(void* unused) {
  (void)unused;
  call_once(&once, initialize_once);
  return 0;
}

static int contention_worker(void* mutex_ptr) {
  mtx_t* mutex = (mtx_t*)mutex_ptr;
  if (mtx_trylock(mutex) != thrd_busy) return 1;
  if (mtx_unlock(mutex) != thrd_error) return 2;
  if (__davecc_mtx_timedlock_for(mutex, 2000) != thrd_timedout) return 3;
  return 0;
}

static int wake_worker(void* mutex_ptr) {
  mtx_t* mutex = (mtx_t*)mutex_ptr;
  if (mtx_lock(mutex) != thrd_success) return 1;
  counter = 1234;
  return mtx_unlock(mutex) == thrd_success ? 0 : 2;
}

int main(void) {
  thrd_t threads[kThreads];
  int result;

  if (mtx_init(&counter_mutex, mtx_plain) != thrd_success) return 1;
  for (int i = 0; i < kThreads; ++i) {
    if (thrd_create(&threads[i], increment_worker, 0) != thrd_success) return 2;
  }
  for (int i = 0; i < kThreads; ++i) {
    if (thrd_join(threads[i], &result) != thrd_success) return 3;
    if (result != 0) return 30 + result;
  }
  if (counter != kThreads * kIterations) return 4;

  mtx_t recursive;
  if (mtx_init(&recursive, mtx_recursive | mtx_timed) != thrd_success) return 5;
  if (mtx_lock(&recursive) != thrd_success ||
      mtx_trylock(&recursive) != thrd_success ||
      mtx_unlock(&recursive) != thrd_success ||
      mtx_unlock(&recursive) != thrd_success) {
    return 6;
  }

  if (mtx_lock(&counter_mutex) != thrd_success) return 7;
  if (thrd_create(&threads[0], contention_worker, &counter_mutex) !=
      thrd_success) {
    return 8;
  }
  if (thrd_join(threads[0], &result) != thrd_success || result != 0) return 9;
  if (mtx_unlock(&counter_mutex) != thrd_success) return 10;

  once.state = 0;
  once_count = 0;
  for (int i = 0; i < kThreads; ++i) {
    if (thrd_create(&threads[i], once_worker, 0) != thrd_success) return 11;
  }
  for (int i = 0; i < kThreads; ++i) {
    if (thrd_join(threads[i], &result) != thrd_success || result != 0) return 12;
  }
  if (once_count != 1) return 13;

  if (mtx_lock(&counter_mutex) != thrd_success) return 14;
  if (thrd_create(&threads[0], wake_worker, &counter_mutex) != thrd_success)
    return 15;
  for (int i = 0; i < 10; ++i) thrd_yield();
  if (mtx_unlock(&counter_mutex) != thrd_success) return 16;
  if (thrd_join(threads[0], &result) != thrd_success || result != 0) return 17;
  if (counter != 1234) return 18;

  mtx_destroy(&recursive);
  mtx_destroy(&counter_mutex);
  return 0;
}
