// RUN: -std=c++23

#if __cplusplus != 202302L
#error "__cplusplus does not advertise C++23"
#endif

#if __cpp_concepts != 202002L
#error "__cpp_concepts has the wrong value"
#endif

#if __cpp_consteval != 202211L
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

#if __cpp_constexpr != 202211L
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

#if __cpp_multidimensional_subscript != 202211L
#error "__cpp_multidimensional_subscript has the wrong value"
#endif

#if __cpp_size_t_suffix != 202011L
#error "__cpp_size_t_suffix has the wrong value"
#endif

#if __cpp_implicit_move != 202207L
#error "__cpp_implicit_move has the wrong value"
#endif

#if __cpp_range_based_for != 202211L
#error "__cpp_range_based_for has the wrong value"
#endif

#if __cpp_named_character_escapes != 202207L
#error "__cpp_named_character_escapes has the wrong value"
#endif

#if __STDCPP_FLOAT32_T__ != 1
#error "__STDCPP_FLOAT32_T__ has the wrong value"
#endif

#if __STDCPP_FLOAT64_T__ != 1
#error "__STDCPP_FLOAT64_T__ has the wrong value"
#endif

#if __cpp_deduction_guides != 202207L
#error "__cpp_deduction_guides has the wrong value"
#endif

#if 1uz != 1
#error "size_t literals must work in preprocessing expressions"
#endif

int main(void) {
  return 0;
}
