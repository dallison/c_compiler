// variant's converting constructor and assignment choose the alternative by
// overload resolution over the imaginary functions F(T_i) ([variant.ctor]/14):
// a string literal picks std::string, an int never narrows to float, and bool
// is only chosen for a bool argument.
#include <string>
#include <type_traits>
#include <variant>

struct FromInt {
  int v;
  FromInt(int x) : v(x) {}
};

int main() {
  std::variant<int, std::string> a = "text";
  if (a.index() != 1 || std::get<1>(a) != "text") return 1;

  std::variant<float, long> b = 5;
  if (b.index() != 1 || std::get<1>(b) != 5) return 2;

  std::variant<bool, std::string> c = "c";
  if (c.index() != 1) return 3;
  std::variant<bool, std::string> d = true;
  if (d.index() != 0 || !std::get<0>(d)) return 4;

  std::variant<std::string, FromInt> e = 3;
  if (e.index() != 1 || std::get<1>(e).v != 3) return 5;

  std::variant<int, std::string> f;
  f = "assigned";
  if (f.index() != 1 || std::get<1>(f) != "assigned") return 6;
  f = 9;
  if (f.index() != 0 || std::get<0>(f) != 9) return 7;

  if (std::is_constructible<std::variant<float, double>, int>::value) return 8;
  if (!std::is_constructible<std::variant<int, std::string>, const char*>::value)
    return 9;
  return 0;
}
