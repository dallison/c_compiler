// RUN: -std=c++20
// EXPECT: Deduced template arguments do not match alias template

template <typename T, typename U>
struct Pair {
  T first;
  U second;
};

template <typename U>
using IntPair = Pair<int, U>;

void fail_new_alias_ctad_mismatch() {
  auto value = new IntPair('x', 1);
  (void)value;
}
