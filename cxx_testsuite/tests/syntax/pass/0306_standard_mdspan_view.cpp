// RUN: -std=c++23

#include <array>
#include <mdspan>
#include <span>
#include <type_traits>

struct offset_layout {
  template <class Extents>
  class mapping;
};

template <class Extents>
class offset_layout::mapping {
 public:
  using extents_type = Extents;
  using index_type = typename extents_type::index_type;
  using size_type = typename extents_type::size_type;
  using rank_type = typename extents_type::rank_type;
  using layout_type = offset_layout;

 private:
  std::layout_right::mapping<extents_type> base_;

 public:
  constexpr mapping() noexcept = default;
  constexpr mapping(const extents_type& extents) noexcept : base_(extents) {}

  constexpr const extents_type& extents() const noexcept {
    return base_.extents();
  }

  constexpr index_type required_span_size() const noexcept {
    return base_.required_span_size() + 1;
  }

  template <class... Indices>
  constexpr index_type operator()(Indices... indices) const noexcept {
    return 1 + base_(indices...);
  }

  static constexpr bool is_always_unique() noexcept { return true; }
  static constexpr bool is_always_exhaustive() noexcept { return false; }
  static constexpr bool is_always_strided() noexcept { return true; }
  static constexpr bool is_unique() noexcept { return true; }
  static constexpr bool is_exhaustive() noexcept { return false; }
  static constexpr bool is_strided() noexcept { return true; }

  constexpr index_type stride(rank_type rank_index) const noexcept {
    return base_.stride(rank_index);
  }
};

template <class ElementType>
struct offset_accessor {
  using offset_policy = offset_accessor;
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

using view_extents =
    std::extents<int, 2, std::dynamic_extent, 4>;
using view_type = std::mdspan<int, view_extents>;
using fixed_view =
    std::mdspan<int, std::extents<int, 2, 3, 4>>;

static_assert(std::is_same_v<view_type::extents_type, view_extents>);
static_assert(std::is_same_v<view_type::layout_type, std::layout_right>);
static_assert(
    std::is_same_v<view_type::accessor_type, std::default_accessor<int>>);
static_assert(std::is_same_v<view_type::mapping_type,
                             std::layout_right::mapping<view_extents>>);
static_assert(std::is_same_v<view_type::element_type, int>);
static_assert(std::is_same_v<view_type::value_type, int>);
static_assert(std::is_same_v<view_type::reference, int&>);
static_assert(view_type::rank() == 3);
static_assert(view_type::rank_dynamic() == 1);
static_assert(view_type::static_extent(0) == 2);
static_assert(view_type::static_extent(1) == std::dynamic_extent);
static_assert(std::is_default_constructible_v<view_type>);
static_assert(!std::is_default_constructible_v<fixed_view>);
static_assert(std::is_constructible_v<view_type, int*, int>);
static_assert(
    std::is_constructible_v<view_type, int*, const view_extents&>);

using mutable_dynamic_view =
    std::mdspan<int, std::dextents<int, 2>>;
using const_dynamic_view =
    std::mdspan<const int, std::dextents<long, 2>>;
using mutable_mapping = mutable_dynamic_view::mapping_type;
using const_mapping = const_dynamic_view::mapping_type;
using mutable_accessor = mutable_dynamic_view::accessor_type;
using const_accessor = const_dynamic_view::accessor_type;
static_assert(
    std::is_constructible_v<const_mapping, const mutable_mapping&>);
static_assert(
    std::is_constructible_v<const_accessor, const mutable_accessor&>);
static_assert(std::is_constructible_v<const int*, int* const&>);
static_assert(std::is_constructible_v<std::dextents<long, 2>,
                                      std::dextents<int, 2>>);
static_assert(
    std::is_constructible_v<const_dynamic_view, mutable_dynamic_view>);
static_assert(std::is_convertible_v<mutable_dynamic_view,
                                    const_dynamic_view>);

void check_mdspan_conversion(const mutable_dynamic_view& source) {
  const_dynamic_view converted = source;
  (void)converted;
}

int global_data[24]{};
std::mdspan from_array(global_data);
std::mdspan from_pointer(global_data + 0);
std::mdspan from_sizes(global_data, 2, 3, 4);
std::array<int, 3> global_extents{2, 3, 4};
std::mdspan from_extent_array(global_data, global_extents);
std::span<int, 3> global_extent_span(global_extents);
std::mdspan from_extent_span(global_data, global_extent_span);
std::layout_left::mapping<std::extents<int, 2, 3, 4>> global_left;
std::mdspan from_mapping(global_data, global_left);
offset_accessor<int> global_accessor;
std::mdspan from_accessor(global_data, global_left, global_accessor);

static_assert(std::is_same_v<
              decltype(from_array),
              std::mdspan<int, std::extents<std::size_t, 24>>>);
using pointer_view_type = decltype(from_pointer);
static_assert(pointer_view_type::rank() == 0);
static_assert(std::is_same_v<
              decltype(from_sizes),
              std::mdspan<int, std::dextents<std::size_t, 3>>>);
static_assert(std::is_same_v<
              decltype(from_mapping),
              std::mdspan<int, std::extents<int, 2, 3, 4>,
                          std::layout_left>>);
static_assert(std::is_same_v<
              decltype(from_accessor),
              std::mdspan<int, std::extents<int, 2, 3, 4>,
                          std::layout_left, offset_accessor<int>>>);

void check_mdspan_surface(int* data) {
  view_type view(data, 3);
  int& element = view[1, 2, 3];
  offset_layout::mapping<view_extents> custom_mapping(view_extents(3));
  std::mdspan custom_view(data, custom_mapping);
  std::swap(view, view);
  (void)element;
  (void)custom_view;
  (void)view.size();
  (void)view.empty();
  (void)view.data_handle();
  (void)view.mapping();
  (void)view.accessor();
  (void)view.is_unique();
  (void)view.is_exhaustive();
  (void)view.is_strided();
  (void)view.stride(0);
}
