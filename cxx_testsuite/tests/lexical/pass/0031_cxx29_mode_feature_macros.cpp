// RUN: -std=c++29

#if __cplusplus != 202700L
#error "__cplusplus does not advertise C++29"
#endif

#if __cpp_impl_reflection != 202603L
#error "C++29 must retain C++26 reflection support"
#endif

int main(void) {
  return 0;
}
