// RUN: -std=c++23

#if __cplusplus != 202302L
#error "__cplusplus does not advertise C++23"
#endif

#if __cpp_concepts != 202002L
#error "__cpp_concepts has the wrong value"
#endif

#if __cpp_consteval != 201811L
#error "__cpp_consteval has the wrong value"
#endif

#if __cpp_constinit != 201907L
#error "__cpp_constinit has the wrong value"
#endif

#if __cpp_impl_coroutine != 201902L
#error "__cpp_impl_coroutine has the wrong value"
#endif

#if __cpp_modules != 201907L
#error "__cpp_modules has the wrong value"
#endif

#if __cpp_char8_t != 201811L
#error "__cpp_char8_t has the wrong value"
#endif

#if __cpp_impl_three_way_comparison != 201907L
#error "__cpp_impl_three_way_comparison has the wrong value"
#endif

#if __cpp_using_enum != 201907L
#error "__cpp_using_enum has the wrong value"
#endif

#if __cpp_conditional_explicit != 201806L
#error "__cpp_conditional_explicit has the wrong value"
#endif

#if __cpp_constexpr != 202002L
#error "__cpp_constexpr has the wrong value"
#endif

#if __cpp_constexpr_dynamic_alloc != 201907L
#error "__cpp_constexpr_dynamic_alloc has the wrong value"
#endif

#if __cpp_static_call_operator != 202207L
#error "__cpp_static_call_operator has the wrong value"
#endif

#if __cpp_if_consteval != 202106L
#error "__cpp_if_consteval has the wrong value"
#endif

#if __cpp_explicit_this_parameter != 202110L
#error "__cpp_explicit_this_parameter has the wrong value"
#endif

#if __cpp_auto_cast != 202110L
#error "__cpp_auto_cast has the wrong value"
#endif

int main(void) {
  return 0;
}
