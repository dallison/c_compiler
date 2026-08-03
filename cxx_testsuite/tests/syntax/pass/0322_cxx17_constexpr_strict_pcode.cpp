// RUN: -std=c++17 -fconstexpr-eval=pcode

constexpr int cxx17_lambda_value() {
  auto combine = [](int left, int right) constexpr {
    return left * 10 + right;
  };
  return combine(4, 2);
}

static_assert(cxx17_lambda_value() == 42,
              "C++17 strict pcode constexpr lambda");
