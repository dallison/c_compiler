#include <threads.h>

int main(void) {
  if (__davecc_hardware_concurrency() == 0) {
    return 1;
  }

  struct timespec duration = {0, 2000000};
  long long before = __davecc_monotonic_time_us();
  if (thrd_sleep(&duration, 0) != 0) {
    return 2;
  }
  long long elapsed = __davecc_monotonic_time_us() - before;
  return elapsed >= 1500 ? 0 : 3;
}
