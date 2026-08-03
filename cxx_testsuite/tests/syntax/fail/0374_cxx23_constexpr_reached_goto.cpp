// RUN: -std=c++23 -fconstexpr-eval=pcode
// EXPECT: static_assert expression is not an integer constant expression

constexpr int reached_goto() {
  goto rejected;
rejected:
  return 1;
}

static_assert(reached_goto() == 1);
