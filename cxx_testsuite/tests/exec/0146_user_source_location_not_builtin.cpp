// RUN: -std=c++20
// EXPECT_EXIT: 0
// A user-defined type named `source_location` outside namespace std must not be
// mistaken for std::source_location: its `current()` default argument and its
// `line()` accessor must behave as ordinary user code, not compiler builtins.
// The parameter is deliberately named `line` so that, were the builtin to be
// (incorrectly) applied, __builtin_LINE would override the user default of 5.

namespace mylib {
struct source_location {
  int v;
  static constexpr source_location current(int line = 5) {
    return source_location{line};
  }
  constexpr int line() const { return v; }
};
}  // namespace mylib

int main(void) {
  constexpr int l = mylib::source_location::current().line();
  static_assert(l == 5, "user default argument must win over builtin");
  return l == 5 ? 0 : 1;
}
