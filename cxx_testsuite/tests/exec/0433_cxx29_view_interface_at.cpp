// RUN: -std=c++29 -fexceptions
// EXPECT_EXIT: 0

#include <concepts>
#include <list>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <version>

#if __cpp_lib_view_interface != 202606L
#error "__cpp_lib_view_interface has the wrong value"
#endif

template <class View>
concept has_at = requires(View& view) { view.at(0); };

using pointer_view = std::ranges::subrange<int*, int*>;
using list_view = std::ranges::ref_view<std::list<int>>;
using unsized_pointer_view =
    std::ranges::subrange<int*, std::unreachable_sentinel_t>;

static_assert(has_at<pointer_view>);
static_assert(!has_at<list_view>);
static_assert(!has_at<unsized_pointer_view>);
static_assert(
    std::same_as<decltype(std::declval<pointer_view&>().at(0)), int&>);
static_assert(
    std::same_as<decltype(std::declval<const pointer_view&>().at(0)), int&>);

constexpr bool test_constexpr_primitives() {
  int values[] = {2, 4, 6};
  pointer_view view(values, values + 3);
  return view[1] == 4 && std::ranges::distance(view) == 3;
}

static_assert(test_constexpr_primitives());

constexpr bool test_constexpr_at() {
  int values[] = {2, 4, 6};
  pointer_view view(values, values + 3);
  view.at(1) = 5;
  ++view.at(0);
  view.at(2) += 1;
  return view.at(0) == 3 && view.at(1) == 5 && view.at(2) == 7;
}

static_assert(test_constexpr_at());

int main() {
  int values[] = {10, 20, 30};
  pointer_view view(values, values + 3);
  const pointer_view& constant = view;

  if (view.at(0) != 10 || constant.at(2) != 30) {
    return 1;
  }
  constant.at(1) = 25;
  if (values[1] != 25) {
    return 2;
  }

  bool caught_negative = false;
  try {
    static_cast<void>(view.at(-1));
  } catch (const std::out_of_range&) {
    caught_negative = true;
  }
  if (!caught_negative) {
    return 3;
  }

  bool caught_end = false;
  try {
    static_cast<void>(view.at(3));
  } catch (const std::out_of_range&) {
    caught_end = true;
  }
  return caught_end ? 0 : 4;
}
