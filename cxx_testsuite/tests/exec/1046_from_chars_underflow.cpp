// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <charconv>
#include <limits>
#include <system_error>

int main() {
  double zero_huge = 4;
  const char zero_text[] = "0e9999";
  auto zero_in =
      std::from_chars(zero_text, zero_text + 6, zero_huge);
  if (zero_in.ec != std::errc() || zero_huge != 0.0 ||
      zero_in.ptr != zero_text + 6) {
    return 1;
  }

  double under = 4;
  const char under_text[] = "1e-1000";
  auto under_in =
      std::from_chars(under_text, under_text + 7, under);
  if (under_in.ec != std::errc::result_out_of_range || under != 0.0) {
    return 2;
  }

  double neg_under = 4;
  const char neg_text[] = "-1e-1000";
  auto neg_in = std::from_chars(neg_text, neg_text + 8, neg_under);
  if (neg_in.ec != std::errc::result_out_of_range || neg_under != 0.0) {
    return 3;
  }

  float narrow = 4;
  const char narrow_text[] = "1e-50";
  auto narrow_in =
      std::from_chars(narrow_text, narrow_text + 5, narrow);
  if (narrow_in.ec != std::errc::result_out_of_range || narrow != 0.0f) {
    return 4;
  }

  float small = 0;
  const char small_text[] = "1e-40";
  auto small_in = std::from_chars(small_text, small_text + 5, small);
  if (small_in.ec != std::errc() || !(small > 0.0f) || !(small < 1e-38f)) {
    return 5;
  }

  double sub = 0;
  const char sub_text[] = "1e-320";
  auto sub_in = std::from_chars(sub_text, sub_text + 6, sub);
  if (sub_in.ec != std::errc() || !(sub > 0.0) || !(sub < 1e-300)) {
    return 6;
  }

  double hex_under = 4;
  const char hex_text[] = "1p-9999999";
  auto hex_in = std::from_chars(hex_text, hex_text + 10, hex_under,
                                std::chars_format::hex);
  if (hex_in.ec != std::errc::result_out_of_range || hex_under != 0.0) {
    return 7;
  }

  double still = 0;
  const char still_text[] = "1e20";
  auto still_in = std::from_chars(still_text, still_text + 4, still);
  if (still_in.ec != std::errc() || still < 9e19 || still > 1.1e20) {
    return 8;
  }

  return 0;
}
