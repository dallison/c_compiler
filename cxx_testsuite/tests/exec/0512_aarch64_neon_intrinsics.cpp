// RUN: -std=c++20
// EXPECT_EXIT: 0
// TARGETS: aarch64

#include <arm_neon.h>

#if !defined(__ARM_NEON) || !defined(__ARM_NEON__)
#error "AArch64 must advertise NEON"
#endif

static uint8x8_t load_control(const uint8_t* bytes) {
  return vld1_u8(bytes);
}

int main() {
  const uint8_t bytes[8] = {3, 0x80, 7, 3, 0xfe, 9, 3, 0xff};
  uint8x8_t control = load_control(bytes);

  uint8x8_t matches = vceq_u8(control, vdup_n_u8(3));
  uint64_t match_bits = vget_lane_u64(vreinterpret_u64_u8(matches), 0);
  if (match_bits != 0x00ff0000ff0000ffULL) {
    return 1;
  }

  int8x8_t signed_control = vreinterpret_s8_u8(control);
  if (signed_control[0] != 3 || signed_control[1] != -128 ||
      signed_control[4] != -2 || signed_control[7] != -1) {
    return 4;
  }
  uint8x8_t full = vcge_s8(signed_control, vdup_n_s8(0));
  uint64_t full_bits = vget_lane_u64(vreinterpret_u64_u8(full), 0);
  if (full_bits != 0x00ffff00ffff00ffULL) {
    return 2;
  }
  uint64_t greater_bits = vget_lane_u64(
      vreinterpret_u64_u8(vcgt_s8(signed_control, vdup_n_s8(0))), 0);
  uint64_t less_equal_bits = vget_lane_u64(
      vreinterpret_u64_u8(vcle_s8(signed_control, vdup_n_s8(0))), 0);
  uint64_t equal_bits = vget_lane_u64(
      vreinterpret_u64_u8(vceq_s8(signed_control, vdup_n_s8(-1))), 0);
  if (greater_bits != 0x00ffff00ffff00ffULL ||
      less_equal_bits != 0xff0000ff0000ff00ULL ||
      equal_bits != 0xff00000000000000ULL) {
    return 6;
  }

  uint8x8_t special = vclt_s8(signed_control, vdup_n_s8(0));
  uint8_t output[8] = {};
  vst1_u8(output, special);
  if (output[0] != 0 || output[1] != 0xff ||
      output[4] != 0xff || output[7] != 0xff) {
    return 3;
  }

  uint8x8_t left = {1, 2, 3, 4, 250, 0, 0x80, 0xff};
  uint8x8_t right = {5, 2, 1, 7, 10, 1, 0x7f, 0xfe};
  uint8x8_t sum = left + right;
  uint8x8_t difference = left - right;
  uint8x8_t bits = (left & right) | (left ^ right);
  uint8x8_t greater = left > right;
  if (sum[0] != 6 || sum[4] != 4 || difference[3] != 253 ||
      bits[6] != 0xff || greater[0] != 0 || greater[4] != 0xff ||
      greater[6] != 0xff || greater[7] != 0xff) {
    return 5;
  }

  const uint8_t wide_bytes[16] = {
      3, 0x80, 7, 3, 0xfe, 9, 3, 0xff, 3, 1, 2, 3, 4, 5, 6, 7};
  uint8x16_t wide = vld1q_u8(wide_bytes);
  uint8x16_t wide_matches = vceqq_u8(wide, vdupq_n_u8(3));
  if (wide_matches[0] != 0xff || wide_matches[1] != 0 ||
      wide_matches[8] != 0xff || wide_matches[11] != 0xff ||
      wide_matches[15] != 0) {
    return 7;
  }
  uint8_t wide_out[16] = {};
  vst1q_u8(wide_out, wide_matches);
  if (wide_out[0] != 0xff || wide_out[3] != 0xff || wide_out[15] != 0) {
    return 8;
  }

  int32x4_t i32_left = {1, 2, 3, 4};
  int32x4_t i32_right = {10, 20, 30, 40};
  int32x4_t i32_sum = vaddq_s32(i32_left, i32_right);
  if (i32_sum[0] != 11 || i32_sum[3] != 44) {
    return 9;
  }

  float32x4_t f32_left = {1.f, 2.f, 3.f, 4.f};
  float32x4_t f32_right = {5.f, 6.f, 7.f, 8.f};
  float32x4_t f32_sum = vaddq_f32(f32_left, f32_right);
  float32x4_t f32_prod = vmulq_f32(f32_left, f32_right);
  if (f32_sum[0] != 6.f || f32_sum[3] != 12.f ||
      f32_prod[1] != 12.f || f32_prod[2] != 21.f) {
    return 10;
  }
  float64x2_t f64_left = {1.5, 2.5};
  float64x2_t f64_right = {0.5, 1.5};
  float64x2_t f64_sum = vaddq_f64(f64_left, f64_right);
  if (f64_sum[0] != 2.0 || f64_sum[1] != 4.0) {
    return 11;
  }
  return 0;
}
