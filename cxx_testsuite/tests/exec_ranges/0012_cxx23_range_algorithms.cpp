// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <algorithm>
#include <ranges>
#include <vector>

struct record {
  int key;
  int value;
};

int main() {
  int values[] = {1, 2, 3, 4};
  auto add = [](int left, int right) { return left + right; };
  auto subtract = [](int left, int right) { return left - right; };

  if (std::ranges::fold_left(values, 0, subtract) != -10) return 1;
  if (std::ranges::fold_right(values, 0, subtract) != -2) return 2;

  auto first = std::ranges::fold_left_first(values, add);
  if (!first || *first != 10) return 3;

  auto last = std::ranges::fold_right_last(values, add);
  if (!last || *last != 10) return 4;

  auto with_iter =
      std::ranges::fold_left_with_iter(values, 0, add);
  if (with_iter.in != values + 4 || with_iter.value != 10) return 5;

  auto first_with_iter =
      std::ranges::fold_left_first_with_iter(values, add);
  if (first_with_iter.in != values + 4 ||
      !first_with_iter.value || *first_with_iter.value != 10) {
    return 6;
  }

  auto empty = std::ranges::subrange(values, values);
  if (std::ranges::fold_left(empty, 7, add) != 7) return 7;
  if (std::ranges::fold_left_first(empty, add)) return 8;
  if (std::ranges::fold_right_last(empty, add)) return 9;

  auto non_common = std::views::counted(values, 4);
  if (std::ranges::fold_left(non_common, 0, add) != 10) return 10;

  if (!std::ranges::contains(values, 3)) return 11;
  if (std::ranges::contains(values, 9)) return 12;

  record records[] = {{1, 10}, {2, 20}, {3, 30}};
  if (!std::ranges::contains(records, 2, &record::key)) return 13;

  int middle[] = {2, 3};
  int missing[] = {3, 5};
  if (!std::ranges::contains_subrange(values, middle)) return 14;
  if (std::ranges::contains_subrange(values, missing)) return 15;
  if (!std::ranges::contains_subrange(values, empty)) return 16;

  int prefix[] = {1, 2};
  int long_prefix[] = {1, 2, 3, 4, 5};
  if (!std::ranges::starts_with(values, prefix)) return 17;
  if (std::ranges::starts_with(values, middle)) return 18;
  if (std::ranges::starts_with(values, long_prefix)) return 19;
  if (!std::ranges::starts_with(values, empty)) return 20;

  int suffix[] = {3, 4};
  if (!std::ranges::ends_with(values, suffix)) return 21;
  if (std::ranges::ends_with(values, middle)) return 22;
  if (!std::ranges::ends_with(values, empty)) return 23;

  int projected_haystack[] = {11, 22, 33, 44};
  int projected_needle[] = {2, 3};
  auto last_digit = [](int value) { return value % 10; };
  if (!std::ranges::contains_subrange(
          projected_haystack, projected_needle,
          std::ranges::equal_to(), last_digit, std::identity())) {
    return 24;
  }

  auto copied = std::ranges::to<std::vector<int>>(values);
  if (copied.size() != 4 || copied[0] != 1 || copied[3] != 4) return 25;

  auto odd = [](int value) { return value % 2 != 0; };
  auto filtered =
      values | std::views::filter(odd) |
      std::ranges::to<std::vector<int>>();
  if (filtered.size() != 2 ||
      filtered[0] != 1 || filtered[1] != 3) {
    return 26;
  }

  int rows[][2] = {{5, 6}, {7, 8}};
  auto nested =
      std::ranges::to<std::vector<std::vector<int>>>(rows);
  if (nested.size() != 2 || nested[0].size() != 2 ||
      nested[0][1] != 6 || nested[1][0] != 7) {
    return 27;
  }

  return 0;
}
