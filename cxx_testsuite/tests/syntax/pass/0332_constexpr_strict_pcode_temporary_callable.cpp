// RUN: -std=c++20 -fconstexpr-eval=pcode

struct positive_callable {
  consteval bool operator()(int value) const { return value > 0; }
};

static_assert(positive_callable{}(3));
