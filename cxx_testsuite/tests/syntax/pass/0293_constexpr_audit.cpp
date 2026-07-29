// RUN: -std=c++20

struct Aggregate {
  int first;
  int second;
};

consteval Aggregate make_aggregate(int value) {
  return {value, value + 1};
}

consteval int sum_aggregate(Aggregate value) {
  return value.first + value.second;
}

static_assert(sum_aggregate(make_aggregate(20)) == 41);

constexpr int immediate_lambda_value =
    [](int value) consteval { return value + 2; }(40);
static_assert(immediate_lambda_value == 42);

constexpr int recursive_depth(int value) {
  try {
    return value == 0 ? 0 : recursive_depth(value - 1) + 1;
  } catch (...) {
    return -1;
  }
}

static_assert(recursive_depth(64) == 64);

constexpr int nonthrowing_try(bool select_first) {
  try {
    return select_first ? 7 : 9;
  } catch (...) {
    return -1;
  }
}

static_assert(nonthrowing_try(true) == 7);
static_assert(nonthrowing_try(false) == 9);

constexpr bool pointer_ordering() {
  int values[3] = {1, 2, 3};
  return &values[0] < &values[1] && &values[1] <= &values[1] &&
         &values[2] > &values[0] && &values[2] >= &values[2] &&
         values + 1 < values + 3;
}

static_assert(pointer_ordering());

union ActiveUnion {
  int integer;
  long other;

  constexpr ActiveUnion(long value) : other(value) {}
};

constexpr long read_active_union_member() {
  ActiveUnion value(17L);
  return value.other;
}

static_assert(read_active_union_member() == 17L);

constexpr Aggregate make_static_aggregate() {
  return {11, 12};
}

static Aggregate static_aggregate = make_static_aggregate();
