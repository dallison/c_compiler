#include <threads.h>

#if !defined(__x86_64__)
int main(void) {
  return 0;
}
#else

static int ns_init_count;
static int block_init_count;

int make_ns_value(void) {
  ns_init_count++;
  return 42;
}

int make_block_value(void) {
  block_init_count++;
  return 43;
}

thread_local int ns_scalar = make_ns_value();

int use_block(void) {
  thread_local int block_scalar = make_block_value();
  return block_scalar;
}

int worker(void* arg) {
  (void)arg;
  if (ns_scalar != 42) {
    return 1;
  }
  if (use_block() != 43) {
    return 2;
  }
  if (use_block() != 43) {
    return 3;
  }
  return 0;
}

int main(void) {
  if (ns_init_count != 1 || ns_scalar != 42) {
    return 11;
  }
  if (block_init_count != 0) {
    return 12;
  }
  if (use_block() != 43 || block_init_count != 1) {
    return 13;
  }
  if (use_block() != 43 || block_init_count != 1) {
    return 14;
  }

  thrd_t t;
  if (thrd_create(&t, worker, NULL) != thrd_success) {
    return 15;
  }
  int result = 99;
  if (thrd_join(t, &result) != thrd_success || result != 0) {
    return 16 + result;
  }
  if (ns_init_count != 2 || block_init_count != 2) {
    return 17;
  }
  return 0;
}

#endif
