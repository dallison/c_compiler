// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <ranges>
#include <tuple>

int main() {
  int values[] = {1, 2, 3, 4};
  auto pairs = std::views::adjacent<2>(values);
  if (pairs.size() != 3) {
    return 1;
  }

  int count = 0;
  int total = 0;
  for (auto [left, right] : pairs) {
    total += left * 10 + right;
    ++count;
  }
  if (count != 3 || total != 69) {
    return 2;
  }

  for (auto pair : pairs) {
    std::get<0>(pair) += 10;
  }
  if (values[0] != 11 || values[1] != 12 ||
      values[2] != 13 || values[3] != 4) {
    return 3;
  }

  int triple_values[] = {1, 2, 3, 4, 5};
  auto triples = triple_values | std::views::adjacent<3>;
  count = 0;
  total = 0;
  for (auto triple : triples) {
    total += std::get<0>(triple) + std::get<1>(triple) +
             std::get<2>(triple);
    ++count;
  }
  if (triples.size() != 3 || count != 3 || total != 27) {
    return 4;
  }

  int one[] = {7};
  auto empty_pairs = std::views::adjacent<2>(one);
  if (empty_pairs.size() != 0 ||
      empty_pairs.begin() != empty_pairs.end()) {
    return 5;
  }

  int short_values[] = {1, 2, 3};
  auto empty_fours = std::views::adjacent<4>(short_values);
  if (empty_fours.size() != 0 ||
      empty_fours.begin() != empty_fours.end()) {
    return 6;
  }

  int products[] = {1, 2, 3, 4};
  auto multiplied = std::views::adjacent_transform<2>(
      products, [](int left, int right) { return left * right; });
  total = 0;
  for (int value : multiplied) {
    total += value;
  }
  if (multiplied.size() != 3 || total != 20) {
    return 7;
  }

  auto added = products | std::views::adjacent_transform<2>(
      [](int left, int right) { return left + right; });
  total = 0;
  for (int value : added) {
    total += value;
  }
  if (total != 15) {
    return 8;
  }

  auto singles = std::views::adjacent<1>(products);
  count = 0;
  total = 0;
  for (auto single : singles) {
    total += std::get<0>(single);
    ++count;
  }
  if (singles.size() != 4 || count != 4 || total != 10) {
    return 9;
  }

  auto counted = std::views::counted(products, 4);
  auto counted_pairs = counted | std::views::adjacent<2>;
  count = 0;
  total = 0;
  for (auto pair : counted_pairs) {
    total += std::get<0>(pair) + std::get<1>(pair);
    ++count;
  }
  if (count != 3 || total != 15) {
    return 10;
  }

  count = 0;
  for (auto pair : std::views::pairwise(products)) {
    if (std::get<1>(pair) != std::get<0>(pair) + 1) {
      return 11;
    }
    ++count;
  }
  if (count != 3) {
    return 12;
  }

  auto differences = products | std::views::pairwise_transform(
      [](int left, int right) { return right - left; });
  total = 0;
  for (int value : differences) {
    total += value;
  }
  if (total != 3) {
    return 13;
  }

  return 0;
}
