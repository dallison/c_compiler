// RUN: -std=c++23 -O2
// EXPECT_EXIT: 0

#include <coroutine>

struct Promise;

struct Task {
  using promise_type = Promise;
  std::coroutine_handle<Promise> handle;
};

volatile int live_ranges;
volatile int destroyed_ranges;
volatile int unhandled_count;

struct SuspendOnce {
  bool await_ready() const noexcept {
    return false;
  }

  void await_suspend(std::coroutine_handle<>) const noexcept {
  }

  void await_resume() const noexcept {
  }
};

struct Promise {
  int value;

  Task get_return_object() {
    return {std::coroutine_handle<Promise>::from_promise(*this)};
  }

  std::suspend_always initial_suspend() noexcept {
    return {};
  }

  std::suspend_always final_suspend() noexcept {
    return {};
  }

  void return_value(int result) noexcept {
    value = result;
  }

  void unhandled_exception() noexcept {
    unhandled_count++;
    value = -100;
  }
};

struct tracked_range {
  int values[2];

  tracked_range() {
    values[0] = 20;
    values[1] = 22;
    live_ranges++;
  }

  ~tracked_range() {
    live_ranges--;
    destroyed_ranges++;
  }

  tracked_range& view() {
    return *this;
  }

  int* begin() {
    return values;
  }

  int* end() {
    return values + 2;
  }
};

tracked_range make_range() {
  return tracked_range();
}

Task complete_range() {
  int sum = 0;
  for (int value : make_range().view()) {
    co_await SuspendOnce{};
    if (live_ranges != 1) {
      co_return -1;
    }
    sum += value;
  }
  co_return live_ranges == 0 && destroyed_ranges == 1 ? sum : -2;
}

Task return_from_range() {
  for (int value : make_range().view()) {
    co_await SuspendOnce{};
    co_return value + live_ranges;
  }
  co_return -1;
}

Task abandon_range() {
  for (int value : make_range().view()) {
    (void)value;
    co_await SuspendOnce{};
  }
  co_return -1;
}

#if defined(__cpp_exceptions)
Task throw_from_range() {
  for (int value : make_range().view()) {
    co_await SuspendOnce{};
    throw value;
  }
  co_return -1;
}
#endif

int main() {
  Task completed = complete_range();
  completed.handle.resume();
  if (live_ranges != 1 || destroyed_ranges != 0) {
    return 1;
  }
  completed.handle.resume();
  completed.handle.resume();
  if (!completed.handle.done() || completed.handle.promise().value != 42 ||
      live_ranges != 0 || destroyed_ranges != 1) {
    return 2;
  }
  completed.handle.destroy();
  if (destroyed_ranges != 1) {
    return 3;
  }

  Task returned = return_from_range();
  returned.handle.resume();
  returned.handle.resume();
  if (!returned.handle.done() || returned.handle.promise().value != 21 ||
      live_ranges != 0 || destroyed_ranges != 2) {
    return 4;
  }
  returned.handle.destroy();
  if (destroyed_ranges != 2) {
    return 5;
  }

  Task abandoned = abandon_range();
  abandoned.handle.resume();
  if (live_ranges != 1 || destroyed_ranges != 2) {
    return 6;
  }
  abandoned.handle.destroy();
  if (live_ranges != 0 || destroyed_ranges != 3) {
    return 7;
  }

#if defined(__cpp_exceptions)
  Task thrown = throw_from_range();
  thrown.handle.resume();
  thrown.handle.resume();
  if (unhandled_count != 1) {
    return 8;
  }
  if (live_ranges != 0 || destroyed_ranges != 4) {
    return 9;
  }
#endif
  return 0;
}
