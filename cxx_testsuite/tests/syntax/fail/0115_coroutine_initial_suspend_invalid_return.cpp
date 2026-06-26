// RUN: -std=c++20
// EXPECT: await_suspend must return void, bool, void*, or a coroutine handle

struct BadSuspendReturn {
  bool await_ready(void) {
    return false;
  }
  int await_suspend(void* handle) {
    (void)handle;
    return 1;
  }
  void await_resume(void) {
  }
};

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
  BadSuspendReturn initial_suspend(void) {
    BadSuspendReturn awaiter = {};
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

Task rejected_initial_suspend_return(void) {
  co_return 1;
}
