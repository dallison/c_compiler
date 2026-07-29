// RUN: -std=c++20

#if __cpp_char8_t != 201811L
#error "__cpp_char8_t must advertise the implemented C++20 feature"
#endif

char8_t value = u8'x';
