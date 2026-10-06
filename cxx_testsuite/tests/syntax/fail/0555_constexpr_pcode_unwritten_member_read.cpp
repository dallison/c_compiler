// RUN: -std=c++20 -fconstexpr-eval=pcode
// EXPECT: indeterminate value read in constexpr pcode

// Writing one member of an uninitialized local leaves the other indeterminate,
// and reading it is not a constant expression.
struct Pair {
  int a, b;
};

constexpr int read_unwritten_member() {
  Pair pair;
  pair.a = 1;
  return pair.b;
}

static_assert(read_unwritten_member() == 0);
