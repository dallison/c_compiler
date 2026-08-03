// RUN: -std=c++20 -fconstexpr-eval=pcode

constexpr int first_argument(int value, ...) {
  return value;
}

static_assert(first_argument(42, 1, 2, 3) == 42);
