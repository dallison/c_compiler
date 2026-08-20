// RUN: -std=c++26

#include <system_error>

#ifdef __cpp_lib_format
#error "<system_error> must not expose formatting before C++29"
#endif

#include <format>

template <class T>
concept supports_debug_format =
    requires(T& formatter) { formatter.set_debug_format(); };

static_assert(
    !supports_debug_format<std::formatter<std::error_code, char>>);
