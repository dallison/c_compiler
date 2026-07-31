// RUN: -std=c++20
#include <ranges>

template <class Range>
int sum(Range&& range) {
  int result = 0;
  for (auto&& value : range) result += static_cast<int>(value);
  return result;
}

int main() {
  int values[] = {1, 2, 3, 4, 5, 6};
  if (sum(std::views::all(values)) != 21) return 1;
  if (sum(std::views::empty<int>) != 0) return 2;
  if (sum(std::views::single(7)) != 7) return 3;
  if (sum(std::views::iota(1, 5)) != 10) return 4;
  if (sum(std::views::iota(3) | std::views::take(4)) != 18) return 5;
  auto view =
      values | std::views::transform([](int value) { return value * 2; });
  auto first = view.begin();
  auto last = view.end();
  if (first == last) return 11;
  if (*first != 2) return 12;
  ++first;
  if (first == last) return 13;
  if (*first != 4) return 14;
  int total = 0;
  for (int value : view) total += value;
  if (total != 42) return 15;
  auto second =
      values | std::views::transform([](int value) { return value * 2; });
  if (sum(second) != 42) return 16;
  if (sum(values |
          std::views::transform([](int value) { return value * 2; })) != 42) {
    return 17;
  }
  return 0;
}
