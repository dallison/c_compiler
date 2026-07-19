#include <threads.h>

struct TlsBox {
  int id;
  TlsBox(int v);
  ~TlsBox();
};

thread_local int tls_ctor_count;
thread_local TlsBox first(1);
thread_local TlsBox second(2);

static int dtor_log[16];
static int dtor_log_len;

TlsBox::TlsBox(int v) {
  id = v;
  tls_ctor_count++;
}

TlsBox::~TlsBox() {
  if (dtor_log_len < 16) {
    dtor_log[dtor_log_len++] = id;
  }
}

struct BlockBox {
  BlockBox();
  ~BlockBox();
  int value;
};

thread_local int block_ctor_count;

BlockBox::BlockBox() {
  block_ctor_count++;
  value = 11;
}

BlockBox::~BlockBox() {
  if (dtor_log_len < 16) {
    dtor_log[dtor_log_len++] = 100;
  }
}

int use_block(void) {
  thread_local BlockBox box;
  return box.value;
}

static int worker_results[2];
static int worker_dtor_base[2];

int worker(void* arg) {
  int id = *(int*)arg;
  if (tls_ctor_count != 2) {
    return 100 + id;
  }
  int v = use_block();
  if (block_ctor_count != 1 || v != 11) {
    return 200 + id;
  }
  if (use_block() != 11 || block_ctor_count != 1) {
    return 300 + id;
  }
  first.id = id;
  worker_results[id - 1] = first.id;
  worker_dtor_base[id - 1] = dtor_log_len;
  return id;
}

static int check_dtor_order(int base, int id) {
  // Block destructor (100), then second (2), then first (per-thread id).
  if (dtor_log_len < base + 3) {
    return 0;
  }
  return dtor_log[base] == 100 && dtor_log[base + 1] == 2 &&
         dtor_log[base + 2] == id;
}

int main(void) {
  if (tls_ctor_count != 2) {
    return 1;
  }
  if (first.id != 1 || second.id != 2) {
    return 2;
  }

  thrd_t t1;
  thrd_t t2;
  int a1 = 1;
  int a2 = 2;
  if (thrd_create(&t1, worker, &a1) != thrd_success) {
    return 3;
  }

  int r1 = 0;
  if (thrd_join(t1, &r1) != thrd_success || r1 != 1) {
    return 5;
  }
  if (!check_dtor_order(worker_dtor_base[0], 1)) {
    return 801;
  }

  if (thrd_create(&t2, worker, &a2) != thrd_success) {
    return 4;
  }

  int r2 = 0;
  if (thrd_join(t2, &r2) != thrd_success || r2 != 2) {
    return 6;
  }
  if (!check_dtor_order(worker_dtor_base[1], 2)) {
    return 802 + dtor_log[worker_dtor_base[1]] * 100 +
           (dtor_log_len > worker_dtor_base[1] + 1
                ? dtor_log[worker_dtor_base[1] + 1]
                : 0);
  }
  if (worker_results[0] != 1 || worker_results[1] != 2) {
    return 7;
  }
  if (first.id != 1) {
    return 9;
  }
  return 0;
}
