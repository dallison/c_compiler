// RUN: -std=c++26

#if __has_cpp_attribute(indeterminate) != 202403L
#error "[[indeterminate]] must report the C++26 feature value"
#endif

#if !__has_builtin(__builtin_observable_checkpoint)
#error "observable checkpoint builtin must be discoverable"
#endif

