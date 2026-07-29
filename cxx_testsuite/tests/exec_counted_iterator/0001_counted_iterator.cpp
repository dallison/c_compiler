// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <ranges>

int main() {
  int values[] = {2, 4, 6, 8};
  std::counted_iterator<int*> current(values, 3);

  if (*current != 2) {
    return 1;
  }
  if (current.count() != 3) {
    return 7;
  }
  if (std::ranges::distance(current, std::default_sentinel) != 3) {
    return 6;
  }

  ++current;
  if (*current != 4 || current.count() != 2) {
    return 2;
  }

  current += 2;
  if (current != std::default_sentinel || current.count() != 0) {
    return 3;
  }

  auto range = std::views::counted(values + 1, 2);
  if (range.size() != 2 || std::ranges::distance(range) != 2) {
    return 4;
  }

  int sum = 0;
  for (int value : range) {
    sum += value;
  }
  return sum == 10 ? 0 : 5;
}
