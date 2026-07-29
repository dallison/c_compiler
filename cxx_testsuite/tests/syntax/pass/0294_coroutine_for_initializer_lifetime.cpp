// RUN: -std=c++23

#include <coroutine>

struct task {
  struct promise_type {
    task get_return_object();
    std::suspend_never initial_suspend();
    std::suspend_never final_suspend() noexcept;
    std::suspend_always yield_value(int);
    void return_void();
    void unhandled_exception();
  };
};

struct state {
  int value;
  ~state();
};

task valid() {
  for (state current{0}; current.value != 2; ++current.value) {
    co_yield current.value;
  }
}
