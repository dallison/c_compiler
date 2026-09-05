#ifndef _ARM_NEON_H
#define _ARM_NEON_H

#if !defined(__aarch64__) && !defined(__arm__)
#error "NEON intrinsics are only available on ARM targets"
#endif

#include <stdint.h>

typedef int8_t int8x8_t __attribute__((vector_size(8)));
typedef uint8_t uint8x8_t __attribute__((vector_size(8)));
typedef uint64_t uint64x1_t __attribute__((vector_size(8)));
typedef float float32x2_t __attribute__((vector_size(8)));

typedef int8_t int8x16_t __attribute__((vector_size(16)));
typedef uint8_t uint8x16_t __attribute__((vector_size(16)));
typedef int16_t int16x8_t __attribute__((vector_size(16)));
typedef uint16_t uint16x8_t __attribute__((vector_size(16)));
typedef int32_t int32x4_t __attribute__((vector_size(16)));
typedef uint32_t uint32x4_t __attribute__((vector_size(16)));
typedef int64_t int64x2_t __attribute__((vector_size(16)));
typedef uint64_t uint64x2_t __attribute__((vector_size(16)));
typedef float float32x4_t __attribute__((vector_size(16)));
typedef double float64x2_t __attribute__((vector_size(16)));

typedef union {
  int8x8_t s8;
  uint8x8_t u8;
  uint64x1_t u64;
} __dave_neon64_alias;

typedef union {
  int8x16_t s8;
  uint8x16_t u8;
  int32x4_t s32;
  uint32x4_t u32;
  float32x4_t f32;
} __dave_neon128_alias;

#define __NEON_INLINE static __inline__ __attribute__((always_inline))

__NEON_INLINE uint8x8_t vld1_u8(const uint8_t* source) {
  uint8x8_t result;
  for (int i = 0; i < 8; ++i) result[i] = source[i];
  return result;
}

__NEON_INLINE void vst1_u8(uint8_t* destination, uint8x8_t value) {
  for (int i = 0; i < 8; ++i) destination[i] = value[i];
}

__NEON_INLINE uint8x8_t vdup_n_u8(uint8_t value) {
  uint8x8_t result;
  for (int i = 0; i < 8; ++i) result[i] = value;
  return result;
}

__NEON_INLINE int8x8_t vdup_n_s8(int8_t value) {
  int8x8_t result;
  for (int i = 0; i < 8; ++i) result[i] = value;
  return result;
}

__NEON_INLINE uint8x8_t vceq_u8(uint8x8_t left, uint8x8_t right) {
  return left == right;
}

__NEON_INLINE uint8x8_t vceq_s8(int8x8_t left, int8x8_t right) {
  __dave_neon64_alias result;
  result.s8 = left == right;
  return result.u8;
}

__NEON_INLINE uint8x8_t vcge_s8(int8x8_t left, int8x8_t right) {
  __dave_neon64_alias result;
  result.s8 = left >= right;
  return result.u8;
}

__NEON_INLINE uint8x8_t vclt_s8(int8x8_t left, int8x8_t right) {
  __dave_neon64_alias result;
  result.s8 = left < right;
  return result.u8;
}

__NEON_INLINE uint8x8_t vcgt_s8(int8x8_t left, int8x8_t right) {
  __dave_neon64_alias result;
  result.s8 = left > right;
  return result.u8;
}

__NEON_INLINE uint8x8_t vcle_s8(int8x8_t left, int8x8_t right) {
  __dave_neon64_alias result;
  result.s8 = left <= right;
  return result.u8;
}

__NEON_INLINE uint64x1_t vreinterpret_u64_u8(uint8x8_t value) {
  __dave_neon64_alias result;
  result.u8 = value;
  return result.u64;
}

__NEON_INLINE int8x8_t vreinterpret_s8_u8(uint8x8_t value) {
  return (int8x8_t)value;
}

__NEON_INLINE uint8x8_t vreinterpret_u8_s8(int8x8_t value) {
  return (uint8x8_t)value;
}

__NEON_INLINE uint64_t vget_lane_u64(uint64x1_t value, const int lane) {
  return value[lane];
}

__NEON_INLINE uint8x16_t vld1q_u8(const uint8_t* source) {
  uint8x16_t result;
  for (int i = 0; i < 16; ++i) result[i] = source[i];
  return result;
}

__NEON_INLINE void vst1q_u8(uint8_t* destination, uint8x16_t value) {
  for (int i = 0; i < 16; ++i) destination[i] = value[i];
}

__NEON_INLINE uint8x16_t vdupq_n_u8(uint8_t value) {
  uint8x16_t result;
  for (int i = 0; i < 16; ++i) result[i] = value;
  return result;
}

__NEON_INLINE int8x16_t vdupq_n_s8(int8_t value) {
  int8x16_t result;
  for (int i = 0; i < 16; ++i) result[i] = value;
  return result;
}

__NEON_INLINE uint8x16_t vceqq_u8(uint8x16_t left, uint8x16_t right) {
  return left == right;
}

__NEON_INLINE uint8x16_t vcgtq_s8(int8x16_t left, int8x16_t right) {
  __dave_neon128_alias result;
  result.s8 = left > right;
  return result.u8;
}

__NEON_INLINE int32x4_t vaddq_s32(int32x4_t left, int32x4_t right) {
  return left + right;
}

__NEON_INLINE float32x4_t vaddq_f32(float32x4_t left, float32x4_t right) {
  return left + right;
}

__NEON_INLINE float32x4_t vmulq_f32(float32x4_t left, float32x4_t right) {
  return left * right;
}

__NEON_INLINE float64x2_t vaddq_f64(float64x2_t left, float64x2_t right) {
  return left + right;
}

#undef __NEON_INLINE

#endif
