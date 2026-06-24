// RUN: -std=c++20

struct SuspendNever {
  bool await_ready(void) {
    return true;
  }
  void await_suspend(int handle) {
    (void)handle;
  }
  void await_resume(void) {
  }
};

struct ReadyInt {
  int value;
  bool await_ready(void) {
    return true;
  }
  void await_suspend(int handle) {
    (void)handle;
  }
  int await_resume(void) {
    return value;
  }
};

struct Task;
typedef struct Task Task;
struct Promise;

struct Task {
  typedef Promise promise_type;
  int value;
};

struct Promise {
  int value;
  Task get_return_object(void) {
    Task task = {value};
    return task;
  }

  SuspendNever initial_suspend(void) {
    SuspendNever value = {};
    return value;
  }

  SuspendNever final_suspend(void) {
    SuspendNever value = {};
    return value;
  }

  void return_value(int result) {
    value = result;
  }

  void return_void(void) {
    value = 0;
  }

  void unhandled_exception(void) {
  }
};

Task coroutine_value(void) {
  co_return 17;
}

Task coroutine_void_value(void) {
  co_return;
}

Task coroutine_await_value(void) {
  ReadyInt awaiter = {19};
  int value = co_await awaiter;
  co_return value + 4;
}

void use_coroutine_frontend(void) {
  Task value = coroutine_value();
  Task empty = coroutine_void_value();
  Task awaited = coroutine_await_value();
  (void)value;
  (void)empty;
  (void)awaited;
}
