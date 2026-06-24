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

struct SuspendOnce {
  bool await_ready(void) {
    return false;
  }
  void await_suspend(int handle) {
    (void)handle;
  }
  int await_resume(void) {
    return 58;
  }
};

struct Task;
typedef struct Task Task;
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
  co_return 23;
}

Task coroutine_void_value(void) {
  co_return;
}

Task coroutine_await_value(void) {
  ReadyInt awaiter = {37};
  int value = co_await awaiter;
  co_return value + 5;
}

Task coroutine_suspend_once(void) {
  SuspendOnce awaiter = {};
  int value = co_await awaiter;
  co_return value + 2;
}

int main(void) {
  Task value = coroutine_value();
  if (value.value != 23) {
    return 1;
  }

  Task empty = coroutine_void_value();
  if (empty.value != 0) {
    return 2;
  }

  Task awaited = coroutine_await_value();
  if (awaited.value != 42) {
    return 3;
  }

  Task started = coroutine_suspend_once();
  if (started.value != 0) {
    return 4;
  }

  Task resumed = coroutine_suspend_once();
  if (resumed.value != 60) {
    return 5;
  }

  return 0;
}
