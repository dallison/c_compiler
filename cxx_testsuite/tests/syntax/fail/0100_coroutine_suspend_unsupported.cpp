// RUN: -std=c++20
// EXPECT: coroutine final_suspend suspension is not supported yet

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
  Awaiter yield_value(int result) {
    (void)result;
    Awaiter awaiter = {};
    return awaiter;
  }
  void unhandled_exception(void) {
  }
};

Task suspended_coroutine(void) {
  co_yield 3;
  co_return 0;
}
