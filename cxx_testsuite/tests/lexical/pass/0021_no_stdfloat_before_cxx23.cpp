// RUN: -std=c++20

#ifdef __STDCPP_FLOAT32_T__
#error "__STDCPP_FLOAT32_T__ must not be defined before C++23"
#endif

#ifdef __STDCPP_FLOAT64_T__
#error "__STDCPP_FLOAT64_T__ must not be defined before C++23"
#endif

int main() { return 0; }
