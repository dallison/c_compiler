// RUN: -std=c++14 -fconstexpr-eval=audit

constexpr int sum_to(int limit) {
  int result = 0;
  for (int value = 1; value <= limit; ++value) {
    result += value;
  }
  return result;
}

static_assert(sum_to(10) == 55);
