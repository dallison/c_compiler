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
  return 0;
}
