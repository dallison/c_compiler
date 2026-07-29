// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <ranges>

int main() {
  int values[] = {2, 4, 6, 8};
  auto counted = std::views::counted(values, 3L);
  auto common = counted | std::views::common;

  int sum = 0;
  for (auto iterator = common.begin(); iterator != common.end(); ++iterator) {
    sum += *iterator;
  }

  return sum == 12 && common.size() == 3 ? 0 : 1;
}
