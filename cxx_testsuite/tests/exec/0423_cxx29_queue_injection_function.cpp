// RUN: -std=c++29
// EXPECT_EXIT: 0

#include <meta>

int main() {
  consteval {
    std::meta::queue_injection(
        ^{ constexpr int local_answer = \(29); });
  }
  return local_answer != 29;
}
