// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <atomic>

using Function = int (*)(int);

std::atomic<Function> function(nullptr);

static int add_one(int value) { return value + 1; }

int main() {
  function = add_one;
  if ((*function)(4) != 5) {
    return 1;
  }
  return 0;
}
