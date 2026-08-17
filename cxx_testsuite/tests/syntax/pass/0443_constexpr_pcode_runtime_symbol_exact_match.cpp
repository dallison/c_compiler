// RUN: -std=c++20 -fconstexpr-eval=pcode

constexpr unsigned long malloc(unsigned long value) {
  return value + 1;
}

static_assert(malloc(41) == 42);
