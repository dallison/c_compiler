#include <threads.h>

static int completed;

static int finish_worker(void* argument) {
  int value = *(int*)argument;
  __atomic_fetch_add(&completed, value, 0);
  return value;
}

static int create_worker(void* argument) {
  thrd_t child;
  if (thrd_create(&child, finish_worker, argument) != thrd_success) {
    return 1;
  }
  int child_result = 0;
  if (thrd_join(child, &child_result) != thrd_success || child_result != 1) {
    return 2;
  }
  return 0;
}

int main(void) {
  int one = 1;
  thrd_t stale = 0;
  for (int i = 0; i < 40; i++) {
    thrd_t worker;
    if (thrd_create(&worker, finish_worker, &one) != thrd_success) {
      return 1;
    }
    int result = 0;
    if (thrd_join(worker, &result) != thrd_success || result != 1) {
      return 2;
    }
    if (i == 0) {
      stale = worker;
    }
  }

  thrd_t replacement;
  if (thrd_create(&replacement, finish_worker, &one) != thrd_success ||
      thrd_join(stale, 0) == thrd_success) {
    return 3;
  }
  int replacement_result = 0;
  if (thrd_join(replacement, &replacement_result) != thrd_success ||
      replacement_result != 1) {
    return 4;
  }

  thrd_t creators[4];
  for (int i = 0; i < 4; i++) {
    if (thrd_create(&creators[i], create_worker, &one) != thrd_success) {
      return 5;
    }
  }
  for (int i = 0; i < 4; i++) {
    int creator_result = 0;
    if (thrd_join(creators[i], &creator_result) != thrd_success ||
        creator_result != 0) {
      return 6;
    }
  }

  for (int i = 0; i < 40; i++) {
    thrd_t worker;
    if (thrd_create(&worker, finish_worker, &one) != thrd_success ||
        thrd_detach(worker) != thrd_success) {
      return 7;
    }
    int target = 46 + i;
    while (__atomic_load_n(&completed, 2) < target) {
      thrd_yield();
    }
  }

  thrd_t final_worker;
  if (thrd_create(&final_worker, finish_worker, &one) != thrd_success) {
    return 8;
  }
  int final_result = 0;
  if (thrd_join(final_worker, &final_result) != thrd_success ||
      final_result != 1) {
    return 9;
  }
  return __atomic_load_n(&completed, 2) == 86 ? 0 : 10;
}
