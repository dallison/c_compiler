// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <iterator>

struct InputIterator {
  using difference_type = long;
  using value_type = int;
  using pointer = int*;
  using reference = int&;
  using iterator_category = std::input_iterator_tag;

  int* p;

  int& operator*() const { return *p; }
  InputIterator& operator++() {
    ++p;
    return *this;
  }
  void operator++(int) { ++p; }
  bool operator==(const InputIterator&) const = default;
};

int main() {
  int values[] = {2, 4, 6, 8, 10};
  using Counted = std::counted_iterator<int*>;

  Counted empty;
  if (empty.count() != 0 || empty != std::default_sentinel ||
      std::default_sentinel != empty) {
    return 1;
  }

  Counted current(values, 4);
  if (current.base() != values || current.count() != 4 ||
      *current != 2 || current.operator->() != values) {
    return 2;
  }

  *current = 3;
  Counted old = current++;
  if (*old != 3 || old.count() != 4 ||
      *current != 4 || current.count() != 3) {
    return 3;
  }

  ++current;
  if (*current != 6 || current.count() != 2) {
    return 4;
  }
  --current;
  if (*current != 4 || current.count() != 3) {
    return 5;
  }

  current += 2;
  if (*current != 8 || current.count() != 1) {
    return 6;
  }
  current -= 1;
  if (*current != 6 || current.count() != 2 || current[1] != 8) {
    return 7;
  }

  Counted first(values, 4);
  Counted third = first + 2;
  Counted fourth = 3 + first;
  if (third.base() != values + 2 || third.count() != 2 ||
      fourth.base() != values + 3 || fourth.count() != 1 ||
      third - first != 2 || first - third != -2 ||
      !(first < third) || !(third > first) ||
      !(first <= third) || !(third >= first)) {
    return 8;
  }
  third = fourth - 1;
  if (third.base() != values + 2 || third.count() != 2) {
    return 9;
  }

  if (std::default_sentinel - first != 4 ||
      first - std::default_sentinel != -4 ||
      std::ranges::distance(first, std::default_sentinel) != 4) {
    return 10;
  }

  Counted advanced(values, 4);
  long remainder =
      std::ranges::advance(advanced, 6L, std::default_sentinel);
  if (remainder != 2 || advanced != std::default_sentinel ||
      advanced.base() != values + 4) {
    return 11;
  }

  Counted made = std::make_counted_iterator(values + 1, 3L);
  if (*made != 4 || made.count() != 3) {
    return 12;
  }

  std::counted_iterator<const int*> const_counted = made;
  std::counted_iterator<const int*> assigned;
  assigned = made;
  if (*const_counted != 4 || const_counted.count() != 3 ||
      assigned.base() != values + 1 || assigned.count() != 3) {
    return 13;
  }

  Counted left(values, 1);
  Counted right(values + 4, 1);
  std::ranges::iter_swap(left, right);
  if (values[0] != 10 || values[4] != 3) {
    return 14;
  }

  InputIterator input{values + 1};
  std::counted_iterator<InputIterator> input_counted(input, 3);
  int sum = 0;
  while (input_counted != std::default_sentinel) {
    sum += *input_counted;
    input_counted++;
  }
  if (sum != 18 || input_counted.count() != 0) {
    return 15;
  }

  return 0;
}
