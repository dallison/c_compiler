// RUN: -std=c++20 -fconstexpr-eval=pcode

constexpr int increment(int value) {
  return value + 1;
}

constexpr int invoke_indirect(int (*function)(int), int value) {
  return function(value);
}

static_assert(invoke_indirect(increment, 41) == 42);
