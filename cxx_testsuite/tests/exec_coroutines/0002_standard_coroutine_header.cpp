// RUN: -std=c++20

#include <coroutine>

struct Promise;

struct Task {
  using promise_type = Promise;
  int value;
};

struct Promise {
  int value;

  Task get_return_object(void) {
    Task task = {value};
    return task;
  }

  std::suspend_never initial_suspend(void) {
    std::suspend_never awaiter = {};
    return awaiter;
  }

  std::suspend_never final_suspend(void) {
    std::suspend_never awaiter = {};
    return awaiter;
  }

  void return_value(int result) {
    value = result;
  }

  void unhandled_exception(void) {
  }
};

Task coroutine_header_smoke(int input) {
  co_return input + 4;
}

int main(void) {
  Task result = coroutine_header_smoke(38);
  if (result.value != 42) {
    return 1;
  }

  std::coroutine_traits<Task, int>::promise_type* promise = 0;
  (void)promise;

  std::coroutine_handle<Promise> handle =
      std::coroutine_handle<Promise>::from_address(0);
  if (handle) {
    return 2;
  }
  return 0;
}
