// RUN: -std=c++20
// EXPECT: coroutine frame-owned awaiter requires a copy or move constructor

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

struct NonCopyAwaiter {
  int value;
  NonCopyAwaiter();
  NonCopyAwaiter(const NonCopyAwaiter& other) = delete;
  bool await_ready(void) {
    return false;
  }
  void await_suspend(void* handle) {
    (void)handle;
  }
  int await_resume(void) {
    return value;
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

Task rejected_named_noncopy_awaiter(void) {
  NonCopyAwaiter awaiter;
  int value = co_await awaiter;
  co_return value;
}
