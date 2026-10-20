// RUN: -std=c++20
// EXPECT_EXIT: 0
// An unqualified name followed by `<` is a template-name when ordinary lookup
// finds nothing or only non-template functions ([temp.names]/3), so the
// function template is found by argument-dependent lookup.

namespace N {
struct S {
  int v;
};
template <int I>
int get(S s) {
  return s.v + I;
}
template <class T>
T convert(S s) {
  return static_cast<T>(s.v);
}
}  // namespace N

int get(int x) {
  return x;
}

template <class T>
int CallGet(T t) {
  return get<2>(t);
}

template <class T>
long CallConvert(T t) {
  return convert<long>(t);
}

int main() {
  N::S s{10};
  if (CallGet(s) != 12) return 1;
  if (CallConvert(s) != 10) return 2;
  if (get<3>(s) != 13) return 3;
  if (convert<char>(s) != 10) return 4;
  if (get(7) != 7) return 5;
  int a = 1, b = 2, c = 3;
  bool comparison = a < b > (c);
  if (comparison) return 6;
  return 0;
}
