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

  std::atomic_uint_least8_t small(3);
  std::atomic_size_t size_value(4);
  if (small.load() != 3 || size_value.load() != 4) {
    return 14;
  }

  std::atomic_int free_value;
  std::atomic_init(&free_value, 5);
  if (std::atomic_fetch_or(&free_value, 2) != 5 ||
      std::atomic_fetch_and_explicit(
          &free_value, 6, std::memory_order_relaxed) != 7 ||
      std::atomic_fetch_xor(&free_value, 3) != 6 ||
      std::atomic_exchange(&free_value, 9) != 5) {
    return 15;
  }
  int free_expected = 9;
  if (!std::atomic_compare_exchange_strong(
          &free_value, &free_expected, 10) ||
      std::atomic_load(&free_value) != 10) {
    return 16;
  }

  std::atomic_flag flag = ATOMIC_FLAG_INIT;
  if (std::atomic_flag_test_and_set(&flag) ||
      !std::atomic_flag_test_and_set_explicit(
          &flag, std::memory_order_relaxed)) {
    return 17;
  }
  std::atomic_flag_clear(&flag);
  if (std::atomic_flag_test_and_set(&flag)) {
    return 18;
  }

  volatile std::atomic_int volatile_value(1);
  if (volatile_value.fetch_add(2) != 1 ||
      std::atomic_load(&volatile_value) != 3) {
    return 19;
  }

  std::atomic_signal_fence(std::memory_order_seq_cst);
  std::atomic_thread_fence(std::memory_order_seq_cst);
  return 0;
}
