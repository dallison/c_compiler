// RUN: -std=c++20
// EXPECT: coroutine function cannot be constexpr or consteval

struct Promise;

struct Task {
  using promise_type = Promise;
};

struct Promise {
  Task get_return_object();
  int initial_suspend();
  int final_suspend();
  void return_value(int);
  void unhandled_exception();
};

consteval Task immediate_coroutine() {
  co_return 1;
}
