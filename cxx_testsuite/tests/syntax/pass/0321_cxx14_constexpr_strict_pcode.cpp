// RUN: -std=c++14 -fconstexpr-eval=pcode

constexpr int cxx14_sum(int limit) {
  int result = 0;
  for (int i = 0; i <= limit; ++i) {
    result += i;
  }
  return result;
}

static_assert(cxx14_sum(9) == 45, "C++14 strict pcode constexpr");
