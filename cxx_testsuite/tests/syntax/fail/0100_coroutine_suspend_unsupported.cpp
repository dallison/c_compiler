// RUN: -std=c++20
// EXPECT: coroutine suspension is not supported yet

struct Awaiter {
  bool await_ready(void) {
    return false;
  }
  void await_suspend(int handle) {
    (void)handle;
  }
  int await_resume(void) {
    return 3;
  }
};

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
  Awaiter initial_suspend(void) {
    Awaiter awaiter = {};
    return awaiter;
  }
  Awaiter final_suspend(void) {
    Awaiter awaiter = {};
    return awaiter;
  }
  void return_value(int result) {
    value = result;
  }
  void unhandled_exception(void) {
  }
};

Task suspended_coroutine(void) {
  Awaiter first = {};
  Awaiter second = {};
  int first_value = co_await first;
  int second_value = co_await second;
  co_return first_value + second_value;
}
