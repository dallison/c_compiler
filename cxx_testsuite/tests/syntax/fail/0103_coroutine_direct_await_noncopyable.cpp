// RUN: -std=c++20
// EXPECT: coroutine frame-owned awaiter requires a copy constructor

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

NonCopyAwaiter make_noncopy_awaiter(void);

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

Task rejected_direct_noncopy_awaiter(void) {
  int value = co_await make_noncopy_awaiter();
  co_return value;
}
