// RUN: -std=c++26

#if __cpp_trivial_union != 202603L
#error "__cpp_trivial_union must advertise the final C++26 wording"
#endif
