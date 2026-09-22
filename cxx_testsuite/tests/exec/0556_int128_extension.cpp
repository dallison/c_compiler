// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <type_traits>
#include <cstddef>
#include <limits>

static_assert(sizeof(__int128) == 16, "");
static_assert(sizeof(unsigned __int128) == 16, "");
static_assert(sizeof(__int128_t) == 16, "");
static_assert(sizeof(__uint128_t) == 16, "");
static_assert(alignof(__int128) == 16, "");
static_assert(__SIZEOF_INT128__ == 16, "");

static_assert(std::is_integral<__int128>::value, "");
static_assert(std::is_integral<unsigned __int128>::value, "");
static_assert(std::is_signed<__int128>::value, "");
static_assert(std::is_unsigned<unsigned __int128>::value, "");
static_assert(!std::is_same<wchar_t, int>::value, "");
static_assert(std::is_integral<wchar_t>::value, "");

struct Conv {
  constexpr explicit operator int() const { return 1; }
  constexpr explicit operator wchar_t() const { return 2; }
  constexpr explicit operator __int128() const { return 3; }
  constexpr explicit operator unsigned __int128() const { return 4; }
};

constexpr __int128 ShiftHi(unsigned long long hi, unsigned long long lo) {
  return (static_cast<__int128>(hi) << 64) + static_cast<__int128>(lo);
}

int main() {
  __int128 a = 7;
  unsigned __int128 b = 9;
  a = a + 1;
  b = b + static_cast<unsigned __int128>(a);
  __int128 wide = ShiftHi(1, 2);
  unsigned long long hi = static_cast<unsigned long long>(
      static_cast<unsigned __int128>(wide) >> 64);
  unsigned long long lo = static_cast<unsigned long long>(wide);
  Conv c;
  int i = static_cast<int>(c);
  wchar_t w = static_cast<wchar_t>(c);
  __int128 s = static_cast<__int128>(c);
  unsigned __int128 u = static_cast<unsigned __int128>(c);
  if (i != 1 || w != 2 || s != 3 || u != 4) {
    return 1;
  }
  if (hi != 1 || lo != 2) {
    return 2;
  }
  if (a != 8 || b != 17) {
    return 3;
  }
  if (std::numeric_limits<__int128>::digits != 127) {
    return 4;
  }
  if (std::numeric_limits<unsigned __int128>::digits != 128) {
    return 5;
  }
  return 0;
}
