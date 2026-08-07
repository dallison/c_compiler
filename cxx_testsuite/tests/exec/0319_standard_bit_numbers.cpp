// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <bit>
#include <numbers>

int main() {
  float one = 1.0f;
  unsigned int bits = std::bit_cast<unsigned int>(one);
  if (bits != 0x3f800000u ||
      std::bit_cast<float>(bits) != one) {
    return 1;
  }

  if (std::rotl<unsigned char>(0x81u, 1) != 0x03u ||
      std::rotr<unsigned char>(0x81u, 1) != 0xc0u ||
      std::countl_zero<unsigned short>(0x0100u) != 7 ||
      std::countr_zero<unsigned short>(0x0100u) != 8 ||
      std::popcount<unsigned long long>(0xf00000000000000full) != 8) {
    return 2;
  }

  for (unsigned int value = 1; value < 1000; ++value) {
    unsigned int floor = std::bit_floor(value);
    unsigned int ceil = std::bit_ceil(value);
    if (!std::has_single_bit(floor) || !std::has_single_bit(ceil) ||
        floor > value || ceil < value ||
        std::bit_width(value) <= 0) {
      return 3;
    }
  }

  double circle = std::numbers::pi * 4.0;
  long double ratio =
      std::numbers::phi_v<long double> /
      std::numbers::inv_sqrt3_v<long double>;
  if (circle < 12.5663 || circle > 12.5664 ||
      ratio < 2.802 || ratio > 2.803) {
    return 4;
  }
  return 0;
}
