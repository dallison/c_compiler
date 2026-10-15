// An operand of enumeration type makes non-member operator functions
// candidates even when no operand has class type ([over.match.oper]/3), and
// only those whose matching parameter is that enumeration.  std::regex passes
// `icase | nosubs` to a constructor that also has a (const char*, size_t)
// overload, so taking the built-in `|` picked the wrong constructor.
#include <type_traits>

namespace flags {
enum Opt { kA = 1, kB = 2 };
inline Opt operator|(Opt left, Opt right) {
  return static_cast<Opt>(static_cast<int>(left) | static_cast<int>(right) |
                          64);
}
inline Opt operator~(Opt value) {
  return static_cast<Opt>(~static_cast<int>(value) & 0xff);
}
inline Opt& operator|=(Opt& left, Opt right) { return left = left | right; }

enum class Scoped { kX = 1, kY = 4 };
constexpr Scoped operator&(Scoped left, Scoped right) {
  return static_cast<Scoped>(static_cast<int>(left) & static_cast<int>(right) |
                             8);
}
}  // namespace flags

struct FromInt {
  FromInt(int) {}
};
// Not a candidate for two enumeration operands: no parameter is an enum.
inline int operator+(FromInt, FromInt) { return 1000; }

namespace generic {
enum Bits { kLow = 1, kHigh = 2 };
template <class E, class = std::enable_if_t<std::is_enum_v<E>>>
constexpr E operator^(E left, E right) {
  return static_cast<E>(static_cast<int>(left) + static_cast<int>(right) * 10);
}
}  // namespace generic

// absl::LogSeverity: operators mixing two scoped enumerations are not
// candidates for two operands of the same one, which use the built-in `<`.
namespace severity {
enum class Level : int { kInfo = 0, kError = 2 };
enum class AtLeast : int { kInfo = 0, kInfinity = 1000 };
enum class AtMost : int { kNegativeInfinity = -1000, kInfo = 0 };
constexpr bool operator<(Level, AtLeast) { return false; }
constexpr bool operator<(AtLeast, Level) { return false; }
constexpr bool operator<(Level, AtMost) { return false; }
constexpr bool operator<(AtMost, Level) { return false; }
constexpr Level Normalize(Level s) {
  Level n = s;
  if (n < Level::kInfo) n = Level::kInfo;
  return n;
}
static_assert(Level::kInfo < Level::kError);
static_assert(!(Level::kError < AtLeast::kInfinity));
static_assert(Normalize(static_cast<Level>(-3)) == Level::kInfo);
}  // namespace severity

struct Regexish {
  using flag_type = flags::Opt;
  static constexpr flag_type icase = flags::kA;
  static constexpr flag_type nosubs = flags::kB;
  Regexish(const char*, flag_type f) : kind(1), value(f) {}
  Regexish(const char*, unsigned long) : kind(2), value(0) {}
  int kind;
  int value;
};

int main() {
  Regexish r("x", Regexish::icase | Regexish::nosubs);
  if (r.kind != 1 || r.value != 67) return 1;
  flags::Opt opt = flags::kA;
  opt |= flags::kB;
  if (opt != 67) return 2;
  if (~flags::kA != 0xfe) return 3;
  static_assert(static_cast<int>(flags::Scoped::kX & flags::Scoped::kY) == 8);
  if (flags::kA + flags::kB != 3) return 4;
  if (generic::kLow ^ generic::kHigh != 21) return 5;
  return 0;
}
