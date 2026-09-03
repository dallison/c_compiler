// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <algorithm>
#include <execution>
#include <type_traits>

static int doubled(int value) {
  return value * 2;
}

int main() {
  static_assert(std::is_execution_policy<
                    std::execution::sequenced_policy>::value);
  static_assert(std::is_execution_policy_v<
                    std::execution::parallel_policy>);

  int values[] = {4, 1, 3, 2};
  int output[4] = {};
  std::sort(std::execution::par, values, values + 4);
  std::transform(std::execution::seq, values, values + 4, output, doubled);
  if (values[0] != 1 || values[3] != 4 ||
      output[0] != 2 || output[3] != 8) return 1;
  return 0;
}
