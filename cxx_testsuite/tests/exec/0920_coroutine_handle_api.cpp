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
  auto again = std::noop_coroutine();
  if (!noop || !again || noop.done() || again.done()) {
    return 4;
  }
  if (noop.address() == empty.address() || noop.address() != again.address()) {
    return 5;
  }
  if (&noop.promise() != &again.promise()) {
    return 6;
  }
  noop.resume();
  noop.destroy();
  if (noop.done()) {
    return 7;
  }
  std::coroutine_handle<> erased = noop;
  erased.resume();
  erased.destroy();
  if (!erased || erased.done() || erased.address() != noop.address()) {
    return 8;
  }
  std::suspend_always always = {};
  std::suspend_never never = {};
  if (always.await_ready() || !never.await_ready()) {
    return 9;
  }
  always.await_suspend(erased);
  never.await_suspend(erased);
  always.await_resume();
  never.await_resume();
  return 0;
}
