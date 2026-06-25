// RUN: -std=c++20
// EXPECT: Ambiguous overload for operator co_await

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

namespace adl_left {
struct Base {
  int value;
};
}

namespace adl_right {
struct Awaitable : adl_left::Base {
  int value;
};

SuspendNever operator co_await(Awaitable& awaitable);
}

namespace adl_left {
SuspendNever operator co_await(adl_right::Awaitable& awaitable);
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

Task rejected_adl_operator_co_await_ambiguous(void) {
  adl_right::Awaitable awaitable = {};
  int value = co_await awaitable;
  co_return value;
}
