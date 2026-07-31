// RUN: -std=c++23

#include <array>
#include <mdspan>
#include <span>
#include <type_traits>

#if __cpp_lib_mdspan != 202207L
#error "__cpp_lib_mdspan has the wrong value"
#endif

using mixed_extents =
    std::extents<int, 2, std::dynamic_extent, 4, std::dynamic_extent>;

static_assert(mixed_extents::rank() == 4);
static_assert(mixed_extents::rank_dynamic() == 2);
static_assert(mixed_extents::static_extent(0) == 2);
static_assert(mixed_extents::static_extent(1) == std::dynamic_extent);
static_assert(mixed_extents::static_extent(2) == 4);
static_assert(std::is_same_v<mixed_extents::index_type, int>);
static_assert(std::is_same_v<mixed_extents::size_type, unsigned int>);
static_assert(std::is_same_v<mixed_extents::rank_type, std::size_t>);
static_assert(std::is_constructible_v<mixed_extents, int, int>);
static_assert(std::is_constructible_v<mixed_extents, int, int, int, int>);
static_assert(
    std::is_constructible_v<mixed_extents, const std::array<int, 2>&>);

using dynamic_three = std::dextents<std::size_t, 3>;
using sequence_three = std::make_index_sequence<3>;
static_assert(
    std::is_same_v<sequence_three, std::index_sequence<0, 1, 2>>);
static_assert(dynamic_three::rank() == 3);
static_assert(dynamic_three::rank_dynamic() == 3);
static_assert(dynamic_three::static_extent(0) == std::dynamic_extent);
static_assert(
    std::is_constructible_v<std::extents<int, 2, 3, 4>, dynamic_three>);
static_assert(std::is_constructible_v<std::default_accessor<const int>,
                                      std::default_accessor<int>>);

void check_extents_conversion(const dynamic_three& source) {
  std::extents<int, 2, 3, 4> converted(source);
}

void check_accessor_conversion() {
  std::default_accessor<const int> accessor(std::default_accessor<int>{});
}
