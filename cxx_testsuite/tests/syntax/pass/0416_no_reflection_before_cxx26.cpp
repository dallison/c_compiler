// RUN: -std=c++23

#include <version>

#ifdef __cpp_lib_reflection
#error "__cpp_lib_reflection must not be defined before C++26"
#endif
#ifdef __cpp_lib_define_static
#error "__cpp_lib_define_static must not be defined before C++26"
#endif
