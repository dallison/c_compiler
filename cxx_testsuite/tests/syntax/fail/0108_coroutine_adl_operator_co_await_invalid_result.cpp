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

namespace adl_bad_coawait {
struct Awaitable {
  int value;
};

int operator co_await(Awaitable& awaitable) {
  return awaitable.value;
}
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

Task rejected_adl_operator_co_await_result(void) {
  adl_bad_coawait::Awaitable awaitable = {1};
  int value = co_await awaitable;
  co_return value;
}
