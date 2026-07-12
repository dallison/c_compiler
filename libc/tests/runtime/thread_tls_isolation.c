// Two worker threads and main each keep independent __thread storage.
#include <threads.h>

__thread int tls_val = 100;

static int worker(void* arg) {
  int id = *(int*)arg;
  if (tls_val != 100) {
    return 10 + id;
  }
  tls_val = id;
  if (thrd_equal(thrd_current(), 0)) {
    return 20 + id;
  }
  return id;
}

int main(void) {
  thrd_t t1;
  thrd_t t2;
  int a1 = 1;
  int a2 = 2;

  if (thrd_create(&t1, worker, &a1) != thrd_success) {
    return 1;
  }
  if (thrd_create(&t2, worker, &a2) != thrd_success) {
    return 2;
  }

  tls_val = 999;

  int r1 = 0;
  int r2 = 0;
  if (thrd_join(t1, &r1) != thrd_success || r1 != 1) {
    return 3;
  }
  if (thrd_join(t2, &r2) != thrd_success || r2 != 2) {
    return 4;
  }
  if (tls_val != 999) {
    return 5;
  }
  return 0;
}
