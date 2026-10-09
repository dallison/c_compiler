// RUN: -std=c++20
// EXPECT_EXIT: 0
// Function templates that differ only in a parameter declared in a
// non-deduced context ([temp.deduct.type]) have distinct specializations even
// when their substituted parameter types are equal, so each call runs its own
// template. std::string_view's comparisons rely on this to compare with
// anything convertible to the view ([string.view.comparison]).
#include <string>
#include <string_view>
#include <type_traits>

template <class T> struct V {
  const T* p;
  unsigned long n;
  V(const T* s) : p(s), n(0) {
    while (s[n]) n++;
  }
};
template <class T> int Which(V<T>, V<T>) { return 1; }
template <class T> int Which(V<T>, std::type_identity_t<V<T>> b) {
  return 2 + static_cast<int>(b.n) * 10;
}
template <class T> int Which(std::type_identity_t<V<T>> a, V<T>) {
  return 3 + static_cast<int>(a.n) * 10;
}

template <class T> constexpr int Pick(V<T>*, T) { return 1; }
template <class T> constexpr int Pick(V<T>*, typename std::type_identity<T>::type*) {
  return 2;
}

int Compare(const std::string& s, std::string_view v) {
  return (v == s) + (s == v) * 2 + (v < s) * 4 + (s != v) * 8 + (s > v) * 16;
}

int main() {
  V<char> x("abc");
  if (Which(x, x) != 1) return 1;
  if (Which(x, "ab") != 22) return 2;
  if (Which("a", x) != 13) return 3;
  V<int>* none = nullptr;
  int i = 0;
  if (Pick(none, 1) != 1 || Pick(none, &i) != 2) return 4;

  std::string s = "hello";
  if (Compare(s, "hello") != 3) return 5;
  if (Compare(s, "hellp") != 8) return 6;
  if (Compare(s, "hell") != 4 + 8 + 16) return 7;
  std::string_view v = "hello";
  if (!(v == "hello") || !("hello" == v) || v < "hello" || !(v < "help"))
    return 8;
  if ((v <=> "help") >= 0 || ("help" <=> v) <= 0) return 9;
  return 0;
}
