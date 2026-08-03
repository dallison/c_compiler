// RUN: -std=c++11 -fconstexpr-eval=pcode

constexpr int cxx11_sum(int value) {
  return value == 0 ? 0 : value + cxx11_sum(value - 1);
}

static_assert(cxx11_sum(9) == 45, "C++11 strict pcode constexpr");
