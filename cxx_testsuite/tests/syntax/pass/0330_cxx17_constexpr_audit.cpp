// RUN: -std=c++17 -fconstexpr-eval=audit

constexpr int lambda_result() {
  return [](int value) constexpr { return value * 3; }(14);
}

static_assert(lambda_result() == 42);
