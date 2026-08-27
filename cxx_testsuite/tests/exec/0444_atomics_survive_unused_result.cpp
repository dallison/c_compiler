// RUN: -std=c++20 -O2
// EXPECT_EXIT: 0

// An atomic operation updates memory whether or not anything reads the value it
// returns, but the target-instruction dead-code pass decides what to keep from
// the register result alone.  An atomic store has no register result at all, and
// a read-modify-write has one that a statement-expression discards, so both were
// deleted at -O2 while the memory they were supposed to update kept its old
// value.
//
// ARM lost the store: `atomic_store` and `atomic_fence` were not listed among
// the opcodes that are not expressions, so `value.store(...)` compiled to a
// prologue and an epilogue with nothing in between.  ARM, x86-64 and RISC-V all
// lost the read-modify-writes, which cannot be handled that way because they do
// produce a value; they need the pass to ask separately whether an instruction
// has an effect beyond its result.  AArch64 already asked.
//
// x86-64 also carries `atomic_thread_fence` on a nop, which the same pass
// deleted.  There is no check for it below because a missing fence is not
// observable under a single-threaded interpreter.

#include <atomic>

static unsigned int add_target = 3u;
static unsigned int sub_target = 20u;
static unsigned int add_fetch_target = 100u;
static unsigned int cas_target = 3u;

int main() {
  // A store has no register result.  Keep the value inside 32 bits: unsigned
  // long is 32 bits on ARM.
  std::atomic<unsigned long> stored{0};
  stored.store(0x12abcdefUL);
  if (stored.load() != 0x12abcdefUL) {
    return 1;
  }

  // A read-modify-write whose returned value is discarded.
  __atomic_fetch_add(&add_target, 7u, __ATOMIC_SEQ_CST);
  if (add_target != 10u) {
    return 2;
  }

  __atomic_fetch_sub(&sub_target, 5u, __ATOMIC_SEQ_CST);
  if (sub_target != 15u) {
    return 3;
  }

  __atomic_add_fetch(&add_fetch_target, 11u, __ATOMIC_SEQ_CST);
  if (add_fetch_target != 111u) {
    return 4;
  }

  unsigned int expected = 3u;
  __atomic_compare_exchange_n(&cas_target, &expected, 9u, false,
                              __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  if (cas_target != 9u) {
    return 5;
  }

  // The same shapes through <atomic>, where the discarded result comes from an
  // inlined member function rather than from the builtin directly.
  std::atomic<unsigned int> counter{4u};
  counter.fetch_add(6u);
  if (counter.load() != 10u) {
    return 6;
  }

  counter.fetch_sub(3u);
  if (counter.load() != 7u) {
    return 7;
  }

  unsigned int seen = 7u;
  counter.compare_exchange_strong(seen, 21u);
  if (counter.load() != 21u) {
    return 8;
  }

  return 0;
}
