// RUN: -std=c++20

#include <coroutine>

struct Promise;

struct Task {
  using promise_type = Promise;
  std::coroutine_handle<Promise> handle;
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
  }
};

Task make_value() {
  co_await std::suspend_always{};
  co_return 42;
}

int main() {
  Task task = make_value();
  if (!task.handle) {
    return 1;
  }
  if (task.handle.done()) {
    return 4;
  }
  std::coroutine_handle<> handle =
      std::coroutine_handle<>::from_address(task.handle.address());

  handle.resume();
  if (handle.done()) {
    return 2;
  }

  handle();
  if (!handle.done() || task.handle.promise().value != 42) {
    return 3;
  }

  handle.destroy();
  return 0;
}
