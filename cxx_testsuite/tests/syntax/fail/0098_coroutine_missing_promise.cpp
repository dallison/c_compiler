// RUN: -std=c++20
// EXPECT: coroutine return type must provide promise_type

struct NotATask {
  int value;
};

NotATask missing_promise(void) {
  co_return 1;
}
