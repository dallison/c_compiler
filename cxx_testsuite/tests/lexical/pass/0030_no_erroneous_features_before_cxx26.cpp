// RUN: -std=c++23

#if __has_cpp_attribute(indeterminate)
#error "[[indeterminate]] must not be advertised before C++26"
#endif

