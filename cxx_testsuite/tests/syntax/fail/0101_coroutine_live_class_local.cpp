// RUN: -std=c++20
// EXPECT: coroutine local live across suspension is not supported yet

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

struct Awaiter {
  int value;
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

struct Box {
  int value;
  Box(int input);
  Box(const Box& other) = delete;
};

Box::Box(int input) {
  value = input;
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

Task rejected_live_class_local(void) {
  Box box(7);
  Awaiter awaiter = {3};
  int value = co_await awaiter;
  co_return box.value + value;
}
