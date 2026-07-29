// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <ranges>

template <class Range>
int sum(Range&& range) {
  int result = 0;
  for (auto&& value : range) {
    result += static_cast<int>(value);
  }
  return result;
}

int main() {
  int values[] = {1, 2, 3, 4, 5, 6};

  if (sum(std::views::all(values)) != 21) return 1;
  if (sum(std::views::empty<int>) != 0) return 2;
  if (sum(std::views::single(7)) != 7) return 3;
  if (sum(std::views::iota(1, 5)) != 10) return 4;
  if (sum(std::views::iota(3) | std::views::take(4)) != 18) return 5;

  if (sum(values |
          std::views::transform([](int value) { return value * 2; })) != 42) {
    return 6;
  }
  if (sum(values | std::views::filter([](int value) {
            return value % 2 == 0;
          })) != 12) {
    return 7;
  }
  if (sum(values | std::views::take(3)) != 6) return 8;
  if (sum(values | std::views::take_while([](int value) {
            return value < 4;
          })) != 6) {
    return 9;
  }
  if (sum(values | std::views::drop(4)) != 11) return 10;
  if (sum(values | std::views::drop_while([](int value) {
            return value < 5;
          })) != 11) {
    return 11;
  }

  int reversed = 0;
  for (int value : values | std::views::reverse) {
    reversed = reversed * 10 + value;
  }
  if (reversed != 654321) return 12;

  auto common = values | std::views::take(2) | std::views::common;
  if (sum(common) != 3) return 13;
  if (sum(std::views::counted(values + 1, 3)) != 9) return 14;

  auto chained =
      values |
      std::views::filter([](int value) { return value % 2 == 1; }) |
      std::views::transform([](int value) { return value * 10; }) |
      std::views::take(2);
  if (sum(chained) != 40) return 15;

  return 0;
}
