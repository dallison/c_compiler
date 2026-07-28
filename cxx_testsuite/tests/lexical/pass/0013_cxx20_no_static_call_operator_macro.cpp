// RUN: -std=c++20

#ifdef __cpp_static_call_operator
#error "__cpp_static_call_operator must not be defined before C++23"
#endif

#ifdef __cpp_if_consteval
#error "__cpp_if_consteval must not be defined before C++23"
#endif

#ifdef __cpp_explicit_this_parameter
#error "__cpp_explicit_this_parameter must not be defined before C++23"
#endif

int main(void) {
  return 0;
}
