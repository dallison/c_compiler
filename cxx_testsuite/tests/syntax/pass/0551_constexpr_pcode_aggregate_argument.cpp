// RUN: -std=c++20 -fconstexpr-eval=pcode

struct Pair {
  int x;
  int y;
};

constexpr int sum(Pair p) {
  return p.x + p.y;
}

static_assert(sum(Pair{20, 22}) == 42);
