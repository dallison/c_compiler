#ifndef _STDBIT_H
#define _STDBIT_H

#if defined(__cplusplus)
#error "<stdbit.h> is a C header; use <bit> in C++"
#elif !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
#error "<stdbit.h> requires C23 or later"
#else

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

#define __STDC_VERSION_STDBIT_H__ 202311L

#define __STDC_ENDIAN_LITTLE__ 1234
#define __STDC_ENDIAN_BIG__ 4321
#define __STDC_ENDIAN_NATIVE__ __STDC_ENDIAN_LITTLE__

#if defined(__LP64__)
#define INTPTR_WIDTH 64
#define UINTPTR_WIDTH 64
#elif defined(__6502__)
#define INTPTR_WIDTH 16
#define UINTPTR_WIDTH 16
#else
#define INTPTR_WIDTH 32
#define UINTPTR_WIDTH 32
#endif

#define __DAVECC_STDBIT_FUNCTIONS(suffix, type, width)                     \
  [[unsequenced]] static unsigned int                                      \
  stdc_leading_zeros_##suffix(type value) {                                \
    return (unsigned int)__davecc_clz(value, width);                        \
  }                                                                         \
  [[unsequenced]] static unsigned int                                      \
  stdc_leading_ones_##suffix(type value) {                                 \
    return (unsigned int)__davecc_clz((type)(value ^ (type)-1), width);     \
  }                                                                         \
  [[unsequenced]] static unsigned int                                      \
  stdc_trailing_zeros_##suffix(type value) {                               \
    return (unsigned int)__davecc_ctz(value, width);                        \
  }                                                                         \
  [[unsequenced]] static unsigned int                                      \
  stdc_trailing_ones_##suffix(type value) {                                \
    return (unsigned int)__davecc_ctz((type)(value ^ (type)-1), width);     \
  }                                                                         \
  [[unsequenced]] static unsigned int                                      \
  stdc_first_leading_zero_##suffix(type value) {                           \
    unsigned int count = stdc_leading_ones_##suffix(value);                \
    return count == (width) ? 0U : count + 1U;                              \
  }                                                                         \
  [[unsequenced]] static unsigned int                                      \
  stdc_first_leading_one_##suffix(type value) {                            \
    unsigned int count = stdc_leading_zeros_##suffix(value);               \
    return count == (width) ? 0U : count + 1U;                              \
  }                                                                         \
  [[unsequenced]] static unsigned int                                      \
  stdc_first_trailing_zero_##suffix(type value) {                          \
    unsigned int count = stdc_trailing_ones_##suffix(value);               \
    return count == (width) ? 0U : count + 1U;                              \
  }                                                                         \
  [[unsequenced]] static unsigned int                                      \
  stdc_first_trailing_one_##suffix(type value) {                           \
    unsigned int count = stdc_trailing_zeros_##suffix(value);              \
    return count == (width) ? 0U : count + 1U;                              \
  }                                                                         \
  [[unsequenced]] static unsigned int                                      \
  stdc_count_zeros_##suffix(type value) {                                  \
    return (unsigned int)(width) -                                         \
           (unsigned int)__davecc_popcount(value, width);                   \
  }                                                                         \
  [[unsequenced]] static unsigned int                                      \
  stdc_count_ones_##suffix(type value) {                                   \
    return (unsigned int)__davecc_popcount(value, width);                   \
  }                                                                         \
  [[unsequenced]] static bool stdc_has_single_bit_##suffix(                 \
      type value) {                                                         \
    return value != 0 && (value & (type)(value - 1)) == 0;                  \
  }                                                                         \
  [[unsequenced]] static unsigned int stdc_bit_width_##suffix(              \
      type value) {                                                         \
    return (unsigned int)(width) - stdc_leading_zeros_##suffix(value);      \
  }                                                                         \
  [[unsequenced]] static type stdc_bit_floor_##suffix(type value) {         \
    return value == 0                                                       \
               ? (type)0                                                    \
               : (type)((type)1 <<                                         \
                        (stdc_bit_width_##suffix(value) - 1U));              \
  }                                                                         \
  [[unsequenced]] static type stdc_bit_ceil_##suffix(type value) {          \
    return value <= 1                                                       \
               ? (type)1                                                    \
               : (type)((type)1 <<                                         \
                        stdc_bit_width_##suffix((type)(value - 1)));         \
  }

__DAVECC_STDBIT_FUNCTIONS(uc, unsigned char, UCHAR_WIDTH)
__DAVECC_STDBIT_FUNCTIONS(us, unsigned short, USHRT_WIDTH)
__DAVECC_STDBIT_FUNCTIONS(ui, unsigned int, UINT_WIDTH)
__DAVECC_STDBIT_FUNCTIONS(ul, unsigned long, ULONG_WIDTH)
__DAVECC_STDBIT_FUNCTIONS(ull, unsigned long long, ULLONG_WIDTH)

#undef __DAVECC_STDBIT_FUNCTIONS

#define __DAVECC_STDBIT_SELECT(value, name)                                 \
  _Generic((value),                                                         \
      unsigned char: name##_uc,                                             \
      unsigned short: name##_us,                                            \
      unsigned int: name##_ui,                                              \
      unsigned long: name##_ul,                                             \
      unsigned long long: name##_ull,                                       \
      unsigned _BitInt(8): name##_uc,                                       \
      unsigned _BitInt(16): name##_us,                                      \
      unsigned _BitInt(32): name##_ui,                                      \
      unsigned _BitInt(64): name##_ull)

#define stdc_leading_zeros(value)                                           \
  __DAVECC_STDBIT_SELECT((value), stdc_leading_zeros)(value)
#define stdc_leading_ones(value)                                            \
  __DAVECC_STDBIT_SELECT((value), stdc_leading_ones)(value)
#define stdc_trailing_zeros(value)                                          \
  __DAVECC_STDBIT_SELECT((value), stdc_trailing_zeros)(value)
#define stdc_trailing_ones(value)                                           \
  __DAVECC_STDBIT_SELECT((value), stdc_trailing_ones)(value)
#define stdc_first_leading_zero(value)                                      \
  __DAVECC_STDBIT_SELECT((value), stdc_first_leading_zero)(value)
#define stdc_first_leading_one(value)                                       \
  __DAVECC_STDBIT_SELECT((value), stdc_first_leading_one)(value)
#define stdc_first_trailing_zero(value)                                     \
  __DAVECC_STDBIT_SELECT((value), stdc_first_trailing_zero)(value)
#define stdc_first_trailing_one(value)                                      \
  __DAVECC_STDBIT_SELECT((value), stdc_first_trailing_one)(value)
#define stdc_count_zeros(value)                                             \
  __DAVECC_STDBIT_SELECT((value), stdc_count_zeros)(value)
#define stdc_count_ones(value)                                              \
  __DAVECC_STDBIT_SELECT((value), stdc_count_ones)(value)
#define stdc_has_single_bit(value)                                          \
  __DAVECC_STDBIT_SELECT((value), stdc_has_single_bit)(value)
#define stdc_bit_width(value)                                               \
  __DAVECC_STDBIT_SELECT((value), stdc_bit_width)(value)
#define stdc_bit_floor(value)                                               \
  ((typeof(value))__DAVECC_STDBIT_SELECT((value), stdc_bit_floor)(value))
#define stdc_bit_ceil(value)                                                \
  ((typeof(value))__DAVECC_STDBIT_SELECT((value), stdc_bit_ceil)(value))

#endif

#endif /* _STDBIT_H */
