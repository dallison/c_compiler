// RUN: -std=c++20
// EXPECT: coroutine function cannot be constexpr or consteval

struct Promise;

struct Task {
  typedef Promise promise_type;
};

struct Promise {
  Task get_return_object(void);
  int initial_suspend(void);
  int final_suspend(void);
  void return_value(int value);
  void unhandled_exception(void);
};

constexpr Task constexpr_coroutine(void) {
  co_return 1;
}
