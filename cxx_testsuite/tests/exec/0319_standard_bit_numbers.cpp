// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <bit>
#include <numbers>

template <class T>
struct test_width;
template <> struct test_width<unsigned char> { static constexpr int value = 8; };
template <> struct test_width<unsigned short> { static constexpr int value = 16; };
template <> struct test_width<unsigned int> {
  static constexpr int value = sizeof(unsigned int) * 8;
};
template <> struct test_width<unsigned long> {
  static constexpr int value = sizeof(unsigned long) * 8;
};
template <> struct test_width<unsigned long long> {
  static constexpr int value = sizeof(unsigned long long) * 8;
};

template <class T>
int reference_clz(T value) {
  int result = 0;
  for (int bit = test_width<T>::value - 1; bit >= 0; --bit) {
    if ((value & static_cast<T>(static_cast<T>(1) << bit)) != 0) {
      break;
    }
    ++result;
  }
  return result;
}

template <class T>
int reference_ctz(T value) {
  int result = 0;
  const int width = test_width<T>::value;
  while (result < width && (value & static_cast<T>(1)) == 0) {
    value = static_cast<T>(value >> 1);
    ++result;
  }
  return result;
}

template <class T>
int reference_popcount(T value) {
  int result = 0;
  const int width = test_width<T>::value;
  for (int bit = 0; bit < width; ++bit) {
    result += static_cast<int>(value & static_cast<T>(1));
    value = static_cast<T>(value >> 1);
  }
  return result;
}

template <class T>
int test_runtime_bits(T seed) {
  const int width = test_width<T>::value;
  volatile T zero = seed;
  zero = static_cast<T>(0);
  if (std::countl_zero(zero) != width) return 1;
  if (std::countr_zero(zero) != width) return 2;
  if (std::popcount(zero) != 0) return 3;
  T value = seed;
  for (int i = 0; i < 8; ++i) {
    value = static_cast<T>(value * static_cast<T>(29) + static_cast<T>(17));
    if (std::countl_zero(value) != reference_clz(value)) return 4;
    if (std::countr_zero(value) != reference_ctz(value)) return 5;
    if (std::popcount(value) != reference_popcount(value)) return 6;
    for (int amount = -width * 2 - 3; amount <= width * 2 + 3;
         amount += width / 2 + 1) {
      T left = std::rotl(value, amount);
      T right = std::rotr(value, amount);
      if (std::rotr(left, amount) != value) return 7;
      if (std::rotl(right, amount) != value) return 8;
    }
  }
  return 0;
}

int main() {
  float one = 1.0f;
  using float_bits =
      std::conditional_t<sizeof(float) == sizeof(unsigned int),
                         unsigned int, unsigned long>;
  float_bits bits = std::bit_cast<float_bits>(one);
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

  unsigned int runtime_seed =
      static_cast<unsigned int>(bits) ^ 0x3f923456u;
  volatile unsigned int runtime_word = 0x12345678u;
  volatile int runtime_shift = -67;
  if (std::rotl(static_cast<unsigned int>(runtime_word),
                static_cast<int>(runtime_shift)) != 0x02468acfu) return 6;
  if (std::rotr(static_cast<unsigned int>(runtime_word),
                static_cast<int>(runtime_shift)) != 0x91a2b3c0u) return 7;
  volatile unsigned long long runtime_wide = 0x0123456789abcdefull;
  volatile int runtime_wide_shift = 8;
  if (std::rotl(static_cast<unsigned long long>(runtime_wide),
                static_cast<int>(runtime_wide_shift)) !=
      0x23456789abcdef01ull) return 7;
  if (std::rotr(static_cast<unsigned long long>(runtime_wide),
                static_cast<int>(runtime_wide_shift)) !=
      0xef0123456789abcdull) return 8;
  int bit_error = test_runtime_bits<unsigned char>(
      static_cast<unsigned char>(runtime_seed));
  if (bit_error != 0) return 10 + bit_error;
  bit_error = test_runtime_bits<unsigned short>(
      static_cast<unsigned short>(runtime_seed));
  if (bit_error != 0) return 20 + bit_error;
  bit_error = test_runtime_bits<unsigned int>(runtime_seed);
  if (bit_error != 0) return 30 + bit_error;
  bit_error = test_runtime_bits<unsigned long>(
      static_cast<unsigned long>(runtime_seed) * 0x100000001ul);
  if (bit_error != 0) return 40 + bit_error;
  bit_error = test_runtime_bits<unsigned long long>(
      static_cast<unsigned long long>(runtime_seed) * 0x100000001ull);
  if (bit_error != 0) return 50 + bit_error;

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
