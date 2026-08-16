// RUN: -std=c++20

#include <stacktrace>
#include <version>

#ifdef __cpp_lib_stacktrace
#error "__cpp_lib_stacktrace must not be defined before C++23"
#endif

int stacktrace_is_gated_before_cxx23;
