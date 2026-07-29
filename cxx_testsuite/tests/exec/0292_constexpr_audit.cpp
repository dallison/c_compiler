// RUN: -std=c++20
// EXPECT_EXIT: 0

struct Aggregate {
  int first;
  int second;
};

constexpr Aggregate make_aggregate(int value) {
  return {value, value + 1};
}

static Aggregate static_aggregate = make_aggregate(20);

constexpr int recursive_depth(int value) {
  try {
    return value == 0 ? 0 : recursive_depth(value - 1) + 1;
  } catch (...) {
    return -1;
  }
}

constexpr int nonthrowing_try() {
  try {
    return 17;
  } catch (...) {
    return -1;
  }
}

constexpr bool pointer_ordering() {
  int values[2] = {1, 2};
  return &values[0] < &values[1] && values + 1 > values;
}

constexpr int immediate_lambda_value =
    [](int value) consteval { return value + 1; }(41);

static_assert(recursive_depth(64) == 64);
static_assert(nonthrowing_try() == 17);
static_assert(pointer_ordering());
static_assert(immediate_lambda_value == 42);

int main() {
  if (static_aggregate.first != 20 || static_aggregate.second != 21) return 1;
  return 0;
}
