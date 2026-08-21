// RUN: -std=c++29
// EXPECT_EXIT: 0

#include <bit>
#include <climits>
#include <cstdint>
#include <type_traits>
#include <version>

template <class T>
concept has_bit_reverse = requires(T value) {
  std::bit_reverse(value);
};

template <class T, class S>
concept has_safe_shift = requires(T value, S amount) {
  std::shl(value, amount);
  std::shr(value, amount);
};

constexpr std::uint32_t shift_u32(std::uint32_t value, int amount) {
  return value >> amount;
}

static_assert(__cpp_lib_bitops == 202607L);
static_assert(has_bit_reverse<unsigned int>);
static_assert(!has_bit_reverse<int>);
static_assert(has_safe_shift<int, long long>);
static_assert(!has_safe_shift<bool, int>);
static_assert(!has_safe_shift<unsigned int, char>);

static_assert(std::is_same_v<decltype(std::shl(
                                 static_cast<unsigned char>(1), 1)),
                             unsigned char>);
static_assert(noexcept(std::shl(1, 1)));
static_assert(noexcept(std::shr(1, 1)));
static_assert(noexcept(std::bit_reverse(1u)));
static_assert(noexcept(std::bit_compress(1u, 1u)));
static_assert(noexcept(std::bit_expand(1u, 1u)));
static_assert(!noexcept(std::bit_repeat(1u, 1)));

constexpr bool test_bit_operations() {
  if (std::bit_reverse(static_cast<std::uint32_t>(0x00001234)) !=
          0x2c480000u ||
      std::bit_reverse(static_cast<unsigned char>(0x81)) !=
          static_cast<unsigned char>(0x81)) {
    return false;
  }
  if (std::bit_repeat(static_cast<std::uint32_t>(0xc), 4) !=
          0xccccccccu ||
      std::bit_repeat(static_cast<unsigned char>(0x2), 2) !=
          static_cast<unsigned char>(0xaa)) {
    return false;
  }

  constexpr std::uint32_t value = 0xd6b39a5cu;
  constexpr std::uint32_t mask = 0x0f0f00ffu;
  std::uint32_t compressed = std::bit_compress(value, mask);
  if (std::bit_expand(compressed, mask) != (value & mask) ||
      std::bit_compress(
          std::bit_expand(static_cast<std::uint32_t>(0x5a3), mask), mask) !=
          0x5a3u) {
    return false;
  }

  return std::shl(3u, 4) == 48u &&
         std::shr(48u, 4) == 3u &&
         std::shl(1u, 32) == 0u &&
         std::shr(1u, 32) == 0u &&
         std::shr(-4, 1) == -2 &&
         std::shr(-4, 100) == -1 &&
         std::shl(-4, -1) == -2 &&
         std::shr(-4, -1) == -8 &&
         std::shl(1u, INT_MIN) == 0u &&
         std::shr(1u, INT_MIN) == 0u;
}

static_assert(std::bit_reverse(static_cast<std::uint32_t>(0x00001234)) ==
              0x2c480000u);
static_assert(std::bit_repeat(static_cast<std::uint32_t>(0xc), 4) ==
              0xccccccccu);
static_assert(shift_u32(static_cast<std::uint32_t>(0xd6b39a5c), 8) ==
              0x00d6b39au);
static_assert(std::bit_compress(static_cast<unsigned short>(0xd),
                                static_cast<unsigned short>(0x5)) ==
              static_cast<unsigned short>(0x3));
static_assert(std::bit_expand(static_cast<unsigned short>(0x3),
                              static_cast<unsigned short>(0x5)) ==
              static_cast<unsigned short>(0x5));
static_assert(std::shl(3u, 4) == 48u &&
              std::shr(48u, 4) == 3u &&
              std::shl(1u, 32) == 0u &&
              std::shr(1u, 32) == 0u);
static_assert(std::shr(-4, 1) == -2 &&
              std::shr(-4, 100) == -1 &&
              std::shl(-4, -1) == -2 &&
              std::shr(-4, -1) == -8);
static_assert(std::shl(1u, INT_MIN) == 0u &&
              std::shr(1u, INT_MIN) == 0u);
static_assert(std::bit_expand(
                  std::bit_compress(
                      static_cast<std::uint32_t>(0xd6b39a5c),
                      static_cast<std::uint32_t>(0x0f0f00ff)),
                  static_cast<std::uint32_t>(0x0f0f00ff)) ==
              (static_cast<std::uint32_t>(0xd6b39a5c) &
               static_cast<std::uint32_t>(0x0f0f00ff)));
static_assert(std::bit_compress(
                  std::bit_expand(
                      static_cast<std::uint32_t>(0x5a3),
                      static_cast<std::uint32_t>(0x0f0f00ff)),
                  static_cast<std::uint32_t>(0x0f0f00ff)) ==
              static_cast<std::uint32_t>(0x5a3));
static_assert(test_bit_operations());

bool test_byte_permutations() {
  for (unsigned int value = 0; value < 256; ++value) {
    unsigned char byte = static_cast<unsigned char>(value);
    if (std::bit_reverse(std::bit_reverse(byte)) != byte) {
      return false;
    }
    for (unsigned int mask_value = 0; mask_value < 256; ++mask_value) {
      unsigned char mask = static_cast<unsigned char>(mask_value);
      unsigned char compressed = std::bit_compress(byte, mask);
      if (std::bit_expand(compressed, mask) !=
          static_cast<unsigned char>(byte & mask)) {
        return false;
      }
      int selected = std::popcount(mask);
      unsigned char low_mask =
          selected == 8
              ? static_cast<unsigned char>(0xff)
              : static_cast<unsigned char>((1u << selected) - 1);
      if (std::bit_compress(std::bit_expand(byte, mask), mask) !=
          static_cast<unsigned char>(byte & low_mask)) {
        return false;
      }
    }
  }
  return true;
}

int main() {
  if (!test_bit_operations()) {
    return 1;
  }
  return test_byte_permutations() ? 0 : 2;
}
