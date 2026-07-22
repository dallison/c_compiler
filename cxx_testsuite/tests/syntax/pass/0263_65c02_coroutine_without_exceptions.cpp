// RUN: -target 65c02 -std=c++20

#include <coroutine>

struct Promise;

struct Task {
  using promise_type = Promise;
};

struct Promise {
  Task get_return_object() {
    return {};
  }

  std::suspend_never initial_suspend() noexcept {
    return {};
  }

  std::suspend_never final_suspend() noexcept {
    return {};
  }

  void return_void() noexcept {
  }

  void unhandled_exception() noexcept {
  }
};

Task run_without_exceptions() {
  co_return;
}
