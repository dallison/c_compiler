// RUN: -std=c++11
// EXPECT: Missing : in conditional expression
// `::` in place of `:` must diagnose and not crash in conditional analysis.
// An anonymous class with an empty nested-name-specifier must not crash
// when computing variable alignment.
struct S {
  static int n;
};
int f(int x) {
  return x ? S::n :: 0;
}
class :: : {} a;
