// Join must propagate negative thread return values unambiguously.
#include <threads.h>

static int worker(void* arg) {
  (void)arg;
  return -7;
}

int main(void) {
  thrd_t t;
  if (thrd_create(&t, worker, NULL) != thrd_success) {
    return 1;
  }
  int result = 0;
  if (thrd_join(t, &result) != thrd_success) {
    return 2;
  }
  if (result != -7) {
    return 3;
  }
  return 0;
}
