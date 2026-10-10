// A parenthesized constant variable is an lvalue: decltype((c)) and a
// decltype(auto) variable or return deduced from (c) are references to it,
// not copies of its value.
template <class T, class U>
struct Same {
  static constexpr bool value = false;
};
template <class T>
struct Same<T, T> {
  static constexpr bool value = true;
};

constexpr int c = 5;
int g = 1;
struct S {
  int m;
  static constexpr int k = 3;
};
S s{2};

static_assert(Same<decltype((c)), const int&>::value);
static_assert(Same<decltype(c), const int>::value);
static_assert(Same<decltype((S::k)), const int&>::value);

decltype(auto) a = (c);
decltype(auto) b = c;
decltype(auto) d = (g);
decltype(auto) e = (S::k);
decltype(auto) f = (s.m);
static_assert(Same<decltype(a), const int&>::value);
static_assert(Same<decltype(b), const int>::value);
static_assert(Same<decltype(d), int&>::value);
static_assert(Same<decltype(e), const int&>::value);
static_assert(Same<decltype(f), int&>::value);

decltype(auto) ReturnConstant() { return (c); }
decltype(auto) ReturnValue() { return c; }
static_assert(Same<decltype(ReturnConstant()), const int&>::value);
static_assert(Same<decltype(ReturnValue()), int>::value);

int main() {
  constexpr int lc = 7;
  decltype(auto) x = (lc);
  static_assert(Same<decltype(x), const int&>::value);
  decltype(auto) y = (c + 0);
  static_assert(Same<decltype(y), int>::value);
  if (&a != &c || &e != &S::k || &x != &lc) return 1;
  if (&ReturnConstant() != &c) return 2;
  if (a + b + e + x + y != 25) return 3;
  return 0;
}
