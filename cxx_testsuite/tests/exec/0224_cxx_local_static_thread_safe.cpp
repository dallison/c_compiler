#include <threads.h>

#if !defined(__x86_64__)
int main() {
  return 0;
}
#else

static int initialization_count;

int make_value() {
  initialization_count++;
  for (volatile int i = 0; i < 100000; i++) {
  }
  return 42;
}

int shared_value() {
  static int value = make_value();
  return value;
}

int worker(void*) {
  return shared_value() == 42 ? 0 : 1;
}

int main() {
  thrd_t first;
  thrd_t second;
  if (thrd_create(&first, worker, nullptr) != thrd_success ||
      thrd_create(&second, worker, nullptr) != thrd_success) {
    return 2;
  }
  int first_result = 0;
  int second_result = 0;
  if (thrd_join(first, &first_result) != thrd_success ||
      thrd_join(second, &second_result) != thrd_success) {
    return 3;
  }
  if (first_result != 0 || second_result != 0) {
    return 4;
  }
  return initialization_count == 1 ? 0 : 5;
}

#endif
