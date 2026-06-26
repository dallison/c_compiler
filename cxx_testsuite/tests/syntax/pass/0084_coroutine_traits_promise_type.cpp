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

struct TraitsPromise;
struct WrongPromise;

struct Task {
  using promise_type = WrongPromise;
  int value;
};

namespace std {
template <class R, class Arg>
struct coroutine_traits {
  using promise_type = TraitsPromise;
};
}

struct TraitsPromise {
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

Task coroutine_traits_promise_type(int input) {
  co_return input + 1;
}
