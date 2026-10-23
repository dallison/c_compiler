// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <atomic>
#include <cstdint>

static std::atomic<std::uint64_t> value(0);

int main() {
#if defined(__riscv) && __riscv_xlen == 32
  if (value.is_lock_free()) {
    return 1;
  }
#else
  if (!value.is_lock_free()) {
    return 1;
  }
#endif
  value.store(0x100000001ull, std::memory_order_relaxed);
  if (value.load(std::memory_order_acquire) != 0x100000001ull) {
    return 2;
  }
  std::uint64_t previous =
      value.fetch_add(0x200000002ull, std::memory_order_acq_rel);
  if (previous != 0x100000001ull ||
      value.load(std::memory_order_relaxed) != 0x300000003ull) {
    return 3;
  }
  std::uint64_t expected = 0x300000003ull;
  if (!value.compare_exchange_strong(expected, 0x400000004ull,
                                    std::memory_order_seq_cst,
                                    std::memory_order_acquire)) {
    return 4;
  }
  expected = 0;
  if (value.compare_exchange_strong(expected, 9ull) ||
      expected != 0x400000004ull ||
      value.load(std::memory_order_relaxed) != 0x400000004ull) {
    return 5;
  }
  return 0;
}
