// RUN: -std=c++26

#include <ranges>
#include <version>

#ifdef __cpp_lib_view_interface
#error "__cpp_lib_view_interface must not be defined before C++29"
#endif

template <class View>
concept has_at = requires(View& view) { view.at(0); };

static_assert(!has_at<std::ranges::subrange<int*, int*>>);
