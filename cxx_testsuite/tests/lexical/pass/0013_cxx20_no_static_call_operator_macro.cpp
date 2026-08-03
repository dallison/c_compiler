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

#ifdef __cpp_auto_cast
#error "__cpp_auto_cast must not be defined before C++23"
#endif

#ifdef __cpp_multidimensional_subscript
#error "__cpp_multidimensional_subscript must not be defined before C++23"
#endif

#ifdef __cpp_size_t_suffix
#error "__cpp_size_t_suffix must not be defined before C++23"
#endif

#ifdef __cpp_implicit_move
#error "__cpp_implicit_move must not be defined before C++23"
#endif

#ifdef __cpp_named_character_escapes
#error "__cpp_named_character_escapes must not be defined before C++23"
#endif

#if __cpp_consteval != 201811L
#error "__cpp_consteval must retain its C++20 value in C++20"
#endif

#if __cpp_constexpr != 202002L
#error "__cpp_constexpr must retain its C++20 value in C++20"
#endif

#if __cpp_range_based_for != 201603L
#error "__cpp_range_based_for must retain its C++17 value in C++20"
#endif

#if __cpp_deduction_guides != 201703L
#error "__cpp_deduction_guides must retain its C++17 value in C++20"
#endif

int main(void) {
  return 0;
}
