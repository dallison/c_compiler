// RUN: -std=c++26
// EXPECT_EXIT: 0

#include <execution>

int main() {
  auto result = std::this_thread::sync_wait(std::execution::just(42));
  if (!result) {
    return 1;
  }
  if (result->__0 != 42) {
    return 2;
  }
  return 0;
}
