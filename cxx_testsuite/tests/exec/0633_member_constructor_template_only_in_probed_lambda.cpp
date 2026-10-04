// RUN: -std=c++23
// EXPECT_EXIT: 0

// `expected<long, int>(int)` is only named inside a lambda whose return type
// `and_then` deduces.  The constructor template specialization must still be
// emitted once the lambda body is generated.
#include <expected>

int main() {
  std::expected<int, int> value(3);
  auto chained = value.and_then(
      [](int& item) { return std::expected<long, int>(item * 2); });
  return chained && *chained == 6 ? 0 : 1;
}
