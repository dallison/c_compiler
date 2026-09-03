#include <threads.h>

static tss_t key;
static int destructor_calls;
static int marker;

static void destroy_value(void* value) {
  if (value != &marker) {
    destructor_calls = -100;
    return;
  }
  ++destructor_calls;
  if (destructor_calls < 3) {
    (void)tss_set(key, &marker);
  }
}

static int worker(void* argument) {
  if (tss_get(key) != 0)
    return 1;
  if (tss_set(key, argument) != thrd_success)
    return 2;
  return tss_get(key) == argument ? 0 : 3;
}

int main(void) {
  thrd_t thread;
  int result = -1;
  if (tss_create(&key, destroy_value) != thrd_success)
    return 1;
  if (tss_get(key) != 0 || tss_set(key, &marker) != thrd_success)
    return 2;
  if (thrd_create(&thread, worker, &marker) != thrd_success)
    return 3;
  if (thrd_join(thread, &result) != thrd_success || result != 0)
    return 4;
  if (destructor_calls != 3 || tss_get(key) != &marker)
    return 5;
  tss_delete(key);
  return tss_set(key, &marker) == thrd_error ? 0 : 6;
}
