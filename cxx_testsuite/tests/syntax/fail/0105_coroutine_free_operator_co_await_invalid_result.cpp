// RUN: -std=c++20
// EXPECT: co_await operand must be an awaiter object

struct SuspendNever {
  bool await_ready(void) {
    return true;
  }
  void await_suspend(void* handle) {
    (void)handle;
  }
  void await_resume(void) {
  }
};

struct BadFreeAwaitable {
  int value;
};

int operator co_await(BadFreeAwaitable& awaitable) {
  return awaitable.value;
}

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
  SuspendNever initial_suspend(void) {
    SuspendNever awaiter = {};
    return awaiter;
  }
  SuspendNever final_suspend(void) {
    SuspendNever awaiter = {};
    return awaiter;
  }
  void return_value(int result) {
    value = result;
  }
  void unhandled_exception(void) {
  }
};

Task rejected_free_operator_co_await_result(void) {
  BadFreeAwaitable awaitable = {1};
  int value = co_await awaitable;
  co_return value;
}
