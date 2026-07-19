#include <threads.h>

struct Box {
  int id;
  Box(int v);
  ~Box();
};

static int dtor_log[8];
static int dtor_log_len;

Box::Box(int v) {
  id = v;
}

Box::~Box() {
  if (dtor_log_len < 8) {
    dtor_log[dtor_log_len++] = id;
  }
}

// Declared first so the compiler registers B's destructor before A's.
int touch_b(void) {
  thread_local Box b(2);
  return b.id;
}

int touch_a(void) {
  thread_local Box a(1);
  return a.id;
}

static int worker_dtor_base;

int worker(void* arg) {
  (void)arg;
  if (touch_a() != 1 || touch_b() != 2) {
    return 1;
  }
  worker_dtor_base = dtor_log_len;
  return 0;
}

int main(void) {
  thrd_t t;
  if (thrd_create(&t, worker, NULL) != thrd_success) {
    return 2;
  }
  int result = 99;
  if (thrd_join(t, &result) != thrd_success || result != 0) {
    return 3 + result;
  }
  if (dtor_log_len < worker_dtor_base + 2) {
    return 10 + dtor_log_len;
  }
  // Constructed A then B; destructors must run B then A.
  if (dtor_log[worker_dtor_base] != 2 || dtor_log[worker_dtor_base + 1] != 1) {
    return 20 + dtor_log[worker_dtor_base] * 10 +
           (dtor_log_len > worker_dtor_base + 1
                ? dtor_log[worker_dtor_base + 1]
                : 0);
  }
  return 0;
}
