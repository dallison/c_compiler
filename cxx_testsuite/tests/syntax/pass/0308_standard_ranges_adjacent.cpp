// RUN: -std=c++23

#include <ranges>
#include <tuple>
#include <type_traits>

void check_adjacent_views() {
  int values[] = {1, 2, 3, 4};

  auto pairs = std::views::adjacent<2>(values);
  static_assert(std::ranges::view<decltype(pairs)>);
  static_assert(std::ranges::forward_range<decltype(pairs)>);
  static_assert(std::ranges::sized_range<decltype(pairs)>);
  static_assert(std::tuple_size_v<
                    std::ranges::range_reference_t<decltype(pairs)>> == 2);
  static_assert(std::same_as<
                std::tuple_element_t<
                    0, std::ranges::range_reference_t<decltype(pairs)>>,
                int&>);
  static_assert(std::same_as<
                std::tuple_element_t<
                    1, std::ranges::range_reference_t<decltype(pairs)>>,
                int&>);

  auto triples = values | std::views::adjacent<3>;
  static_assert(std::ranges::view<decltype(triples)>);
  static_assert(std::tuple_size_v<
                    std::ranges::range_reference_t<decltype(triples)>> == 3);

  auto singles = std::views::adjacent<1>(values);
  static_assert(std::tuple_size_v<
                    std::ranges::range_reference_t<decltype(singles)>> == 1);

  auto combine = [](int left, int right) -> long {
    return left + right;
  };
  static_assert(std::regular_invocable<decltype(combine)&, int&, int&>);
  auto transformed =
      std::views::adjacent_transform<2>(values, combine);
  static_assert(std::ranges::view<decltype(transformed)>);
  static_assert(std::ranges::forward_range<decltype(transformed)>);
  static_assert(std::same_as<
                std::ranges::range_value_t<decltype(transformed)>, long>);

  auto piped =
      values | std::views::adjacent_transform<2>(combine);
  static_assert(std::ranges::view<decltype(piped)>);

  auto counted = std::views::counted(values, 4);
  static_assert(std::ranges::forward_range<decltype(counted)>);
  static_assert(!std::ranges::common_range<decltype(counted)>);
  auto counted_pairs = counted | std::views::adjacent<2>;
  static_assert(std::ranges::forward_range<decltype(counted_pairs)>);

  auto pairwise = std::views::pairwise(values);
  static_assert(std::ranges::view<decltype(pairwise)>);
  auto pairwise_transformed =
      values | std::views::pairwise_transform(combine);
  static_assert(std::ranges::view<decltype(pairwise_transformed)>);
}
