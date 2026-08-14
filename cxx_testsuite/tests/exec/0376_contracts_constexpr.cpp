// RUN: -std=c++26
// EXPECT_EXIT: 0

constexpr int increment(const int value)
    pre (value >= 0)
    post (result: result == value + 1) {
  contract_assert (value < 10);
  return value + 1;
}

static_assert(increment(4) == 5);

consteval int immediate(const int value)
    pre (value > 0) {
  return value * 2;
}

static_assert(immediate(3) == 6);

int main() {
  return increment(8) == 9 ? 0 : 1;
}
