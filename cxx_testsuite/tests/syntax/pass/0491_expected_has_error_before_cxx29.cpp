// RUN: -std=c++26

#include <expected>
#include <version>

#if __cpp_lib_expected != 202211L
#error "__cpp_lib_expected changed before C++29"
#endif

template <class T>
concept has_error_observer =
    requires(const T& value) { value.has_error(); };

static_assert(!has_error_observer<std::expected<int, int>>);
static_assert(!has_error_observer<std::expected<void, int>>);
