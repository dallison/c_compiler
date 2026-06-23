// RUN: -std=c++20
// EXPECT: Deduced template arguments do not match alias template

template <typename T, typename U>
struct Pair {
  T first;
  U second;
};

template <typename T>
using PointerPair = Pair<T*, T*>;

void fail_partial_alias_ctad_structured_parameter_mismatch() {
  int value = 1;
  char other = 'x';
  PointerPair bad{&value, &other};
}
