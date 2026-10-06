// RUN: -std=c++20 -fconstexpr-eval=audit

// std::bit_cast between scalars of the same size reinterprets the bytes in a
// constant expression, so <limits> can build infinity and NaN from their bits.
#include <bit>
#include <cstdint>
#include <limits>

static_assert(std::bit_cast<std::uint32_t>(1.0f) == 0x3f800000u);
static_assert(std::bit_cast<float>(0x40490fdbu) > 3.14159f);
static_assert(std::bit_cast<std::uint64_t>(-2.0) == 0xc000000000000000ull);
static_assert(std::bit_cast<std::int32_t>(0xffffffffu) == -1);
static_assert(std::bit_cast<std::uint16_t>(std::int16_t{-2}) == 0xfffeu);

static_assert(std::numeric_limits<float>::infinity() >
              std::numeric_limits<float>::max());
static_assert(std::numeric_limits<double>::infinity() >
              std::numeric_limits<double>::max());
static_assert(std::numeric_limits<float>::quiet_NaN() !=
              std::numeric_limits<float>::quiet_NaN());
