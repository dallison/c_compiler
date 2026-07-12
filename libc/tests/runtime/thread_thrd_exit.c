// Worker terminates via thrd_exit; fini runs once and join sees the status.
#include <threads.h>

static int worker(void* arg) {
  (void)arg;
  thrd_exit(42);
  return 99;
}

int main(void) {
  thrd_t t;
  if (thrd_create(&t, worker, NULL) != thrd_success) {
    return 1;
  }
  int result = 0;
  if (thrd_join(t, &result) != thrd_success || result != 42) {
    return 2;
  }
  return 0;
}
