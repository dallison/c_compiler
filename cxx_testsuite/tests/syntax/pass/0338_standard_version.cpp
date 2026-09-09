// RUN: -std=c++20

#include <version>

#if __cpp_lib_is_constant_evaluated != 201811L
#error "__cpp_lib_is_constant_evaluated has the wrong value"
#endif
#if __cpp_lib_bit_cast != 201806L
#error "__cpp_lib_bit_cast has the wrong value"
#endif
#if __cpp_lib_bitops != 201907L
#error "__cpp_lib_bitops has the wrong value"
#endif
#if __cpp_lib_endian != 201907L
#error "__cpp_lib_endian has the wrong value"
#endif
#if __cpp_lib_format != 201907L
#error "__cpp_lib_format has the wrong value"
#endif
#if __cpp_lib_int_pow2 != 202002L
#error "__cpp_lib_int_pow2 has the wrong value"
#endif
#if __cpp_lib_math_constants != 201907L
#error "__cpp_lib_math_constants has the wrong value"
#endif
#if __cpp_lib_memory_resource != 201603L
#error "__cpp_lib_memory_resource has the wrong value"
#endif
#if __cpp_lib_sample != 201603L
#error "__cpp_lib_sample has the wrong value"
#endif
#if __cpp_lib_source_location != 201907L
#error "__cpp_lib_source_location has the wrong value"
#endif
#if __cpp_lib_span != 202002L
#error "__cpp_lib_span has the wrong value"
#endif
#if __cpp_lib_syncbuf != 201803L
#error "__cpp_lib_syncbuf has the wrong value"
#endif

#if defined(__6502__) && defined(__cpp_lib_atomic_ref)
#error "single-threaded 65(C)02 profile must not advertise atomic_ref"
#endif

int main() {
  return 0;
}
