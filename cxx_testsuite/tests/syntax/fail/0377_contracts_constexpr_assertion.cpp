// RUN: -std=c++26
// EXPECT: static_assert expression is not an integer constant expression

constexpr int checked(const int value) {
  contract_assert (value != 0);
  return value;
}

static_assert(checked(0) == 0);
