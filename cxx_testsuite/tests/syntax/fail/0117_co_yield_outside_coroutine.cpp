// RUN: -std=c++20
// EXPECT: coroutine return type must provide promise_type

int rejected_co_yield_outside_coroutine(void) {
  co_yield 1;
  return 2;
}
