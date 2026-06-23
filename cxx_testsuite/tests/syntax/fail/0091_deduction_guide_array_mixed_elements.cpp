// RUN: -std=c++20
// EXPECT: Could not deduce template arguments for ArrayAggregate

template <typename T, int N>
struct ArrayAggregate {
  T values[N];
};

void fail_array_aggregate_mixed_elements() {
  ArrayAggregate mixed{{1, 'a'}};
}
