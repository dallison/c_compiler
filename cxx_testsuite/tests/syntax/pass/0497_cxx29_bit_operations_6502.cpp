// RUN: -target 6502 -std=c++29

#include <bit>
#include <cstdint>

constexpr std::uint32_t shift_u32(std::uint32_t value, int amount) {
  return value >> amount;
}

static_assert(shift_u32(static_cast<std::uint32_t>(0xd6b39a5c), 8) ==
              0x00d6b39au);
static_assert(std::bit_compress(static_cast<std::uint32_t>(0xd6b39a5c),
                                static_cast<std::uint32_t>(0x0f0f00ff)) ==
              0x0000635cu);
static_assert(std::bit_expand(static_cast<std::uint32_t>(0x635c),
                              static_cast<std::uint32_t>(0x0f0f00ff)) ==
              0x0603005cu);
