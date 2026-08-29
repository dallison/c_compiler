// RUN: -std=c++20 -O2
// EXPECT_EXIT: 0
// Constant evaluation lowers the callee to p-code and interprets it, but the
// code generator takes its pass selection from the compiler rather than from
// the target it is handed.  Pointing the target at p-code therefore left the
// real target's loop passes running over p-code, whose lowering depends on the
// loop shape those passes rewrite, and the fold produced a wrong constant.

constexpr int sum_to(int limit) {
  int sum = 0;
  for (int i = 0; i <= limit; ++i) {
    sum += i;
  }
  return sum;
}

constexpr int count_down(int value) {
  int count = 0;
  while (value > 0) {
    --value;
    ++count;
  }
  return count;
}

constexpr int nested(int outer) {
  int total = 0;
  for (int i = 0; i < outer; ++i) {
    for (int j = 0; j <= i; ++j) {
      total += j;
    }
  }
  return total;
}

constexpr int folded_sum = sum_to(5);
constexpr int folded_count = count_down(4);
constexpr int folded_nested = nested(4);

static_assert(folded_sum == 15);
static_assert(folded_count == 4);
static_assert(folded_nested == 10);

int main(void) {
  if (folded_sum != 15) {
    return 1;
  }
  if (folded_count != 4) {
    return 2;
  }
  if (folded_nested != 10) {
    return 3;
  }
  return 0;
}
