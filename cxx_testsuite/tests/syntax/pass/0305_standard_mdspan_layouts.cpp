// RUN: -std=c++23

#include <array>
#include <mdspan>
#include <type_traits>

using static_extents = std::extents<int, 2, 3, 4>;
using dynamic_extents = std::dextents<int, 3>;
using right_mapping = std::layout_right::mapping<static_extents>;
using left_mapping = std::layout_left::mapping<static_extents>;
using stride_mapping = std::layout_stride::mapping<static_extents>;
using dynamic_stride_mapping =
    std::layout_stride::mapping<dynamic_extents>;

struct offset_mapping {
  using extents_type = static_extents;

  static constexpr bool is_always_unique() noexcept { return true; }
  static constexpr bool is_always_exhaustive() noexcept { return false; }
  static constexpr bool is_always_strided() noexcept { return true; }

  constexpr extents_type extents() const noexcept { return {}; }

  constexpr int stride(std::size_t rank_index) const noexcept {
    return rank_index == 0 ? 12 : rank_index == 1 ? 4 : 1;
  }

  constexpr int operator()(int i, int j, int k) const noexcept {
    return 1 + i * 12 + j * 4 + k;
  }
};

static_assert(std::is_same_v<right_mapping::extents_type, static_extents>);
static_assert(std::is_same_v<right_mapping::index_type, int>);
static_assert(std::is_same_v<right_mapping::size_type, unsigned int>);
static_assert(std::is_same_v<right_mapping::rank_type, std::size_t>);
static_assert(std::is_same_v<right_mapping::layout_type,
                             std::layout_right>);
static_assert(std::is_same_v<left_mapping::layout_type, std::layout_left>);
static_assert(std::is_same_v<stride_mapping::layout_type,
                             std::layout_stride>);

constexpr right_mapping right{};
static_assert(right.required_span_size() == 24);
static_assert(right.stride(0) == 12);
static_assert(right.stride(1) == 4);
static_assert(right.stride(2) == 1);
static_assert(right(1, 1, 2) == 18);
static_assert(right_mapping::is_always_unique());
static_assert(right_mapping::is_always_exhaustive());
static_assert(right_mapping::is_always_strided());

constexpr left_mapping left{};
static_assert(left.required_span_size() == 24);
static_assert(left.stride(0) == 1);
static_assert(left.stride(1) == 2);
static_assert(left.stride(2) == 6);
static_assert(left(1, 1, 2) == 15);

using zero_extents = std::extents<int>;
using zero_stride_type = std::layout_stride::mapping<zero_extents>;
constexpr std::layout_right::mapping<zero_extents> zero_right{};
constexpr std::layout_left::mapping<zero_extents> zero_left{};
constexpr zero_stride_type zero_stride{};
static_assert(zero_right.required_span_size() == 1);
static_assert(zero_left.required_span_size() == 1);
static_assert(zero_stride.required_span_size() == 1);
static_assert(zero_right() == 0);
static_assert(zero_left() == 0);
static_assert(zero_stride() == 0);
static_assert(zero_stride.is_exhaustive());
static_assert(zero_stride_type::is_always_exhaustive());

using empty_extents = std::extents<int, 2, 0, 4>;
using empty_stride_type = std::layout_stride::mapping<empty_extents>;
constexpr std::layout_right::mapping<empty_extents> empty_right{};
constexpr std::layout_left::mapping<empty_extents> empty_left{};
constexpr empty_stride_type empty_stride{};
static_assert(empty_right.required_span_size() == 0);
static_assert(empty_left.required_span_size() == 0);
static_assert(empty_stride.required_span_size() == 0);
static_assert(empty_stride.is_exhaustive());
static_assert(empty_stride_type::is_always_exhaustive());

static_assert(!dynamic_stride_mapping::is_always_exhaustive());

constexpr stride_mapping default_stride{};
static_assert(default_stride == right);
static_assert(default_stride.required_span_size() == 24);
static_assert(default_stride(1, 1, 2) == 18);
static_assert(default_stride.is_exhaustive());
static_assert(default_stride.stride(0) == 12);
static_assert(std::is_convertible_v<right_mapping, stride_mapping>);
static_assert(std::is_convertible_v<left_mapping, stride_mapping>);
static_assert(!std::is_convertible_v<stride_mapping, right_mapping>);
static_assert(!std::is_convertible_v<stride_mapping, left_mapping>);
static_assert(std::is_nothrow_constructible_v<right_mapping, stride_mapping>);
static_assert(std::is_nothrow_constructible_v<left_mapping, stride_mapping>);
static_assert(std::is_constructible_v<stride_mapping, offset_mapping>);
static_assert(!std::is_convertible_v<offset_mapping, stride_mapping>);

using rank_one_extents = std::extents<int, 7>;

void check_array_and_span_stride_construction() {
  dynamic_extents shape(2, 3, 4);
  std::layout_right::mapping<dynamic_extents> dynamic_right(shape);
  std::layout_left::mapping<dynamic_extents> dynamic_left(shape);
  std::array<int, 3> strides{20, 5, 1};
  stride_mapping from_array(static_extents{}, strides);
  int span_values[] = {12, 4, 1};
  stride_mapping from_span(
      static_extents{}, std::span<int, 3>(span_values));
  stride_mapping from_left = left;
  stride_mapping from_right = right;
  right_mapping right_again(default_stride);
  left_mapping left_again(from_left);
  std::layout_right::mapping<rank_one_extents> rank_one_right;
  std::layout_left::mapping<rank_one_extents> rank_one_left(
      rank_one_right);
  bool offset_mapping_is_different =
      !(default_stride == offset_mapping{});
  (void)dynamic_right;
  (void)dynamic_left;
  (void)from_array;
  (void)from_span;
  (void)from_right;
  (void)right_again;
  (void)left_again;
  (void)rank_one_left;
  (void)offset_mapping_is_different;
}

