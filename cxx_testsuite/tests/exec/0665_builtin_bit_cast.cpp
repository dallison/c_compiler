// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <bit>
#include <cstdint>

struct Pair {
  std::uint16_t low;
  std::uint16_t high;
};

struct Bytes {
  unsigned char b[4];
};

static_assert(std::bit_cast<std::uint32_t>(1.0f) == 0x3f800000u);
static_assert(std::bit_cast<float>(0x40000000u) == 2.0f);
static_assert(std::bit_cast<std::uint64_t>(1.0) == 0x3ff0000000000000ull);
static_assert(__builtin_bit_cast(std::int32_t, 0xffffffffu) == -1);

constexpr std::uint32_t pack(std::uint16_t low, std::uint16_t high) {
  return std::bit_cast<std::uint32_t>(Pair{low, high});
}
static_assert(pack(0x1234, 0xabcd) == 0xabcd1234u);

constexpr Pair split(std::uint32_t value) {
  return std::bit_cast<Pair>(value);
}
static_assert(split(0xabcd1234u).low == 0x1234);
static_assert(split(0xabcd1234u).high == 0xabcd);

constexpr int first_byte(std::uint32_t value) {
  return std::bit_cast<Bytes>(value).b[0];
}
static_assert(first_byte(0x11223344u) == 0x44);

template <class To, class From>
To runtime_cast(From from) {
  return std::bit_cast<To>(from);
}

int main() {
  volatile float f = 1.5f;
  if (runtime_cast<std::uint32_t>(static_cast<float>(f)) != 0x3fc00000u) {
    return 1;
  }
  volatile std::uint32_t u = 0xc0000000u;
  if (runtime_cast<float>(static_cast<std::uint32_t>(u)) != -2.0f) {
    return 2;
  }
  volatile double d = -0.0;
  if (std::bit_cast<std::uint64_t>(static_cast<double>(d)) !=
      0x8000000000000000ull) {
    return 3;
  }
  Pair pair = runtime_cast<Pair>(static_cast<std::uint32_t>(u) | 0x5678u);
  if (pair.low != 0x5678 || pair.high != 0xc000) {
    return 4;
  }
  Bytes bytes{{1, 2, 3, 4}};
  if (std::bit_cast<std::uint32_t>(bytes) != 0x04030201u) {
    return 5;
  }
  if (__builtin_bit_cast(std::int16_t, static_cast<std::uint16_t>(u >> 16)) !=
      -16384) {
    return 6;
  }
  return 0;
}
