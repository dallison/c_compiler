// Constant comparisons of __int128 values must use all 128 bits: 2^64 - 1
// widened to __int128 is positive even though its low word is all ones.
constexpr __int128 kLowOnes = static_cast<__int128>(~0ull);
constexpr __int128 kNegative = -5;
constexpr __int128 kHigh = static_cast<__int128>(1) << 64;
constexpr unsigned __int128 kUnsignedTop = ~static_cast<unsigned __int128>(0);

static_assert(!(kLowOnes < 0));
static_assert(kLowOnes > 0);
static_assert(kNegative < 0);
static_assert(kNegative < kLowOnes);
static_assert(kHigh > kLowOnes);
static_assert(kHigh != static_cast<__int128>(0));
static_assert(kHigh != 0);
static_assert(kUnsignedTop > kHigh);
static_assert(static_cast<__int128>(static_cast<unsigned long long>(-1)) >= 0);

enum class Wide : unsigned long long { kAll = ~0ull };
static_assert(static_cast<__int128>(Wide::kAll) > 0);

constexpr bool Positive(__int128 v) { return v > 0; }
static_assert(Positive(kLowOnes));

__int128 g_shifted = static_cast<__int128>(1) << 100;
unsigned __int128 g_all = ~static_cast<unsigned __int128>(0);
__int128 g_negative = -2;
__int128 g_copy = kHigh;
struct Holder {
  int tag;
  __int128 value;
};
Holder g_holder = {1, kHigh + 3};

int main() {
  bool folded = kLowOnes < 0;
  if (folded) return 1;
  if ((g_shifted >> 100) != 1) return 2;
  if (g_all + 1 != 0 || (g_all >> 64) != ~0ull) return 3;
  if (g_negative != -2 || (g_negative >> 64) != -1) return 4;
  if (g_copy != kHigh || (g_copy >> 64) != 1) return 5;
  if (g_holder.tag != 1 || g_holder.value != kHigh + 3) return 6;
  constexpr __int128 local = kHigh * 2;
  __int128 copy = local;
  if ((copy >> 64) != 2) return 7;
  return 0;
}
