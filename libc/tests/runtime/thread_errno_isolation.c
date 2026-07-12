// Each guest thread keeps its own errno; main errno stays unchanged.
#include <errno.h>
#include <threads.h>

static int worker(void* arg) {
  int id = *(int*)arg;
  errno = 100 + id;
  if (errno != 100 + id) {
    return 20 + id;
  }
  return id;
}

int main(void) {
  errno = 42;

  thrd_t t1;
  thrd_t t2;
  int a1 = 1;
  int a2 = 2;
  int r1 = 0;
  int r2 = 0;

  if (thrd_create(&t1, worker, &a1) != thrd_success) {
    return 1;
  }
  if (thrd_create(&t2, worker, &a2) != thrd_success) {
    return 2;
  }
  if (thrd_join(t1, &r1) != thrd_success || r1 != 1) {
    return 3;
  }
  if (thrd_join(t2, &r2) != thrd_success || r2 != 2) {
    return 4;
  }
  if (errno != 42) {
    return 5;
  }
  return 0;
}
