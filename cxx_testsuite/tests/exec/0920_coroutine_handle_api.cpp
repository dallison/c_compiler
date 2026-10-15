// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <coroutine>

int main() {
  std::coroutine_handle<> empty;
  std::coroutine_handle<> also;
  if (empty) {
    return 1;
  }
  if (!(empty == also)) {
    return 2;
  }
  auto noop = std::noop_coroutine();
  if (!noop) {
    return 4;
  }
  if (noop.address() == empty.address()) {
    return 5;
  }
  noop.resume();
  if (noop.done()) {
    return 6;
  }
  return 0;
}
