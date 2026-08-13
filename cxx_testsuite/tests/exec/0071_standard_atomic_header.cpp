#include <atomic>

int main(void) {
  std::atomic_int value(4);
  if (value.load() != 4) {
    return 1;
  }
  value.store(7);
  if (value.load() != 7) {
    return 2;
  }
  if (value.fetch_add(3) != 7) {
    return 3;
  }
  if (++value != 11) {
    return 4;
  }
  if (--value != 10) {
    return 5;
  }
  if (value.fetch_sub(1) != 10) {
    return 6;
  }

  int expected = 5;
  if (value.compare_exchange_strong(expected, 12)) {
    return 7;
  }
  if (expected != 9 || value.load() != 9) {
    return 8;
  }
  if (!value.compare_exchange_weak(expected, 13)) {
    return 9;
  }
  if (value.load() != 13) {
    return 10;
  }

  struct adjacent_bools {
    std::atomic_bool first;
    std::atomic_bool second;
  } flags{false, true};
  if (flags.first.load() || !flags.second.load()) {
    return 11;
  }
  flags.first.store(true);
  if (!flags.first.load() || !flags.second.load()) {
    return 12;
  }
  flags.second.store(false);
  if (!flags.first.load() || flags.second.load()) {
    return 13;
  }

  std::atomic_thread_fence(std::memory_order_seq_cst);
  return 0;
}
