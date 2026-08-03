// RUN: -std=c++20 -fconstexpr-eval=pcode
// EXPECT: static_assert expression is not an integer constant expression

constexpr int reached_trap() {
  __builtin_trap();
  return 1;
}

static_assert(reached_trap() == 1);
