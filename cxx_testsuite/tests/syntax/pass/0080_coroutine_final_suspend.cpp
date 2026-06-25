// RUN: -std=c++20

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

struct FinalSuspendAlways {
  bool await_ready(void) {
    return false;
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
  SuspendNever initial_suspend(void) {
    SuspendNever awaiter = {};
    return awaiter;
  }
  FinalSuspendAlways final_suspend(void) {
    FinalSuspendAlways awaiter = {};
    return awaiter;
  }
  void return_value(int result) {
    value = result;
  }
  void unhandled_exception(void) {
  }
};

Task finally_suspended_coroutine(void) {
  co_return 3;
}
