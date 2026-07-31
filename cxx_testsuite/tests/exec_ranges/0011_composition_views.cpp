// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <ranges>
#include <tuple>

int main() {
  int repeated_sum = 0;
  for (int value : std::views::repeat(17, 4)) {
    repeated_sum += value;
  }
  if (repeated_sum != 68) return 1;

  auto bounded = std::views::repeat(5, 4);
  if (bounded.size() != 4 || bounded.begin()[2] != 5) return 2;
  if (bounded.end() - bounded.begin() != 4) return 3;

  auto empty_repeat = std::views::repeat(9, 0);
  if (empty_repeat.begin() != empty_repeat.end()) return 4;

  repeated_sum = 0;
  for (int value : std::views::repeat(3) | std::views::take(5)) {
    repeated_sum += value;
  }
  if (repeated_sum != 15) return 5;

  int first[] = {1, 2};
  int second[] = {10, 20, 30};
  auto product = std::views::cartesian_product(first, second);
  if (product.size() != 6) return 6;

  int expected_first[] = {1, 1, 1, 2, 2, 2};
  int expected_second[] = {10, 20, 30, 10, 20, 30};
  int index = 0;
  for (auto values : product) {
    if (std::get<0>(values) != expected_first[index]) return 7;
    if (std::get<1>(values) != expected_second[index]) return 8;
    ++index;
  }
  if (index != 6) return 9;

  char flags[] = {'a', 'b'};
  int triples = 0;
  for (auto values :
       std::views::cartesian_product(first, second, flags)) {
    if (std::get<2>(values) != flags[triples % 2]) return 10;
    ++triples;
  }
  if (triples != 12) return 11;

  auto empty_second = std::ranges::subrange(second, second);
  auto empty_product =
      std::views::cartesian_product(first, empty_second);
  if (empty_product.begin() != empty_product.end()) return 12;
  if (empty_product.size() != 0) return 13;

  auto counted_first = std::views::counted(first, 2);
  auto counted_second = std::views::counted(second, 2);
  int counted_pairs = 0;
  for (auto values :
       std::views::cartesian_product(counted_first, counted_second)) {
    (void)values;
    ++counted_pairs;
  }
  if (counted_pairs != 4) return 14;

  int nothing = 0;
  for (auto values : std::views::cartesian_product()) {
    (void)values;
    ++nothing;
  }
  if (nothing != 0) return 15;

  int values[] = {1, 2, 3, 4, 5};
  using piece = std::ranges::subrange<int*, int*>;
  piece pieces[] = {
      piece(values, values + 2),
      piece(values + 2, values + 3),
      piece(values + 3, values + 5),
  };

  int expected_joined[] = {1, 2, 9, 3, 9, 4, 5};
  index = 0;
  for (int value : pieces | std::views::join_with(9)) {
    if (value != expected_joined[index]) return 16;
    ++index;
  }
  if (index != 7) return 17;

  int delimiter[] = {0, -1};
  int expected_range_joined[] = {1, 2, 0, -1, 3, 0, -1, 4, 5};
  index = 0;
  for (int value : std::views::join_with(pieces, delimiter)) {
    if (value != expected_range_joined[index]) return 18;
    ++index;
  }
  if (index != 9) return 19;

  piece empty_pieces[] = {
      piece(values, values),
      piece(values + 2, values + 3),
      piece(values + 5, values + 5),
  };
  int expected_empty_joined[] = {7, 3, 7};
  index = 0;
  for (int value : empty_pieces | std::views::join_with(7)) {
    if (value != expected_empty_joined[index]) return 20;
    ++index;
  }
  if (index != 3) return 21;

  for (int& value : pieces | std::views::join_with(delimiter)) {
    if (value > 0) {
      value += 10;
    }
  }
  if (values[0] != 11 || values[4] != 15) return 22;

  return 0;
}
