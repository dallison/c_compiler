// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <array>
#include <mdspan>
#include <span>

template <class ElementType>
struct shifted_accessor {
  using offset_policy = shifted_accessor;
  using element_type = ElementType;
  using reference = element_type&;
  using data_handle_type = element_type*;

  constexpr reference access(data_handle_type pointer,
                             std::size_t index) const noexcept {
    return pointer[index + 1];
  }

  constexpr data_handle_type offset(data_handle_type pointer,
                                    std::size_t index) const noexcept {
    return pointer + index;
  }
};

int main() {
  int data[24];
  for (int i = 0; i < 24; i++) {
    data[i] = i;
  }

  std::mdspan right(data, 2, 3, 4);
  if (right.rank() != 3 || right.rank_dynamic() != 3 ||
      right.extent(0) != 2 || right.extent(1) != 3 ||
      right.extent(2) != 4 || right.size() != 24 ||
      right.empty() || right[1, 1, 2] != 18 ||
      right.stride(0) != 12 || !right.is_unique() ||
      !right.is_exhaustive() || !right.is_strided()) {
    return 1;
  }
  right[1, 1, 2] = 81;
  if (data[18] != 81 || right.data_handle() != data) {
    return 2;
  }

  std::array<int, 3> dimensions{2, 3, 4};
  std::mdspan from_array(data, dimensions);
  std::span<int, 3> dimension_span(dimensions);
  std::mdspan from_span(data, dimension_span);
  if (from_array[1, 0, 3] != 15 || from_span[0, 2, 1] != 9) {
    return 3;
  }

  using fixed_extents = std::extents<int, 2, 3, 4>;
  std::layout_left::mapping<fixed_extents> left_mapping;
  std::mdspan left(data, left_mapping);
  if (left[1, 1, 2] != data[15] ||
      left.mapping().stride(2) != 6) {
    return 4;
  }

  int padded_data[34];
  for (int i = 0; i < 34; i++) {
    padded_data[i] = 100 + i;
  }
  std::array<int, 3> padded_strides{20, 5, 1};
  std::layout_stride::mapping<fixed_extents> padded_mapping(
      fixed_extents{}, padded_strides);
  std::mdspan padded(padded_data, padded_mapping);
  if (padded[1, 1, 2] != 127 || padded.size() != 24 ||
      padded.is_exhaustive()) {
    return 5;
  }

  std::mdspan<const int, std::dextents<long, 3>> const_view = right;
  if (const_view[1, 1, 2] != 81 ||
      const_view.extent(2) != 4) {
    return 6;
  }

  std::mdspan one_dimensional(data);
  std::mdspan scalar(data + 5);
  if (one_dimensional.extent(0) != 24 ||
      one_dimensional[7] != 7 || scalar[] != 5) {
    return 7;
  }

  using empty_extents = std::extents<std::size_t, 0>;
  std::mdspan<int, empty_extents> empty(data);
  if (!empty.empty() || empty.size() != 0) {
    return 8;
  }

  int shifted_data[7] = {10, 11, 12, 13, 14, 15, 16};
  using shifted_extents = std::extents<int, 2, 3>;
  std::layout_right::mapping<shifted_extents> shifted_mapping;
  shifted_accessor<int> shifted_policy;
  std::mdspan shifted(shifted_data, shifted_mapping, shifted_policy);
  if (shifted[1, 2] != 16) {
    return 9;
  }

  int other_data[6] = {30, 31, 32, 33, 34, 35};
  std::mdspan first(data, 2, 3);
  std::mdspan second(other_data, 2, 3);
  std::swap(first, second);
  if (first.data_handle() != other_data || first[1, 2] != 35 ||
      second.data_handle() != data || second[1, 2] != 5) {
    return 10;
  }

  return 0;
}
