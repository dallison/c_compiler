// RUN: -std=c++20
// EXPECT: Deduced template arguments do not match alias template

template <typename T, typename U>
struct Pair {
  T first;
  U second;
};

template <typename U>
using IntPair = Pair<int, U>;

void fail_partial_alias_ctad_fixed_argument_mismatch() {
  IntPair bad{'x', 1};
}
