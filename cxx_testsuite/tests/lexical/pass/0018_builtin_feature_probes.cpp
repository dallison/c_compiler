// RUN: -std=c++20

#ifndef __has_builtin
#error "__has_builtin must be defined"
#endif
#ifndef __has_attribute
#error "__has_attribute must be defined"
#endif
#ifndef __has_cpp_attribute
#error "__has_cpp_attribute must be defined"
#endif
#ifndef __has_feature
#error "__has_feature must be defined"
#endif

#if !defined(__has_builtin) || !defined(__has_attribute)
#error "defined() must agree with #ifdef for compiler probes"
#endif

#if !__has_builtin(__builtin_expect)
#error "__builtin_expect must be discoverable"
#endif
#if !__has_builtin(__builtin_prefetch)
#error "__builtin_prefetch must be discoverable"
#endif
#if !__has_builtin(__builtin_trap)
#error "__builtin_trap must be discoverable"
#endif
#if !__has_builtin(__builtin_unreachable)
#error "__builtin_unreachable must be discoverable"
#endif
#if __has_builtin(__builtin_not_a_real_builtin)
#error "unknown builtins must not be reported as supported"
#endif

#if !__has_attribute(noreturn)
#error "implemented GNU attributes must be discoverable"
#endif
#if __has_attribute(vector_size)
#error "unsupported vector attributes must not be discoverable"
#endif

#if __has_cpp_attribute(nodiscard) < 201907L
#error "nodiscard support must report its standard feature value"
#endif
#if __has_cpp_attribute(no_unique_address)
#error "ignored layout attributes must not be reported as supported"
#endif

#if !__has_feature(cxx_exceptions)
#error "enabled exceptions must be discoverable"
#endif
#if !__has_feature(cxx_constexpr)
#error "C++ constexpr support must be discoverable"
#endif

#if __ATOMIC_RELAXED != 0 || __ATOMIC_CONSUME != 1 || \
    __ATOMIC_ACQUIRE != 2 || __ATOMIC_RELEASE != 3 || \
    __ATOMIC_ACQ_REL != 4 || __ATOMIC_SEQ_CST != 5
#error "atomic memory-order predefined macros have incorrect values"
#endif

#if __GNUC__ != 4 || __GNUC_MINOR__ != 2 || __GNUC_PATCHLEVEL__ != 1
#error "GCC compatibility version macros are incomplete"
#endif

int main(void) {
  return 0;
}
