// RUN: -std=c++20
// EXPECT: Deduced template arguments do not match alias template

template <typename T, typename U>
struct Pair {
  T first;
  U second;
};

template <typename T, int N>
using ArrayPointerPair = Pair<T (*)[N], T (*)[N]>;

void fail_partial_alias_ctad_array_bound_mismatch() {
  int first[2] = {1, 2};
  int second[3] = {3, 4, 5};
  ArrayPointerPair bad{&first, &second};
}
