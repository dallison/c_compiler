// `const T&&` is not a forwarding reference ([temp.deduct.call]/3): an lvalue
// argument cannot bind to it, so the `const T&` overload is chosen and a
// deleted `const T&&` overload (as in std::cref) is never selected for lvalues.
#include <functional>

template <class T> int f(const T&) { return 1; }
template <class T> int f(const T&&) = delete;
template <class T> int g(const T&) { return 1; }
template <class T> int g(const T&&) { return 2; }
template <class T> int h(T&&) { return 3; }

int main() {
  int v = 0;
  const int c = 0;
  if (f(v) != 1 || f(c) != 1) return 1;
  if (g(v) != 1 || g(c) != 1) return 2;
  if (g(1) != 2 || g(static_cast<const int&&>(c)) != 2) return 3;
  if (h(v) != 3 || h(c) != 3 || h(1) != 3) return 4;
  std::reference_wrapper<const int> r = std::cref(v);
  v = 7;
  if (r.get() != 7) return 5;
  return 0;
}
