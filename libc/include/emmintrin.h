#ifndef _EMMINTRIN_H_INCLUDED
#define _EMMINTRIN_H_INCLUDED

#if !defined(__x86_64__) && !defined(__i386__)
#error "SSE2 intrinsics are only available on x86 targets"
#endif

typedef float __m128 __attribute__((vector_size(16), may_alias));
typedef double __m128d __attribute__((vector_size(16), may_alias));
typedef long long __m128i __attribute__((vector_size(16), may_alias));
typedef long long __m128i_u
    __attribute__((vector_size(16), may_alias, aligned(1)));

typedef signed char __v16qi __attribute__((vector_size(16)));
typedef unsigned char __v16qu __attribute__((vector_size(16)));
typedef short __v8hi __attribute__((vector_size(16)));
typedef unsigned short __v8hu __attribute__((vector_size(16)));
typedef int __v4si __attribute__((vector_size(16)));
typedef unsigned int __v4su __attribute__((vector_size(16)));
typedef long long __v2di __attribute__((vector_size(16)));
typedef unsigned long long __v2du __attribute__((vector_size(16)));

typedef union {
  __m128i m128i;
  __v16qi i8;
  __v16qu u8;
  __v8hi i16;
  __v8hu u16;
  __v4si i32;
  __v4su u32;
  __v2di i64;
  __v2du u64;
} __dave_m128i_alias;

#define __DEFAULT_FN_ATTRS \
  __attribute__((always_inline, __nodebug__, __target__("sse2")))

static __inline__ __m128i __DEFAULT_FN_ATTRS _mm_setzero_si128(void) {
  __m128i result = {0, 0};
  return result;
}

static __inline__ __m128i __DEFAULT_FN_ATTRS _mm_set1_epi8(char value) {
  __dave_m128i_alias result;
  for (int i = 0; i < 16; ++i) result.i8[i] = value;
  return result.m128i;
}

static __inline__ __m128i __DEFAULT_FN_ATTRS _mm_set1_epi16(short value) {
  __dave_m128i_alias result;
  for (int i = 0; i < 8; ++i) result.i16[i] = value;
  return result.m128i;
}

static __inline__ __m128i __DEFAULT_FN_ATTRS _mm_set1_epi32(int value) {
  __dave_m128i_alias result;
  for (int i = 0; i < 4; ++i) result.i32[i] = value;
  return result.m128i;
}

static __inline__ __m128i __DEFAULT_FN_ATTRS
_mm_set_epi32(int e3, int e2, int e1, int e0) {
  __dave_m128i_alias result;
  result.i32[0] = e0;
  result.i32[1] = e1;
  result.i32[2] = e2;
  result.i32[3] = e3;
  return result.m128i;
}

static __inline__ __m128i __DEFAULT_FN_ATTRS
_mm_set_epi64x(long long e1, long long e0) {
  __m128i result = {e0, e1};
  return result;
}

static __inline__ __m128i __DEFAULT_FN_ATTRS
_mm_loadu_si128(const __m128i_u* source) {
  __m128i result;
  unsigned char* destination_bytes = (unsigned char*)&result;
  const unsigned char* source_bytes = (const unsigned char*)source;
  for (int i = 0; i < 16; ++i) destination_bytes[i] = source_bytes[i];
  return result;
}

static __inline__ __m128i __DEFAULT_FN_ATTRS
_mm_load_si128(const __m128i* source) {
  return *source;
}

static __inline__ void __DEFAULT_FN_ATTRS
_mm_storeu_si128(__m128i_u* destination, __m128i value) {
  unsigned char* destination_bytes = (unsigned char*)destination;
  const unsigned char* source_bytes = (const unsigned char*)&value;
  for (int i = 0; i < 16; ++i) destination_bytes[i] = source_bytes[i];
}

static __inline__ void __DEFAULT_FN_ATTRS
_mm_store_si128(__m128i* destination, __m128i value) {
  *destination = value;
}

static __inline__ __m128i __DEFAULT_FN_ATTRS
_mm_cmpeq_epi8(__m128i left, __m128i right) {
  __dave_m128i_alias a;
  __dave_m128i_alias b;
  __dave_m128i_alias result;
  a.m128i = left;
  b.m128i = right;
  result.i8 = a.i8 == b.i8;
  return result.m128i;
}

static __inline__ __m128i __DEFAULT_FN_ATTRS
_mm_cmpgt_epi8(__m128i left, __m128i right) {
  __dave_m128i_alias a;
  __dave_m128i_alias b;
  __dave_m128i_alias result;
  a.m128i = left;
  b.m128i = right;
  result.i8 = a.i8 > b.i8;
  return result.m128i;
}

static __inline__ __m128i __DEFAULT_FN_ATTRS
_mm_and_si128(__m128i left, __m128i right) {
  return left & right;
}

static __inline__ __m128i __DEFAULT_FN_ATTRS
_mm_andnot_si128(__m128i left, __m128i right) {
  return (~left) & right;
}

static __inline__ __m128i __DEFAULT_FN_ATTRS
_mm_or_si128(__m128i left, __m128i right) {
  return left | right;
}

static __inline__ __m128i __DEFAULT_FN_ATTRS
_mm_xor_si128(__m128i left, __m128i right) {
  return left ^ right;
}

static __inline__ __m128i __DEFAULT_FN_ATTRS
_mm_subs_epi8(__m128i left, __m128i right) {
  __dave_m128i_alias a;
  __dave_m128i_alias b;
  __dave_m128i_alias result;
  a.m128i = left;
  b.m128i = right;
  for (int i = 0; i < 16; ++i) {
    int value = (int)a.i8[i] - (int)b.i8[i];
    if (value > 127) value = 127;
    if (value < -128) value = -128;
    result.i8[i] = (signed char)value;
  }
  return result.m128i;
}

static __inline__ int __DEFAULT_FN_ATTRS _mm_movemask_epi8(__m128i value) {
  __dave_m128i_alias lanes;
  lanes.m128i = value;
  int result = 0;
  for (int i = 0; i < 16; ++i)
    result |= ((unsigned char)lanes.u8[i] >> 7) << i;
  return result;
}

static __inline__ long long __DEFAULT_FN_ATTRS
_mm_cvtsi128_si64(__m128i value) {
  return value[0];
}

#undef __DEFAULT_FN_ATTRS

#endif
