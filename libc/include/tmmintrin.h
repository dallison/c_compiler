#ifndef _TMMINTRIN_H_INCLUDED
#define _TMMINTRIN_H_INCLUDED

#include <emmintrin.h>

#define __DEFAULT_FN_ATTRS \
  __attribute__((always_inline, __nodebug__, __target__("ssse3")))

static __inline__ __m128i __DEFAULT_FN_ATTRS
_mm_sign_epi8(__m128i value, __m128i signs) {
  __dave_m128i_alias input;
  __dave_m128i_alias sign;
  __dave_m128i_alias result;
  input.m128i = value;
  sign.m128i = signs;
  for (int i = 0; i < 16; ++i) {
    if (sign.i8[i] == 0)
      result.i8[i] = 0;
    else if (sign.i8[i] < 0)
      result.i8[i] = (signed char)(-input.i8[i]);
    else
      result.i8[i] = input.i8[i];
  }
  return result.m128i;
}

static __inline__ __m128i __DEFAULT_FN_ATTRS
_mm_shuffle_epi8(__m128i value, __m128i indices) {
  __dave_m128i_alias input;
  __dave_m128i_alias index;
  __dave_m128i_alias result;
  input.m128i = value;
  index.m128i = indices;
  for (int i = 0; i < 16; ++i) {
    unsigned char lane = index.u8[i];
    result.u8[i] = (lane & 0x80) != 0 ? 0 : input.u8[lane & 0x0f];
  }
  return result.m128i;
}

#undef __DEFAULT_FN_ATTRS

#endif
