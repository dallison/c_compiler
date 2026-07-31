// RUN: -std=c++23

#include <ranges>
#include <tuple>
#include <type_traits>

void check_composition_views() {
  auto bounded = std::views::repeat(17, 4);
  static_assert(std::ranges::view<decltype(bounded)>);
  static_assert(std::ranges::random_access_range<decltype(bounded)>);
  static_assert(std::ranges::sized_range<decltype(bounded)>);
  static_assert(std::ranges::common_range<decltype(bounded)>);
  static_assert(std::same_as<
                std::ranges::range_reference_t<decltype(bounded)>,
                const int&>);

  auto unbounded = std::views::repeat(3);
  static_assert(std::ranges::view<decltype(unbounded)>);
  static_assert(!std::ranges::common_range<decltype(unbounded)>);
  auto repeated = unbounded | std::views::take(5);
  static_assert(std::ranges::view<decltype(repeated)>);

  int left[] = {1, 2};
  long right[] = {10, 20, 30};
  auto product = std::views::cartesian_product(left, right);
  static_assert(std::ranges::view<decltype(product)>);
  static_assert(std::ranges::input_range<decltype(product)>);
  static_assert(std::ranges::sized_range<decltype(product)>);
  static_assert(std::tuple_size_v<
                    std::ranges::range_reference_t<decltype(product)>> == 2);
  static_assert(std::same_as<
                std::tuple_element_t<
                    0, std::ranges::range_reference_t<decltype(product)>>,
                int&>);
  static_assert(std::same_as<
                std::tuple_element_t<
                    1, std::ranges::range_reference_t<decltype(product)>>,
                long&>);

  char flags[] = {'a', 'b'};
  auto triple = std::views::cartesian_product(left, right, flags);
  static_assert(std::tuple_size_v<
                    std::ranges::range_reference_t<decltype(triple)>> == 3);

  auto empty_product = std::views::cartesian_product();
  static_assert(std::ranges::view<decltype(empty_product)>);

  auto counted = std::views::counted(right, 3);
  auto counted_product = std::views::cartesian_product(left, counted);
  static_assert(std::ranges::view<decltype(counted_product)>);

  int rows[][2] = {{1, 2}, {3, 4}, {5, 6}};
  auto joined = std::views::join_with(rows, 0);
  static_assert(std::ranges::view<decltype(joined)>);
  static_assert(std::ranges::input_range<decltype(joined)>);
  static_assert(std::same_as<
                std::ranges::range_value_t<decltype(joined)>, int>);

  int delimiter[] = {0, -1};
  auto joined_range = rows | std::views::join_with(delimiter);
  static_assert(std::ranges::view<decltype(joined_range)>);

  auto joined_value = rows | std::views::join_with(9);
  static_assert(std::ranges::view<decltype(joined_value)>);
}
