// RUN: -std=c++26
// EXPECT: postcondition of a coroutine cannot odr-use a non-reference parameter

struct suspend_never {
  bool await_ready() { return true; }
  void await_suspend(int) {}
  void await_resume() {}
};

struct promise;

struct task {
  using promise_type = promise;
};

struct promise {
  task get_return_object() { return {}; }
  suspend_never initial_suspend() { return {}; }
  suspend_never final_suspend() { return {}; }
  void return_value(int) {}
  void unhandled_exception() {}
};

task invalid_coroutine(const int value)
    post (value >= 0) {
  co_return value;
}
