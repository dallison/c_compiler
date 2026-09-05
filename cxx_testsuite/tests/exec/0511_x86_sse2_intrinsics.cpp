// RUN: -std=c++20
// EXPECT_EXIT: 0
// TARGETS: x86_64

#include <emmintrin.h>
#include <tmmintrin.h>

int main() {
  alignas(16) signed char bytes[16] = {
      3, 4, 3, 9, -1, 3, 8, 3, 1, 2, 3, 4, 5, 6, 7, 8};
  __m128i value = _mm_loadu_si128(reinterpret_cast<const __m128i*>(bytes));
  __m128i threes = _mm_set1_epi8(3);
  if (_mm_movemask_epi8(_mm_cmpeq_epi8(value, threes)) != 0x04a5) {
    return 1;
  }

  __m128i high = _mm_cmpgt_epi8(value, _mm_setzero_si128());
  if (_mm_movemask_epi8(high) != 0xffef) {
    return 2;
  }

  __m128i converted =
      _mm_subs_epi8(_mm_and_si128(_mm_set1_epi8(-128), value),
                    _mm_set1_epi8(2));
  signed char converted_bytes[16] = {};
  _mm_storeu_si128(reinterpret_cast<__m128i*>(converted_bytes), converted);
  if (converted_bytes[0] != -2 || converted_bytes[4] != -128) {
    return 3;
  }

  __m128i only_twos =
      _mm_andnot_si128(_mm_set1_epi8(-2), _mm_set1_epi8(3));
  if (_mm_movemask_epi8(_mm_cmpeq_epi8(only_twos, _mm_set1_epi8(1))) !=
      0xffff) {
    return 4;
  }

  __m128i signs = _mm_set_epi32(0x01010101, 0x01010101,
                                0x01010101, 0x010101ff);
  __m128i signed_value = _mm_sign_epi8(value, signs);
  signed char output[16] = {};
  _mm_storeu_si128((__m128i_u*)output, signed_value);
  if (output[0] != -3 || output[1] != 4 || output[15] != 8) {
    return 5;
  }

  __m128i indices = _mm_set_epi32(0x80808080, 0x80808080,
                                  0x80808080, 0x00010203);
  __m128i shuffled = _mm_shuffle_epi8(value, indices);
  _mm_storeu_si128((__m128i_u*)output, shuffled);
  if (output[0] != 9 || output[1] != 3 ||
      output[2] != 4 || output[3] != 3 || output[4] != 0) {
    return 6;
  }
  return 0;
}
