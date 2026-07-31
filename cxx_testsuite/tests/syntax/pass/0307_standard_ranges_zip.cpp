// RUN: -std=c++23

#include <ranges>
#include <tuple>
#include <type_traits>

void check_zip_views() {
  int values[] = {1, 2, 3};
  long weights[] = {4, 5};

  auto zipped = std::views::zip(values, weights);
  static_assert(std::ranges::view<decltype(zipped)>);
  static_assert(std::ranges::input_range<decltype(zipped)>);
  static_assert(std::ranges::sized_range<decltype(zipped)>);
  static_assert(std::tuple_size_v<
                    std::ranges::range_reference_t<decltype(zipped)>> == 2);
  static_assert(std::same_as<
                std::tuple_element_t<
                    1, std::ranges::range_reference_t<decltype(zipped)>>,
                long&>);

  auto combine = [](int value, long weight) -> long {
    return value + weight;
  };
  static_assert(std::is_invocable_v<decltype(combine)&, int&, long&>);
  static_assert(std::regular_invocable<decltype(combine)&, int&, long&>);
  auto transformed = std::views::zip_transform(combine, values, weights);
  static_assert(std::ranges::view<decltype(transformed)>);
  static_assert(std::same_as<
                std::ranges::range_value_t<decltype(transformed)>, long>);

  auto enumerated = std::views::enumerate(values);
  static_assert(std::ranges::view<decltype(enumerated)>);
  static_assert(std::ranges::sized_range<decltype(enumerated)>);
  static_assert(std::tuple_size_v<
                    std::ranges::range_reference_t<decltype(enumerated)>> == 2);
}
