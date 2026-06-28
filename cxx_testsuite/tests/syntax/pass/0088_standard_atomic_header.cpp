// RUN: -std=c++20
#include <atomic>

int standard_atomic_header_values(void) {
  std::atomic_int value(3);
  int loaded = value.load();
  value.store(loaded + 2, std::memory_order_seq_cst);
  int old_add = value.fetch_add(4);
  int new_inc = ++value;
  int new_dec = --value;
  int expected = new_dec;
  bool exchanged = value.compare_exchange_strong(expected, 42);
  std::atomic_thread_fence();
  return loaded + old_add + new_inc + new_dec + expected + exchanged +
         value.load();
}
