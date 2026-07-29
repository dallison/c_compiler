// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <ranges>
#include <type_traits>

int main() {
  int values[] = {1, 2, 3, 4, 5};

  auto counted = std::views::counted(values + 1, 3L);
  auto common = counted | std::views::common;
  static_assert(std::ranges::common_range<decltype(common)>);
  static_assert(std::ranges::sized_range<decltype(common)>);
  static_assert(std::same_as<
                std::ranges::iterator_t<decltype(common)>,
                std::ranges::sentinel_t<decltype(common)>>);

  if (common.size() != 3) {
    return 1;
  }
  if (std::ranges::distance(common) != 3) {
    return 5;
  }

  int sum = 0;
  for (auto iterator = common.begin(); iterator != common.end(); ++iterator) {
    sum += *iterator;
  }
  if (sum != 9) {
    return 2;
  }

  auto taken = values | std::views::take(2) | std::views::common;
  sum = 0;
  for (auto iterator = taken.begin(); iterator != taken.end(); ++iterator) {
    sum += *iterator;
  }
  if (sum != 3) {
    return 3;
  }

  auto already_common = values | std::views::common;
  static_assert(std::ranges::common_range<decltype(already_common)>);
  if (already_common.begin() != values ||
      already_common.end() != values + 5) {
    return 4;
  }

  return 0;
}
