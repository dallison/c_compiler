// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <array>
#include <mdspan>
#include <span>
#include <type_traits>

int main() {
  using shape_type =
      std::extents<int, 2, std::dynamic_extent, 4,
                   std::dynamic_extent>;

  shape_type from_dynamic(3, 5);
  if (from_dynamic.extent(0) != 2 || from_dynamic.extent(1) != 3 ||
      from_dynamic.extent(2) != 4 || from_dynamic.extent(3) != 5) {
    return 1;
  }

  shape_type from_all(2, 3, 4, 5);
  if (from_all != from_dynamic) {
    return 2;
  }

  std::array<short, 2> array_values{6, 8};
  shape_type from_array(array_values);
  if (from_array.extent(1) != 6 || from_array.extent(3) != 8) {
    return 3;
  }

  short span_values[] = {7, 9};
  shape_type from_span{std::span<short, 2>(span_values)};
  if (from_span.extent(1) != 7 || from_span.extent(3) != 9) {
    return 4;
  }

  std::dextents<int, 3> dynamic_shape(2, 3, 4);
  std::extents<long, 2, 3, 4> static_shape(dynamic_shape);
  if (static_shape.extent(0) != 2 || static_shape.extent(1) != 3 ||
      static_shape.extent(2) != 4) {
    return 5;
  }

  std::extents deduced(3, 4);
  static_assert(
      std::is_same_v<decltype(deduced),
                     std::extents<std::size_t, std::dynamic_extent,
                                  std::dynamic_extent>>);
  if (deduced.extent(0) != 3 || deduced.extent(1) != 4) {
    return 6;
  }

  std::dextents<int, 2> zero_shape;
  if (zero_shape.extent(0) != 0 || zero_shape.extent(1) != 0) {
    return 7;
  }

  int data[] = {2, 3, 5, 7};
  std::default_accessor<int> accessor;
  std::default_accessor<const int> const_accessor(accessor);
  accessor.access(data, 1) = 11;
  if (data[1] != 11 || *accessor.offset(data, 2) != 5 ||
      const_accessor.access(data, 3) != 7) {
    return 8;
  }

  return 0;
}
