// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <algorithm>
#include <ranges>
#include <type_traits>

struct ObservedIterator {
  using difference_type = long;
  using value_type = int;
  using pointer = int*;
  using reference = int&;
  using iterator_category = std::input_iterator_tag;

  int* p;
  int* increments;

  int& operator*() const { return *p; }
  ObservedIterator& operator++() {
    ++p;
    ++*increments;
    return *this;
  }
  void operator++(int) { ++*this; }
  bool operator==(const ObservedIterator& other) const {
    return p == other.p;
  }
};

int main() {
  int values[] = {1, 2, 3, 4, 5};
  auto counted = std::views::counted(values + 1, 3L);

  using CountedRange = decltype(counted);
  static_assert(std::ranges::sized_range<CountedRange>);
  static_assert(!std::ranges::common_range<CountedRange>);
  static_assert(std::is_same_v<
                std::ranges::iterator_t<CountedRange>,
                std::counted_iterator<int*>>);
  static_assert(std::is_same_v<
                std::ranges::sentinel_t<CountedRange>,
                std::default_sentinel_t>);

  if (counted.size() != 3) {
    return 1;
  }
  if (std::ranges::distance(counted) != 3) {
    return 7;
  }

  int sum = 0;
  for (int value : counted) {
    sum += value;
  }
  if (sum != 9) {
    return 2;
  }

  if (std::ranges::count(counted, 3) != 1 ||
      std::ranges::count(counted, 9) != 0) {
    return 3;
  }

  auto empty = std::views::counted(values, 0L);
  if (!empty.empty() || empty.size() != 0 ||
      empty.begin() != std::default_sentinel) {
    return 4;
  }

  int increments = 0;
  ObservedIterator observed{values, &increments};
  auto observed_range = std::views::counted(observed, 3L);
  if (increments != 0 || observed_range.size() != 3) {
    return 5;
  }

  sum = 0;
  for (int value : observed_range) {
    sum += value;
  }
  if (sum != 6 || increments != 3) {
    return 6;
  }

  return 0;
}
