// RUN: -std=c++11 -fconstexpr-eval=audit

constexpr int factorial(int value) {
  return value < 2 ? 1 : value * factorial(value - 1);
}

static_assert(factorial(6) == 720);
