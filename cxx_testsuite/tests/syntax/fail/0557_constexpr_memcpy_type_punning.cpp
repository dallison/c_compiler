// RUN: -std=c++20 -fconstexpr-eval=pcode
// EXPECT: call to non-constexpr function 'memcpy'

// A constant expression may not reinterpret bytes with memcpy; that is what
// std::bit_cast is for.
#include <cstring>

constexpr float pun() {
  unsigned bits = 0x3f800000u;
  float value = 0;
  std::memcpy(&value, &bits, sizeof value);
  return value;
}

static_assert(pun() == 1.0f);
