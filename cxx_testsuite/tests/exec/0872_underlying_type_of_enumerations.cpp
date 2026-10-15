// std::underlying_type names the enumeration's real storage type, so casting
// a 64-bit enumerator through it keeps every bit.
#include <stdint.h>
#include <type_traits>

enum Flags { kA = 1, kB = 2 };
enum Signed { kMinusOne = -1 };
enum class S8 : int8_t { a };
enum class U8 : uint8_t { a };
enum class U16 : uint16_t { a };
enum class U64 : uint64_t { kAll = ~0ull };
enum I64 : int64_t { kI64 };
enum class Scoped { x };

static_assert(std::is_same_v<std::underlying_type_t<Flags>, unsigned>);
static_assert(std::is_same_v<std::underlying_type_t<Signed>, int>);
static_assert(std::is_same_v<std::underlying_type_t<S8>, signed char>);
static_assert(std::is_same_v<std::underlying_type_t<U8>, unsigned char>);
static_assert(std::is_same_v<std::underlying_type_t<U16>, unsigned short>);
static_assert(std::is_same_v<std::underlying_type_t<U64>, uint64_t>);
static_assert(std::is_same_v<std::underlying_type_t<I64>, int64_t>);
static_assert(std::is_same_v<std::underlying_type_t<Scoped>, int>);

template <class E>
constexpr bool RoundTrips(E e) {
  return static_cast<E>(static_cast<std::underlying_type_t<E>>(e)) == e;
}

int main() {
  if (static_cast<std::underlying_type_t<U64>>(U64::kAll) != ~0ull) return 1;
  if (!RoundTrips(U64::kAll)) return 2;
  if (!RoundTrips(S8::a)) return 3;
  return 0;
}
