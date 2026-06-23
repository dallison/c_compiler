// RUN: -std=c++20
// EXPECT: Deduced template arguments do not match alias template

template <typename T, typename U>
struct Pair {
  T first;
  U second;
};

template <typename T>
using SamePair = Pair<T, T>;

void fail_partial_alias_ctad_repeated_parameter_mismatch() {
  SamePair bad{1, 'x'};
}
