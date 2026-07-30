#include <threads.h>

static unsigned int value;
static unsigned int entered;

static int wait_worker(void* argument) {
  (void)argument;
  unsigned int expected = 0;
  __atomic_store_n(&entered, 1, 3);
  int result =
      __davecc_addr_wait(&value, &expected, sizeof(expected), -1);
  return result == thrd_success && __atomic_load_n(&value, 2) == 1 ? 0 : 1;
}

int main(void) {
  thrd_t worker;
  if (thrd_create(&worker, wait_worker, 0) != thrd_success) {
    return 1;
  }
  while (__atomic_load_n(&entered, 2) == 0) {
    thrd_yield();
  }
  __atomic_store_n(&value, 1, 3);
  if (__davecc_addr_wake(&value, 0) != thrd_success) {
    return 2;
  }
  int worker_result = 0;
  if (thrd_join(worker, &worker_result) != thrd_success ||
      worker_result != 0) {
    return 3;
  }

  unsigned int unchanged = 7;
  unsigned int expected = 7;
  int timeout = __davecc_addr_wait(
      &unchanged, &expected, sizeof(expected), 1000);
  if (timeout != thrd_timedout) {
    return 4;
  }

  unsigned long long unchanged_wide = 9;
  unsigned long long expected_wide = 9;
  timeout = __davecc_addr_wait(&unchanged_wide, &expected_wide,
                               sizeof(expected_wide), 1000);
  return timeout == thrd_timedout ? 0 : 5;
}
