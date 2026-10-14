// Namespace-scope variables are mangled with their namespace, so `a::x` and
// `b::x` are distinct objects, and `extern const T v[] = ...` in a namespace is
// a definition ([basic.def]/2).
namespace a {
int x = 1;
extern const char full[];
extern int plain;
}  // namespace a

namespace b {
int x = 2;
}

namespace a {
extern const char full[] = "xyz";
int plain = 3;
}  // namespace a

extern const int kLimit = 40;

int main() {
  if (a::x != 1 || b::x != 2) return 1;
  a::x = 5;
  if (b::x != 2) return 2;
  if (a::full[0] != 'x' || a::full[2] != 'z') return 3;
  if (a::plain + kLimit != 43) return 4;
  return 0;
}
