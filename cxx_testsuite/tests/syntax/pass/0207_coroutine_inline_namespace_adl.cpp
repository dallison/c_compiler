// RUN: -std=c++20
struct SuspendOnce {
  bool await_ready(void) const { return true; }
  void await_suspend(void*) const {}
  int await_resume(void) const { return 7; }
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
  SuspendOnce initial_suspend(void) { return SuspendOnce{}; }
  SuspendOnce final_suspend(void) { return SuspendOnce{}; }
  void return_value(int result) { value = result; }
  void unhandled_exception(void) {}
};

namespace coro_outer {
inline namespace coro_inline {
struct Awaitable {};
}
SuspendOnce operator co_await(Awaitable) {
  return SuspendOnce{};
}
}  // namespace coro_outer

Task coro_test(void) {
  coro_outer::Awaitable value{};
  co_return co_await value;
}

int main(void) {
  Task task = coro_test();
  (void)task;
  return 0;
}
