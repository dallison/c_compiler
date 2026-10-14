// RUN: -std=c++17
// EXPECT_EXIT: 0

// C++ allows any simple-declaration as a for init-statement, including a
// static local.  Abseil's ABSL_LOG_EVERY_POW_2 keeps its per-site state in
// `for (static State s; ...)`, nested inside one-shot `for` loops.

#include <stdint.h>

struct State {
  int n = 0;
  bool ShouldLog() {
    ++n;
    return (n & (n - 1)) == 0;
  }
  uint32_t counter() const { return n; }
};

int LogEveryPow2(int iterations) {
  int total = 0;
  for (int i = 0; i < iterations; i++) {
    for (bool go(true); go; go = false)
      for (static State st; go && st.ShouldLog(); go = false)
        for (const uint32_t C __attribute__((__unused__)) = st.counter(); go;
             go = false)
          total += C;
  }
  return total;
}

int main() {
  if (LogEveryPow2(5) != 1 + 2 + 4) return 1;
  // The static persists across calls: counts 6..9 log only at 8.
  if (LogEveryPow2(4) != 8) return 2;
  return 0;
}
