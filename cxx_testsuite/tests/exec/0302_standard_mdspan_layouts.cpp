// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <array>
#include <mdspan>

int main() {
  using extents_type =
      std::extents<int, std::dynamic_extent, 3, std::dynamic_extent>;
  extents_type shape(2, 4);

  std::layout_right::mapping<extents_type> right(shape);
  if (right.required_span_size() != 24 || right.stride(0) != 12 ||
      right.stride(1) != 4 || right.stride(2) != 1 ||
      right(1, 1, 2) != 18) {
    return 1;
  }

  std::layout_left::mapping<extents_type> left(shape);
  if (left.required_span_size() != 24 || left.stride(0) != 1 ||
      left.stride(1) != 2 || left.stride(2) != 6 ||
      left(1, 1, 2) != 15) {
    return 2;
  }

  std::array<int, 3> padded_strides{20, 5, 1};
  std::layout_stride::mapping<extents_type> padded(shape, padded_strides);
  if (padded.required_span_size() != 34 || padded.stride(0) != 20 ||
      padded.stride(1) != 5 || padded.stride(2) != 1 ||
      padded(1, 1, 2) != 27 || padded.is_exhaustive()) {
    return 3;
  }

  std::layout_stride::mapping<extents_type> from_right = right;
  std::layout_stride::mapping<extents_type> from_left = left;
  if (from_right(1, 1, 2) != 18 || !from_right.is_exhaustive() ||
      from_left(1, 1, 2) != 15 || !from_left.is_exhaustive()) {
    return 4;
  }

  using empty_extents = std::extents<int, 2, 0, 4>;
  if (std::layout_right::mapping<empty_extents>().required_span_size() != 0 ||
      std::layout_left::mapping<empty_extents>().required_span_size() != 0 ||
      std::layout_stride::mapping<empty_extents>().required_span_size() != 0) {
    return 5;
  }

  return 0;
}
