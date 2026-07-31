// RUN: -std=c++23

#include <algorithm>
#include <memory>
#include <optional>
#include <ranges>
#include <type_traits>
#include <vector>

struct record {
  int key;
};

void check_cxx23_range_algorithms() {
  int values[] = {1, 2, 3};
  int part[] = {2, 3};

  auto add = [](int left, int right) { return left + right; };
  static_assert(std::same_as<
                decltype(std::ranges::fold_left(values, 0, add)), int>);
  static_assert(std::same_as<
                decltype(std::ranges::fold_left_first(values, add)),
                std::optional<int>>);
  static_assert(std::same_as<
                decltype(std::ranges::fold_right(values, 0, add)), int>);
  static_assert(std::same_as<
                decltype(std::ranges::fold_right_last(values, add)),
                std::optional<int>>);

  auto with_iter =
      std::ranges::fold_left_with_iter(values, 0, add);
  (void)with_iter.in;
  (void)with_iter.value;

  auto first_with_iter =
      std::ranges::fold_left_first_with_iter(values, add);
  (void)first_with_iter.in;
  (void)first_with_iter.value;

  auto counted = std::views::counted(values, 3);
  auto counted_fold =
      std::ranges::fold_left_with_iter(counted, 0, add);
  (void)counted_fold;

  bool has_value = std::ranges::contains(values, 2);
  bool has_part = std::ranges::contains_subrange(values, part);
  bool begins = std::ranges::starts_with(values, part);
  bool ends = std::ranges::ends_with(values, part);
  (void)has_value;
  (void)has_part;
  (void)begins;
  (void)ends;

  record records[] = {{1}, {2}};
  bool projected =
      std::ranges::contains(records, 2, &record::key);
  (void)projected;

  auto converted = std::ranges::to<std::vector<int>>(values);
  auto piped = values | std::ranges::to<std::vector<int>>();
  auto deduced = std::ranges::to<std::vector>(values);
  auto allocated =
      std::ranges::to<std::vector<int>>(values, std::allocator<int>());
  auto allocated_pipe =
      values |
      std::ranges::to<std::vector<int>>(std::allocator<int>());
  static_assert(std::same_as<decltype(converted), std::vector<int>>);
  static_assert(std::same_as<decltype(piped), std::vector<int>>);
  static_assert(std::same_as<decltype(deduced), std::vector<int>>);
  static_assert(std::same_as<decltype(allocated), std::vector<int>>);
  static_assert(std::same_as<decltype(allocated_pipe), std::vector<int>>);

  int rows[][2] = {{1, 2}, {3, 4}};
  auto nested =
      std::ranges::to<std::vector<std::vector<int>>>(rows);
  static_assert(std::same_as<
                decltype(nested), std::vector<std::vector<int>>>);
}
