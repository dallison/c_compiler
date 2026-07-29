// RUN: -std=c++17

#ifdef __cpp_char8_t
#error "__cpp_char8_t must not be defined before C++20"
#endif

int char8_t = 1;
