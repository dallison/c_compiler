// An unscoped enumeration without a fixed underlying type promotes to the
// first of int, unsigned int, long, ... that holds all its values
// ([conv.prom]/3), even when its storage type is unsigned.  Protobuf masks
// tagged pointers with `p & ~kMask`; promoting to unsigned int instead
// truncated the pointer to 32 bits.
#include <stdint.h>

enum Flags { kArenaBit = 0x1, kMutableBit = 0x2, kMask = 0x3 };
enum Big { kHuge = 0x80000000u };
enum Negative { kMinusOne = -1 };
enum Fixed : unsigned short { kFixed = 7 };
enum Wide : unsigned long long { kWide = 1ull << 40 };

template <typename T, typename U>
struct Same {
  static constexpr bool value = false;
};
template <typename T>
struct Same<T, T> {
  static constexpr bool value = true;
};

static_assert(Same<decltype(~kMask), int>::value);
static_assert(Same<decltype(+kMask), int>::value);
static_assert(Same<decltype(-kMask), int>::value);
static_assert(Same<decltype(kMask + 0), int>::value);
static_assert(Same<decltype(kMask + 0u), unsigned>::value);
static_assert(Same<decltype(kMask << 1), int>::value);
static_assert(Same<decltype(+kHuge), unsigned>::value);
static_assert(Same<decltype(+kMinusOne), int>::value);
static_assert(Same<decltype(+kFixed), int>::value);
static_assert(Same<decltype(+kWide), unsigned long long>::value);
static_assert(kMask - 4 == -1);
static_assert(~kMask == -4);

int main() {
  uintptr_t tagged = (uintptr_t)0x1017688a1ull;
  if ((tagged & ~kMask) != (uintptr_t)0x1017688a0ull) return 1;
  Flags f = kMask;
  if ((tagged & ~f) != (uintptr_t)0x1017688a0ull) return 2;
  long difference = f - 4;
  if (difference != -1) return 3;
  long complement = ~f;
  if (complement != -4) return 4;
  int shifted = kMutableBit << 30;
  if (shifted >= 0) return 5;
  Big big = kHuge;
  if (big + 0ll != 0x80000000ll) return 6;
  return 0;
}
