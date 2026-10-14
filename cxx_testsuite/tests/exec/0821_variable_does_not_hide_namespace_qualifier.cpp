// The name before `::` finds only namespaces, types, and templates, so a
// variable or parameter with the namespace's name does not hide it.
namespace tb::color {
enum class Fixed { kNotSet, kRed };
static constexpr int kNormal = 3;
struct Color {
  int mod = kNormal;
  Fixed fixed = Fixed::kNotSet;
};
inline int Set(Color c) { return c.mod; }
}  // namespace tb::color

namespace tb {
struct Logger {
  int color = 1;
  int Str(color::Color color) { return color::Set(color); }
  int Member() { return color::kNormal + color; }
  static int Make(color::Color color = {.mod = color::kNormal,
                                        .fixed = color::Fixed::kNotSet}) {
    return color.fixed == color::Fixed::kNotSet ? color.mod : -1;
  }
};

int Free(int color) { return color::kNormal + color; }

int LocalType() {
  struct color {
    enum { kNormal = 10 };
  };
  return color::kNormal;
}
}  // namespace tb

int main() {
  tb::Logger logger;
  if (logger.Str(tb::color::Color{}) != 3) return 1;
  if (logger.Member() != 4) return 2;
  if (tb::Logger::Make() != 3) return 3;
  if (tb::Free(1) != 4) return 4;
  int color = 5;
  if (tb::color::kNormal + color != 8) return 5;
  if (tb::LocalType() != 10) return 6;
  return 0;
}
